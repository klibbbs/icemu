import fs from 'fs';

export class Generator {

    constructor(spec, layout) {
        this.files = {};
        this.spec = spec;
        this.layout = layout;
    }

    printInfo() {
        const width = Math.max(...Object.keys(this.files).map(k => k.length));

        Object.entries(this.files).forEach(([file, data]) => {
            const lines = (data.match(/\n/g) || []).length;

            console.log(`${(file + ':').padEnd(width + 1)} ${lines}`);
        });
    }

    generateAll(dir) {
        this.generateC(dir);
    }

    generateC(dir) {

        // C language variables
        const C = {
            type: {
                bool: 'bool_t',
                1: 'bit_t',
                8: 'unsigned char',
                16: 'unsigned short',
            },
            device: this.spec.id,
            device_type: `${this.spec.id}_t`,
            device_caps: this.spec.id.toUpperCase(),
            getPinSym: (pin, idx) => {
                if (idx === undefined) {
                    return (`${pin.type}_${pin.id}`).toUpperCase();
                } else {
                    return (`${pin.type}_${pin.id}_${idx}`).toUpperCase();
                }
            },
            getLoadEnum: (load) => {
                switch (load.type) {
                    case 'on':
                        return 'PULL_UP';
                    case 'off':
                        return 'PULL_DOWN';
                    default:
                        throw new TypeError(`Unsupported load type '${type}'`);
                }
            },
            getTransistorEnum: (transistor) => {
                switch (transistor.type) {
                    case 'nmos':
                        return 'TRANSISTOR_NMOS';
                    case 'pmos':
                        return 'TRANSISTOR_PMOS';
                    default:
                        throw new TypeError(`Unsupported transistor type '${transistor.type}'`);
                }
            },
            getTopologyEnum: (transistor) => {
                switch (transistor.topology) {
                    case 'single':
                        return 'TOPOLOGY_SINGLE';
                    case 'parallel':
                        return 'TOPOLOGY_PARELLEL';
                    case 'series':
                        return 'TOPOLOGY_SERIES';
                    default:
                        throw new TypeError(
                            `Unsupported transistor topology '${transistor.topology}'`);
                }
            },
        };

        // Generate source files
        this.files[`${dir}${C.device}.h`] = generateC_device_h(C, this.spec, this.layout);
        this.files[`${dir}${C.device}.c`] = generateC_device_c(C, this.spec, this.layout);
        this.files[`${dir}adapter.h`] = generateC_adapter_h(C, this.spec, this.layout);
        this.files[`${dir}adapter.c`] = generateC_adapter_c(C, this.spec, this.layout);
        this.files[`${dir}memory.h`] = generateC_memory_h(C, this.spec, this.layout);
        this.files[`${dir}memory.c`] = generateC_memory_c(C, this.spec, this.layout);
        this.files[`${dir}controller.h`] = generateC_controller_h(C, this.spec, this.layout);
        this.files[`${dir}layout.h`] = generateC_layout_h(C, this.spec, this.layout);

        // Write to disk
        Object.entries(this.files).forEach(([file, data]) => {
            fs.createWriteStream(file, { encoding: 'ascii', mode: 0x644 }).write(data);
        });
    }
}

// --- Helpers ---

function join(lines) {
    return (lines.join("\n") + "\n").replace(/\n\n\n*/g, "\n\n").replace(/\n*$/, "\n");
}

function tab(tabs, lines) {
    const TAB = '    ',
        indent = TAB.repeat(tabs);

    if (Array.isArray(lines)) {
        return lines.map(line => tab(tabs, line)).join("\n");
    } else {
        return lines === '' ? '' : lines.replace(/^/gm, indent);
    }
}

function comment(msg, level) {
    switch (level) {
        case 3:
            return [
                '/* ---' + '-'.repeat(msg.length) + '--- */',
                '/*    ' + msg + '    */',
                '/* ---' + '-'.repeat(msg.length) + '--- */',
            ].join("\n");
        case 2:
            return `/* --- ${msg} --- */`;
        case 1:
        default:
            return `/* ${msg} */`;
    }
}

// --- C Files ---

