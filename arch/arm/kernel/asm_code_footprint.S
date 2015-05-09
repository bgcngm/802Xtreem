/* arch/arm/kernel/asm_code_footprint.S
 *
 * Copyright (C) 2013 HTC, Inc.
 * Author: jerry_white <jerry_white@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * asm_code_footprint() support.
 * The footprint values are PC Address and Instruction.
 * The footprint values are saved in asm_code_footrpint local stack.
 * 
 * Footprint format:
 *   AC AC AC AC <PC ADDR> <ARM CODE> CA CA CA CA
 */

#ifdef CONFIG_ASM_CODE_FOOTPRINT

#include <linux/linkage.h>

.text

ENTRY(asm_code_footprint)
	stmfd   sp!, {r11 - r12, lr}
	adr     r12, __asm_code_footprint_local_stack   @ the local stack
	ldr     r11, =0xACACACAC
	stmia   r12, {r11}
	add     r12, #4

	stmia   r12, {lr}
	add     r12, #4

	ldr     r11, [lr]
	stmia   r12, {r11}
	add     r12, #4

	ldr     r11, =0xCACACACA
	stmia   r12, {r11}
	ldmfd   sp!, {r11 - r12, pc}

__asm_code_footprint_local_stack:
	.space  4 * 4                                   @ local stack; ACACACAC PC CODE CACACACA
ENDPROC(asm_code_footprint)
#endif

