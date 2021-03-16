export class Layout {

    MAX_WIDTH = 16;

    constructor(spec) {

        // Normalize nodes
        this.nodeIds = spec.nodeIds.sort((a, b) => {
            if (typeof(a) === 'number' && typeof(b) === 'number') {
                return a - b;
            } else {
                return String(a).localeCompare(String(b));
            }
        });

        this.nodesById = Object.fromEntries(Object.entries(this.nodeIds).map(([idx, elem]) => {
            return [elem, idx];
        }));

        this.nodeNames = Object.fromEntries(Object.entries(spec.nodeNames).map(([name, nodes]) => {
            return [name, nodes.map(n => this.nodesById[n])];
        }));

        this.nodeCount = this.nodeIds.length;

        // Build memory model
        this.memory = {
            word: spec.memory.word,
            word_type: Math.pow(2, Math.ceil(Math.max(0, Math.log2(spec.memory.word / 8)))) * 8,
            addr: spec.memory.address,
            addr_type: Math.pow(2, Math.ceil(Math.max(0, Math.log2(spec.memory.address / 8)))) * 8,
            count: Math.pow(2, spec.memory.address) * 8 / spec.memory.word,
        };

        // Build pin sets
        const srcs = [spec.on, spec.off];
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
            ...[
                ...pins_rw_hex.map(n => this.buildPin(n, this.nodeNames[n], 'pin', false, true)),
                ...pins_rw_bin.map(n => this.buildPin(n, this.nodeNames[n], 'pin', true, true)),
                ...pins_ro_hex.map(n => this.buildPin(n, this.nodeNames[n], 'pin', false, false)),
                ...pins_ro_bin.map(n => this.buildPin(n, this.nodeNames[n], 'pin', true, false)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
            ...[
                ...regs_ro_hex.map(n => this.buildPin(n, this.nodeNames[n], 'reg', false, false)),
                ...regs_ro_bin.map(n => this.buildPin(n, this.nodeNames[n], 'reg', true, false)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
        );

        // Build source pins
        this.on = this.buildPin(spec.on, this.nodeNames[spec.on], 'src', false, false);
        this.off = this.buildPin(spec.off, this.nodeNames[spec.off], 'src', false, false);

        // Build unique load list
        this.loads = spec.loads.map(l => this.buildLoad(l))
            .sort((a, b) => this.compareLoads(a, b))
            .filter((v, i, a) => a.findIndex(l => this.compareLoads(v, l) === 0) === i);

        // Build unique transistor list
        this.transistors = spec.transistors.map(t => this.buildTransistor(t))
            .sort((a, b) => this.compareTransistors(a, b))
            .filter((v, i, a) => a.findIndex(t => this.compareTransistors(v, t) === 0) === i);
    }

    printInfo() {
        console.log(`Nodes:       ${this.nodeIds.length}`);
        console.log(`Pins:        ${this.pins.length}`);
        console.log(`Loads:       ${this.loads.length}`);
        console.log(`Transistors: ${this.transistors.length}`);
    }

    // --- Pin ---

    buildPin(name, nodes, type, binary, writable) {
        const len = nodes.length,
              bits = (len === 1) ? 1 : Math.pow(2, Math.ceil(Math.max(0, Math.log2(len / 8)))) * 8;

        if (len < 1) {
            throw new TypeError(`Node set for pin '${name}' cannot be empty`);
        } else if (bits > Layout.MAX_WIDTH) {
            throw new TypeError(
                `Pin ${name} with ${bits} bits is wider than supported max of ${Layout.MAX_WIDTH}`);
        }

        return {
            id: name,
            name: `${name}.${type}`,
            type: type,
            nodes: nodes,
            bits: bits,
            base: (bits === 1) ? 10 : binary ? 2 : 16,
            writable: writable,
        };
    }

    // --- Load ---

    buildLoad(tuple) {
        return {
            type: tuple[0],
            node: this.nodesById[tuple[1]],
        };
    }

    compareLoads(a, b) {
        return a.type.localeCompare(b.type) ||
            a.node - b.node;
    }

    // --- Transistor ---

    buildTransistor(tuple) {
        return {
            type: tuple[0],
            topology: 'single',
            gates: [this.nodesById[tuple[1]]],
            channel: [this.nodesById[tuple[2]], this.nodesById[tuple[3]]].sort((a, b) => a - b),
        };
    }

    compareTransistors(a, b) {
        return a.type.localeCompare(b.type) ||
            this.compareGates(a.gates, b.gates) ||
            a.channel[0] - b.channel[0] ||
            a.channel[1] - b.channel[1];
    }

    findParallelTransistor(t) {
        return this.transistors.findIndex(v => {
            return v.topology !== 'series' &&
                v.channel[0] === t.channel[0] &&
                v.channel[1] === t.channel[1];
        });
    }

    // --- Gate ---

    compareGates(a, b) {
        for (let g = 0; g < a.length && g < b.length; g++) {
            if (a[g] === b[g]) {
                continue;
            } else {
                return a[g] - b[g];
            }
        }

        return a.length - b.length;
    }
}
