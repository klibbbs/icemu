export class Cache {

    constructor() {
        this.cache = {};
    }

    hasMismatch(type, cx, dx) {
        return this.cache[buildKey(type, cx, dx)] ? true : false;
    }

    cacheMismatch(type, cx, dx) {
        this.cache[buildKey(type, cx, dx)] = true;
    }
}

// --- Private ---

function buildKey(type, cx, dx) {
    return `${type}|${cx}|${dx}`;
}
