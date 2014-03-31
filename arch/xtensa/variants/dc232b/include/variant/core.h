/*
 * Xtensa processor core configuration information.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1999-2007 Tensilica Inc.
 */

#ifndef _XTENSA_CORE_CONFIGURATION_H
#define _XTENSA_CORE_CONFIGURATION_H






#define XCHAL_HAVE_BE			0	
#define XCHAL_HAVE_WINDOWED		1	
#define XCHAL_NUM_AREGS			32	
#define XCHAL_NUM_AREGS_LOG2		5	
#define XCHAL_MAX_INSTRUCTION_SIZE	3	
#define XCHAL_HAVE_DEBUG		1	
#define XCHAL_HAVE_DENSITY		1	
#define XCHAL_HAVE_LOOPS		1	
#define XCHAL_HAVE_NSA			1	
#define XCHAL_HAVE_MINMAX		1	
#define XCHAL_HAVE_SEXT			1	
#define XCHAL_HAVE_CLAMPS		1	
#define XCHAL_HAVE_MUL16		1	
#define XCHAL_HAVE_MUL32		1	
#define XCHAL_HAVE_MUL32_HIGH		0	
#define XCHAL_HAVE_DIV32		1	
#define XCHAL_HAVE_L32R			1	
#define XCHAL_HAVE_ABSOLUTE_LITERALS	1	
#define XCHAL_HAVE_CONST16		0	
#define XCHAL_HAVE_ADDX			1	
#define XCHAL_HAVE_WIDE_BRANCHES	0	
#define XCHAL_HAVE_PREDICTED_BRANCHES	0	
#define XCHAL_HAVE_CALL4AND12		1	
#define XCHAL_HAVE_ABS			1	
	
	
#define XCHAL_HAVE_RELEASE_SYNC		1	
#define XCHAL_HAVE_S32C1I		1	
#define XCHAL_HAVE_SPECULATION		0	
#define XCHAL_HAVE_FULL_RESET		1	
#define XCHAL_NUM_CONTEXTS		1	
#define XCHAL_NUM_MISC_REGS		2	
#define XCHAL_HAVE_TAP_MASTER		0	
#define XCHAL_HAVE_PRID			1	
#define XCHAL_HAVE_THREADPTR		1	
#define XCHAL_HAVE_BOOLEANS		0	
#define XCHAL_HAVE_CP			1	
#define XCHAL_CP_MAXCFG			8	
#define XCHAL_HAVE_MAC16		1	
#define XCHAL_HAVE_VECTORFPU2005	0	
#define XCHAL_HAVE_FP			0	
#define XCHAL_HAVE_VECTRA1		0	
#define XCHAL_HAVE_VECTRALX		0	
#define XCHAL_HAVE_HIFI2		0	



#define XCHAL_NUM_WRITEBUFFER_ENTRIES	8	
#define XCHAL_INST_FETCH_WIDTH		4	
#define XCHAL_DATA_WIDTH		4	
#define XCHAL_UNALIGNED_LOAD_EXCEPTION	1	
#define XCHAL_UNALIGNED_STORE_EXCEPTION	1	

#define XCHAL_SW_VERSION		701001	

#define XCHAL_CORE_ID			"dc232b"	

#define XCHAL_CORE_DESCRIPTION		"Diamond 232L Standard Core Rev.B (LE)"
#define XCHAL_BUILD_UNIQUE_ID		0x0000BEEF	

#define XCHAL_HW_CONFIGID0		0xC56307FE	
#define XCHAL_HW_CONFIGID1		0x0D40BEEF	
#define XCHAL_HW_VERSION_NAME		"LX2.1.1"	
#define XCHAL_HW_VERSION_MAJOR		2210	
#define XCHAL_HW_VERSION_MINOR		1	
#define XCHAL_HW_VERSION		221001	
#define XCHAL_HW_REL_LX2		1
#define XCHAL_HW_REL_LX2_1		1
#define XCHAL_HW_REL_LX2_1_1		1
#define XCHAL_HW_CONFIGID_RELIABLE	1
#define XCHAL_HW_MIN_VERSION_MAJOR	2210	
#define XCHAL_HW_MIN_VERSION_MINOR	1	
#define XCHAL_HW_MIN_VERSION		221001	
#define XCHAL_HW_MAX_VERSION_MAJOR	2210	
#define XCHAL_HW_MAX_VERSION_MINOR	1	
#define XCHAL_HW_MAX_VERSION		221001	



