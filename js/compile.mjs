import fs from 'fs';

import { Spec } from './lib/spec.mjs';
import { Layout } from './lib/layout.mjs';
import { Generator } from './lib/generator.mjs';

const STYLE_BOLD = '\x1B[0;1m';
const STYLE_NONE = '\x1B[0;0m';

// Find icemu.json file
const file = 'icemu.json';
const dir = (process.argv.length > 2 ? process.argv[2] + '/' : '').replace(/\/+$/, '/');
const path = `${dir}${file}`;

// Construct device spec from JSON
console.log(`${STYLE_BOLD}Validating device spec...${STYLE_NONE}`);

try {
    var spec = new Spec(JSON.parse(fs.readFileSync(path, { encoding: 'ascii' })));
} catch (e) {
    console.error(`Error parsing ${path}: ${e.message}`);
    process.exit(1);
}

// Construct layout from device spec
console.log(`${STYLE_BOLD}Compiling device layout...${STYLE_NONE}`);

try {
    var layout = new Layout(spec, {
        reduceTransistors: false,
        reduceCircuits: false,
    });
} catch (e) {
    throw e;
    console.error(`Error compiling device layout: ${e.message}`);
    process.exit(1);
}

// Generate source
console.log(`${STYLE_BOLD}Generating source code...${STYLE_NONE}`);

try {
    var generator = new Generator(spec, layout);

    generator.generateAll(dir);
} catch (e) {
    console.error(`Error generating source code: ${e.message}`);
    process.exit(1);
}

// Print info
console.log()
console.log(`${STYLE_BOLD}Device Spec${STYLE_NONE}`);
spec.printInfo();

console.log();
console.log(`${STYLE_BOLD}Device Layout${STYLE_NONE}`);
layout.printInfo();

console.log();
console.log(`${STYLE_BOLD}Generated Files${STYLE_NONE}`);
generator.printInfo();
