bits 64
section .text
extern high_level_exception_handler

%macro PUSH_ALL 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov rax, cr0
	push rax
	mov rax, cr2
	push rax
	mov rax, cr3
	push rax
	mov rax, cr4
	push rax
%endmacro

%macro POP_ALL 0
	add rsp, 8 * 4
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro ISR_NOERR 1
global isr_stub_%1
isr_stub_%1:
	push qword 0
	push qword %1
	jmp exception_common
%endmacro

%macro ISR_ERR 1
global isr_stub_%1
isr_stub_%1:
	push qword %1
	jmp exception_common
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR 21

exception_common:
	PUSH_ALL
	mov rdi, rsp
	call high_level_exception_handler
	mov rsp, rax
	POP_ALL
	add rsp, 16
	iretq

section .rodata
global exception_stub_table
exception_stub_table:
	dq isr_stub_0
	dq isr_stub_1
	dq isr_stub_2
	dq isr_stub_3
	dq isr_stub_4
	dq isr_stub_5
	dq isr_stub_6
	dq isr_stub_7
	dq isr_stub_8
	dq isr_stub_9
	dq isr_stub_10
	dq isr_stub_11
	dq isr_stub_12
	dq isr_stub_13
	dq isr_stub_14
	dq isr_stub_15
	dq isr_stub_16
	dq isr_stub_17
	dq isr_stub_18
	dq isr_stub_19
	dq isr_stub_20
	dq isr_stub_21
