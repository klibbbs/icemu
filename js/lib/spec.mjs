export class Spec {

    constructor(spec) {
        this.type = validateEnum('type', spec.type, [
            'device',
            'load',
            'transistor',
            'buffer',
        ]);

        // Component info
        this.id = validateIdentifier('id', spec.id);
        this.name = validateString('name', spec.name);

        if (this.type === 'device') {

            // Memory model
            if (spec.memory) {
                this.memory = validateStruct('memory', spec.memory, {
                    word: (field, val) => validateWidth(field, val),
                    address: (field, val) => validateInt(field, val, 1),
                })
            }
        } else {

            // Circuit parameters
            this.enabled = spec.enabled === false ? false : true;
            this.limit = spec.limit;
            this.args = validateArray('args', spec.args, validateAny);
        }

        // Node names
        this.nodeNames = validateMap('nodes', spec.nodes, validateIdentifier, validateNodeSet);

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
            this.on = validateSingleNodeName('on', spec.on, this.nodeNames);
        }

        if (spec.off) {
            this.off = validateSingleNodeName('off', spec.off, this.nodeNames);
        }

        // Pin sets
        if (spec.inputs) {
            this.inputs = validateArray('inputs', spec.inputs, (field, val) => {
                return validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.inputs = [];
        }

        if (spec.outputs) {
            this.outputs = validateArray('outputs', spec.outputs, (field, val) => {
                return validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.outputs = [];
        }

        if (spec.registers) {
            this.registers = validateArray('registers', spec.registers, (field, val) => {
                return validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.registers = [];
        }

        if (spec.flags) {
            this.flags = validateArray('flags', spec.flags, (field, val) => {
                return validateNodeName(field, val, this.nodeNames);
            });
        } else {
            this.flags = [];
        }

        // Components
        if (spec.loads) {
            this.loads = validateArray('loads', spec.loads, (field, val) => {
                return validateTuple(field, val, [
                    (field, val) => validateEnum(field, val, ['on', 'off']),
                    validateNode
                ]);
            });
        } else {
            this.loads = [];
        }

        if (spec.transistors) {
            this.transistors = validateArray('transistors', spec.transistors, (field, val) => {
                return validateTuple(field, val, [
                    (field, val) => validateEnum(field, val, ['nmos', 'pmos']),
                    validateNode,
                    validateNode,
                    validateNode,
                ]);
            });
        } else {
            this.transistors = [];
        }

        if (spec.buffers) {
            this.buffers = validateArray('buffers', spec.buffers, (field, val) => {
                return validateTuple(field, val, [
                    (field, val) => validateEnum(field, val, ['nmos', 'pmos', 'cmos']),
                    validateBool,
                    validateNode,
                    validateNode,
                ]);
            });
        } else {
            this.buffers = [];
        }

        // Circuits
        if (spec.circuits) {
            this.circuits = validateArray('circuits', spec.circuits, (field, val) => {
                return validateSpec(field, val);
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
    }
}

// --- Validators ---

function validateAny(field, val) {
    return val;
}

function validateBool(field, val) {
    if (val !== true && val !== false) {
        throw new TypeError(`Field '${field}' must be a boolean`);
    }

    return val;
}

function validateInt(field, val, min, max) {
    if (val !== parseInt(val, 10)) {
        throw new TypeError(`Field '${field}' must be an integer`);
    }

    if (min !== undefined && val < min) {
        throw new TypeError(`Field '${field}' may not be less than ${min}`);
    }

    if (max !== undefined && val > max) {
        throw new TypeError(`Field '${field}' may not be greater than ${max}`);
    }

    return val;
}

function validateIndex(field, val) {
    return validateInt(field, val, 0);
}

function validateString(field, val) {
    if (typeof val !== 'string') {
        throw new TypeError(`Field '${field}' must be a string`);
    }

    return val;
}

function validateWidth(field, val) {
    validateInt(field, val, 8);

    if (Math.log2(val / 8) % 0) {
        throw new TypeError(`Field '${field}' must be a legal data width`);
    }

    return val;
}

function validateIdentifier(field, val) {
    validateString(field, val);

    if (!val.match(/^[a-z_][a-z0-9_]*$/)) {
        throw new TypeError(`Field '${field}' must be a legal identifier`);
    }

    return val;
}

function validateTuple(field, val, validators) {
    if (Array.isArray(val)) {
        if (val.length != validators.length) {
            throw new TypeError(`Field '${field}' must have length ${validators.length}`);
        }

        for (const [idx, elem] of Object.entries(val)) {
            validators[idx](`${field}[${idx}]`, elem);
        }
    } else {
        throw new TypeError(`Field '${field}' must be an array`);
    }

    return val;
}

function validateStruct(field, val, validators) {
    if (typeof val !== 'object' || val === null || Array.isArray(val)) {
        throw new TypeError(`Field '${field}' must be an object`);
    }

    let obj = {};

    for (const [key, validator] of Object.entries(validators)) {
        if (val[key] === undefined) {
            throw new TypeError(`Field ${field} is missing key '${key}'`);
        }

        obj[key] = validator(`${field}.${key}`, val[key]);
    }

    return obj;
}

function validateArray(field, val, min, max, validator) {
    if (typeof min === 'function') {
        validator = min;
        min = max = undefined;
    } else if (typeof max === 'function') {
        validator = max;
        max = min;
    }

    if (Array.isArray(val)) {
        if (min !== undefined && min === max && val.length !== min) {
            throw new TypeError(`Field '${field}' must have exactly ${min} elements`);
        }

        if (min !== undefined && val.length < min) {
            throw new TypeError(`Field '${field}' must have at least ${min} elements`);
        }

        if (max !== undefined && val.length > max) {
            throw new TypeError(`Field '${field}' must have at most ${max} elements`);
        }

        return val.map((elem, idx) => {
            return validator(`${field}[${idx}]`, elem);
        });
    } else {
        throw new TypeError(`Field '${field}' must be an array`);
    }

    return val;
}

function validateMap(field, val, keyValidator, valueValidator) {
    if (typeof val !== 'object' || val === null || Array.isArray(val)) {
        throw new TypeError(`Field '${field}' must be an object`);
    }

    let obj = {};

    for (const [key, value] of Object.entries(val)) {
        obj[keyValidator(`${field}.keys.${key}`, key)] = valueValidator(`${field}.${key}`, value);
    }

    return obj;
}

function validateEnum(field, val, options) {
    for (const option of options) {
        if (option === val) {
            return val;
        }
    }

    throw new TypeError(`Field '${field}' must be one of [${options}]`);
}

function validateNode(field, val) {
    if (typeof val !== 'string' && val !== parseInt(val, 10)) {
        throw new TypeError(`Node index in field '${field}' must be an integer or a string`);
    }

    return val;
}

function validateNodeSet(field, val, width) {
    if (Array.isArray(val)) {
        if (width === undefined) {
            return validateArray(field, val, 1, undefined, validateIndex);
        } else {
            return validateArray(field, val, width, width, validateIndex);
        }
    } else {
        if (width > 1) {
            throw new TypeError(`Field '${field}' must be an array of length ${width}`);
        }

        return [validateIndex(field, val)];
    }
}

function validateNodeName(field, val, nodeNames) {
    validateString(field, val);

    if (nodeNames[val] === undefined) {
        throw new TypeError(`Node name in field '${field}' is not defined`);
    }

    return val;
}

function validateSingleNodeName(field, val, nodeNames) {
    validateNodeName(field, val, nodeNames);

    if (Array.isArray(nodeNames[val]) && nodeNames[val].length > 1) {
        throw new TypeError(`Node ${field} must be singular`);
    }

    return val;
}

function validateSpec(field, val) {
    try {
        return new Spec(val);
    } catch (e) {
        throw new TypeError(`${e.message} in ${field}`);
    }
}