#define XCHAL_ICACHE_LINESIZE		32	
#define XCHAL_DCACHE_LINESIZE		32	
#define XCHAL_ICACHE_LINEWIDTH		5	
#define XCHAL_DCACHE_LINEWIDTH		5	

#define XCHAL_ICACHE_SIZE		16384	
#define XCHAL_DCACHE_SIZE		16384	

#define XCHAL_DCACHE_IS_WRITEBACK	1	






#ifndef XTENSA_HAL_NON_PRIVILEGED_ONLY


#define XCHAL_HAVE_PIF			1	


#define XCHAL_ICACHE_SETWIDTH		7
#define XCHAL_DCACHE_SETWIDTH		7

#define XCHAL_ICACHE_WAYS		4
#define XCHAL_DCACHE_WAYS		4

#define XCHAL_ICACHE_LINE_LOCKABLE	1
#define XCHAL_DCACHE_LINE_LOCKABLE	1
#define XCHAL_ICACHE_ECC_PARITY		0
#define XCHAL_DCACHE_ECC_PARITY		0

#define XCHAL_CA_BITS			4



#define XCHAL_NUM_INSTROM		0	
#define XCHAL_NUM_INSTRAM		0	
#define XCHAL_NUM_DATAROM		0	
#define XCHAL_NUM_DATARAM		0	
#define XCHAL_NUM_URAM			0	
#define XCHAL_NUM_XLMI			0	



#define XCHAL_HAVE_INTERRUPTS		1	
#define XCHAL_HAVE_HIGHPRI_INTERRUPTS	1	
#define XCHAL_HAVE_NMI			1	
#define XCHAL_HAVE_CCOUNT		1	
#define XCHAL_NUM_TIMERS		3	
#define XCHAL_NUM_INTERRUPTS		22	
#define XCHAL_NUM_INTERRUPTS_LOG2	5	
#define XCHAL_NUM_EXTINTERRUPTS		17	
#define XCHAL_NUM_INTLEVELS		6	
#define XCHAL_EXCM_LEVEL		3	
	

#define XCHAL_INTLEVEL1_MASK		0x001F80FF
#define XCHAL_INTLEVEL2_MASK		0x00000100
#define XCHAL_INTLEVEL3_MASK		0x00200E00
#define XCHAL_INTLEVEL4_MASK		0x00001000
#define XCHAL_INTLEVEL5_MASK		0x00002000
#define XCHAL_INTLEVEL6_MASK		0x00000000
#define XCHAL_INTLEVEL7_MASK		0x00004000

#define XCHAL_INTLEVEL1_ANDBELOW_MASK	0x001F80FF
#define XCHAL_INTLEVEL2_ANDBELOW_MASK	0x001F81FF
#define XCHAL_INTLEVEL3_ANDBELOW_MASK	0x003F8FFF
#define XCHAL_INTLEVEL4_ANDBELOW_MASK	0x003F9FFF
#define XCHAL_INTLEVEL5_ANDBELOW_MASK	0x003FBFFF
#define XCHAL_INTLEVEL6_ANDBELOW_MASK	0x003FBFFF
#define XCHAL_INTLEVEL7_ANDBELOW_MASK	0x003FFFFF

#define XCHAL_INT0_LEVEL		1
#define XCHAL_INT1_LEVEL		1
#define XCHAL_INT2_LEVEL		1
#define XCHAL_INT3_LEVEL		1
#define XCHAL_INT4_LEVEL		1
#define XCHAL_INT5_LEVEL		1
#define XCHAL_INT6_LEVEL		1
#define XCHAL_INT7_LEVEL		1
#define XCHAL_INT8_LEVEL		2
#define XCHAL_INT9_LEVEL		3
#define XCHAL_INT10_LEVEL		3
#define XCHAL_INT11_LEVEL		3
#define XCHAL_INT12_LEVEL		4
#define XCHAL_INT13_LEVEL		5
#define XCHAL_INT14_LEVEL		7
#define XCHAL_INT15_LEVEL		1
#define XCHAL_INT16_LEVEL		1
#define XCHAL_INT17_LEVEL		1
#define XCHAL_INT18_LEVEL		1
#define XCHAL_INT19_LEVEL		1
#define XCHAL_INT20_LEVEL		1
#define XCHAL_INT21_LEVEL		3
#define XCHAL_DEBUGLEVEL		6	
#define XCHAL_HAVE_DEBUG_EXTERN_INT	1	
#define XCHAL_NMILEVEL			7	

