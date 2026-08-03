/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  dwl common part header file
--
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "dwl_defs.h"
#include "dwl.h"
#include "dwlthread.h"
//#include "memalloc.h"
//#include <assert.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <sys/timeb.h>
//#include <sys/types.h>


#ifndef DEC_MODULE_PATH
#define DEC_MODULE_PATH         "/tmp/dev/hx170"
#endif

#ifndef MEMALLOC_MODULE_PATH
#define MEMALLOC_MODULE_PATH    "/tmp/dev/memalloc"
#endif

#define HX170PP_REG_START       0xF0
#define HX170DEC_REG_START      0x4

#define HX170PP_FUSE_CFG         99
#define HX170DEC_FUSE_CFG        57

#define DWL_DECODER_INT ((DWLReadReg(dec_dwl, HX170DEC_REG_START) >> 11) & 0xFFU)
#define DWL_PP_INT      ((DWLReadReg(dec_dwl, HX170PP_REG_START) >> 11) & 0xFFU)

#define DEC_IRQ_ABORT          (1 << 11)
#define DEC_IRQ_RDY            (1 << 12)
#define DEC_IRQ_BUS            (1 << 13)
#define DEC_IRQ_BUFFER         (1 << 14)
#define DEC_IRQ_ASO            (1 << 15)
#define DEC_IRQ_ERROR          (1 << 16)
#define DEC_IRQ_SLICE          (1 << 17)
#define DEC_IRQ_TIMEOUT        (1 << 18)

#define PP_IRQ_RDY             (1 << 12)
#define PP_IRQ_BUS             (1 << 13)

#define DWL_HW_ENABLE_BIT       0x000001    /* 0th bit */

#ifdef _DWL_HW_PERFORMANCE
/* signal that decoder/pp is enabled */
void DwlDecoderEnable(void);
void DwlPpEnable(void);
#endif

//typedef struct mc_listener_thread_params_
//{
//    int fd;
//    int bStopped;
//    unsigned int nDecCores;
//    unsigned int nPPCores;
//    sem_t sc_dec_rdy_sem[MAX_ASIC_CORES];
//    sem_t sc_pp_rdy_sem[MAX_ASIC_CORES];
//    DWLIRQCallbackFn *pCallback[MAX_ASIC_CORES];
//    void *pCallbackArg[MAX_ASIC_CORES];
//} mc_listener_thread_params;

/* wrapper information */
typedef struct hX170dwl
{
    u32 clientType;
    int fd;                  /* decoder device file */
    int fd_mem;              /* /dev/mem for mapping */
    int fd_memalloc;         /* linear memory allocator */
    u32 numCores;
    u32 regSize;             /* IO mem size */
    g1_addr_t freeLinMem;    /* Start address of free linear memory */
    g1_addr_t freeRefFrmMem; /* Start address of free reference frame memory */
    int semid;
    int sigio_needed;
    struct mc_listener_thread_params_ *sync_params;

    u32 bPPReserved;
} hX170dwl_t;

i32 DWLWaitPpHwReady(const void *instance, i32 coreID, u32 timeout);
i32 DWLWaitDecHwReady(const void *instance, i32 coreID, u32 timeout);
u32 *DWLMapRegisters(int mem_dev, unsigned long base,
                     unsigned int regSize, u32 write);
void DWLUnmapRegisters(const void *io, unsigned int regSize);
