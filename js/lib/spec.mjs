export class Spec {

    constructor(spec) {

        // Device info
        this.id = validateIdentifier('id', spec.id);
        this.name = validateString('name', spec.name);

        // Memory model
        this.memory = validateStruct('memory', spec.memory, {
            word: (field, val) => validateWidth(field, val),
            address: (field, val) => validateInt(field, val, 1),
        });

        // Node names
        this.nodeNames = validateMap('nodes', spec.nodes, validateIdentifier, validateNodeSet);

        // Node assignments
        this.on = validateNodeName('on', spec.on, this.nodeNames);
        this.off = validateNodeName('off', spec.off, this.nodeNames);

        this.inputs = validateArray('inputs', spec.inputs, (field, val) => {
            return validateNodeName(field, val, this.nodeNames);
        });

        this.outputs = validateArray('outputs', spec.outputs, (field, val) => {
            return validateNodeName(field, val, this.nodeNames);
        });

        this.registers = validateArray('registers', spec.registers, (field, val) => {
            return validateNodeName(field, val, this.nodeNames);
        });

        this.flags = validateArray('flags', spec.flags, (field, val) => {
            return validateNodeName(field, val, this.nodeNames);
        });

        // Components
        this.loads = validateArray('loads', spec.loads, (field, val) => {
            return validateTuple(field, val, [
                (field, val) => validateEnum(field, val, ['on', 'off']),
                validateNode
            ])
        });

        this.transistors = validateArray('transistors', spec.transistors, (field, val) => {
            return validateTuple(field, val, [
                (field, val) => validateEnum(field, val, ['nmos', 'pmos']),
                validateNode,
                validateNode,
                validateNode,
            ])
        });

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
    }
}

// --- Validators ---

function validateInt(field, val, min, max) {
    if (val !== parseInt(val, 10)) {
        throw new TypeError(`Field '${field}' must be an integer`);
    }

    if (min !== undefined && min != null && val < min) {
        throw new TypeError(`Field '${field}' may not be less than ${min}`);
    }

    if (max !== undefined && max != null && val > max) {
        throw new TypeError(`Field '${field}' may not be greater than ${max}`);
    }

    return val;
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

    if (!val.match(/[a-z][a-z0-9]*/i)) {
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

function validateArray(field, val, validator) {
    if (Array.isArray(val)) {
        for (const [idx, elem] of Object.entries(val)) {
            validator(`${field}[${idx}]`, elem);
        }
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
        obj[keyValidator(`${val}.keys.${key}`, key)] = valueValidator(`${val}.${key}`, value);
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

function validateNodeSet(field, val) {
    if (Array.isArray(val)) {
        if (val.length === 0) {
            throw new TypeError(`Node set '${field}' cannot be empty`);
        }

        for (const [idx, elem] of Object.entries(val)) {
            validateNode(`${field}[${idx}]`, elem);
        }

        return val;
    } else {
        return [validateNode(field, val)];
    }
}

function validateNodeName(field, val, nodeNames) {
    validateString(field, val);

    if (nodeNames[val] === undefined) {
        throw new TypeError(`Node name in field '${field}' is not defined`);
    }

    return val;
}
