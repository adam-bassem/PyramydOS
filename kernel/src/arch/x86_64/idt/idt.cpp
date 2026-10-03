#include "idt.hpp"

#include <console/console.hpp>
#include <utils.hpp>

namespace
{
	bool int_set[NUM_INTS] = { false };
	arch::x86_64::idt::idt_entry idt_entries[NUM_INTS] = { 0 };

	const char* exception_names[] =
	{
		"#DE - Division Error",
		"#DB - Debug",
		"NMI - Non-maskable Interrupt",
		"#BP - Breakpoint",
		"#OF - Overflow",
		"#BR - Bound Range Exceeded",
		"#UD - Invalid Opcode",
		"#NM - Device Not Available",
		"#DF - Double Fault",
		"Copressor Segment Overrun",
		"#TS - Invalid TSS",
		"#NP - Segment Not Present",
		"#SS - Stack-Segment Fault",
		"#GP - General Protection Fault",
		"#PF - Page Fault",
		"RESERVED",
		"#MF - x87 Floating-Point Exception",
		"#AC - Alignment Check",
		"#MC - Machine Check",
		"#XM/#XF - SIMD Floating-Point Exception",
		"#VE - Virtualisation Exception",
		"#CP - Control Protection Exception",
		"RESERVED", "RESERVED", "RESERVED", "RESERVED", "RESERVED", "RESERVED",
		"#HV - Hypervisor Injection Exception",
		"#VC - VMM Communication Exception",
		"#SX - Security Exception",
		"RESERVED",
		"Triple Fault"
	};
}

extern "C" void high_level_exception_handler(int_context_t* ctx)
{
	console::kprintf("*******************************");
	console::kprintf("***    EXCEPTION OCCURED    ***");
	console::kprintf("*******************************");

	console::kprintf("Exception Details:");
	console::kprintf("Exception: %s", exception_names[ctx->vector]);
	console::kprintf("rax=%016llx rbx=%016llx rdx=%016llx rcx=%016llx", ctx->rax, ctx->rbx, ctx->rdx, ctx->rcx);
	console::kprintf("rbp=%016llx rdi=%016llx rsi=%016llx r8 =%016llx", ctx->rbp, ctx->rdi, ctx->rsi, ctx->r8);
	console::kprintf("r9 =%016llx r10=%016llx r11=%016llx r12=%016llx", ctx->r9, ctx->r10, ctx->r11, ctx->r12);
	console::kprintf("r13=%016llx r14=%016llx r15=%016llx", ctx->r13, ctx->r14, ctx->r15);
	console::kprintf("rip=%016llx cs =%016llx rfl=%016llx rsp=%016llx", ctx->rip, ctx->cs, ctx->rflags, ctx->rsp);
	console::kprintf("ss=%016llx", ctx->ss);
	console::kprintf("error code = %llu (%llx)", ctx->error_code);
	console::kprintf("cr0=%016llx cr2=%016llx cr3=%016llx cr3=%016llx", ctx->cr0, ctx->cr2, ctx->cr3, ctx->cr4);
	
	hcf_g();
}

void arch::x86_64::idt::init()
{
	static const idtr idtr_ptr
	{
		.limit = sizeof(idt_entries) - 1,
		.base = reinterpret_cast<uint64_t>(&idt_entries)
	};

	auto setex =
	[&](bool err, uint8_t vec, uint8_t type)
	{
		set_handler(exception_stub_table[vec], vec, type | ENTRY_DPL0 | ENTRY_PRESENT);
	};

	setex(false, 0, ENTRY_INTERRUPT_GATE);
	setex(false, 1, ENTRY_INTERRUPT_GATE);
	setex(false, 2, ENTRY_INTERRUPT_GATE);
	setex(false, 3, ENTRY_TRAP_GATE);
	setex(false, 4, ENTRY_TRAP_GATE);
	setex(false, 5, ENTRY_INTERRUPT_GATE);
	setex(false, 6, ENTRY_INTERRUPT_GATE);
	setex(false, 7, ENTRY_INTERRUPT_GATE);
	setex(true, 8, ENTRY_INTERRUPT_GATE);
	setex(true, 10, ENTRY_INTERRUPT_GATE);
	setex(true, 11, ENTRY_INTERRUPT_GATE);
	setex(true, 12, ENTRY_INTERRUPT_GATE);
	setex(true, 13, ENTRY_INTERRUPT_GATE);
	setex(true, 14, ENTRY_INTERRUPT_GATE);
	setex(false, 16, ENTRY_INTERRUPT_GATE);
	setex(true, 17, ENTRY_INTERRUPT_GATE);
	setex(false, 18, ENTRY_INTERRUPT_GATE);
	setex(false, 19, ENTRY_INTERRUPT_GATE);
	setex(false, 20, ENTRY_INTERRUPT_GATE);
	setex(true, 21, ENTRY_INTERRUPT_GATE);

	asm volatile
	(
		"cli\n"
		"lidt %0\n"
		"sti\n"
		:
		: "m" (idtr_ptr)
		: "memory"
	);
}

void arch::x86_64::idt::set_handler(void* handler, uint8_t vector, uint8_t flags)
{
	if (handler)
	{
		int_set[vector] = true;

		uint64_t handler_ptr = reinterpret_cast<uint64_t>(handler);
		idt_entry* e = &idt_entries[vector];

		e->zero = 0;
		e->off_low = handler_ptr & 0xFFFF;
		e->off_mid = (handler_ptr >> 16) & 0xFFFF;
		e->off_high = (handler_ptr >> 32) & 0xFFFFFFFF;
		e->ist = 0;
		e->attr = flags;
		e->selector = 0x08;
	}
	else
	{
		if (vector < 48) return;

		int_set[vector] = false;
		idt_entries[vector] = { 0 };
	}
}
