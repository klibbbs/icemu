import { Bits } from './bits.mjs';

export class Pin {

    static ON = 'on';
    static OFF = 'off';
    static READ_ONLY = 'ro';
    static READ_WRITE = 'rw';

    constructor(id, type, nodes, mode, base) {
        const bits = (nodes.length === 1) ? 1 : Bits.roundUpToType(nodes.length);

        this.id = id;
        this.type = type;
        this.name = `${id}.${type}`;
        this.nodes = nodes;
        this.bits = bits;
        this.base = base ? base : bits === 1 ? 10 : 16;
        this.mode = mode;
        this.readable = (mode === Pin.READ_ONLY) || (mode === Pin.READ_WRITE);
        this.writable = (mode === Pin.READ_WRITE);
    }

    getAllNodes() {
        return this.nodes;
    }
}
