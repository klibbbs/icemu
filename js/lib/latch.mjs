export class Latch {

    constructor(type, out, nout, data) {
        this.type = type;
        this.out = out;
        this.nout = nout;
        this.data = data;
    }

    static fromSpec(spec) {
        return new Latch(spec[0], spec[1], spec[2], spec[3]);
    }

    getAllNodes() {
        return [this.out, this.nout, this.data];
    }

    getSpec() {
        return [this.type, this.out, this.nout, this.data];
    }
}
