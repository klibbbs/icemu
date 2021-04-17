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
        return this.getComponents(type).length;
    }

    getComponent(type, idx) {
        const c = this.components[type][idx];

        return (c && !c.__reduce) ? c : null;
    }

    getComponents(type) {
        return this.components[type].filter(c => !c.__reduce).sort(TYPES[type].compare);
    }

    getIndices(type) {
        return this.getComponents(type).map(c => c.idx);
    }

    getIndicesByNode(type, group, node) {
        const idxs = this.maps[type][group][node];

        return idxs ? idxs : [];
    }

    getAllNodes(type) {
        return [].concat(...this.getComponents(type).map(c => c.getAllNodes()));
    }

    getSpecs(type) {
        return this.getComponents(type).map(c => c.getSpec());
    }

    addComponents(type, specs) {
        let components = specs.map(spec => new TYPES[type](-1, ...spec));

        components.forEach((c, i) => {

            // Assign the next available index
            c.idx = this.components[type].length;

            // Reduce duplicates
            if (components.findIndex(cc => TYPES[type].compare(c, cc) === 0) !== i ||
                this.components[type].some(cc => TYPES[type].compare(c, cc) === 0)) {

                c.__reduce = true;
            }

            // Add to component list
            this.components[type].push(c);

            // Add unreduced components to node maps
            if (!c.__reduce) {
                Components.getGroups(type).forEach(group => {
                    const groupNodes = c.getGroupNodes(group);

                    if (groupNodes) {
                        groupNodes.forEach(n => {
                            if (this.maps[type][group][n]) {
                                this.maps[type][group][n].push(c.idx);
                            } else {
                                this.maps[type][group][n] = [c.idx];
                            }
                        });
                    }
                });
            }
        });
    }

    reduceComponents(type, indices) {
        indices.forEach(idx => {
            let c = this.components[type][idx];

            // Flag component as reduced
            c.__reduce = true;

            // Remove from node maps
            Components.getGroups(type).forEach(group => {
                const groupNodes = c.getGroupNodes(group);

                if (groupNodes) {
                    groupNodes.forEach(n => {
                        this.maps[type][group][n] =
                            this.maps[type][group][n].filter(i => i !== idx);
                    });
                }
            });
        });
    }
}
