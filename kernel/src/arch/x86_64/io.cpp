#include "io.hpp"

uint8_t arch::x86_64::io::inb(uint16_t port)
{
	uint8_t value;
	asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

uint16_t arch::x86_64::io::inw(uint16_t port)
{
	uint16_t value;
	asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
	return value;	
}

uint32_t arch::x86_64::io::inl(uint16_t port)
{
	uint32_t value;
	asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

void arch::x86_64::io::outb(uint16_t port, uint8_t data)
{
	asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

void arch::x86_64::io::outw(uint16_t port, uint16_t data)
{
	asm volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
}

void arch::x86_64::io::outl(uint16_t port, uint32_t data)
{
	asm volatile ("outl %0, %1" : : "a"(data), "Nd"(port));
}
