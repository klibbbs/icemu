export class Load {

    constructor(type, node) {
        this.type = type;
        this.node = node;
    }

    static fromSpec(spec) {
        return new Load(spec[0], spec[1]);
    }

    getAllNodes() {
        return [this.node];
    }

    getSpec() {
        return [this.type, this.node];
    }
}
