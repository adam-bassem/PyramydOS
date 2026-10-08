# PyramydOS Project

PyramydOS is a hobby operating system written primarily in C++, targeting x86_64 systems (currently, might expand).

### Features implemented

- Global Descriptor Table (x86_64)
- Interrupt Descriptor Table (x86_64)
- Physical Memory Manager (x86_64)
- Virtual Memory Manager (x86_64)
- Growing heap
- Console (using Flanterm)
- Custom ACPI implementation (x86_64)
- HPET timer (x86_64)
- APIC (x86_64)
	- I/O APIC
	- LAPIC
	- x2APIC and xAPIC
- LAPIC timer (x86_64)

### Building and running PyramydOS

You can build PyramydOS using the shell command:

```bash
make
```

And you can build and run PyramydOS using the command:

```bash
make run
```

### Prerequisites to run PyramydOS

You need the following installed:

- GCC
- LD
- QEMU (x86_64)
- Make

### Project Structure

```
kernel/src/			Project root
├── allocator			Growing heap allocator
├── arch				Architecture-specific code
│   └── x86_64				x86_64
│       ├── acpi				ACPI code
│       ├── apic				APIC code
│       ├── gdt					GDT code
│       ├── idt					IDT code
│       ├── irq					IRQ code
│       ├── mm					Memory management code
│       └── timers				Timers code
├── console				Console code
├── ext					Extras
│   └── flanterm			Flanterm
├── interrupts			Interrupts code (cross-arch)
└── timers				Timers code (cross-arch)
```

### License

Check the `LICENSE` file.

### Extra note

Thanks for taking a moment to check out my project!

If you liked PyramydOS, don't forget to star the repository.
Every star means a lot to me!

— Adam Bassem and contributors.
