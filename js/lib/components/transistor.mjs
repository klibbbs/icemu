import { Validator } from '../validator.mjs';

export class Transistor {

    constructor(idx, type, gate, drain, source) {
        this.idx = idx;
        this.type = type;
        this.gate = gate;
        this.channel = [drain, source].sort((a, b) => a - b);
    }

    static compare(a, b) {
        return a.type.localeCompare(b.type) ||
            a.gate - b.gate ||
            a.channel[0] - b.channel[0] ||
            a.channel[1] - b.channel[1];
    }

    static compatible(a, b) {
        return a.type === b.type;
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (field, val) => Validator.validateEnum(field, val, ['nmos', 'pmos']),
            Validator.validateNode,
            Validator.validateNode,
            Validator.validateNode,
        ]);
    }

    static getGroups() {
        return ['gate', 'channel'];
    }

    getSpec() {
        return [this.type, this.gate, this.channel[0], this.channel[1]];
    }

    getAllNodes() {
        return [this.gate, this.channel[0], this.channel[1]];
    }

    getGroupNodes(group) {
        return {
            gate: [this.gate],
            channel: this.channel,
        }[group];
    }
}