#define XCHAL_INT0_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT1_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT2_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT3_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT4_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT5_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT6_TYPE 	XTHAL_INTTYPE_TIMER
#define XCHAL_INT7_TYPE 	XTHAL_INTTYPE_SOFTWARE
#define XCHAL_INT8_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT9_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT10_TYPE 	XTHAL_INTTYPE_TIMER
#define XCHAL_INT11_TYPE 	XTHAL_INTTYPE_SOFTWARE
#define XCHAL_INT12_TYPE 	XTHAL_INTTYPE_EXTERN_LEVEL
#define XCHAL_INT13_TYPE 	XTHAL_INTTYPE_TIMER
#define XCHAL_INT14_TYPE 	XTHAL_INTTYPE_NMI
#define XCHAL_INT15_TYPE 	XTHAL_INTTYPE_EXTERN_EDGE
#define XCHAL_INT16_TYPE 	XTHAL_INTTYPE_EXTERN_EDGE
#define XCHAL_INT17_TYPE 	XTHAL_INTTYPE_EXTERN_EDGE
#define XCHAL_INT18_TYPE 	XTHAL_INTTYPE_EXTERN_EDGE
#define XCHAL_INT19_TYPE 	XTHAL_INTTYPE_EXTERN_EDGE
#define XCHAL_INT20_TYPE 	XTHAL_INTTYPE_EXTERN_EDGE
#define XCHAL_INT21_TYPE 	XTHAL_INTTYPE_EXTERN_EDGE

#define XCHAL_INTTYPE_MASK_UNCONFIGURED	0xFFC00000
#define XCHAL_INTTYPE_MASK_SOFTWARE	0x00000880
#define XCHAL_INTTYPE_MASK_EXTERN_EDGE	0x003F8000
#define XCHAL_INTTYPE_MASK_EXTERN_LEVEL	0x0000133F
#define XCHAL_INTTYPE_MASK_TIMER	0x00002440
#define XCHAL_INTTYPE_MASK_NMI		0x00004000
#define XCHAL_INTTYPE_MASK_WRITE_ERROR	0x00000000

#define XCHAL_TIMER0_INTERRUPT		6	
#define XCHAL_TIMER1_INTERRUPT		10	
#define XCHAL_TIMER2_INTERRUPT		13	
#define XCHAL_TIMER3_INTERRUPT		XTHAL_TIMER_UNCONFIGURED
#define XCHAL_NMI_INTERRUPT		14	

#define XCHAL_INTLEVEL2_NUM		8
#define XCHAL_INTLEVEL4_NUM		12
#define XCHAL_INTLEVEL5_NUM		13
#define XCHAL_INTLEVEL7_NUM		14



#define XCHAL_EXTINT0_NUM		0	
#define XCHAL_EXTINT1_NUM		1	
#define XCHAL_EXTINT2_NUM		2	
#define XCHAL_EXTINT3_NUM		3	
#define XCHAL_EXTINT4_NUM		4	
#define XCHAL_EXTINT5_NUM		5	
#define XCHAL_EXTINT6_NUM		8	
#define XCHAL_EXTINT7_NUM		9	
#define XCHAL_EXTINT8_NUM		12	
#define XCHAL_EXTINT9_NUM		14	
#define XCHAL_EXTINT10_NUM		15	
#define XCHAL_EXTINT11_NUM		16	
#define XCHAL_EXTINT12_NUM		17	
#define XCHAL_EXTINT13_NUM		18	
#define XCHAL_EXTINT14_NUM		19	
#define XCHAL_EXTINT15_NUM		20	
#define XCHAL_EXTINT16_NUM		21	



#define XCHAL_XEA_VERSION		2	
#define XCHAL_HAVE_XEA1			0	
#define XCHAL_HAVE_XEA2			1	
#define XCHAL_HAVE_XEAX			0	
#define XCHAL_HAVE_EXCEPTIONS		1	
#define XCHAL_HAVE_MEM_ECC_PARITY	0	
#define XCHAL_HAVE_VECTOR_SELECT	1	
#define XCHAL_HAVE_VECBASE		1	
#define XCHAL_VECBASE_RESET_VADDR	0xD0000000  
#define XCHAL_VECBASE_RESET_PADDR	0x00000000
#define XCHAL_RESET_VECBASE_OVERLAP	0

