export class Cache {

    constructor() {
        this.cache = {};
    }

    hasMismatch(type, cx, dx, state) {
        return this.cache[buildKey(type, cx, dx)] ? true : false;
    }

    cacheMismatch(type, cx, dx, state) {
        if (state.isEmpty()) {
            this.cache[buildKey(type, cx, dx)] = true;
        }
    }
}

// --- Private ---

function buildKey(type, cx, dx) {
    return `${type}|${cx}|${dx}`;
}
