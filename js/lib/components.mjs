import { Load } from './components/load.mjs';
import { Transistor } from './components/transistor.mjs';
import { Buffer } from './components/buffer.mjs';
import { Function } from './components/function.mjs';
import { Cell } from './components/cell.mjs';

const TYPES = {
    load: Load,
    transistor: Transistor,
    buffer: Buffer,
    'function': Function,
    cell: Cell,
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

    static getTypes() {
        return Object.keys(TYPES);
    }

    static getArgs(type) {
        validateType(type);

        return TYPES[type].getArgs();
    }

    static areCompatible(type, a, b) {
        validateType(type);

        return TYPES[type].compatible(a, b);
    }

    constructor() {
        this.components = Object.fromEntries(Object.keys(TYPES).map(key => [key, []]));
        this.maps = Object.fromEntries(Object.keys(TYPES).map(key => [key, {}]));

        Components.getTypes().forEach(t => this.rebuildMaps(t));
    }

    getCount(type) {
        validateType(type);

        return this.components[type].length;
    }

    getComponent(type, idx) {
        return this.getComponents(type)[idx];
    }

    getComponents(type, sort) {
        validateType(type);

        if (sort) {
            this.components[type].sort(TYPES[type].compare);
            this.rebuildMaps(type);
        }

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

    addSpecs(type, specs) {
        validateType(type);

        this.addComponents(type, specs.map(s => TYPES[type].fromSpec(s)));
    }

    addComponents(type, components) {
        validateType(type);

        components = components
            .filter((v, i, a) => a.findIndex(c => TYPES[type].compare(v, c) === 0) === i)
            .filter(v => this.components[type].every(c => TYPES[type].compare(c, v) !== 0));

        this.components[type].push(...components);

        this.rebuildMaps(type);
    }

    addNewComponent(type, args) {
        validateType(type);

        this.addComponents(type, [new TYPES[type](...args)]);
    }

    removeComponents(type, indices) {
        validateType(type);

        indices.forEach(idx => {
            this.components[type][idx].__remove__ = true;
        });

        this.components[type] = this.components[type].filter(c => !c.__remove__);

        this.rebuildMaps(type);
    }

    rebuildMaps(type) {
        validateType(type);

        this.maps[type] = Object.fromEntries(TYPES[type].getArgs().map(arg => [
            arg,
            this.components[type].reduce((map, c, idx) => {
                const argNodes = c.getArgNodes(arg);

                if (argNodes) {
                    argNodes.forEach(n => {
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
}
