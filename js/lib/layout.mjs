import { Pin } from './pin.mjs';
import { Load } from './load.mjs';
import { Transistor } from './transistor.mjs';

export class Layout {

    constructor(spec, options) {

        // --- Build components ---

        // Device properties
        this.type = spec.type;

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
        this.loads = spec.loads.map(l => new Load(...l))
            .sort((a, b) => compareLoads(a, b))
            .filter((v, i, a) => a.findIndex(l => compareLoads(v, l) === 0) === i);

        // Build unique transistor list
        this.transistors = spec.transistors.map(t => new Transistor(t[0], t[1], [t[2], t[3]]))
            .sort((a, b) => compareTransistors(a, b))
            .filter((v, i, a) => a.findIndex(t => compareTransistors(v, t) === 0) === i);

        // Build unique latch list
        this.latches = spec.latches.map(d => new Latch())
            .sort((a, b) => compareLatches(a, b))
            .filter((v, i, a) => a.findIndex(d => compareLatches(v, d) === 0) === i);

        // Build node-to-component maps
        this.buildComponentMaps();

        // --- Reduce components ---

        if (options.reduceTransistors) {
            while (this.reduceTransistors());
        }

        if (options.reduceCircuits) {
            spec.circuits.map(c => new Layout(c, {})).forEach(circuit => {
                while (this.reduceCircuit(circuit));
            });
        }

        // --- Normalize nodes ---

        // Map nodes to unique indices
        const nodeIds = [].concat(
            ...this.pins.map(p => p.nodes),
            this.loads.map(l => l.node),
            ...this.transistors.filter(t => !t.reduced).map(t => t.channel.concat(t.gates)),
            ...this.latches.map(d => [d.data, d.out, d.nout]),
        ).filter((v, i, a) => a.indexOf(v) === i).sort((a, b) => a - b);

        this.nodes = Object.fromEntries(nodeIds.map((node, idx) => [node, idx]));

        // Replace nodes with unique indices in each component
        this.pins.forEach(p => { p.nodes = p.nodes.map(n => this.nodes[n]) });
        this.loads.forEach(l => { l.node = this.nodes[l.node] });
        this.transistors.forEach(t => {
            t.gates = t.gates.map(n => this.nodes[n]);
            t.channel = t.channel.map(n => this.nodes[n]);
        });
        this.latches.forEach(d => {
            d.data = this.nodes[d.data];
            d.out = this.nodes[d.out];
            d.nout = this.nodes[d.nout];
        });

        // --- Build gates ---

        // Find unique gate sets
        this.gates = this.transistors.map(t => t.gates)
            .filter((v, i, a) => a.findIndex(g => compareArrays(v, g) === 0) === i)
            .sort((a, b) => compareArrays(a, b));

        // Map transistors to gate sets
        this.transistors.forEach(t => {
            t.gatesId = this.gates.findIndex(g => compareArrays(t.gates, g) === 0);
        });

        this.transistors.sort((a, b) => compareTransistors(a, b));

        // --- Calculate counts ---

        this.counts = {
            nodes: Object.keys(this.nodes).length,
            pins: this.pins.length,
            loads: this.loads.length,
            transistors: this.transistors.length,
            gates: this.transistors.reduce((total, t) => total + t.gates.length, 0),
            latches: this.latches.length,
        };

        this.gatesWidth = Math.max(...this.gates.map(g => g.length));
    }

