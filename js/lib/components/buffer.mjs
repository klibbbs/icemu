import { Validator } from '../validator.mjs';

export class Buffer {

    constructor(idx, logic, inverting, input, output) {
        this.idx = idx;
        this.logic = logic;
        this.inverting = inverting;
        this.input = input;
        this.output = output;
    }

    static compare(a, b) {
        return a.logic.localeCompare(b.logic) ||
            a.inverting - b.inverting ||
            a.input - b.input ||
            a.output - b.output;
    }

    static compatible(a, b) {
        return a.logic === b.logic && a.inverting === b.inverting;
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (field, val) => Validator.validateEnum(field, val, ['nmos', 'pmos', 'cmos', 'ttl']),
            Validator.validateBool,
            Validator.validateNode,
            Validator.validateNode,
        ]);
    }

    static getGroups() {
        return ['input', 'output'];
    }

    getSpec() {
        return [this.logic, this.inverting, this.input, this.output];
    }

    getAllNodes() {
        return [this.input, this.output];
    }

    getGroupNodes(group) {
        return {
            input: [this.input],
            output: [this.output],
        }[group];
    }

    remapNodes(map) {
        this.input = map[this.input];
        this.output = map[this.output];
    }
}
