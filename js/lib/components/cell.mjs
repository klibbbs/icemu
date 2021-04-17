import { Util } from '../util.mjs';
import { Validator } from '../validator.mjs';

const MAX_INPUTS = 2;
const MAX_OUTPUTS = 2;
const MAX_WRITES = 2;
const MAX_READS = 2;

const GROUPS = [
    ...[...Array(MAX_INPUTS).keys()].map(k => `input_${k + 1}`),
    ...[...Array(MAX_OUTPUTS).keys()].map(k => `output_${k + 1}`),
    ...[...Array(MAX_WRITES).keys()].map(k => `write_${k + 1}`),
    ...[...Array(MAX_READS).keys()].map(k => `read_${k + 1}`),
];

const TYPES = {
    d_latch: {
        inputs: 1,
    },
};

export class Cell {

    constructor(idx, logic, type, inputs, outputs, writes, reads) {
        this.idx = idx;
        this.logic = logic;
        this.type = type;
        this.inputs = inputs;
        this.outputs = outputs;
        this.writes = writes || [];
        this.reads = reads || [];

        if (TYPES[type] === undefined) {
            throw new Error(`Unknown cell type '${type}'`);
        }

        if (TYPES[type].inputs !== undefined && inputs.length !== TYPES[type].inputs) {
            throw new Error(`Cell type '${type}' must have ${TYPES[type].inputs} input nodes`);
        }

        if (TYPES[type].outputs !== undefined && outputs.length !== TYPES[type].outputs) {
            throw new Error(`Cell type '${type}' must have ${TYPES[type].outputs} output nodes`);
        }

        if (TYPES[type].writes !== undefined && writes.length !== TYPES[type].writes) {
            throw new Error(`Cell type '${type}' must have ${TYPES[type].writes} write nodes`);
        }

        if (TYPES[type].reads !== undefined && reads.length !== TYPES[type].reads) {
            throw new Error(`Cell type '${type}' must have ${TYPES[type].reads} read nodes`);
        }
    }

    static compare(a, b) {
        return a.logic.localeCompare(b.logic) ||
            a.type.localeCompare(b.type) ||
            Util.compareArrays(a.inputs, b.inputs) ||
            Util.compareArrays(a.outputs, b.outputs) ||
            Util.compareArrays(a.writes, b.writes) ||
            Util.compareArrays(a.reads, b.reads);
    }

    static compatible(a, b) {
        return a.logic === b.logic &&
            a.type === b.type &&
            a.inputs.length === b.inputs.length &&
            a.outputs.length === b.outputs.length &&
            a.writes.length === b.writes.length &&
            a.reads.length === b.reads.length;
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (f, v) => Validator.validateEnum(f, v, Object.keys(TYPES)),
            (f, v) => Validator.validateEnum(f, v, ['nmos', 'pmos', 'cmos', 'ttl']),
            (f, v) => Validator.validateArray(f, v, Validator.validateNode),
            (f, v) => Validator.validateArray(f, v, Validator.validateNode),
            (f, v) => Validator.validateArray(f, v, Validator.validateNode),
            (f, v) => Validator.validateArray(f, v, Validator.validateNode),
        ]);
    }

    static getGroups() {
        return GROUPS;
    }

    getSpec() {
        return [this.logic, this.type, this.inputs, this.outputs, this.writes, this.reads];
    }

    getAllNodes() {
        return [...this.inputs, ...this.outputs, ...this.writes, ...this.reads];
    }

    getGroupNodes(group) {
        let groupNodes = {};

        for (let i = 0; i < MAX_INPUTS; i++) {
            if (this.inputs.length > i) {
                groupNodes[`input_${i + 1}`] = [this.inputs[i]];
            } else {
                groupNodes[`input_${i + 1}`] = [];
            }
        }

        for (let o = 0; o < MAX_OUTPUTS; o++) {
            if (this.outputs.length > o) {
                groupNodes[`output_${o + 1}`] = [this.outputs[o]];
            } else {
                groupNodes[`output_${o + 1}`] = [];
            }
        }

        for (let w = 0; w < MAX_WRITES; w++) {
            if (this.writes.length > w) {
                groupNodes[`write_${w + 1}`] = [this.writes[w]];
            } else {
                groupNodes[`write_${w + 1}`] = [];
            }
        }

        for (let r = 0; r < MAX_READS; r++) {
            if (this.reads.length > r) {
                groupNodes[`read_${r + 1}`] = [this.reads[r]];
            } else {
                groupNodes[`read_${r + 1}`] = [];
            }
        }

        return groupNodes[group];
    }

    remapNodes(map) {
        this.inputs = this.inputs.map(n => map[n]);
        this.outputs = this.outputs.map(n => map[n]);
        this.writes = this.writes.map(n => map[n]);
        this.reads = this.reads.map(n => map[n]);
    }
}
