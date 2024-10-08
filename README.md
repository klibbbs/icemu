# ICEMU

ICEMU is a transistor-level IC emulator, [heavily influenced by the perfect6502 and visual6502 projects](#credits).

## About

I got it in my head that I would write an NES emulator, and after several weeks of research this turned into an interest in writing a half-cycle-accurate MOS 6502 emulator. It's a popular chip for emulation, and this has been done before (by both projects credited below).

As written, ICEMU is a general-purpose emulator for any digital IC, provided the transistor netlist is known. The ICEMU runtime implements a simple language for writing test cases and manipulating memory in a simulated environment.

In addition to transistor-level emulation, ICEMU also includes native emulation of higher-level sub-components like logic gates and flip-flops. The ICEMU compiler (written in Node.js) searches the netlist for known transistor sub-graphs and replaces them with these sub-components, the aim being to improve performance by reducing the number of emulated components, while preserving identical behavior.

## Setup

`$ make runtime`
`$ make tests`

## Usage

_I'm writing this after several years, so my memory is fuzzy; what follows is merely a general guide._

### Emulation

The [icemu.h](/icemu.h) header provides the interface via which the low-level emulator (`icemu_t`) for any device is constructed from its netlist (`icemu_layout_t`). At this level of emulation, the only possible actions are writing a bit to a node, reading a bit from a node, and synchronizing circuit to account for any writes.

Each emulated device must define an _adapter_ that is returned by an [externally linked function](https://github.com/klibbbs/icemu/blob/0c1910aa6bcc3c7627b4cfea167de8cdcd92ffcb/mos6502/adapter.h#L6) matching the `adapter_func` declaration in [runtime.h](/runtime.h). The emulated device is compiled into a shared object that can then be loaded dynamically by the runtime application with the `.device` command.

The layout of the [mos6502](/mos6502) device directory is as follows:
- [layout.h](/layout.h) &mdash; Device layout, generated by [netlist compiler](#compilation).
- [mos6502.c](/mos6502.c) &mdash; Device-specific emulator using [icemu.h](/icemu.h) with named setters and getters for pins on the device.
- [memory.c](/memory.c) &mdash; Board memory emulation.
- [controller.c](/controller.c) &mdash; High-level emulation of things like clock steps and reset sequence.
- [adapter.c](/adapter.c) &mdash; Adapter for ICEMU runtime, tying together memory, controller, and pin interfaces.

The ICEMU runtime (`./runtime`) uses the adapter to run `*.ice` scripts, which can be used to measure performance or implement regression tests. Examples for MOS 6502 can be found in [mos6502/tests](/mos6502/tests).

### Compilation

The ICEMU compiler (`bin/compile`) uses a netlist of transistors and voltage loads [defined in JSON](/mos6502/icemu.json) to generate a [chip layout](/mos6502/layout.h). The `circuits` property allows known sub-graphs of transistors to be reduced to predefined components for faster emulation.

# Credits

ICEMU is inspired by and derived from [perfect6502](https://github.com/mist64/perfect6502), written by Michael Steil, which in turn was derived from [visual6502](https://github.com/trebonian/visual6502), written by Greg James, Brian Silverman, and Barry Silverman.

See original licenses for derived code in the [MOS 6502 layout](mos6502/layout.h) and the core [transistor emulation logic](icemu.c).

Identifying potential subcomponents for reducing the MOS 6502 is basically impossible without a schematic on hand. I made extensive use [this block diagram](https://www.weihenstephan.org/~michaste/pagetable/6502/6502.jpg) from an [article by Michael Steil](https://www.pagetable.com/?p=39) and the transistor-level schematic constructed from a die photo by Beregnyei Balazs found [here](https://www.nesdev.org/wiki/Visual6502wiki/Balazs%27_schematic_and_documents).
