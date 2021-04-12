import { Pin } from './pin.mjs';
import { Load } from './load.mjs';
import { Transistor } from './transistor.mjs';
import { Buffer } from './buffer.mjs';

import { Components } from './components.mjs';

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

        this.components.addSpecs('load', spec.loads);
        this.components.addSpecs('transistor', spec.transistors);
        this.components.addSpecs('buffer', spec.buffers);

        // --- Reduce components ---

        if (options.reduceCircuits) {
            this.circuits = spec.circuits
                .filter(c => c.enabled && !(c.limit <= 0))
                .map(c => new Layout(c, {}));

            this.circuits.forEach(circuit => {
                let limit = circuit.spec.limit ? circuit.spec.limit : Number.MAX_SAFE_INTEGER,
                    count = 0;

                process.stdout.write(`Reducing circuit '${circuit.spec.id}'...`);

                while (count < limit && this.reduceCircuit(circuit)) {
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
        };

        // --- Extract components ---

        this.loads = this.components.getComponents('load');
        this.transistors = this.components.getComponents('transistor');
        this.buffers = this.components.getComponents('buffer');
    }

    reduceCircuit(circuit) {
        const device = this;

        // State must be immutable
        function copyState(state) {
            return {
                device: {
                    nodes: Object.fromEntries(Object.entries(state.device.nodes)),
                    loads: Object.fromEntries(Object.entries(state.device.loads)),
                    transistors: Object.fromEntries(Object.entries(state.device.transistors)),
                    buffers: Object.fromEntries(Object.entries(state.device.buffers)),
                },
                circuit: {
                    nodes: Object.fromEntries(Object.entries(state.circuit.nodes)),
                    loads: Object.fromEntries(Object.entries(state.circuit.loads)),
                    transistors: Object.fromEntries(Object.entries(state.circuit.transistors)),
                    buffers: Object.fromEntries(Object.entries(state.circuit.buffers)),
                },
            };
        }

        // Component matchers
        function matchNodes(cnx, dnx, state) {

            // Check if nodes are already matched or already in use
            if (state.circuit.nodes[cnx] === dnx && state.device.nodes[dnx] === cnx) {
                return state;
            }

            if (state.circuit.nodes[cnx] !== undefined || state.device.nodes[dnx] !== undefined) {
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

            // Compare load counts
            for (const arg of Components.getArgs('load')) {
                const cls = circuit.components.getComponentsByNode('load', arg, cnx),
                      dls = device.components.getComponentsByNode('load', arg, dnx);

                if (cls && !dls || cls && dls.length < cls.length) {
                    return false;
                }

                if (internal) {
                    if (!cls && dls || cls && dls.length !== cls.length) {
                        return false;
                    }
                }
            }

            // Compare transistor counts
            for (const arg of Components.getArgs('transistor')) {
                const cts = circuit.components.getComponentsByNode('transistor', arg, cnx),
                      dts = device.components.getComponentsByNode('transistor', arg, dnx);

                if (cts && !dts || cts && dts.length < cts.length) {
                    return false;
                }

                if (internal) {
                    if (!cts && dts || cts && dts.length !== cts.length) {
                        return false;
                    }
                }
            }

            // Compare buffer counts
            for (const arg of Components.getArgs('buffer')) {
                const cbs = circuit.components.getComponentsByNode('buffer', arg, cnx),
                      dbs = device.components.getComponentsByNode('buffer', arg, dnx);

                if (cbs && !dbs || cbs && dbs.length < cbs.length) {
                    return false;
                }

                if (internal) {
                    if (!cbs && dbs || cbs && dbs.length !== cbs.length) {
                        return false;
                    }
                }
            }

            // Update state
            let testState = copyState(state);

            testState.circuit.nodes[cnx] = dnx;
            testState.device.nodes[dnx] = cnx;

            // Match loads
            for (const arg of Components.getArgs('load')) {
                const cls = circuit.components.getComponentsByNode('load', arg, cnx),
                      dls = device.components.getComponentsByNode('load', arg, dnx);

                if (cls) {
                    if (!cls.every(clx => {
                        if (testState = findLoad(clx, dls, testState)) {
                            return true;
                        } else {
                            return false;
                        }
                    })) {
                        return false;
                    }
                }
            }

            // Match transistors
            for (const arg of Components.getArgs('transistor')) {
                const cts = circuit.components.getComponentsByNode('transistor', arg, cnx),
                      dts = device.components.getComponentsByNode('transistor', arg, dnx);

                if (cts) {
                    if (!cts.every(ctx => {
                        if (testState = findTransistor(ctx, dts, testState)) {
                            return true;
                        } else {
                            return false;
                        }
                    })) {
                        return false;
                    }
                }
            }

            // Match buffers
            for (const arg of Components.getArgs('buffer')) {
                const cbs = circuit.components.getComponentsByNode('buffer', arg, cnx),
                      dbs = device.components.getComponentsByNode('buffer', arg, dnx);

                if (cbs) {
                    if (!cbs.every(cbx => {
                        if (testState = findBuffer(cbx, dbs, testState)) {
                            return true;
                        } else {
                            return false;
                        }
                    })) {
                        return false;
                    }
                }
            }

            return testState;
        }

        function matchLoads(clx, dlx, state) {

            // Check if loads are already matched or already in use
            if (state.circuit.loads[clx] === dlx && state.device.loads[dlx] === clx) {
                return state;
            }

            if (state.circuit.loads[clx] !== undefined || state.device.loads[dlx] !== undefined) {
                return false;
            }

            // Compare loads
            let cl = circuit.components.getComponent('load', clx),
                dl = device.components.getComponent('load', dlx);

            if (cl.type !== dl.type) {
                return false;
            }

            // Match nodes
            let testState = copyState(state);

            testState.circuit.loads[clx] = dlx;
            testState.device.loads[dlx] = clx;

            return matchNodes(cl.node, dl.node, testState);
        }

        function matchTransistors(ctx, dtx, state) {

            // Check if transistors are already matched or already in use
            if (state.circuit.transistors[ctx] === dtx &&
                state.device.transistors[dtx] === ctx) {

                return state;
            }

            if (state.circuit.transistors[ctx] !== undefined ||
                state.device.transistors[dtx] !== undefined) {

                return false;
            }

            // Compare transistors
            let ct = circuit.components.getComponent('transistor', ctx),
                dt = device.components.getComponent('transistor', dtx);

            if (ct.type !== dt.type) {
                return false;
            }

            // Match nodes
            let testState = copyState(state);

            testState.circuit.transistors[ctx] = dtx;
            testState.device.transistors[dtx] = ctx;

            testState = matchNodes(ct.gate, dt.gate, testState);

            if (!testState) {
                return false;
            }

            let chanState = false;

            if ((chanState = matchNodes(ct.channel[0], dt.channel[0], testState)) &&
                (chanState = matchNodes(ct.channel[1], dt.channel[1], chanState))) {

                return chanState;
            }

            if ((chanState = matchNodes(ct.channel[0], dt.channel[1], testState)) &&
                (chanState = matchNodes(ct.channel[1], dt.channel[0], chanState))) {

                return chanState;
            }

            return false;
        }

        function matchBuffers(cbx, dbx, state) {

            // Check if buffers are already matched or already in use
            if (state.circuit.buffers[cbx] === dbx &&
                state.device.buffers[dbx] === cbx) {

                return state;
            }

            if (state.circuit.buffers[cbx] !== undefined ||
                state.device.buffers[dbx] !== undefined) {

                return false;
            }

            // Compare buffers
            let cb = circuit.components.getComponent('buffer', cbx),
                db = device.components.getComponent('buffer', dbx);

            if (cb.logic !== db.logic || cb.inverting !== db.inverting) {
                return false;
            }

            // Match nodes
            let testState = copyState(state);

            testState.circuit.buffers[cbx] = dbx;
            testState.device.buffers[dbx] = cbx;

            if ((testState = matchNodes(cb.input, db.input, testState)) &&
                (testState = matchNodes(cb.output, db.output, testState))) {

                return testState;
            }

            return false;
        }

        // Component finders
        function findLoad(clx, dlxs, state) {
            for (const dlx of dlxs) {
                let newState = matchLoads(clx, dlx, state);

                if (newState) {
                    return newState;
                }
            }

            return false;
        }

        function findTransistor(ctx, dtxs, state) {
            for (const dtx of dtxs) {
                let newState = matchTransistors(ctx, dtx, state);

                if (newState) {
                    return newState;
                }
            }

            return false;
        }

        function findBuffer(cbx, dbxs, state) {
            for (const dbx of dbxs) {
                let newState = matchBuffers(cbx, dbx, state);

                if (newState) {
                    return newState;
                }
            }

            return false;
        }

        // Initialize search state
        let state = {
            device: {
                nodes: {},
                loads: {},
                transistors: {},
                buffers: {},
            },
            circuit: {
                nodes: {},
                loads: {},
                transistors: {},
                buffers: {},
            },
        };

        // Match
        if (!circuit.components.getComponents('buffer').every((cb, cbx) => {
            if (state = findBuffer(cbx, device.components.getIndices('buffer'), state)) {
                return true;
            } else {
                return false;
            }
        })) {
            return false;
        }

        if (!circuit.components.getComponents('transistor').every((ct, ctx) => {
            if (state = findTransistor(ctx, device.components.getIndices('transistor'), state)) {
                return true;
            } else {
                return false;
            }
        })) {
            return false;
        }

        if (!circuit.components.getComponents('load').every((cl, clx) => {
            if (state = findLoad(clx, device.components.getIndices('load'), state)) {
                return true;
            } else {
                return false;
            }
        })) {
            return false;
        }

        // Reduce all device components that matched the circuit
        Object.values(state.circuit.loads).forEach(dlx => {
            device.components.getComponent('load', dlx).reduced = true;
        });

        Object.values(state.circuit.transistors).forEach(dtx => {
            device.components.getComponent('transistor', dtx).reduced = true;
        });

        Object.values(state.circuit.buffers).forEach(dbx => {
            device.components.getComponent('buffer', dbx).reduced = true;
        });

        device.components.filterComponents('load', l => !l.reduced);
        device.components.filterComponents('transistor', t => !t.reduced);
        device.components.filterComponents('buffer', b => !b.reduced);

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

                    return state.circuit.nodes[pin.nodes[0]];
                } else {
                    return pin.nodes.map(n => state.circuit.nodes[n]);
                }
            } else {
                return arg;
            }
        }

        const args = circuit.args.map(parseArg);

        switch (circuit.type) {
            case 'reduce':
                // Reduce components only
                break;
            case 'load':
                device.components.addComponents('load', [new Load(...args)]);
                break;
            case 'transistor':
                device.components.addComponents('transistor', [new Transistor(...args)]);
                break;
            case 'buffer':
                device.components.addComponents('buffer', [new Buffer(...args)]);
                break;
            default:
                throw new Error(`Unsupported circuit type '${circuit.type}'`);
        }

        return true;
    }

    normalizeNodes(reduceNodes) {

        // Find all nodes in use
        const nodes = [].concat(
            ...this.pins.map(p => p.getAllNodes()),
            ...this.components.getComponents('load').map(l => l.getAllNodes()),
            ...this.components.getComponents('transistor').map(t => t.getAllNodes()),
            ...this.components.getComponents('buffer').map(b => b.getAllNodes()),
        ).filter((v, i, a) => a.indexOf(v) === i).sort((a, b) => a - b);

        if (reduceNodes) {

            // Map nodes to unique indices
            this.nodes = Object.fromEntries(nodes.map((node, idx) => [node, idx]));

            // Replace nodes with unique indices in each component
            this.pins.forEach(p => { p.nodes = p.nodes.map(n => this.nodes[n]) });

            this.components.getComponents('load').forEach(l => {
                l.node = this.nodes[l.node];
            });

            this.components.getComponents('transistor').forEach(t => {
                t.gate = this.nodes[t.gate];
                t.channel = t.channel.map(n => this.nodes[n]);
            });

            this.components.getComponents('buffer').forEach(b => {
                b.input = this.nodes[b.input];
                b.output = this.nodes[b.output];
            });
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
        };
    }
}
