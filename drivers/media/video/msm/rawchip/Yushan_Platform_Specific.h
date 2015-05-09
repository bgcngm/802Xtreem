/********************************************************************************
################################################################################
#                             (C) STMicroelectronics 2012
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 2 and only version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#------------------------------------------------------------------------------
#                             Imaging Division
################################################################################
File Name:	Yushan_Platform_Specific.h
Author:		Rajdeep Patel
Description:Platform specific header file.
********************************************************************************/

/* Defined for only proto */
#define ST_SPECIFIC							1
#ifndef _YUSHAN_PLATFORM_SPECIFIC_H
#define _YUSHAN_PLATFORM_SPECIFIC_H


#define RAWCHIP_INT_TYPE_ERROR (0x01<<0)
#define RAWCHIP_INT_TYPE_NEW_FRAME (0x01<<1)
#define RAWCHIP_INT_TYPE_PDP_EOF_EXECCMD (0x01<<2)
#define RAWCHIP_INT_TYPE_DPP_EOF_EXECCMD (0x01<<3)
#define RAWCHIP_INT_TYPE_DOP_EOF_EXECCMD (0x01<<4)

/* Interrupt functions */
bool_t Yushan_WaitForInterruptEvent (uint8_t bInterruptId ,uint32_t udwTimeOut);
bool_t Yushan_WaitForInterruptEvent2 (uint8_t bInterruptId ,uint32_t udwTimeOut);
uint8_t Yushan_parse_interrupt(int intr_pad, int error_times[TOTAL_INTERRUPT_COUNT]);
void Yushan_Interrupt_Manager_Pad0(void);
void Yushan_Interrupt_Manager_Pad1(void);

#endif
