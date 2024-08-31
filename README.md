# ICEMU

ICEMU is a transistor-accurate IC emulator, [heavily influenced by the perfect6502 and visual6502 projects](#credits).

## About

I got it in my head that I would write an NES emulator, and after several weeks of research this turned into an interest in writing a half-cycle-accurate MOS 6502 emulator. It's a popular chip for emulation, and this has been done before (by both projects credited below).

As written, ICEMU is a general-purpose emulator for any digital IC, provided the transistor netlist is known. The ICEMU runtime implements a simple language for writing test cases and manipulating memory in a simulated environment.

In addition to transistor-level emulation, ICEMU also includes native emulation of higher-level sub-components like logic gates and flip-flops. The ICEMU compiler (written in Node.js) searches the netlist for known transistor sub-graphs and replaces them with these sub-components, the aim being to improve performance (by reducing the number of emulated components) while preserving identical behavior.

## Setup



## Usage

# Credits

ICEMU is inspired by and derived from [perfect6502](https://github.com/mist64/perfect6502), written by Michael Steil, which in turn was derived from [visual6502](https://github.com/trebonian/visual6502), written by Greg James, Brian Silverman, and Barry Silverman.

See original licenses for derived code in the [MOS 6502 layout](mos6502/layout.h) and the core [transistor emulation logic](icemu.c).

Identifying potential subcomponents for reducing the MOS 6502 is basically impossible without a schematic on hand. I made extensive use [this block diagram](https://www.weihenstephan.org/~michaste/pagetable/6502/6502.jpg) from an [article by Michael Steil](https://www.pagetable.com/?p=39) and the transistor-level schematic constructed from a die photo by Beregnyei Balazs found [here](https://www.nesdev.org/wiki/Visual6502wiki/Balazs%27_schematic_and_documents).
