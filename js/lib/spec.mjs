import { Validator } from './validator.mjs';
import { Components } from './components.mjs';
import { Load } from './load.mjs';
import { Transistor } from './transistor.mjs';
import { Buffer } from './buffer.mjs';
import { Function } from './function.mjs';
import { Cell } from './cell.mjs';

export class Spec {

    constructor(spec) {
        this.type = Validator.validateEnum('type', spec.type, [
            'device',
            'reduce',
            ...Components.getTypes(),
        ]);

        // Component info
        this.id = Validator.validateIdentifier('id', spec.id);
        this.name = Validator.validateString('name', spec.name);

        if (this.type === 'device') {

            // Memory model
            if (spec.memory) {
                this.memory = Validator.validateStruct('memory', spec.memory, {
                    word: (field, val) => Validator.validateWidth(field, val),
                    address: (field, val) => Validator.validateInt(field, val, 1),
                })
            }
        } else {

            // Circuit parameters
            this.enabled = spec.enabled === false ? false : true;
            this.limit = spec.limit;
            this.args = Validator.validateArray('args', spec.args, Validator.validateAny);
        }

        // Node names
        this.nodeNames = Validator.validateMap(
            'nodes',
            spec.nodes,
            Validator.validateIdentifier,
            Validator.validateNodeSet);

        let nodeMap = {};

        for (const [name, nodes] of Object.entries(this.nodeNames)) {
            for (const node of nodes) {
                const usedName = nodeMap[node];

                if (usedName === undefined) {
                    nodeMap[node] = name;
                } else {
                    throw new TypeError(`Node index ${node} cannot have multiple names`);
                }
            }
        }

        // Node assignments
        if (spec.on) {
            this.on = Validator.validateSingleNodeName('on', spec.on, this.nodeNames);
        }

        if (spec.off) {
            this.off = Validator.validateSingleNodeName('off', spec.off, this.nodeNames);
        }

        // Pin sets
        if (spec.inputs) {
            this.inputs = Validator.validateArray('inputs', spec.inputs, (field, val) => {
                return Validator.validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.inputs = [];
        }

        if (spec.outputs) {
            this.outputs = Validator.validateArray('outputs', spec.outputs, (field, val) => {
                return Validator.validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.outputs = [];
        }

        if (spec.registers) {
            this.registers = Validator.validateArray('registers', spec.registers, (field, val) => {
                return Validator.validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.registers = [];
        }

        if (spec.flags) {
            this.flags = Validator.validateArray('flags', spec.flags, (field, val) => {
                return Validator.validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.flags = [];
        }

        // Components
        if (spec.loads) {
            this.loads = Validator.validateArray(
                'loads',
                spec.loads,
                Load.validateSpec
            );
        } else {
            this.loads = [];
        }

        if (spec.transistors) {
            this.transistors = Validator.validateArray(
                'transistors',
                spec.transistors,
                Transistor.validateSpec
            );
        } else {
            this.transistors = [];
        }

        if (spec.buffers) {
            this.buffers = Validator.validateArray(
                'buffers',
                spec.buffers,
                Buffer.validateSpec
            );
        } else {
            this.buffers = [];
        }

        if (spec.functions) {
            this.functions = Validator.validateArray(
                'functions',
                spec.functions,
                Function.validateSpec
            );
        } else {
            this.functions = [];
        }

        if (spec.cells) {
            this.cells = Validator.validateArray(
                'cells',
                spec.cells,
                Cell.validateSpec
            );
        } else {
            this.cells = [];
        }

        // Circuits
        if (spec.circuits) {
            this.circuits = Validator.validateArray('circuits', spec.circuits, (field, val) => {
                try {
                    return new Spec(val);
                } catch (e) {
                    throw new TypeError(`${e.message} in ${field}`);
                }
            });
        } else {
            this.circuits = [];
        }

        // Collect unique list of all referenced nodes
        this.nodeIds = [].concat(
            ...Object.entries(this.nodeNames).map((name, set) => set),
            this.loads.map(([type, node]) => node),
            ...this.transistors.map(([type, gate, c1, c2]) => [gate, c1, c2]),
        ).filter((v, i, a) => a.indexOf(v) === i).sort();
    }

    printInfo() {
        console.log(`ID:          ${this.id}`);
        console.log(`Name:        ${this.name}`);
        console.log(`Nodes:       ${this.nodeIds.length}`);
        console.log(`Loads:       ${this.loads.length}`);
        console.log(`Transistors: ${this.transistors.length}`);
        console.log(`Buffers:     ${this.buffers.length}`);
        console.log(`Functions:   ${this.functions.length}`);
        console.log(`Cells:       ${this.cells.length}`);
    }
}
