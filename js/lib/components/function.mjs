import { Util } from '../util.mjs';
import { Validator } from '../validator.mjs';

const MAX_GROUPS = 1;
const MAX_PARAMS = 9;

const PATTERN = /^([a-z]+)\((.*)\)$/;

const OPS = {
    nand: true,
    nor: true,
};

function paramToIndex(param) {
    const matches = param.match(/x(\d+)/);

    if (matches) {
        return matches[1] - 1;
    }

    throw new Error(`Invalid input parameter '${param}'`);
}

function parseExpression(expr) {
    const matches = expr.match(PATTERN);

    if (matches) {
        let [, op, args] = matches;

        if (!OPS[op]) {
            throw new Error(`Unknown function operation '${op}'`);
        }

        args = args.split(/\s*,\s*/);

        return {
            op: op,
            args: args.map(a => parseExpression(a)),
        };
    } else {
        return {
            param: expr,
        };
    }
}

function parseGroups(func) {

    function parseGroupsImpl(func, groups) {
        if (func.param) {
            groups.push([func.param]);
            return;
        }

        if (func.args.some(f => f.param)) {
            groups.push(func.args.filter(f => f.param).map(f => f.param));
        }
        func.args.filter(f => f.op).forEach(f => parseGroupsImpl(f, groups));
    }

    let groups = [];

    parseGroupsImpl(func, groups);

    for (let i = 0; i < MAX_GROUPS; i++) {
        if (groups[i] === undefined) {
            groups[i] = [];
        }
    }

    return groups;
}

function parseParams(groups) {
    let params = [].concat(...groups).sort();

    for (let i = 0; i < MAX_PARAMS; i++) {
        if (params[i] === undefined) {
            params[i] = `x${i + 1}`;
        }
    }

    return params;
}

function generateCode(func) {
    if (func.param) {
        return func.param;
    } else {
        const args = func.args.map(generateCode);

        switch (func.op) {
            case 'nand':
                return '!(' + args.join(' && ') + ')';
            case 'nor':
                return '!(' + args.join(' || ') + ')';
            default:
                throw new Error(`Cannot generate code for function operation '${op}'`);
        }
    }
}

export class Function {

    constructor(logic, expr, inputs, output) {
        this.logic = logic;
        this.expr = expr;
        this.inputs = inputs;
        this.output = output;

        this.func = parseExpression(expr);
        this.groups = parseGroups(this.func);
        this.params = parseParams(this.groups);
        this.code = generateCode(this.func);

        if (this.groups.length > MAX_GROUPS) {
            throw new Error(`Functions with more than ${MAX_GROUPS} groups are not supported`);
        }

        if (this.params.length > MAX_PARAMS) {
            throw new Error(`Functions with more than ${MAX_PARAMS} params are not supported`);
        }
    }

    static compare(a, b) {
        return a.logic.localeCompare(b.logic) ||
            a.expr.localeCompare(b.expr) ||
            Util.compareArrays(a.inputs, b.inputs) ||
            a.output - b.output;
    }

    static compatible(a, b) {
        return a.logic === b.logic && a.expr === b.expr;
    }

    static fromSpec(spec) {
        return new Function(...spec);
    }

    static validateSpec(field, val) {
        return Validator.validateTuple(field, val, [
            (field, val) => Validator.validateEnum(field, val, ['nmos', 'pmos', 'cmos', 'ttl']),
            (field, val) => {
                const expr = validateString(field, val);

                try {
                    parseExpression(expr);
                } catch (e) {
                    throw new TypeError(`${e.message} in ${field}`);
                }
            },
            (field, val) => Validator.validateArray(field, val, Validator.validateNode),
            Validator.validateNode,
        ]);
    }

    static getArgs() {
        return [
            ...[...Array(MAX_GROUPS).keys()].map(k => `group_${k + 1}`),
            'output',
        ];
    }

    getSpec() {
        return [this.logic, this.expr, this.inputs, this.output];
    }

    getAllNodes() {
        return [...this.inputs, this.output];
    }

    getArgNodes(arg) {
        let argNodes = Object.fromEntries([...Array(MAX_GROUPS).keys()].map(k => [
            `group_${k + 1}`,
            this.groups[k].map(p => this.inputs[paramToIndex(p)]),
        ]));

        argNodes.output = [this.output];

        return argNodes[arg];
    }
}
