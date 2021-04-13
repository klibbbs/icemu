import { Validator } from './validator.mjs';

const MAX_GROUPS = 1;
const PATTERN = /^([a-z]+)\((.*)\)$/;
const OPS = {
    nand: {},
};

function valToIndex(val) {
    const matches = val.match(/x(\d+)/);

    if (matches) {
        return matches[1] - 1;
    }

    throw new Error(`Invalid input variable '${val}'`);
}

function parseExpression(expr) {
    const matches = expr.match(PATTERN);

    if (matches) {
        let [, op, args] = matches;

        if (OPS[op] === undefined) {
            throw new Error(`Unknown function operation '${op}'`);
        }

        args = args.split(/\s*,\s*/);

        return {
            op: op,
            args: args.map(a => parseExpression(a)),
        };
    } else {
        return {
            val: expr,
        };
    }
}

function parseGroups(func) {

    function parseGroupsImpl(func, groups) {
        if (func.val) {
            groups.push([func.val]);
            return;
        }

        if (func.args.some(f => f.val)) {
            groups.push(func.args.filter(f => f.val).map(f => f.val));
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

export class Function {

    constructor(logic, expr, inputs, output) {
        this.logic = logic;
        this.expr = expr;
        this.inputs = inputs;
        this.output = output;

        this.func = parseExpression(expr);
        this.groups = parseGroups(this.func);

        if (this.groups.length > MAX_GROUPS) {
            throw new Error(`Functions with more than ${MAX_GROUPS} groups not supported`);
        }
    }

    static compare(a, b) {
        return a.logic.localeCompare(b.logic) ||
            a.expr.localeCompare(b.expr) ||
            a.inputs.length - b.inputs.length ||
            (function (a, b) {
                for (let i = 0; i < a.length; i++) {
                    if (a !== b) {
                        return a - b;
                    }
                }

                return 0;
            }(a.inputs, b.inputs)) ||
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
            this.groups[k].map(v => this.inputs[valToIndex(v)]),
        ]));

        argNodes.output = [this.output];

        return argNodes[arg];
    }
}