    reduceTransistors() {

        // Reduce self-connected transistors
        this.transistors.forEach(t => {
            if (t.channel[0] === t.channel[1]) {
                t.reduced = true;
            }
        });

        // Reduce parallel transistors
        this.transistors.forEach((t, i, a) => {
            if (t.reduced || t.topology === 'series') {
                return;
            }

            this.transistorsByChannel[t.channel[0]].forEach(i2 => {
                let t2 = this.transistors[i2];

                if (i === i2 || t2.reduced || t2.topology === 'series') {
                    return;
                }

                if (t.channel[1] === t2.channel[1] && t.type === t2.type) {
                    t.topology = 'parallel';
                    t.gates = t.gates.concat(t2.gates);
                    t2.reduced = true;
                }
            });
        });

        // Reduce series transistors
        this.transistors.forEach((t, i, a) => {
            if (t.reduced || t.topology === 'parallel') {
                return;
            }

            t.channel.forEach(n => {
                if (this.pinsByNode[n] ||
                    this.loadsByNode[n] ||
                    this.transistorsByGate[n]) {
                    return;
                }

                if (this.transistorsByChannel[n].length !== 2) {
                    return;
                }

                let t2 = this.transistors[this.transistorsByChannel[n].filter(i2 => i2 !== i)[0]];

                if (t2.reduced || t2.topology === 'parallel') {
                    return;
                }

                const n1 = t.channel.filter(c => c !== n)[0];
                const n2 = t2.channel.filter(c => c !== n)[0];

                t.topology = 'series';
                t.gates = t.gates.concat(t2.gates);
                t.channel = [n1, n2].sort((a, b) => a - b);
                t2.reduced = true;
            });
        });

        // Remove reduced transistors
        let reduced = false;

        this.transistors = this.transistors.filter(t => {
            if (t.reduced) {
                reduced = true;
            }

            return !t.reduced;
        });

        // Rebuild node-to-component maps
        this.buildComponentMaps();

        return reduced;
    }

    reduceCircuit(circuit) {
        const device = this;

        // State must be immutable
        function copyState(state) {
            return {
                device: {
                    loads: Object.fromEntries(Object.entries(state.device.loads)),
                    transistors: Object.fromEntries(Object.entries(state.device.transistors)),
                },
                circuit: {
                    loads: Object.fromEntries(Object.entries(state.circuit.loads)),
                    transistors: Object.fromEntries(Object.entries(state.circuit.transistors)),
                },
            };
        }

        // Component matchers
        function matchNodes(cnx, dnx, state) {

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
            let edge = cpin && cpin.type === 'src' || cpin.type === 'pin';

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

            // Match loads
            if (clx) {
                if (state = findLoad(clx, [dlx], state)) {
                    // Continue
                } else {
                    return false;
                }
            }

            // Match transistors
            if (ctgs) {
                if (!ctgs.every(ctx => {
                    if (state = findTransistor(ctx, dtgs, state)) {
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
                    if (state = findTransistor(ctx, dtcs, state)) {
                        return true;
                    } else {
                        return false;
                    }
                })) {
                    return false;
                }
            }

            return state;
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

            testState = matchNodes(ct.gates[0], dt.gates[0], testState);

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
                loads: {},
                transistors: {},
            },
            circuit: {
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

        // Create new component
        if (circuit.type === 'latch') {
            // TODO
            console.log('Found latch');
        } else {
            throw new Error(`Unsupported circuit type '${circuit.type}'`);
        }

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
            t.gates.forEach(g => appendToMap(map, g, tx));

            return map;
        }, {});

        this.transistorsByChannel = this.transistors.reduce((map, t, tx) => {
            t.channel.forEach(c => appendToMap(map, c, tx));

            return map;
        }, {});
    }

    printInfo() {
        console.log(`Nodes:       ${this.counts.nodes}`);
        console.log(`Pins:        ${this.counts.pins}`);
        console.log(`Loads:       ${this.counts.loads}`);
        console.log(`Transistors: ${this.counts.transistors}`);
        console.log(`Gates:       ${this.counts.gates}`);
        console.log(`Latches:     ${this.counts.latches}`);
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
        a.gatesId - b.gatesId ||
        compareArrays(a.gates, b.gates) ||
        compareArrays(a.channel, b.channel);
}

function compareLatches(a, b) {
    return a.in - b.in ||
        a.out - b.out ||
        a.nout - b.nout;
}
