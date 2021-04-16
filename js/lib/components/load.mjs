import { Validator } from '../validator.mjs';

export class Load {

    constructor(idx, type, node) {
        this.idx = idx;
        this.type = type;
        this.node = node;
    }

    static compare(a, b) {
        return a.type.localeCompare(b.type) || a.node - b.node;
    }

    static compatible(a, b) {
        return a.type === b.type;
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (field, val) => Validator.validateEnum(field, val, ['on', 'off']),
            Validator.validateNode,
        ]);
    }

    static getGroups() {
        return ['node'];
    }

    getSpec() {
        return [this.type, this.node];
    }

    getAllNodes() {
        return [this.node];
    }

    getGroupNodes(group) {
        return {
            node: [this.node],
        }[group];
    }
}
