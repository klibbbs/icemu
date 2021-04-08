export class Validator {
    static validateAny(field, val) {
        return val;
    }

    static validateBool(field, val) {
        if (val !== true && val !== false) {
            throw new TypeError(`Field '${field}' must be a boolean`);
        }

        return val;
    }

    static validateInt(field, val, min, max) {
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

    static validateIndex(field, val) {
        return Validator.validateInt(field, val, 0);
    }

    static validateString(field, val) {
        if (typeof val !== 'string') {
            throw new TypeError(`Field '${field}' must be a string`);
        }

        return val;
    }

    static validateWidth(field, val) {
        Validator.validateInt(field, val, 8);

        if (Math.log2(val / 8) % 0) {
            throw new TypeError(`Field '${field}' must be a legal data width`);
        }

        return val;
    }

    static validateIdentifier(field, val) {
        Validator.validateString(field, val);

        if (!val.match(/^[a-z_][a-z0-9_]*$/)) {
            throw new TypeError(`Field '${field}' must be a legal identifier`);
        }

        return val;
    }

    static validateTuple(field, val, validators) {
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

    static validateStruct(field, val, validators) {
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

    static validateArray(field, val, min, max, validator) {
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

    static validateMap(field, val, keyValidator, valueValidator) {
        if (typeof val !== 'object' || val === null || Array.isArray(val)) {
            throw new TypeError(`Field '${field}' must be an object`);
        }

        let obj = {};

        for (const [key, value] of Object.entries(val)) {
            obj[keyValidator(`${field}.keys.${key}`, key)] = valueValidator(`${field}.${key}`, value);
        }

        return obj;
    }

    static validateEnum(field, val, options) {
        for (const option of options) {
            if (option === val) {
                return val;
            }
        }

        throw new TypeError(`Field '${field}' must be one of [${options}]`);
    }

    static validateNode(field, val) {
        if (typeof val !== 'string' && val !== parseInt(val, 10)) {
            throw new TypeError(`Node index in field '${field}' must be an integer or a string`);
        }

        return val;
    }

    static validateNodeSet(field, val, width) {
        if (Array.isArray(val)) {
            if (width === undefined) {
                return Validator.validateArray(field, val, 1, undefined, Validator.validateIndex);
            } else {
                return Validator.validateArray(field, val, width, width, Validator.validateIndex);
            }
        } else {
            if (width > 1) {
                throw new TypeError(`Field '${field}' must be an array of length ${width}`);
            }

            return [Validator.validateIndex(field, val)];
        }
    }

    static validateNodeName(field, val, nodeNames) {
        Validator.validateString(field, val);

        if (nodeNames[val] === undefined) {
            throw new TypeError(`Node name in field '${field}' is not defined`);
        }

        return val;
    }

    static validateSingleNodeName(field, val, nodeNames) {
        Validator.validateNodeName(field, val, nodeNames);

        if (Array.isArray(nodeNames[val]) && nodeNames[val].length > 1) {
            throw new TypeError(`Node ${field} must be singular`);
        }

        return val;
    }
}
