/*
 * Your code here for context_switch
 *
 * It should restores the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:
	addl $0x4, %esp 		// salteo el primer elemento: direccion de retorno

	popl %eax				// guardo la direccion del trapframe en el registro %eax
	movl %eax, %esp   		// muevo el stack pointer al inicio del trapframe

	popal 					// cargo los registros de proposito general	
	popl %es				// cargo el registro %es
	popl %ds				// cargo el registro %ds

	addl $0x8, %esp			// salteo los siguientes dos elementos: trapno, errno
	
	iret					// ejecuto la instruccion: retorno de trap
spin:
	jmp spin
