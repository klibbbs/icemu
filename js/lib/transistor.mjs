export class Transistor {

    constructor(type, gate, channel) {
        this.type = type;
        this.topology = 'single',
        this.gate = gate;
        this.channel = channel.sort((a, b) => a - b);
    }

    getAllNodes() {
        return [this.gate, this.channel[0], this.channel[1]];
    }
}
