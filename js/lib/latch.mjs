export class Latch {

    constructor(type, logic, out, nout, data) {
        this.type = type;
        this.logic = logic;
        this.out = out;
        this.nout = nout;
        this.data = data;
    }

    getAllNodes() {
        return [this.out, this.nout, this.data];
    }
}