function generateC_device_h(C, spec, layout) {
    const include_guard = `INCLUDE_${C.device_caps}_${C.device_caps}_H`;

    return join([
        `#ifndef ${include_guard}`,
        `#define ${include_guard}`,
        '',
        '#include "../icemu.h"',
        '',
        comment(spec.name, 3),
        '',
        'typedef struct {',
        tab(1, 'icemu_t * ic;'),
        `} ${C.device_type};`,

        '',
        comment('Emulator', 2),
        '',
        `${C.device_type} * ${C.device}_init();`,
        `void ${C.device}_destroy(${C.device_type} * ${C.device});`,
        `void ${C.device}_sync(${C.device_type} * ${C.device});`,
        '',
        ...(layout.pins.filter(p => p.type === 'pin').length ? [
            comment('Pin accessors', 2),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.bits === 16).map(p => (
                `${C.type[16]} ${C.device}_get_pin_${p.id}(const ${C.device_type} * ${C.device});`
            )),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.bits === 8).map(p => (
                `${C.type[8]} ${C.device}_get_pin_${p.id}(const ${C.device_type} * ${C.device});`
            )),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.bits === 1).map(p => (
                `${C.type[1]} ${C.device}_get_pin_${p.id}(const ${C.device_type} * ${C.device});`
            )),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.type === 'reg').length ? [
            comment('Register accessors', 2),
            '',
            ...layout.pins.filter(p => p.type === 'reg' && p.bits === 16).map(p => (
                `${C.type[16]} ${C.device}_get_reg_${p.id}(const ${C.device_type} * ${C.device});`
            )),
            '',
            ...layout.pins.filter(p => p.type === 'reg' && p.bits === 8).map(p => (
                `${C.type[8]} ${C.device}_get_reg_${p.id}(const ${C.device_type} * ${C.device});`
            )),
            '',
            ...layout.pins.filter(p => p.type === 'reg' && p.bits === 1).map(p => (
                `${C.type[1]} ${C.device}_get_reg_${p.id}(const ${C.device_type} * ${C.device});`
            )),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.type === 'pin' && p.writable).length ? [
            comment('Pin modifiers', 2),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.writable && p.bits === 16).map(p => (
                `void ${C.device}_set_pin_${p.id}(` +
                    `${C.device_type} * ${C.device}, ${C.type[16]} data, ${C.type.bool} sync);`
            )),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.writable && p.bits === 8).map(p => (
                `void ${C.device}_set_pin_${p.id}(` +
                    `${C.device_type} * ${C.device}, ${C.type[8]} data, ${C.type.bool} sync);`
            )),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.writable && p.bits === 1).map(p => (
                `void ${C.device}_set_pin_${p.id}(` +
                    `${C.device_type} * ${C.device}, ${C.type[1]} data, ${C.type.bool} sync);`
            )),
        ] : []),
        '',
        `#endif /* ${include_guard} */`,
    ]);
}

