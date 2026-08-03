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
--  Description : Basic type definitions.
--
------------------------------------------------------------------------------*/

/*!\file
 * \brief Basic type definitions.
 *
 * Basic numeric data type definitions used in the decoder software.
 */


#ifndef __BASETYPE_H__
#define __BASETYPE_H__

#include "vc_types.h"

#ifndef VC_EXPORT_SYMBOL
#define VC_EXPORT_SYMBOL(symbol)
#endif


/*! \addtogroup common Common definitions
 *  @{
 */

#if defined( __linux__ ) || defined( WIN32 )
//#include <stddef.h>
#endif

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

/* Macro to signal unused parameter. */
#define UNUSED(x) (void)(x)

typedef vc_u8 u8; /**< unsigned 8 bits integer value */
typedef vc_u16 u16; /**< unsigned 16 bits integer value */
typedef vc_u32 u32; /**< unsigned 32 bits integer value */
typedef vc_u64 u64; /**< unsigned 64 bits integer value */

typedef vc_s8 i8; /**< signed 8 bits integer value */
typedef vc_s16 i16; /**< signed 16 bits integer value */
typedef vc_s32 i32; /**< signed 32 bits integer value */
typedef vc_s64 i64;

typedef vc_u64 g1_addr_t;


/*!\cond SWDEC*/
/* SW decoder 16 bits types */
#if defined(VC1SWDEC_16BIT) || defined(MP4ENC_ARM11)
typedef unsigned short u16x;
typedef signed short i16x;
#else
typedef unsigned int u16x;
typedef signed int i16x;
#endif
/*!\endcond */


/*-----------------------------------------------------*/
/*  ychuang added                                      */
/*-----------------------------------------------------*/

#define DWL_USE_DEC_IRQ

#define DEC_TIMEOUT	5000
#define PP_TIMEOUT	5000

/* H264 Debug message setting */
//#define H264DEC_TRACE
//#define PP_TRACE
//#define MEMORY_USAGE_TRACE
//#define _DWL_DEBUG
//#define _DEBUG_PRINT

/* PP debug setting */
//#define TRACE_PP_CTRL   DWLLog(DWL_LOG_INFO, __VA_ARGS__)

/* JPEG debug setting */
//#define JPEGDEC_TRACE


/* Configuration */
#define PP_H264DEC_PIPELINE_SUPPORT
#define PP_JPEGDEC_PIPELINE_SUPPORT
#define PP_PIPELINE_ENABLED

/* data type */
//#define off64_t		uint64_t
//#define sem_t           int


//typedef unsigned int    __u32;

/*! @} - end group common */

#endif /* __BASETYPE_H__ */



