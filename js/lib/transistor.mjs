export class Transistor {

    constructor(type, gate, channel) {
        this.type = type;
        this.topology = 'single',
        this.gates = [gate];
        this.gatesId = null;
        this.channel = channel.sort((a, b) => a - b);
    }
}
