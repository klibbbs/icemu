import { Pin } from './pin.mjs';
import { Load } from './load.mjs';
import { Transistor } from './transistor.mjs';

export class Layout {

    constructor(spec, options) {

        // --- Build components ---

        // Build pin sets
        this.on = spec.on ? new Pin(spec.on, 'src', spec.nodeNames[spec.on], Pin.NONE) : null;
        this.off = spec.off ? new Pin(spec.off, 'src', spec.nodeNames[spec.off], Pin.NONE) : null;

        const pins = [].concat(spec.inputs, spec.outputs).filter((v, i, a) => a.indexOf(v) === i);
        const regs = spec.registers.filter((v, i, a) => a.indexOf(v) === i);

        const pins_rw = pins.filter(n => spec.inputs.includes(n));
        const pins_ro = pins.filter(n => !spec.inputs.includes(n));
        const regs_ro = regs.filter(n => !pins.includes(n));

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

        // Build node-to-component maps
        this.buildComponentMaps();

        // --- Reduce components ---

        if (options.reduceTransistors) {
            while (this.reduceTransistors());
        }

        // --- Normalize nodes ---

        // Map nodes to unique indices
        const nodeIds = [].concat(
            ...this.pins.map(p => p.nodes),
            this.loads.map(l => l.node),
            ...this.transistors.filter(t => !t.reduced).map(t => t.channel.concat(t.gates)),
        ).filter((v, i, a) => a.indexOf(v) === i).sort((a, b) => a - b);

        this.nodes = Object.fromEntries(nodeIds.map((node, idx) => [node, idx]));

        // Replace nodes with indices in pins, loads, and transistors
        this.pins.forEach(p => { p.nodes = p.nodes.map(n => this.nodes[n]) });
        this.loads.forEach(l => { l.node = this.nodes[l.node] });
        this.transistors.forEach(t => {
            t.gates = t.gates.map(n => this.nodes[n]);
            t.channel = t.channel.map(n => this.nodes[n]);
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
        return false;
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
        this.pinsByNode = this.pins.reduce((map, p, idx) => {
            p.nodes.forEach(n => appendToMap(map, n, idx));

            return map;
        }, {});

        this.loadsByNode = Object.fromEntries(this.loads.map((l, idx) => [l.node, idx]));

        this.transistorsByGate = this.transistors.reduce((map, t, idx) => {
            t.gates.forEach(g => appendToMap(map, g, idx));

            return map;
        }, {});

        this.transistorsByChannel = this.transistors.reduce((map, t, idx) => {
            t.channel.forEach(c => appendToMap(map, c, idx));

            return map;
        }, {});
    }

    printInfo() {
        console.log(`Nodes:       ${this.counts.nodes}`);
        console.log(`Pins:        ${this.counts.pins}`);
        console.log(`Loads:       ${this.counts.loads}`);
        console.log(`Transistors: ${this.counts.transistors}`);
        console.log(`Gates:       ${this.counts.gates}`);
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
