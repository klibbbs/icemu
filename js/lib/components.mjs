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

function validateTypeGroup(type, group) {
    validateType(type);

    if (!TYPES[type].getGroups().includes(group)) {
        throw new Error(`Component type '${type}' does not have node group '${group}'`);
    }
}

export class Components {

    static getTypes() {
        return Object.keys(TYPES);
    }

    static getGroups(type) {
        validateType(type);

        return TYPES[type].getGroups();
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

    getComponentGroups(type) {
        validateType(type);

        return TYPES[type].getGroups();
    }

    getComponentsByNode(type, group, node) {
        validateTypeGroup(type, group);

        return this.maps[type][group][node];
    }

    getNodes(type) {
        validateTypeGroup(type);

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

    addComponents(type, specs) {
        validateType(type);

        specs.map(spec => new TYPES[type](-1, ...spec))
            .filter((v, i, a) => a.findIndex(c => TYPES[type].compare(v, c) === 0) === i)
            .filter(v => this.components[type].every(c => TYPES[type].compare(c, v) !== 0))
            .forEach(c => {
                c.idx = this.components[type].length;
                this.components[type].push(c);
            });

        this.rebuildMaps(type);

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

        this.maps[type] = Object.fromEntries(TYPES[type].getGroups().map(group => [
            group,
            this.components[type].reduce((map, c, idx) => {
                const groupNodes = c.getGroupNodes(group);

                if (groupNodes) {
                    groupNodes.forEach(n => {
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
