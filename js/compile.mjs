import fs from 'fs';

import { Spec } from './lib/spec.mjs';
import { Layout } from './lib/layout.mjs';
import { Generator } from './lib/generator.mjs';

const STYLE_BOLD = '\x1B[0;1m';
const STYLE_NONE = '\x1B[0;0m';

// Default options
const options = {
    deviceDir: '',
    reduceNodes: false,
    reduceCircuits: true,
    cacheLayout: false,
}

// Parse command-line options
const argv = process.argv.slice(2);

for (const arg of argv) {
    if (arg[0] === '-') {
        switch (arg) {
            case '--reduce':
                options.reduceNodes = true;
                break;
            case '--no-reduce':
                options.reduceNodes = false;
                break;
            case '--circuits':
                options.reduceCircuits = true;
                break;
            case '--no-circuits':
                options.reduceCircuits = false;
                break;
            case '--cache':
                options.cacheLayout = true;
                break;
            case '--no-cache':
                options.cacheLayout = false;
                break;
            default:
                throw new Error(`Unrecognized flag '${arg}'`);
        }
    } else {
        options.deviceDir = arg;
        break;
    }
}

// Find spec file to use
options.deviceDir = (options.deviceDir + '/').replace(/\/+$/, '/');

const specFile = 'icemu.json',
      specPath = `${options.deviceDir}${specFile}`,
      cacheFile = 'icemu.cache.json',
      cachePath = `${options.deviceDir}${cacheFile}`;

if (!fs.existsSync(specPath)) {
    console.error(`Error: Could not find file '${specPath}'`);
    process.exit(1);
}

// Construct original spec from JSON
console.log(`${STYLE_BOLD}Validating device spec...${STYLE_NONE}`);

try {
    var spec = new Spec(JSON.parse(fs.readFileSync(specPath, { encoding: 'ascii' })));
} catch (e) {
    console.error(`Error parsing ${specPath}: ${e.message}`);
    process.exit(1);
}

// Construct cached spec from JSON
if (fs.existsSync(cachePath)) {
    console.log(`${STYLE_BOLD}Validating device spec (from cache)...${STYLE_NONE}`);

    try {
        var cacheSpec = new Spec(JSON.parse(fs.readFileSync(cachePath, { encoding: 'ascii' })));
    } catch (e) {
        console.error(`Error parsing ${cachePath}: ${e.message}`);
        process.exit(1);
    }
}

// Construct layout from device spec
if (cacheSpec) {
    console.log(`${STYLE_BOLD}Compiling device layout (from cache)...${STYLE_NONE}`);
} else {
    console.log(`${STYLE_BOLD}Compiling device layout...${STYLE_NONE}`);
}

const activeSpec = cacheSpec ? cacheSpec : spec;

try {
    var layout = new Layout(activeSpec, {
        reduceNodes: options.reduceNodes,
        reduceCircuits: options.reduceCircuits,
    });
} catch (e) {
    console.error(`Error compiling device layout: ${e.message}`);
    process.exit(1);
}

// Write new cached spec file
try {
    var newSpec = layout.buildSpec();

    if (options.cacheLayout) {
        fs.writeFileSync(cachePath, JSON.stringify(newSpec, null, "    "));
    }
} catch (e) {
    console.error(`Error caching compiled spec: ${e.message}`);
    process.exit(1);
}

// Generate source
console.log(`${STYLE_BOLD}Generating source code...${STYLE_NONE}`);

try {
    var generator = new Generator(newSpec, layout);

    generator.generateAll(options.deviceDir);
} catch (e) {
    console.error(`Error generating source code: ${e.message}`);
    process.exit(1);
}

// Print info
console.log()
console.log(`${STYLE_BOLD}Device Spec${STYLE_NONE}`);
spec.printInfo();

if (cacheSpec) {
    console.log()
    console.log(`${STYLE_BOLD}Device Spec (from cache)${STYLE_NONE}`);
    cacheSpec.printInfo();
}

console.log();
console.log(`${STYLE_BOLD}Device Layout${STYLE_NONE}`);
layout.printInfo();

console.log();
console.log(`${STYLE_BOLD}Generated Files${STYLE_NONE}`);
generator.printInfo();
