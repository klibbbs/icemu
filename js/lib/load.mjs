import { Validator } from './validator.mjs'

export class Load {

    constructor(type, node) {
        this.type = type;
        this.node = node;
    }

    static fromSpec(spec) {
        return new Load(spec[0], spec[1]);
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (field, val) => Validator.validateEnum(field, val, ['on', 'off']),
            Validator.validateNode,
        ]);
    }

    getAllNodes() {
        return [this.node];
    }

    getSpec() {
        return [this.type, this.node];
    }
}
