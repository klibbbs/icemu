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
                ...pins_rw_hex.map(n => buildPin(n, this.nodeNames[n], 'pin', false, true)),
                ...pins_rw_bin.map(n => buildPin(n, this.nodeNames[n], 'pin', true, true)),
                ...pins_ro_hex.map(n => buildPin(n, this.nodeNames[n], 'pin', false, false)),
                ...pins_ro_bin.map(n => buildPin(n, this.nodeNames[n], 'pin', true, false)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
            ...[
                ...regs_ro_hex.map(n => buildPin(n, this.nodeNames[n], 'reg', false, false)),
                ...regs_ro_bin.map(n => buildPin(n, this.nodeNames[n], 'reg', true, false)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
        );

        // Build source pins
        this.on = buildPin(spec.on, this.nodeNames[spec.on], 'src', false, false);
        this.off = buildPin(spec.off, this.nodeNames[spec.off], 'src', false, false);

        // Build loads
        this.loads = spec.loads.map(l => buildLoad(l, this.nodesById));

        // Build transistors
        this.transistors = spec.transistors.map(t => buildTransistor(t, this.nodesById));
    }

    printInfo() {
        console.log(`Nodes:       ${this.nodeIds.length}`);
        console.log(`Pins:        ${this.pins.length}`);
        console.log(`Loads:       ${this.loads.length}`);
        console.log(`Transistors: ${this.transistors.length}`);
    }
}

// --- Constructors ---

function buildPin(name, nodes, type, binary, writable) {
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

function buildLoad(tuple, nodesById) {
    return {
        type: tuple[0],
        node: nodesById[tuple[1]],
    };
}

function buildTransistor(tuple, nodesById) {
    return {
        type: tuple[0],
        gate: nodesById[tuple[1]],
        channel: [nodesById[tuple[2]], nodesById[tuple[3]]], // TODO: Sort
    };
}
