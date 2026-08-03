/* Copyright 2012 Google Inc. All Rights Reserved. */

#ifndef _HX170DEC_H_
#define _HX170DEC_H_
//#include <linux/ioctl.h>
//#include <linux/types.h>

#include "basetype.h"


struct core_desc
{
    __u32 id; /* id of the core */
    __u32 *regs; /* pointer to user registers */
    __u32 size; /* size of register space */
};

/* Use 'k' as magic number */
#define HX170DEC_IOC_MAGIC  'k'

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define HX170DEC_PP_INSTANCE       1  // _IO(HX170DEC_IOC_MAGIC, 1)
#define HX170DEC_HW_PERFORMANCE    2  // _IO(HX170DEC_IOC_MAGIC, 2)
#define HX170DEC_IOCGHWOFFSET      3  // _IOR(HX170DEC_IOC_MAGIC,  3, unsigned long *)
#define HX170DEC_IOCGHWIOSIZE      4  // _IOR(HX170DEC_IOC_MAGIC,  4, unsigned int *)

#define HX170DEC_IOC_CLI           5  //_IO(HX170DEC_IOC_MAGIC,  5)
#define HX170DEC_IOC_STI           6  //_IO(HX170DEC_IOC_MAGIC,  6)
#define HX170DEC_IOC_MC_OFFSETS    7  //_IOR(HX170DEC_IOC_MAGIC, 7, unsigned long *)
#define HX170DEC_IOC_MC_CORES      8  //_IOR(HX170DEC_IOC_MAGIC, 8, unsigned int *)


#define HX170DEC_IOCS_DEC_PUSH_REG  9  // _IOW(HX170DEC_IOC_MAGIC, 9, struct core_desc *)
#define HX170DEC_IOCS_PP_PUSH_REG   10  //_IOW(HX170DEC_IOC_MAGIC, 10, struct core_desc *)

#define HX170DEC_IOCH_DEC_RESERVE   11  //_IO(HX170DEC_IOC_MAGIC, 11)
#define HX170DEC_IOCT_DEC_RELEASE   12  //_IO(HX170DEC_IOC_MAGIC, 12)
#define HX170DEC_IOCQ_PP_RESERVE    13  //_IO(HX170DEC_IOC_MAGIC, 13)
#define HX170DEC_IOCT_PP_RELEASE    14  //_IO(HX170DEC_IOC_MAGIC, 14)

#define HX170DEC_IOCX_DEC_WAIT      15  //_IOWR(HX170DEC_IOC_MAGIC, 15, struct core_desc *)
#define HX170DEC_IOCX_PP_WAIT       16  //_IOWR(HX170DEC_IOC_MAGIC, 16, struct core_desc *)

#define HX170DEC_IOCS_DEC_PULL_REG  17  //_IOWR(HX170DEC_IOC_MAGIC, 17, struct core_desc *)
#define HX170DEC_IOCS_PP_PULL_REG   18  //_IOWR(HX170DEC_IOC_MAGIC, 18, struct core_desc *)

#define HX170DEC_IOCG_CORE_WAIT     19  //_IOR(HX170DEC_IOC_MAGIC, 19, int *)

#define HX170DEC_IOX_ASIC_ID        20  //_IOWR(HX170DEC_IOC_MAGIC, 20, __u32 *)

#define HX170DEC_DEBUG_STATUS       29  //_IO(HX170DEC_IOC_MAGIC, 29)

#define HX170DEC_IOC_MAXNR 29

#endif /* !_HX170DEC_H_ */
