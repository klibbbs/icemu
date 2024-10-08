export class State {

    constructor(types) {
        this.types = types;

        this.numNodes = 0;
        this.numComponents = 0;

        this.circuit = {
            nodes: {},
            components: {},
        };

        this.device = {
            nodes: {},
            components: {},
        };

        types.forEach(type => {
            this.circuit.components[type] = {};
            this.device.components[type] = {};
        });
    }

    isEmpty() {
        return this.numNodes === 0 && this.numComponents === 0;
    }

    copyWithNode(cx, dx) {
        let copy = new State(this.types);

        // Use existing component maps
        copy.circuit.components = this.circuit.components;
        copy.device.components = this.device.components;
        copy.numComponents = this.numComponents;

        // Copy node maps
        copy.circuit.nodes = {...this.circuit.nodes};
        copy.device.nodes = {...this.device.nodes};

        // Add node
        copy.circuit.nodes[cx] = dx;
        copy.device.nodes[dx] = cx;
        copy.numNodes = this.numNodes + 1;

        return copy;
    }

    copyWithComponent(type, cx, dx) {
        let copy = new State(this.types);

        // Use existing node maps
        copy.circuit.nodes = this.circuit.nodes;
        copy.device.nodes = this.device.nodes;
        copy.numNodes = this.numNodes;

        // Copy component maps
        this.types.forEach(type => {
            copy.circuit.components[type] = {...this.circuit.components[type]};
            copy.device.components[type] = {...this.device.components[type]};
        });

        // Add component
        copy.circuit.components[type][cx] = dx;
        copy.device.components[type][dx] = cx;
        copy.numComponents = this.numComponents + 1;

        return copy;
    }

    getCircuitNode(cx) {
        return this.circuit.nodes[cx];
    }

    getDeviceComponents(type) {
        return Object.values(this.circuit.components[type]);
    }

    doNodesMatch(cx, dx) {
        return this.circuit.nodes[cx] === dx && this.device.nodes[dx] === cx;
    }

    doNodesConflict(cx, dx) {
        if (this.doNodesMatch(cx, dx)) {
            return false;
        }

        return this.circuit.nodes[cx] !== undefined || this.device.nodes[dx] !== undefined;
    }

    doComponentsMatch(type, cx, dx) {
        return this.circuit.components[type][cx] === dx && this.device.components[type][dx] === cx;
    }

    doComponentsConflict(type, cx, dx) {
        if (this.doComponentsMatch(type, cx, dx)) {
            return false;
        }

        return this.circuit.components[type][cx] !== undefined ||
            this.device.components[type][dx] !== undefined;
    }
}
