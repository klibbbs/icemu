import { Pin } from './pin.mjs';
import { Load } from './load.mjs';
import { Transistor } from './transistor.mjs';

export class Layout {

    constructor(spec, options) {
        this.spec = spec;

        // --- Build components ---

        // Device parameters
        this.type = spec.type;
        this.args = spec.args;

        // Build pin sets
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

        // Build unique load list
        this.loads = spec.loads.map(l => Load.fromSpec(l))
            .sort((a, b) => compareLoads(a, b))
            .filter((v, i, a) => a.findIndex(l => compareLoads(v, l) === 0) === i);

        // Build unique transistor list
        this.transistors = spec.transistors.map(t => Transistor.fromSpec(t))
            .sort((a, b) => compareTransistors(a, b))
            .filter((v, i, a) => a.findIndex(t => compareTransistors(v, t) === 0) === i);


        // Build node-to-component maps
        this.buildComponentMaps();

        // --- Reduce components ---

        if (options.reduceCircuits) {
            this.circuits = spec.circuits.filter(c => c.enabled).map(c => new Layout(c, {}));

            this.circuits.forEach(circuit => {
                while (this.reduceCircuit(circuit));
            });
        }

        // --- Normalize nodes ---

        // Find all nodes in use
        const nodes = [].concat(
            ...this.pins.map(p => p.getAllNodes()),
            ...this.loads.map(l => l.getAllNodes()),
            ...this.transistors.map(t => t.getAllNodes()),
        ).filter((v, i, a) => a.indexOf(v) === i).sort((a, b) => a - b);

        if (options.reduceNodes) {

            // Map nodes to unique indices
            this.nodes = Object.fromEntries(nodes.map((node, idx) => [node, idx]));

            // Replace nodes with unique indices in each component
            this.pins.forEach(p => { p.nodes = p.nodes.map(n => this.nodes[n]) });

            this.loads.forEach(l => { l.node = this.nodes[l.node] });

            this.transistors.forEach(t => {
                t.gate = this.nodes[t.gate];
                t.channel = t.channel.map(n => this.nodes[n]);
            });
        } else {

            // Leave nodes unchanged
            this.nodes = Object.fromEntries(nodes.map(node => [node, node]));
        }

        // --- Calculate counts ---

        this.counts = {
            nodes: Math.max(...Object.values(this.nodes)) + 1,
            pins: this.pins.length,
            loads: this.loads.length,
            transistors: this.transistors.length,
        };
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
                },
                circuit: {
                    nodes: Object.fromEntries(Object.entries(state.circuit.nodes)),
                    loads: Object.fromEntries(Object.entries(state.circuit.loads)),
                    transistors: Object.fromEntries(Object.entries(state.circuit.transistors)),
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

            // Check if the circuit pin is an edge node
            let edge = cpin && (cpin.type === 'src' || cpin.type === 'pin');

            // Device pins must be circuit edge pins
            if (!edge && dpin && dpin.type !== 'src') {
                return false;
            }

            // Compare load counts
            let clx = circuit.loadsByNode[cnx],
                dlx = device.loadsByNode[dnx];

            if (clx !== undefined && dlx === undefined) {
                return false;
            }

            if (!edge) {
                if (clx === undefined && dlx !== undefined) {
                    return false;
                }
            }

            // Compare transistor counts
            let ctgs = circuit.transistorsByGate[cnx],
                ctcs = circuit.transistorsByChannel[cnx],
                dtgs = device.transistorsByGate[dnx],
                dtcs = device.transistorsByChannel[dnx];

            if (ctgs && !dtgs || ctgs && dtgs.length < ctgs.length) {
                return false;
            }

            if (ctcs && !dtcs || ctcs && dtcs.length < ctcs.length) {
                return false;
            }

            if (!edge) {
                if (!ctgs && dtgs || ctgs && dtgs.length !== ctgs.length) {
                    return false;
                }

                if (!ctcs && dtcs || ctcs && dtcs.length !== ctcs.length) {
                    return false;
                }
            }

            // Update state
            let testState = copyState(state);

            testState.circuit.nodes[cnx] = dnx;
            testState.device.nodes[dnx] = cnx;

            // Match loads
            if (clx) {
                if (testState = findLoad(clx, [dlx], testState)) {
                    // Continue
                } else {
                    return false;
                }
            }

            // Match transistors
            if (ctgs) {
                if (!ctgs.every(ctx => {
                    if (testState = findTransistor(ctx, dtgs, testState)) {
                        return true;
                    } else {
                        return false;
                    }
                })) {
                    return false;
                }
            }

            if (ctcs) {
                if (!ctcs.every(ctx => {
                    if (testState = findTransistor(ctx, dtcs, testState)) {
                        return true;
                    } else {
                        return false;
                    }
                })) {
                    return false;
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
            let cl = circuit.loads[clx],
                dl = device.loads[dlx];

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
            let ct = circuit.transistors[ctx],
                dt = device.transistors[dtx];

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

        // Match
        let state = {
            device: {
                nodes: {},
                loads: {},
                transistors: {},
            },
            circuit: {
                nodes: {},
                loads: {},
                transistors: {},
            },
        };

        if (!circuit.transistors.every((ct, ctx) => {
            if (state = findTransistor(ctx, Array.from(device.transistors.keys()), state)) {
                return true;
            } else {
                return false;
            }
        })) {
            return false;
        }

        if (!circuit.loads.every((cl, clx) => {
            if (state = findLoad(clx, Array.from(device.loads.keys()), state)) {
                return true;
            } else {
                return false;
            }
        })) {
            return false;
        }

        // Create a new component
        const args = circuit.args.map((arg, idx) => {
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
        });

        switch (circuit.type) {
            default:
                throw new Error(`Unsupported circuit type '${circuit.type}'`);
        }

        console.log(`Reducing '${circuit.type}' circuit...`);

        // Reduce components within circuit
        Object.values(state.circuit.loads).forEach(dlx => {
            device.loads[dlx].reduced = true;
        });

        Object.values(state.circuit.transistors).forEach(dtx => {
            device.transistors[dtx].reduced = true;
        });

        // Remove reduced components
        device.loads = device.loads.filter(l => !l.reduced);
        device.transistors = device.transistors.filter(t => !t.reduced);

        // Rebuild node-to-component maps in the main device
        device.buildComponentMaps();

        return true;
    }

    buildComponentMaps() {
        function appendToMap(map, n, idx) {
            if (map[n]) {
                map[n].push(idx);
            } else {
                map[n] = [idx];
            }
        }

        // Build node-to-component maps
        this.pinsByNode = this.pins.reduce((map, p, px) => {
            p.nodes.forEach(n => {
                map[n] = px;
            });

            return map;
        }, {});

        this.loadsByNode = Object.fromEntries(this.loads.map((l, lx) => [l.node, lx]));

        this.transistorsByGate = this.transistors.reduce((map, t, tx) => {
            appendToMap(map, t.gate, tx);

            return map;
        }, {});

        this.transistorsByChannel = this.transistors.reduce((map, t, tx) => {
            appendToMap(map, t.channel[0], tx);
            appendToMap(map, t.channel[1], tx);

            return map;
        }, {});
    }

    printInfo() {
        console.log(`Nodes:       ${this.counts.nodes}`);
        console.log(`Pins:        ${this.counts.pins}`);
        console.log(`Loads:       ${this.counts.loads}`);
        console.log(`Transistors: ${this.counts.transistors}`);
    }

    buildSpec() {
        return {
            id: this.spec.id,
            name: this.spec.name,
            type: this.spec.type,
            enabled: this.spec.enabled,
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
            loads: this.loads.map(l => l.getSpec()),
            transistors: this.transistors.map(t => t.getSpec()),
        };
    }
}

// --- Comparators ---

function compareArrays(a, b) {
    for (let i = 0; i < a.length && i < b.length; i++) {
        if (a[i] === b[i]) {
            continue;
        } else {
            return a[i] - b[i];
        }
    }

    return a.length - b.length;
}

function compareLoads(a, b) {
    return a.type.localeCompare(b.type) || a.node - b.node;
}

function compareTransistors(a, b) {
    return a.type.localeCompare(b.type) ||
        a.gate - b.gate ||
        compareArrays(a.channel, b.channel);
}
