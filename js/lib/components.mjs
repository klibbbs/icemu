import { Load } from './load.mjs';
import { Transistor } from './transistor.mjs';
import { Buffer } from './buffer.mjs';

const TYPES = {
    load: Load,
    transistor: Transistor,
    buffer: Buffer,
};

// --- Private functions ---

function validateType(type) {
    if (TYPES[type] === undefined) {
        throw new Error(`Unsupported component type '${type}'`);
    }
}

function validateTypeArg(type, arg) {
    validateType(type);

    if (!TYPES[type].getArgs().includes(arg)) {
        throw new Error(`Component type '${type}' does not have argument '${arg}'`);
    }
}

export class Components {

    static getArgs(type) {
        validateType(type);

        return TYPES[type].getArgs();
    }

    constructor() {
        this.components = Object.fromEntries(Object.keys(TYPES).map(key => [key, []]));
        this.maps = Object.fromEntries(Object.keys(TYPES).map(key => [key, {}]));
    }

    getCount(type) {
        validateType(type);

        return this.components[type].length;
    }

    getComponent(type, idx) {
        return this.getComponents(type)[idx];
    }

    getComponents(type) {
        validateType(type);

        return this.components[type];
    }

    getComponentArgs(type) {
        validateType(type);

        return TYPES[type].getArgs();
    }

    getComponentsByNode(type, arg, node) {
        validateTypeArg(type, arg);

        return this.maps[type][arg][node];
    }

    getNodes(type) {
        validateTypeArg(type);

        return [].concat(...this.components[type].map(c => c.getAllNodes()));
    }

    getIndices(type) {
        validateType(type);

        return Array.from(this.components[type].keys());
    }

    getSpecs(type) {
        validateType(type);

        return this.components[type].map(c => c.getSpec());
    }

    filterComponents(type, filter) {
        validateType(type);

        this.components[type] = this.components[type].filter(filter);

        this.rebuildMaps(type);
    }

    addSpecs(type, specs) {
        validateType(type);

        this.addComponents(type, specs.map(s => TYPES[type].fromSpec(s)));
    }

    addComponents(type, components) {
        validateType(type);

        this.components[type].push(...components);

        this.components[type] = this.components[type]
            .filter((v, i, a) => a.findIndex(c => TYPES[type].compare(v, c) === 0) === i)
            .sort(TYPES[type].compare);

        this.rebuildMaps(type);
    }

    rebuildMaps(type) {
        validateType(type);

        this.maps[type] = Object.fromEntries(TYPES[type].getArgs().map(arg => [
            arg,
            this.components[type].reduce((map, c, idx) => {
                const argNodes = c.getArgNodes();

                if (argNodes[arg]) {
                    argNodes[arg].forEach(n => {
                        if (map[n]) {
                            map[n].push(idx);
                        } else {
                            map[n] = [idx];
                        }
                    });
                }

                return map;
            }, {})
        ]));
    }

    validateType(type) {
    }

}
