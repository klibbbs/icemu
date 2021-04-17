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

export class Components {

    static getTypes() {
        return Object.keys(TYPES);
    }

    static getGroups(type) {
        return TYPES[type].getGroups();
    }

    static areCompatible(type, a, b) {
        return TYPES[type].compatible(a, b);
    }

    constructor() {
        this.components = Object.fromEntries(Object.keys(TYPES).map(key => [key, []]));
        this.maps = Object.fromEntries(Object.keys(TYPES).map(key => [key, {}]));

        Components.getTypes().forEach(type => {
            Components.getGroups(type).forEach(group => {
                this.maps[type][group] = {};
            });
        });
    }

    // --- Accessors ---

    getCount(type) {
        return this.components[type].length;
    }

    getComponent(type, idx) {
        return this.getComponents(type)[idx];
    }

    getComponents(type, sort) {
        if (sort) {
            this.components[type].sort(TYPES[type].compare);
            this.rebuildMaps(type);
        }

        return this.components[type];
    }

    getComponentGroups(type) {
        return TYPES[type].getGroups();
    }

    getIndicesByNode(type, group, node) {
        const idxs = this.maps[type][group][node];

        return idxs ? idxs : [];
    }

    getNodes(type) {
        return [].concat(...this.components[type].map(c => c.getAllNodes()));
    }

    getIndices(type) {
        return Array.from(this.components[type].keys());
    }

    getSpecs(type) {
        return this.components[type].map(c => c.getSpec());
    }

    addComponents(type, specs) {
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
        indices.forEach(idx => {
            this.components[type][idx].__remove__ = true;
        });

        this.components[type] = this.components[type].filter(c => !c.__remove__);

        this.rebuildMaps(type);
    }

    rebuildMaps(type) {
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
