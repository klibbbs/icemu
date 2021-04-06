export class Buffer {

    constructor(logic, inverting, input, output) {
        this.logic = logic;
        this.inverting = inverting;
        this.input = input;
        this.output = output;
    }

    static fromSpec(spec) {
        return new Buffer(...spec);
    }

    getAllNodes() {
        return [this.input, this.output];
    }

    getSpec() {
        return [this.logic, this.inverting, this.input, this.output];
    }
}
