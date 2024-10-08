import { Pin } from './pin.mjs';
import { Components } from './components.mjs';
import { State } from './layout/state.mjs';
import { Cache } from './layout/cache.mjs';

export class Layout {

    constructor(spec, options) {
        this.spec = spec;

        // Device parameters
        this.type = spec.type;
        this.args = spec.args;

        // --- Build pins ---

        this.on = spec.on ? new Pin(spec.on, 'src', spec.nodeNames[spec.on], Pin.ON) : null;
        this.off = spec.off ? new Pin(spec.off, 'src', spec.nodeNames[spec.off], Pin.OFF) : null;

        const srcs = [spec.on, spec.off].filter(n => n !== undefined);
        const pins = [].concat(spec.inputs, spec.outputs).filter((v, i, a) => a.indexOf(v) === i);
        const regs = spec.registers.filter((v, i, a) => a.indexOf(v) === i);

        const pins_rw = pins.filter(n => spec.inputs.includes(n) && !srcs.includes(n));
        const pins_ro = pins.filter(n => !spec.inputs.includes(n) && !srcs.includes(n));
        const regs_ro = regs.filter(n => !pins.includes(n) && !srcs.includes(n));

        const pins_rw_hex = pins_rw.filter(n => !spec.flags.includes(n));
        const pins_rw_bin = pins_rw.filter(n => spec.flags.includes(n));
        const pins_ro_hex = pins_ro.filter(n => !spec.flags.includes(n));
        const pins_ro_bin = pins_ro.filter(n => spec.flags.includes(n));
        const regs_ro_hex = regs_ro.filter(n => !spec.flags.includes(n));
        const regs_ro_bin = regs_ro.filter(n => spec.flags.includes(n));

        this.pins = [].concat(
            [
                this.on,
                this.off,
            ].filter(Boolean),
            ...[
                ...pins_rw_hex.map(n => new Pin(n, 'pin', spec.nodeNames[n], Pin.READ_WRITE)),
                ...pins_rw_bin.map(n => new Pin(n, 'pin', spec.nodeNames[n], Pin.READ_WRITE, 2)),
                ...pins_ro_hex.map(n => new Pin(n, 'pin', spec.nodeNames[n], Pin.READ_ONLY)),
                ...pins_ro_bin.map(n => new Pin(n, 'pin', spec.nodeNames[n], Pin.READ_ONLY, 2)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
            ...[
                ...regs_ro_hex.map(n => new Pin(n, 'reg', spec.nodeNames[n], Pin.READ_ONLY)),
                ...regs_ro_bin.map(n => new Pin(n, 'reg', spec.nodeNames[n], Pin.READ_ONLY, 2)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
        );

        // Build node-to-pin map
        this.pinsByNode = this.pins.reduce((map, p, px) => {
            p.nodes.forEach(n => {
                map[n] = px;
            });

            return map;
        }, {});

        // --- Build components ---

        this.components = new Components();

        this.components.addComponents('load', spec.loads);
        this.components.addComponents('transistor', spec.transistors);
        this.components.addComponents('buffer', spec.buffers);
        this.components.addComponents('function', spec.functions);
        this.components.addComponents('cell', spec.cells);

        // --- Reduce components ---

        if (options.reduceCircuits) {
            this.circuits = spec.circuits
                .filter(c => c.enabled && !(c.limit <= 0))
                .map(c => new Layout(c, {}));

            this.circuits.forEach(circuit => {
                let limit = circuit.spec.limit ? circuit.spec.limit : Number.MAX_SAFE_INTEGER,
                    count = 0;

                process.stdout.write(`Reducing circuit '${circuit.spec.id}'...`);

                // Cache known mis-matches
                let cache = new Cache();

                // Iteratively reduce one circuit instance at a time
                while (count < limit && this.reduceCircuit(circuit, cache)) {
                    count++;

                    if (count % 10 === 0) {
                        process.stdout.write('.');
                    }
                }

                console.log(`done with ${count}`);
            });
        }

        // --- Normalize ---

        this.normalizeNodes(options.reduceNodes);

        // --- Calculate counts ---

        this.counts = {
            nodes: Math.max(...Object.values(this.nodes)) + 1,
            pins: this.pins.length,
            loads: this.components.getCount('load'),
            transistors: this.components.getCount('transistor'),
            buffers: this.components.getCount('buffer'),
            functions: this.components.getCount('function'),
            cells: this.components.getCount('cell'),
        };

        // --- Extract components ---

        this.loads = this.components.getComponents('load');
        this.transistors = this.components.getComponents('transistor');
        this.buffers = this.components.getComponents('buffer');
        this.functions = this.components.getComponents('function');
        this.cells = this.components.getComponents('cell');
    }

    reduceCircuit(circuit, cache) {
        const device = this;

        // Component matchers
        function matchNodes(cnx, dnx, state) {

            // Check if nodes are already matched or already in use
            if (state.doNodesMatch(cnx, dnx)) {
                return state;
            }

            if (state.doNodesConflict(cnx, dnx)) {
                return false;
            }

            // Fetch pins
            let cpx = circuit.pinsByNode[cnx],
                dpx = device.pinsByNode[dnx];

            let cpin = cpx === undefined ? null : circuit.pins[cpx],
                dpin = dpx === undefined ? null : device.pins[dpx];

            // Source-type pins must match
            if (cpin && cpin.type === 'src' && (!dpin || dpin.mode !== cpin.mode) ||
                dpin && dpin.type === 'src' && (!cpin || cpin.mode !== dpin.mode)) {

                return false;
            }

            // Check if the circuit node is internal
            const internal = !cpin || (cpin.type !== 'src' && cpin.type !== 'pin');

            // Device pins may not be internal circuit nodes
            if (internal && dpin && dpin.type !== 'src') {
                return false;
            }

            // Compare component counts
            for (const type of Components.getTypes()) {
                for (const group of Components.getGroups(type)) {
                    const cxs = circuit.components.getIndicesByNode(type, group, cnx),
                          dxs = device.components.getIndicesByNode(type, group, dnx);

                    if (internal) {
                        if (dxs.length !== cxs.length) {
                            return false;
                        }
                    } else {
                        if (dxs.length < cxs.length) {
                            return false;
                        }
                    }
                }
            }

            // Update state
            state = state.copyWithNode(cnx, dnx);

            // Match components
            for (const type of Components.getTypes()) {
                for (const group of Components.getGroups(type)) {
                    const cxs = circuit.components.getIndicesByNode(type, group, cnx),
                          dxs = device.components.getIndicesByNode(type, group, dnx);

                    if (!cxs.every(cx => {
                        if (state = findComponent(type, cx, dxs, state)) {
                            return true;
                        } else {
                            return false;
                        }
                    })) {
                        return false;
                    }
                }
            }

            return state;
        }

        function matchComponents(type, cx, dx, state) {

            // Check if components are either matched or otherwise in use
            if (state.doComponentsMatch(type, cx, dx)) {
                return state;
            }

            if (state.doComponentsConflict(type, cx, dx)) {
                return false;
            }

            // Check component compatibility
            let cc = circuit.components.getComponent(type, cx),
                dd = device.components.getComponent(type, dx);

            if (!Components.areCompatible(type, cc, dd)) {
                return false;
            }

            // Match nodes
            state = state.copyWithComponent(type, cx, dx);

            for (const group of Components.getGroups(type)) {
                const cnodes = cc.getGroupNodes(group), dnodes = dd.getGroupNodes(group);

                if (cnodes.length !== dnodes.length) {
                    return false;
                }

                if (cnodes.length === 0) {
                    continue;
                } else if (cnodes.length === 1) {
                    state = matchNodes(cnodes[0], dnodes[0], state)

                    if (!state) {
                        return false;
                    }
                } else if (cnodes.length === 2) {
                    let groupState = false;

                    if ((groupState = matchNodes(cnodes[0], dnodes[0], state)) &&
                        (groupState = matchNodes(cnodes[1], dnodes[1], groupState))) {

                        state = groupState;
                    } else if ((groupState = matchNodes(cnodes[0], dnodes[1], state)) &&
                               (groupState = matchNodes(cnodes[1], dnodes[0], groupState))) {
                        state = groupState;
                    } else {
                        return false;
                    }
                } else {
                    throw new Error('Component groups with greater than 2 nodes not supported');
                }
            }

            return state;
        }

        // Component finders
        function findComponent(type, cx, dxs, state) {
            for (const dx of dxs) {

                // Short-circuit if this is a known mis-match
                if (cache.hasMismatch(type, cx, dx, state)) {
                    continue;
                }

                let newState = matchComponents(type, cx, dx, state);

                if (newState) {
                    return newState;
                }

                // Cache this mis-match for future searches
                cache.cacheMismatch(type, cx, dx, state);
            }

            return false;
        }

        // Initialize search state
        let state = new State(Components.getTypes());

        // Match all components
        for (const type of Components.getTypes()) {
            if (!circuit.components.getIndices(type).every(cx => {
                if (state = findComponent(type, cx, device.components.getIndices(type), state)) {
                    return true;
                } else {
                    return false;
                }
            })) {
                return false;
            }
        }

        // Reduce all device components that matched the circuit
        for (const type of Components.getTypes()) {
            device.components.reduceComponents(type, state.getDeviceComponents(type));
        }

        // Create a new component
        function parseArg(arg, idx) {

            if (Array.isArray(arg)) {
                return arg.map((a, i) => parseArg(a, `${idx}[${i}]`));
            }

            if (typeof arg === 'object' && arg !== null) {
                return Object.fromEntries(
                    Object.entries(arg).map(([k, v]) => [k, parseArg(v, `${idx}.${k}`)])
                );
            }

            if (typeof arg !== 'string') {
                return arg;
            }

            const sigil = arg[0];

            if (sigil === '$' || sigil === '@') {
                const id = arg.substring(1),
                      pin = circuit.pins.filter(p => p.id === id)[0];

                if (pin === undefined) {
                    throw new Error(`Argument ${idx} in '${circuit.type}' expects valid pin`);
                }

                if (sigil === '$') {
                    if (pin.nodes.length !== 1) {
                        throw new Error(`Argument ${idx} in '${circuit.type}' expects scalar pin`);
                    }

                    return state.getCircuitNode(pin.nodes[0]);
                } else {
                    return pin.nodes.map(n => state.getCircuitNode(n));
                }
            } else {
                return arg;
            }
        }

        const args = circuit.args.map(parseArg);

        if (circuit.type === 'reduce') {
            // Reduce components only
        } else {
            device.components.addComponents(circuit.type, [args]);
        }

        return true;
    }

    normalizeNodes(reduceNodes) {

        // Find all nodes in use
        const nodes = [].concat(
            ...this.pins.map(p => p.getAllNodes()),
            ...this.components.getAllNodes('load'),
            ...this.components.getAllNodes('transistor'),
            ...this.components.getAllNodes('buffer'),
            ...this.components.getAllNodes('function'),
            ...this.components.getAllNodes('cell'),
        ).filter((v, i, a) => a.indexOf(v) === i).sort((a, b) => a - b);

        if (reduceNodes) {
            process.stdout.write(`Reducing nodes...`);

            // Map nodes to unique indices
            this.nodes = Object.fromEntries(nodes.map((node, idx) => [node, idx]));

            // Replace nodes with unique indices in each component
            this.pins.forEach(p => { p.nodes = p.nodes.map(n => this.nodes[n]) });

            this.components.getComponents('load').forEach(l => l.remapNodes(this.nodes));
            this.components.getComponents('transistor').forEach(t => t.remapNodes(this.nodes));
            this.components.getComponents('buffer').forEach(b => b.remapNodes(this.nodes));
            this.components.getComponents('function').forEach(f => f.remapNodes(this.nodes));
            this.components.getComponents('cell').forEach(c => c.remapNodes(this.nodes));

            console.log('done');
        } else {

            // Leave nodes unchanged
            this.nodes = Object.fromEntries(nodes.map(node => [node, node]));
        }
    }

    printInfo() {
        console.log(`Nodes:       ${this.counts.nodes}`);
        console.log(`Pins:        ${this.counts.pins}`);
        console.log(`Loads:       ${this.counts.loads}`);
        console.log(`Transistors: ${this.counts.transistors}`);
        console.log(`Buffers:     ${this.counts.buffers}`);
        console.log(`Functions:   ${this.counts.functions}`);
        console.log(`Cells:       ${this.counts.cells}`);
    }

    buildSpec() {
        return {
            id: this.spec.id,
            name: this.spec.name,
            type: this.spec.type,
            enabled: this.spec.enabled,
            limit: this.spec.limit,
            args: this.spec.args,
            memory: this.spec.memory,
            nodes: Object.fromEntries(Object.entries(this.spec.nodeNames).map(([name, set]) => {
                return [name, set.map(n => this.nodes[n])];
            })),
            on: this.spec.on,
            off: this.spec.off,
            inputs: this.spec.inputs,
            outputs: this.spec.outputs,
            registers: this.spec.registers,
            flags: this.spec.flags,
            circuits: this.circuits ? this.circuits.map(c => c.buildSpec()) : undefined,
            loads: this.components.getComponents('load').map(l => l.getSpec()),
            transistors: this.components.getComponents('transistor').map(t => t.getSpec()),
            buffers: this.components.getComponents('buffer').map(b => b.getSpec()),
            functions: this.components.getComponents('function').map(f => f.getSpec()),
            cells: this.components.getComponents('cell').map(c => c.getSpec()),
        };
    }
}