function generateC_device_c(C, spec, layout) {
    return join([
        `#include "${C.device}.h"`,
        '',
        '#include "layout.h"',
        '',
        '#include "../icemu.h"',
        '',
        '#include <stdlib.h>',
        '',
        comment('Emulator', 2),
        '',
        `${C.device_type} * ${C.device}_init() {`,
        '',
        tab(1, comment('Construct IC layout')),
        tab(1, 'const icemu_layout_t layout = {'),
        tab(2, [
            `${C.device_caps}_ON,`,
            `${C.device_caps}_OFF,`,
            `${C.device_caps}_NODE_COUNT,`,
            `${C.device_caps}_LOAD_DEFS,`,
            `${C.device_caps}_LOAD_COUNT,`,
            `${C.device_caps}_TRANSISTOR_DEFS,`,
            `${C.device_caps}_TRANSISTOR_COUNT,`,
            `${C.device_caps}_TRANSISTOR_GATE_COUNT`,
        ]),
        tab(1, '};'),
        '',
        tab(1, comment('Initialize new IC emulator')),
        tab(1, 'icemu_t * ic = icemu_init(&layout);'),
        '',
        tab(1, comment('Initialize new device emulator')),
        tab(1, `${C.device_type} * ${C.device} = malloc(sizeof(${C.device_type}));`),
        '',
        tab(1, `${C.device}->ic = ic;`),
        '',
        tab(1, `return ${C.device};`),
        '}',
        '',
        `void ${C.device}_destroy(${C.device_type} * ${C.device}) {`,
        tab(1, `icemu_destroy(${C.device}->ic);`),
        '',
        tab(1, `free(${C.device});`),
        '}',
        '',
        `void ${C.device}_sync(${C.device_type} * ${C.device}) {`,
        tab(1, `icemu_sync(${C.device}->ic);`),
        '}',
        '',
        ...(layout.pins.filter(p => p.type === 'pin').length ? [
            comment('Pin accessors', 2),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.bits === 16).map(p => [
                `${C.type[16]} ${C.device}_get_pin_${p.id}(const ${C.device_type} * ${C.device}) {`,
                tab(1, 'return 0x0 |'),
                tab(2, p.nodes.map((n, idx) => {
                    const sym = C.getPinSym(p, idx);

                    return `icemu_read_node(${C.device}->ic, ${sym}, PULL_DOWN) << ${idx}`
                }).join(" |\n") + ';'),
                '}',
                '',
            ].join("\n")),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.bits === 8).map(p => [
                `${C.type[8]} ${C.device}_get_pin_${p.id}(const ${C.device_type} * ${C.device}) {`,
                tab(1, 'return 0x0 |'),
                tab(2, p.nodes.map((n, idx) => {
                    const sym = (`${p.type}_${p.id}_${idx}`).toUpperCase();

                    return `icemu_read_node(${C.device}->ic, ${sym}, PULL_DOWN) << ${idx}`
                }).join(" |\n") + ';'),
                '}',
                '',
            ].join("\n")),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.bits === 1).map(p => [
                `${C.type[1]} ${C.device}_get_pin_${p.id}(const ${C.device_type} * ${C.device}) {`,
                tab(1, `return icemu_read_node(${C.device}->ic, ${C.getPinSym(p)}, PULL_FLOAT);`),
                '}',
                '',
            ].join("\n")),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.type === 'reg').length ? [
            comment('Register accessors', 2),
            '',
            ...layout.pins.filter(p => p.type === 'reg' && p.bits === 16).map(p => [
                `${C.type[16]} ${C.device}_get_reg_${p.id}(const ${C.device_type} * ${C.device}) {`,
                tab(1, 'return 0x0 |'),
                tab(2, p.nodes.map((n, idx) => {
                    const sym = C.getPinSym(p, idx);

                    return `icemu_read_node(${C.device}->ic, ${sym}, PULL_DOWN) << ${idx}`
                }).join(" |\n") + ';'),
                '}',
                '',
            ].join("\n")),
            '',
            ...layout.pins.filter(p => p.type === 'reg' && p.bits === 8).map(p => [
                `${C.type[8]} ${C.device}_get_reg_${p.id}(const ${C.device_type} * ${C.device}) {`,
                tab(1, 'return 0x0 |'),
                tab(2, p.nodes.map((n, idx) => {
                    const sym = C.getPinSym(p, idx);

                    return `icemu_read_node(${C.device}->ic, ${sym}, PULL_DOWN) << ${idx}`
                }).join(" |\n") + ';'),
                '}',
                '',
            ].join("\n")),
            '',
            ...layout.pins.filter(p => p.type === 'reg' && p.bits === 1).map(p => [
                `${C.type[1]} ${C.device}_get_reg_${p.id}(const ${C.device_type} * ${C.device}) {`,
                tab(1, `return icemu_read_node(${C.device}->ic, ${C.getPinSym(p)}, PULL_FLOAT);`),
                '}',
                '',
            ].join("\n")),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.type === 'pin' && p.writable).length ? [
            comment('Pin modifiers', 2),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.writable && p.bits === 16).map(p => [
                `void ${C.device}_set_pin_${p.id}(` +
                    `${C.device_type} * ${C.device}, ${C.type[16]} data, ${C.type.bool} sync) {`,
                tab(1, p.nodes.map((n, idx) => (
                    `icemu_write_node(` +
                        `${C.device}->ic, ${C.getPinSym(p, idx)}, data >> ${idx} & 0x01, false);`
                ))),
                '',
                tab(1, 'if (sync) {'),
                tab(2, `icemu_sync(${C.device}->ic);`),
                tab(1, '}'),
                '}',
                '',
            ].join("\n")),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.writable && p.bits === 8).map(p => [
                `void ${C.device}_set_pin_${p.id}(` +
                    `${C.device_type} * ${C.device}, ${C.type[8]} data, ${C.type.bool} sync) {`,
                tab(1, p.nodes.map((n, idx) => (
                    `icemu_write_node(` +
                        `${C.device}->ic, ${C.getPinSym(p, idx)}, data >> ${idx} & 0x01, false);`
                ))),
                '',
                tab(1, 'if (sync) {'),
                tab(2, `icemu_sync(${C.device}->ic);`),
                tab(1, '}'),
                '}',
                '',
            ].join("\n")),
            '',
            ...layout.pins.filter(p => p.type === 'pin' && p.writable && p.bits === 1).map(p => [
                `void ${C.device}_set_pin_${p.id}(` +
                    `${C.device_type} * ${C.device}, ${C.type[1]} data, ${C.type.bool} sync) {`,
                tab(1, `icemu_write_node(${C.device}->ic, ${C.getPinSym(p)}, data, sync);`),
                '}',
                '',
            ].join("\n")),
        ] : []),

    ]);
}