#define XCHAL_RESET_VECTOR0_VADDR	0xFE000000
#define XCHAL_RESET_VECTOR0_PADDR	0xFE000000
#define XCHAL_RESET_VECTOR1_VADDR	0xD8000500
#define XCHAL_RESET_VECTOR1_PADDR	0x00000500
#define XCHAL_RESET_VECTOR_VADDR	0xFE000000
#define XCHAL_RESET_VECTOR_PADDR	0xFE000000
#define XCHAL_USER_VECOFS		0x00000340
#define XCHAL_USER_VECTOR_VADDR		0xD0000340
#define XCHAL_USER_VECTOR_PADDR		0x00000340
#define XCHAL_KERNEL_VECOFS		0x00000300
#define XCHAL_KERNEL_VECTOR_VADDR	0xD0000300
#define XCHAL_KERNEL_VECTOR_PADDR	0x00000300
#define XCHAL_DOUBLEEXC_VECOFS		0x000003C0
#define XCHAL_DOUBLEEXC_VECTOR_VADDR	0xD00003C0
#define XCHAL_DOUBLEEXC_VECTOR_PADDR	0x000003C0
#define XCHAL_WINDOW_OF4_VECOFS		0x00000000
#define XCHAL_WINDOW_UF4_VECOFS		0x00000040
#define XCHAL_WINDOW_OF8_VECOFS		0x00000080
#define XCHAL_WINDOW_UF8_VECOFS		0x000000C0
#define XCHAL_WINDOW_OF12_VECOFS	0x00000100
#define XCHAL_WINDOW_UF12_VECOFS	0x00000140
#define XCHAL_WINDOW_VECTORS_VADDR	0xD0000000
#define XCHAL_WINDOW_VECTORS_PADDR	0x00000000
#define XCHAL_INTLEVEL2_VECOFS		0x00000180
#define XCHAL_INTLEVEL2_VECTOR_VADDR	0xD0000180
#define XCHAL_INTLEVEL2_VECTOR_PADDR	0x00000180
#define XCHAL_INTLEVEL3_VECOFS		0x000001C0
#define XCHAL_INTLEVEL3_VECTOR_VADDR	0xD00001C0
#define XCHAL_INTLEVEL3_VECTOR_PADDR	0x000001C0
#define XCHAL_INTLEVEL4_VECOFS		0x00000200
#define XCHAL_INTLEVEL4_VECTOR_VADDR	0xD0000200
#define XCHAL_INTLEVEL4_VECTOR_PADDR	0x00000200
#define XCHAL_INTLEVEL5_VECOFS		0x00000240
#define XCHAL_INTLEVEL5_VECTOR_VADDR	0xD0000240
#define XCHAL_INTLEVEL5_VECTOR_PADDR	0x00000240
#define XCHAL_INTLEVEL6_VECOFS		0x00000280
#define XCHAL_INTLEVEL6_VECTOR_VADDR	0xD0000280
#define XCHAL_INTLEVEL6_VECTOR_PADDR	0x00000280
#define XCHAL_DEBUG_VECOFS		XCHAL_INTLEVEL6_VECOFS
#define XCHAL_DEBUG_VECTOR_VADDR	XCHAL_INTLEVEL6_VECTOR_VADDR
#define XCHAL_DEBUG_VECTOR_PADDR	XCHAL_INTLEVEL6_VECTOR_PADDR
#define XCHAL_NMI_VECOFS		0x000002C0
#define XCHAL_NMI_VECTOR_VADDR		0xD00002C0
#define XCHAL_NMI_VECTOR_PADDR		0x000002C0
#define XCHAL_INTLEVEL7_VECOFS		XCHAL_NMI_VECOFS
#define XCHAL_INTLEVEL7_VECTOR_VADDR	XCHAL_NMI_VECTOR_VADDR
#define XCHAL_INTLEVEL7_VECTOR_PADDR	XCHAL_NMI_VECTOR_PADDR



#define XCHAL_HAVE_OCD			1	
#define XCHAL_NUM_IBREAK		2	
#define XCHAL_NUM_DBREAK		2	
#define XCHAL_HAVE_OCD_DIR_ARRAY	1	




#define XCHAL_HAVE_TLBS			1	
#define XCHAL_HAVE_SPANNING_WAY		0	
#define XCHAL_HAVE_IDENTITY_MAP		0	
#define XCHAL_HAVE_CACHEATTR		0	
#define XCHAL_HAVE_MIMIC_CACHEATTR	0	
#define XCHAL_HAVE_XLT_CACHEATTR	0	
#define XCHAL_HAVE_PTP_MMU		1	
#define XCHAL_ITLB_ARF_ENTRIES_LOG2	2	
#define XCHAL_DTLB_ARF_ENTRIES_LOG2	2	

#define XCHAL_MMU_ASID_BITS		8	
#define XCHAL_MMU_RINGS			4	
#define XCHAL_MMU_RING_BITS		2	

#endif 


#endif 

