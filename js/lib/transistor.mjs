import { Validator } from './validator.mjs'

export class Transistor {

    constructor(type, gate, channel) {
        this.type = type;
        this.gate = gate;
        this.channel = channel.sort((a, b) => a - b);
    }

    static fromSpec(spec) {
        return new Transistor(spec[0], spec[1], [spec[2], spec[3]]);
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (field, val) => Validator.validateEnum(field, val, ['nmos', 'pmos']),
            Validator.validateNode,
            Validator.validateNode,
            Validator.validateNode,
        ]);
    }

    getAllNodes() {
        return [this.gate, this.channel[0], this.channel[1]];
    }

    getSpec() {
        return [this.type, this.gate, this.channel[0], this.channel[1]];
    }
}
