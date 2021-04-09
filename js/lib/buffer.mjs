import { Validator } from './validator.mjs';

export class Buffer {

    constructor(logic, inverting, input, output) {
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

    static fromSpec(spec) {
        return new Buffer(...spec);
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (field, val) => Validator.validateEnum(field, val, ['nmos', 'pmos', 'cmos', 'ttl']),
            Validator.validateBool,
            Validator.validateNode,
            Validator.validateNode,
        ]);
    }

    static getArgs() {
        return ['input', 'output'];
    }

    getSpec() {
        return [this.logic, this.inverting, this.input, this.output];
    }

    getAllNodes() {
        return [this.input, this.output];
    }

    getArgNodes() {
        return {
            input: [this.input],
            output: [this.output],
        };
    }
}