function generateC_adapter_h(C, spec, layout) {
    const include_guard = `INCLUDE_${C.device_caps}_ADAPTER_H`;

    return join([
        `#ifndef ${include_guard}`,
        `#define ${include_guard}`,
        '',
        '#include "../runtime.h"',
        '',
        `const adapter_t * ${C.device}_adapter();`,
        '',
        `#endif /* ${include_guard} */`,
    ]);
}

function generateC_adapter_c(C, spec, layout) {
    return join ([
        '#include "adapter.h"',
        '',
        `#include "${C.device}.h"`,
        '#include "controller.h"',
        '#include "memory.h"',
        '',
        '#include "../runtime.h"',
        '',
        '#include <stdlib.h>',
        '#include <string.h>',
        '',
        comment('Pin mapping', 2),
        '',
        ...(layout.pins.filter(p => p.bits === 16).length ? [
            'typedef struct {',
            tab(1, [
                'const char * pin;',
                'size_t base;',
                `${C.type[16]} (* read_func)(const ${C.device_type} * ${C.device});`,
                `void (* write_func)` +
                    `(${C.device_type} * ${C.device}, ${C.type[16]} data, ${C.type.bool} sync);`,
            ]),
            '} pin_16_func_t;',
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 8).length ? [
            'typedef struct {',
            tab(1, [
                'const char * pin;',
                'size_t base;',
                `${C.type[8]} (* read_func)(const ${C.device_type} * ${C.device});`,
                `void (* write_func)` +
                    `(${C.device_type} * ${C.device}, ${C.type[8]} data, ${C.type.bool} sync);`,
            ]),
            '} pin_8_func_t;',
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 1).length ? [
            'typedef struct {',
            tab(1, [
                'const char * pin;',
                `${C.type[1]} (* read_func)(const ${C.device_type} * ${C.device});`,
                `void (* write_func)` +
                    `(${C.device_type} * ${C.device}, ${C.type[1]} data, ${C.type.bool} sync);`,
            ]),
            '} pin_1_func_t;',
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 16).length ? [
            `static const pin_16_func_t ${C.device_caps}_PIN_16_MAP[] = {`,
            tab(1, layout.pins.filter(p => p.bits === 16).map(p => (
                `{ "${p.id}.${p.type}", ${p.base}, ${C.device}_get_${p.type}_${p.id}, ` +
                    (p.writable ? `${C.device}_set_${p.type}_${p.id}` : 'NULL') + ' },'
            ))),
            '};',
            '',
            `static const size_t ${C.device_caps}_PIN_16_COUNT = ` +
                `sizeof(${C.device_caps}_PIN_16_MAP) / sizeof(pin_16_func_t);`
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 8).length ? [
            `static const pin_8_func_t ${C.device_caps}_PIN_8_MAP[] = {`,
            tab(1, layout.pins.filter(p => p.bits === 8).map(p => (
                `{ "${p.id}.${p.type}", ${p.base}, ${C.device}_get_${p.type}_${p.id}, ` +
                    (p.writable ? `${C.device}_set_${p.type}_${p.id}` : 'NULL') + ' },'
            ))),
            '};',
            '',
            `static const size_t ${C.device_caps}_PIN_8_COUNT = ` +
                `sizeof(${C.device_caps}_PIN_8_MAP) / sizeof(pin_8_func_t);`
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 1).length ? [
            `static const pin_1_func_t ${C.device_caps}_PIN_1_MAP[] = {`,
            tab(1, layout.pins.filter(p => p.bits === 1).map(p => (
                `{ "${p.id}.${p.type}", ${C.device}_get_${p.type}_${p.id}, ` +
                    (p.writable ? `${C.device}_set_${p.type}_${p.id}` : 'NULL') + ' },'
            ))),
            '};',
            '',
            `static const size_t ${C.device_caps}_PIN_1_COUNT = ` +
                `sizeof(${C.device_caps}_PIN_1_MAP) / sizeof(pin_1_func_t);`
        ] : []),
        '',
        comment('Adapter declarations', 2),
        '',
        'typedef struct {',
        tab(1, [
            `${C.device_type} * ${C.device};`,
            `${C.device}_memory_t * memory;`,
        ]),
        `} ${C.device}_instance_t;`,
        '',
        'static void * adapter_instance_init();',
        'static void adapter_instance_destroy(void * instance);',
        'static void adapter_instance_reset(void * instance);',
        'static void adapter_instance_step(void * instance);',
        'static void adapter_instance_run(void * instance, size_t cycles);',
        'static int adapter_instance_can_read_pin(const void * instance, const char * pin);',
        'static int adapter_instance_can_write_pin(const void * instance, const char * pin);',
        'static value_t adapter_instance_read_pin(const void * instance, const char * pin);',
        'static void adapter_instance_write_pin(' +
            'void * instance, const char * pin, unsigned int data);',
        'static value_t adapter_instance_read_mem(const void * instance, unsigned int addr);',
        'static void adapter_instance_write_mem(' +
            'void * instance, unsigned int addr, unsigned int data);',
        '',
        comment('Adapter singleton', 2),
        '',
        `static const adapter_t ${C.device_caps}_ADAPTER = {`,
        tab(1, [
            `"${spec.id}",`,
            `"${spec.name}",`,
            `${C.device_caps}_MEMORY_ADDR_WIDTH,`,
            `${C.device_caps}_MEMORY_WORD_WIDTH,`,
            'adapter_instance_init,',
            'adapter_instance_destroy,',
            'adapter_instance_reset,',
            'adapter_instance_step,',
            'adapter_instance_run,',
            'adapter_instance_can_read_pin,',
            'adapter_instance_can_write_pin,',
            'adapter_instance_read_pin,',
            'adapter_instance_write_pin,',
            'adapter_instance_read_mem,',
            'adapter_instance_write_mem,',
        ]),
        '};',
        '',
        comment('Public functions', 2),
        '',
        `const adapter_t * ${C.device}_adapter() {`,
        tab(1, `return &${C.device_caps}_ADAPTER;`),
        '}',
        '',
        comment('Adapter functions', 2),
        '',
        'void * adapter_instance_init() {',
        tab(1, [
            `${C.device}_instance_t * ${C.device}_instance = ` +
                `malloc(sizeof(${C.device}_instance_t));`,
            '',
            `${C.device}_instance->${C.device} = ${C.device}_init();`,
            `${C.device}_instance->memory = ${C.device}_memory_init();`,
            '',
            `return ${C.device}_instance;`,
        ]),
        '}',
        '',
        'void adapter_instance_destroy(void * instance) {',
        tab(1, [
            `${C.device}_instance_t * ${C.device}_instance = (${C.device}_instance_t *)instance;`,
            '',
            `${C.device}_destroy(${C.device}_instance->${C.device});`,
            `${C.device}_memory_destroy(${C.device}_instance->memory);`,
            '',
            `free(${C.device}_instance);`,
        ]),
        '}',
        '',
        'void adapter_instance_reset(void * instance) {',
        tab(1, [
            `${C.device}_instance_t * ${C.device}_instance = (${C.device}_instance_t *)instance;`,
            '',
            `${C.device}_controller_reset(${C.device}_instance->${C.device});`,
        ]),
        '}',
        '',
        'void adapter_instance_step(void * instance) {',
        tab(1, [
            `${C.device}_instance_t * ${C.device}_instance = (${C.device}_instance_t *)instance;`,
            '',
            `${C.device}_controller_step(` +
                `${C.device}_instance->${C.device}, ${C.device}_instance->memory);`,
        ]),
        '}',
        '',
        'void adapter_instance_run(void * instance, size_t cycles) {',
        tab(1, [
            `${C.device}_instance_t * ${C.device}_instance = (${C.device}_instance_t *)instance;`,
            '',
            `${C.device}_controller_run(` +
                `${C.device}_instance->${C.device}, ${C.device}_instance->memory, cycles);`,
        ]),
        '}',
        '',
        'int adapter_instance_can_read_pin(const void * instance, const char * pin) {',
        tab(1, 'size_t i;'),
        '',
        ...(layout.pins.filter(p => p.bits === 16).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_16_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_16_MAP[i].pin) == 0) {`),
            tab(3, `return ${C.device_caps}_PIN_16_MAP[i].read_func != NULL;`),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 8).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_8_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_8_MAP[i].pin) == 0) {`),
            tab(3, `return ${C.device_caps}_PIN_8_MAP[i].read_func != NULL;`),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 1).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_1_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_1_MAP[i].pin) == 0) {`),
            tab(3, `return ${C.device_caps}_PIN_1_MAP[i].read_func != NULL;`),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        tab(1, 'return 0;'),
        '}',
        '',
        'int adapter_instance_can_write_pin(const void * instance, const char * pin) {',
        tab(1, 'size_t i;'),
        '',
        ...(layout.pins.filter(p => p.bits === 16).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_16_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_16_MAP[i].pin) == 0) {`),
            tab(3, `return ${C.device_caps}_PIN_16_MAP[i].write_func != NULL;`),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 8).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_8_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_8_MAP[i].pin) == 0) {`),
            tab(3, `return ${C.device_caps}_PIN_8_MAP[i].write_func != NULL;`),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 1).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_1_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_1_MAP[i].pin) == 0) {`),
            tab(3, `return ${C.device_caps}_PIN_1_MAP[i].write_func != NULL;`),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        tab(1, 'return 0;'),
        '}',
        '',
        'value_t adapter_instance_read_pin(const void * instance, const char * pin) {',
        tab(1, [
            `const ${C.device}_instance_t * ${C.device}_instance = `
            + `(${C.device}_instance_t *)instance;`,
            '',
            'value_t val = { 0, 0, 0 };',
            'size_t i;',
            '',
        ]),
        ...(layout.pins.filter(p => p.bits === 16).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_16_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_16_MAP[i].pin) == 0) {`),
            tab(3, `if (${C.device_caps}_PIN_16_MAP[i].read_func == NULL) {`),
            tab(4, 'break;'),
            tab(3, [
                '}',
                '',
                `val.data = ${C.device_caps}_PIN_16_MAP[i].read_func(` +
                    `${C.device}_instance->${C.device});`,
                'val.bits = 16;',
                `val.base = ${C.device_caps}_PIN_16_MAP[i].base;`,
                '',
                'return val;'
            ]),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 8).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_8_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_8_MAP[i].pin) == 0) {`),
            tab(3, `if (${C.device_caps}_PIN_8_MAP[i].read_func == NULL) {`),
            tab(4, 'break;'),
            tab(3, [
                '}',
                '',
                `val.data = ${C.device_caps}_PIN_8_MAP[i].read_func(` +
                    `${C.device}_instance->${C.device});`,
                'val.bits = 8;',
                `val.base = ${C.device_caps}_PIN_8_MAP[i].base;`,
                '',
                'return val;'
            ]),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 1).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_1_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_1_MAP[i].pin) == 0) {`),
            tab(3, `if (${C.device_caps}_PIN_1_MAP[i].read_func == NULL) {`),
            tab(4, 'break;'),
            tab(3, [
                '}',
                '',
                `val.data = ${C.device_caps}_PIN_1_MAP[i].read_func(` +
                    `${C.device}_instance->${C.device});`,
                'val.bits = 1;',
                'val.base = 10;',
                '',
                'return val;'
            ]),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        tab(1, 'return val;'),
        '}',
        '',
        'void adapter_instance_write_pin(void * instance, const char * pin, unsigned int data) {',
        tab(1, [
            `${C.device}_instance_t * ${C.device}_instance = (${C.device}_instance_t *)instance;`,
            'size_t i;',
            '',
        ]),
        ...(layout.pins.filter(p => p.bits === 16).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_16_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_16_MAP[i].pin) == 0) {`),
            tab(3, `if (${C.device_caps}_PIN_16_MAP[i].write_func == NULL) {`),
            tab(4, 'break;'),
            tab(3, [
                '}',
                '',
                `${C.device_caps}_PIN_16_MAP[i].write_func(` +
                    `${C.device}_instance->${C.device}, data, true);`,
                'return;',
            ]),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 8).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_8_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_8_MAP[i].pin) == 0) {`),
            tab(3, `if (${C.device_caps}_PIN_8_MAP[i].write_func == NULL) {`),
            tab(4, 'break;'),
            tab(3, [
                '}',
                '',
                `${C.device_caps}_PIN_8_MAP[i].write_func(` +
                    `${C.device}_instance->${C.device}, data, true);`,
                'return;',
            ]),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '',
        ...(layout.pins.filter(p => p.bits === 1).length ? [
            tab(1, `for (i = 0; i < ${C.device_caps}_PIN_1_COUNT; i++) {`),
            tab(2, `if (strcmp(pin, ${C.device_caps}_PIN_1_MAP[i].pin) == 0) {`),
            tab(3, `if (${C.device_caps}_PIN_1_MAP[i].write_func == NULL) {`),
            tab(4, 'break;'),
            tab(3, [
                '}',
                '',
                `${C.device_caps}_PIN_1_MAP[i].write_func(` +
                    `${C.device}_instance->${C.device}, data, true);`,
                'return;',
            ]),
            tab(2, '}'),
            tab(1, '}'),
        ] : []),
        '}',
        '',
        'value_t adapter_instance_read_mem(const void * instance, unsigned int addr) {',
        tab(1, [
            `const ${C.device}_instance_t * ${C.device}_instance = `
                + `(${C.device}_instance_t *)instance;`,
            '',
            'value_t val;',
            '',
            `val.data = ${C.device}_memory_read(${C.device}_instance->memory, addr);`,
            `val.bits = ${C.device_caps}_MEMORY_WORD_WIDTH;`,
            'val.base = 16;',
            '',
            'return val;',
        ]),
        '}',
        '',
        'void adapter_instance_write_mem(void * instance, unsigned int addr, unsigned int data) {',
        tab(1, [
            `${C.device}_instance_t * ${C.device}_instance = (${C.device}_instance_t *)instance;`,
            '',
            `${C.device}_memory_write(${C.device}_instance->memory, addr, data);`,
        ]),
        '}',
    ]);
}

function generateC_memory_h(C, spec, layout) {
    const include_guard = `INCLUDE_${C.device_caps}_MEMORY_H`;

    const addr_bits = Math.pow(2, Math.ceil(Math.max(0, Math.log2(spec.memory.address / 8)))) * 8;
    const word_count = Math.pow(2, spec.memory.address) * 8 / spec.memory.word;

    return join([
        `#ifndef ${include_guard}`,
        `#define ${include_guard}`,
        '',
        comment('Constants', 2),
        '',
        `enum { ${C.device_caps}_MEMORY_ADDR_WIDTH = ${layout.memory.addr} };`,
        `enum { ${C.device_caps}_MEMORY_WORD_WIDTH = ${layout.memory.word} };`,
        `enum { ${C.device_caps}_MEMORY_WORD_COUNT = ${layout.memory.count} };`,
        '',
        comment('Types', 2),
        '',
        `typedef ${C.type[layout.memory.addr_type]} ${C.device}_addr_t;`,
        `typedef ${C.type[layout.memory.word_type]} ${C.device}_word_t;`,
        '',
        'typedef struct {',
        tab(1, `${C.device}_word_t memory[${C.device_caps}_MEMORY_WORD_COUNT];`),
        `} ${C.device}_memory_t;`,
        '',
        comment('Functions', 2),
        '',
        `${C.device}_memory_t * ${C.device}_memory_init();`,
        `void ${C.device}_memory_destroy(${C.device}_memory_t * memory);`,
        `${C.device}_word_t ${C.device}_memory_read(` +
            `const ${C.device}_memory_t * memory, ${C.device}_addr_t addr);`,
        `void ${C.device}_memory_write(` +
            `${C.device}_memory_t * memory, ${C.device}_addr_t addr, ${C.device}_word_t word);`,
        '',
        `#endif /* ${include_guard} */`,
    ]);
}

function generateC_memory_c(C, spec, layout) {
    return join([
        '#include "memory.h"',
        '',
        '#include <stdlib.h>',
        '',
        `${C.device}_memory_t * ${C.device}_memory_init() {`,
        tab(1, `return (${C.device}_memory_t *)calloc(1, sizeof(${C.device}_memory_t));`),
        '}',
        '',
        `void ${C.device}_memory_destroy(${C.device}_memory_t * memory) {`,
        tab(1, 'free(memory);'),
        '}',
        '',
        `${C.device}_word_t ${C.device}_memory_read(` +
            `const ${C.device}_memory_t * memory, ${C.device}_addr_t addr) {`,
        tab(1, `return memory->memory[addr >> ${Math.log2(layout.memory.word / 8)}];`),
        '}',
        '',
        `void ${C.device}_memory_write(` +
            `${C.device}_memory_t * memory, ${C.device}_addr_t addr, ${C.device}_word_t word) {`,
        tab(1, `memory->memory[addr >> ${Math.log2(layout.memory.word / 8)}] = word;`),
        '}',
    ]);
}

function generateC_controller_h(C, spec, layout) {
    const include_guard = `INCLUDE_${C.device_caps}_CONTROLLER_H`;

    return join ([
        `#ifndef ${include_guard}`,
        `#define ${include_guard}`,
        '',
        `#include "${C.device}.h"`,
        '#include "memory.h"',
        '',
        '#include <stddef.h>',
        '',
        `void ${C.device}_controller_reset(${C.device_type} * ${C.device});`,
        `void ${C.device}_controller_step(` +
            `${C.device_type} * ${C.device}, ${C.device}_memory_t * memory);`,
        `void ${C.device}_controller_run(` +
            `${C.device_type} * ${C.device}, ${C.device}_memory_t * memory, size_t cycles);`,
        '',
        `#endif /* ${include_guard} */`,
    ]);
}

function generateC_layout_h(C, spec, layout) {
    const include_guard = `INCLUDE_${C.device_caps}_LAYOUT_H`;

    const syms = [].concat(...layout.pins.map(p => {
        if (p.bits === 1) {
            return [{ type: p.type, node: p.nodes[0], sym: C.getPinSym(p) }];
        } else {
            return p.nodes.map((n, idx) => ({ type: p.type, node: n, sym: C.getPinSym(p, idx) }));
        }
    }));

    const syms_width = Math.max(...syms.map(p => p.sym.length));

    return join ([
        `#ifndef ${include_guard}`,
        `#define ${include_guard}`,
        '',
        `#include "${C.device}.h"`,
        '',
        '#include "../icemu.h"',
        '',
        '#include <stddef.h>',
        '',
        comment('Node constants', 2),
        '',
        comment('Source nodes'),
        'typedef enum {',
        tab(1, [
            `${C.getPinSym(layout.on).padEnd(syms_width)} = ${layout.on.nodes[0]},`,
            `${C.getPinSym(layout.off).padEnd(syms_width)} = ${layout.off.nodes[0]}`,
        ]),
        `} ${C.device}_src_t;`,
        '',
        ...(syms.filter(p => p.type === 'pin').length ? [
            comment('Pin nodes'),
            'typedef enum {',
            tab(1, syms.filter(p => p.type === 'pin').map(p => (
                `${p.sym.padEnd(syms_width)} = ${p.node}`
            )).join(",\n")),
            `} ${C.device}_pin_t;`,
        ] : []),
        '',
        ...(syms.filter(p => p.type === 'reg').length ? [
            comment('Register nodes'),
            'typedef enum {',
            tab(1, syms.filter(p => p.type === 'reg').map(p => (
                `${p.sym.padEnd(syms_width)} = ${p.node}`
            )).join(",\n")),
            `} ${C.device}_reg_t;`,
        ] : []),
        '',
        comment('Device constants', 2),
        '',
        `const nx_t ${C.device_caps}_ON  = ${C.getPinSym(layout.on)};`,
        `const nx_t ${C.device_caps}_OFF = ${C.getPinSym(layout.off)};`,
        '',
        comment('Component counts', 2),
        '',
        `const size_t ${C.device_caps}_NODE_COUNT = ${Object.keys(layout.nodes).length};`,
        `const size_t ${C.device_caps}_LOAD_COUNT = ${layout.loads.length};`,
        `const size_t ${C.device_caps}_TRANSISTOR_COUNT = ${layout.transistors.length};`,
        `const size_t ${C.device_caps}_TRANSISTOR_GATE_COUNT = ${layout.transistors.length};`,
        '',
        comment('Component definitions', 2),
        '',
        ...(layout.loads.length ? [
            `const load_t ${C.device_caps}_LOAD_DEFS[] = {`,
            tab(1, layout.loads.map(l => `{${C.getLoadEnum(l)}, ${l.node}}`).join(",\n")),
            '};',
        ] : []),
        '',
        ...(layout.transistors.length ? [
            `const nx_t ${C.device_caps}_TRANSISTOR_GATES[][1] = {`,
            tab(1, layout.transistors.map(t => `{${t.gates[0]}}`).join(",\n")),
            '};',
            '',
            `const transistor_t ${C.device_caps}_TRANSISTOR_DEFS[] = {`,
            tab(1, layout.transistors.map((t, i) => (
                `{` +
                    `${C.getTransistorEnum(t)}, ${C.getTopologyEnum(t)}, ` +
                    `${C.device_caps}_TRANSISTOR_GATES[${i}], ${t.gates.length}, ` +
                    `${t.channel[0]}, ${t.channel[1]}` +
                    `}`
            )).join(",\n")),
            '};',
            '',
        ] : []),
        '',
        `#endif /* ${include_guard} */`,
    ]);
}
