export class Cache {

    constructor() {
        this.cache = {};
    }

    hasMismatch(type, cx, dx) {
        return this.cache[type] && this.cache[type][cx] && this.cache[type][cx][dx];
    }

    cacheMismatch(type, cx, dx) {
        if (this.cache[type]) {
            if (this.cache[type][cx]) {
                this.cache[type][cx][dx] = true;
            } else {
                this.cache[type][cx] = {
                    [dx]: true
                }
            }
        } else {
            this.cache[type] = {
                [cx]: {
                    [dx]: true
                }
            };
        }
    }
}
