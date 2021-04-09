import { Validator } from './validator.mjs';

export class Load {

    constructor(type, node) {
        this.type = type;
        this.node = node;
    }

    static compare(a, b) {
        return a.type.localeCompare(b.type) || a.node - b.node;
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

    static getArgs() {
        return ['node'];
    }

    getSpec() {
        return [this.type, this.node];
    }

    getAllNodes() {
        return [this.node];
    }

    getArgNodes() {
        return {
            node: [this.node],
        };
    }
}
