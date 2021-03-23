export class Bits {

    static roundUpToType(bits) {
        return Math.pow(2, Math.ceil(Math.max(0, Math.log2(bits / 8)))) * 8;
    }
}
