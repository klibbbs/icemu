export class Util {

    static compareArrays(a, b) {
        if (a.length === b.length) {
            for (let i = 0; i < a.length; i++) {
                if (a[i] !== b[i]) {
                    return a[i] - b[i];
                }
            }
        } else {
            return a.length - b.length;
        }

        return 0;
    }
}
