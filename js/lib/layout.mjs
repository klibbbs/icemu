export class Layout {

    MAX_WIDTH = 16;

    constructor(spec, options) {

        // --- Build components ---

        // Build source pins
        this.on = buildPin(spec.on, spec.nodeNames[spec.on], 'src', false, false);
        this.off = buildPin(spec.off, spec.nodeNames[spec.off], 'src', false, false);

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
                ...pins_rw_hex.map(n => buildPin(n, spec.nodeNames[n], 'pin', false, true)),
                ...pins_rw_bin.map(n => buildPin(n, spec.nodeNames[n], 'pin', true, true)),
                ...pins_ro_hex.map(n => buildPin(n, spec.nodeNames[n], 'pin', false, false)),
                ...pins_ro_bin.map(n => buildPin(n, spec.nodeNames[n], 'pin', true, false)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
            ...[
                ...regs_ro_hex.map(n => buildPin(n, spec.nodeNames[n], 'reg', false, false)),
                ...regs_ro_bin.map(n => buildPin(n, spec.nodeNames[n], 'reg', true, false)),
            ].sort((a, b) => a.id.localeCompare(b.id)),
        );

        // Build unique load list
        this.loads = spec.loads.map(l => buildLoad(l))
            .sort((a, b) => compareLoads(a, b))
            .filter((v, i, a) => a.findIndex(l => compareLoads(v, l) === 0) === i);

        // Build unique transistor list
        this.transistors = spec.transistors.map(t => buildTransistor(t))
            .sort((a, b) => compareTransistors(a, b))
            .filter((v, i, a) => a.findIndex(t => compareTransistors(v, t) === 0) === i);

        // --- Reduce components ---

        if (options.reduceTransistors) {
            while (this.reduceTransistors());
        }

        // --- Normalize nodes ---

        // Map nodes to unique indices
        const nodeIds = [].concat(
            this.on.nodes,
            this.off.nodes,
            ...this.pins.map(p => p.nodes),
            this.loads.map(l => l.node),
            ...this.transistors.filter(t => !t.reduced).map(t => t.channel.concat(t.gates)),
        ).filter((v, i, a) => a.indexOf(v) === i).sort((a, b) => compareScalar(a, b));

        this.nodes = Object.fromEntries(nodeIds.map((node, idx) => [node, idx]));

        // Replace nodes with indices in pins, loads, and transistors
        this.pins.forEach(p => { p.nodes = p.nodes.map(n => this.nodes[n]) });
        this.on.nodes = this.on.nodes.map(n => this.nodes[n]);
        this.off.nodes = this.off.nodes.map(n => this.nodes[n]);
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

        // Reverse-map nodes to components
        const pinsByNode = this.pins.reduce((map, p, i) => {
            p.nodes.forEach(n => {
                if (map[n]) {
                    map[n].push(i);
                } else {
                    map[n] = [i];
                }
            });

            return map;
        }, {});

        const loadsByNode = Object.fromEntries(this.loads.map((l, i) => [l.node, i]));

        const transistorsByGate = this.transistors.reduce((map, t, i) => {
            t.gates.forEach(g => {
                if (map[g]) {
                    map[g].push(i);
                } else {
                    map[g] = [i];
                }
            });

            return map;
        }, {});

        const transistorsByChannel = this.transistors.reduce((map, t, i) => {
            t.channel.forEach(c => {
                if (map[c]) {
                    map[c].push(i);
                } else {
                    map[c] = [i];
                }
            });

            return map;
        }, {});

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

            transistorsByChannel[t.channel[0]].forEach(i2 => {
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
                if (this.on.nodes[0] === n ||
                    this.off.nodes[0] === n ||
                    pinsByNode[n] ||
                    loadsByNode[n] ||
                    transistorsByGate[n]) {
                    return;
                }

                if (transistorsByChannel[n].length !== 2) {
                    return;
                }

                let t2 = this.transistors[transistorsByChannel[n].filter(i2 => i2 !== i)[0]];

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

        return reduced;
    }

    printInfo() {
        console.log(`Nodes:       ${this.counts.nodes}`);
        console.log(`Pins:        ${this.counts.pins}`);
        console.log(`Loads:       ${this.counts.loads}`);
        console.log(`Transistors: ${this.counts.transistors}`);
        console.log(`Gates:       ${this.counts.gates}`);
    }
}

// --- Builders ---

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

function buildLoad(tuple) {
    return {
        type: tuple[0],
        node: tuple[1],
    };
}

function buildTransistor(tuple) {
    return {
        type: tuple[0],
        topology: 'single',
        gates: [tuple[1]],
        gatesId: null,
        channel: [tuple[2], tuple[3]].sort((a, b) => a - b),
    };
}

// --- Comparators ---

function compareScalar(a, b) {
    if (typeof(a) === 'number' && typeof(b) === 'number') {
        return a - b;
    } else {
        return String(a).localeCompare(String(b));
    }
}

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
