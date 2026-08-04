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
--  Description :  dwl common part
--
------------------------------------------------------------------------------*/

#include <linux/mutex.h>

#include "hantro.h"

#include "basetype.h"
#include "dwl_defs.h"
#include "dwl_linux.h"
#include "linux/vc_os_linux.h"
#include "dwl.h"
#include "dwlthread.h"
//#include "memalloc.h"

//#include <assert.h>
//#include <errno.h>
//#include <fcntl.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <sys/syscall.h>
//#include <sys/timeb.h>
//#include <sys/types.h>
//#include <unistd.h>

#ifdef INTERNAL_TEST
#include "internal_test.h"
#endif


//#undef MEMORY_USAGE_TRACE
#define MEMORY_USAGE_TRACE


#define DWL_PJPEG_E         22  /* 1 bit */
#define DWL_REF_BUFF_E      20  /* 1 bit */

#define DWL_JPEG_EXT_E          31  /* 1 bit */
#define DWL_REF_BUFF_ILACE_E    30  /* 1 bit */
#define DWL_MPEG4_CUSTOM_E      29  /* 1 bit */
#define DWL_REF_BUFF_DOUBLE_E   28  /* 1 bit */

#define DWL_MVC_E           20  /* 2 bits */

#define DWL_DEC_TILED_L     17  /* 2 bits */
#define DWL_DEC_PIC_W_EXT   14  /* 2 bits */
#define DWL_EC_E            12  /* 2 bits */
#define DWL_STRIDE_E        11  /* 1 bit */
#define DWL_FIELD_DPB_E     10  /* 1 bit */
#define DWL_AVS_PLUS_E       6  /* 1 bit */
#define DWL_64BIT_ENV_E      5  /* 1 bit */

#define DWL_CFG_E           24  /* 4 bits */
#define DWL_PP_IN_TILED_L   14  /* 2 bits */

#define DWL_SORENSONSPARK_E 11  /* 1 bit */

#define DWL_H264_FUSE_E          31 /* 1 bit */
#define DWL_MPEG4_FUSE_E         30 /* 1 bit */
#define DWL_MPEG2_FUSE_E         29 /* 1 bit */
#define DWL_SORENSONSPARK_FUSE_E 28 /* 1 bit */
#define DWL_JPEG_FUSE_E          27 /* 1 bit */
#define DWL_VP6_FUSE_E           26 /* 1 bit */
#define DWL_VC1_FUSE_E           25 /* 1 bit */
#define DWL_PJPEG_FUSE_E         24 /* 1 bit */
#define DWL_CUSTOM_MPEG4_FUSE_E  23 /* 1 bit */
#define DWL_RV_FUSE_E            22 /* 1 bit */
#define DWL_VP7_FUSE_E           21 /* 1 bit */
#define DWL_VP8_FUSE_E           20 /* 1 bit */
#define DWL_AVS_FUSE_E           19 /* 1 bit */
#define DWL_MVC_FUSE_E           18 /* 1 bit */

#define DWL_DEC_MAX_1920_FUSE_E  15 /* 1 bit */
#define DWL_DEC_MAX_1280_FUSE_E  14 /* 1 bit */
#define DWL_DEC_MAX_720_FUSE_E   13 /* 1 bit */
#define DWL_DEC_MAX_352_FUSE_E   12 /* 1 bit */
#define DWL_REF_BUFF_FUSE_E       7 /* 1 bit */


#define DWL_PP_FUSE_E                           31  /* 1 bit */
#define DWL_PP_DEINTERLACE_FUSE_E   30  /* 1 bit */
#define DWL_PP_ALPHA_BLEND_FUSE_E   29  /* 1 bit */
#define DWL_PP_MAX_4096_FUSE_E          16  /* 1 bit */
#define DWL_PP_MAX_1920_FUSE_E          15  /* 1 bit */
#define DWL_PP_MAX_1280_FUSE_E          14  /* 1 bit */
#define DWL_PP_MAX_720_FUSE_E           13  /* 1 bit */
#define DWL_PP_MAX_352_FUSE_E           12  /* 1 bit */

#ifdef _DWL_FAKE_HW_TIMEOUT
static void DWLFakeTimeout(u32 * status);
#endif

#define IS_PIPELINE_ENABLED(val)    ((val) & 0x02)

/* shadow HW registers */
u32 dwlShadowRegs[MAX_ASIC_CORES][154];

/* shadow id/config registers */
u32 dwlShadowConfigRegs[MAX_ASIC_CORES][154] = {0};


#ifdef _DWL_DEBUG

static void PrintIrqType(u32 isPP, u32 coreID, u32 status)
{
	if(isPP)
	{
	dev_info(_vc8k_vpu->dev, "PP[%d] IRQ %s\n", coreID,
		status & PP_IRQ_RDY ? "READY" : "BUS ERROR");
	}
	else
	{
	if(status & DEC_IRQ_ABORT)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ ABORT\n", coreID);
	else if (status & DEC_IRQ_RDY)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ READY\n", coreID);
	else if (status & DEC_IRQ_BUS)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ BUS ERROR\n", coreID);
	else if (status & DEC_IRQ_BUFFER)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ BUFFER\n", coreID);
	else if (status & DEC_IRQ_ASO)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ ASO\n", coreID);
	else if (status & DEC_IRQ_ERROR)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ STREAM ERROR\n", coreID);
	else if (status & DEC_IRQ_SLICE)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ SLICE\n", coreID);
	else if (status & DEC_IRQ_TIMEOUT)
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ TIMEOUT\n", coreID);
	else
		dev_info(_vc8k_vpu->dev, "DEC[%d] IRQ UNKNOWN 0x%08x\n", coreID, status);
	}
}
#endif


/*------------------------------------------------------------------------------
	Function name   : DWLReadAsicCoreCount
	Description     : Return the number of hardware cores available
------------------------------------------------------------------------------*/
u32 DWLReadAsicCoreCount(void)
{
	//unsigned int cores = 0;

	//hx170dec_ioctl(HX170DEC_IOC_MC_CORES, &cores);

	return 1; // (u32)cores;
}

/*------------------------------------------------------------------------------
	Function name   : DWLReadAsicID
	Description     : Read the HW ID. Does not need a DWL instance to run

	Return type     : u32 - the HW ID
------------------------------------------------------------------------------*/
u32 DWLReadAsicID()
{
	u32 id;

	/* id from shadow regs */
	if(dwlShadowConfigRegs[0][0] != 0x00000000)
	return (u32)dwlShadowConfigRegs[0][0];

	/* to shadow regs */
	id = dwlShadowConfigRegs[0][0] = vc_os_mmio_read32(0);

	return id;
}
EXPORT_SYMBOL(DWLReadAsicID);

static void ReadCoreFuse(DWLHwFuseStatus_t *pHwFuseSts)
{
	u32 configReg, fuseReg, fuseRegPp;

	/* Decoder configuration */
	configReg = vc_os_mmio_read32(HX170DEC_SYNTH_CFG);

	/* Decoder fuse configuration */
	fuseReg = vc_os_mmio_read32(HX170DEC_FUSE_CFG);

	pHwFuseSts->h264SupportFuse = (fuseReg >> DWL_H264_FUSE_E) & 0x01U;
	pHwFuseSts->mpeg4SupportFuse = (fuseReg >> DWL_MPEG4_FUSE_E) & 0x01U;
	pHwFuseSts->mpeg2SupportFuse = (fuseReg >> DWL_MPEG2_FUSE_E) & 0x01U;
	pHwFuseSts->sorensonSparkSupportFuse =
		(fuseReg >> DWL_SORENSONSPARK_FUSE_E) & 0x01U;
	pHwFuseSts->jpegSupportFuse = (fuseReg >> DWL_JPEG_FUSE_E) & 0x01U;
	pHwFuseSts->vp6SupportFuse = (fuseReg >> DWL_VP6_FUSE_E) & 0x01U;
	pHwFuseSts->vc1SupportFuse = (fuseReg >> DWL_VC1_FUSE_E) & 0x01U;
	pHwFuseSts->jpegProgSupportFuse = (fuseReg >> DWL_PJPEG_FUSE_E) & 0x01U;
	pHwFuseSts->rvSupportFuse = (fuseReg >> DWL_RV_FUSE_E) & 0x01U;
	pHwFuseSts->avsSupportFuse = (fuseReg >> DWL_AVS_FUSE_E) & 0x01U;
	pHwFuseSts->vp7SupportFuse = (fuseReg >> DWL_VP7_FUSE_E) & 0x01U;
	pHwFuseSts->vp8SupportFuse = (fuseReg >> DWL_VP8_FUSE_E) & 0x01U;
	pHwFuseSts->customMpeg4SupportFuse = (fuseReg >> DWL_CUSTOM_MPEG4_FUSE_E) & 0x01U;
	pHwFuseSts->mvcSupportFuse = (fuseReg >> DWL_MVC_FUSE_E) & 0x01U;

	/* check max. decoder output width */
	if(fuseReg & 0x10000U)
	pHwFuseSts->maxDecPicWidthFuse = 4096;
	else if(fuseReg & 0x8000U)
	pHwFuseSts->maxDecPicWidthFuse = 1920;
	else if(fuseReg & 0x4000U)
	pHwFuseSts->maxDecPicWidthFuse = 1280;
	else if(fuseReg & 0x2000U)
	pHwFuseSts->maxDecPicWidthFuse = 720;
	else if(fuseReg & 0x1000U)
	pHwFuseSts->maxDecPicWidthFuse = 352;

	pHwFuseSts->refBufSupportFuse = (fuseReg >> DWL_REF_BUFF_FUSE_E) & 0x01U;

	/* Pp configuration */
	configReg = vc_os_mmio_read32(HX170PP_SYNTH_CFG);

	if((configReg >> DWL_PP_E) & 0x01U)
	{
	/* Pp fuse configuration */
	fuseRegPp = vc_os_mmio_read32(HX170PP_FUSE_CFG);

	if((fuseRegPp >> DWL_PP_FUSE_E) & 0x01U)
	{
		pHwFuseSts->ppSupportFuse = 1;

		/* check max. pp output width */
		if(fuseRegPp & 0x10000U)
		pHwFuseSts->maxPpOutPicWidthFuse = 4096;
		else if(fuseRegPp & 0x8000U)
		pHwFuseSts->maxPpOutPicWidthFuse = 1920;
		else if(fuseRegPp & 0x4000U)
		pHwFuseSts->maxPpOutPicWidthFuse = 1280;
		else if(fuseRegPp & 0x2000U)
		pHwFuseSts->maxPpOutPicWidthFuse = 720;
		else if(fuseRegPp & 0x1000U)
		pHwFuseSts->maxPpOutPicWidthFuse = 352;

		pHwFuseSts->ppConfigFuse = fuseRegPp;
	}
	else
	{
		pHwFuseSts->ppSupportFuse = 0;
		pHwFuseSts->maxPpOutPicWidthFuse = 0;
		pHwFuseSts->ppConfigFuse = 0;
	}
	}
}

static void ReadCoreConfig(DWLHwConfig_t *pHwCfg)
{
	u32 configReg;
	const u32 asicID = vc_os_mmio_read32(0);

	/* Decoder configuration */
	configReg = vc_os_mmio_read32(HX170DEC_SYNTH_CFG);

	pHwCfg->h264Support = (configReg >> DWL_H264_E) & 0x3U;
	/* check jpeg */
	pHwCfg->jpegSupport = (configReg >> DWL_JPEG_E) & 0x01U;
	if(pHwCfg->jpegSupport && ((configReg >> DWL_PJPEG_E) & 0x01U))
	pHwCfg->jpegSupport = JPEG_PROGRESSIVE;
	pHwCfg->mpeg4Support = (configReg >> DWL_MPEG4_E) & 0x3U;
	pHwCfg->vc1Support = (configReg >> DWL_VC1_E) & 0x3U;
	pHwCfg->mpeg2Support = (configReg >> DWL_MPEG2_E) & 0x01U;
	pHwCfg->sorensonSparkSupport = (configReg >> DWL_SORENSONSPARK_E) & 0x01U;
#ifndef DWL_REFBUFFER_DISABLE
	pHwCfg->refBufSupport = (configReg >> DWL_REF_BUFF_E) & 0x01U;
#else
	pHwCfg->refBufSupport = 0;
#endif
	pHwCfg->vp6Support = (configReg >> DWL_VP6_E) & 0x01U;
#ifdef DEC_X170_APF_DISABLE
	if(DEC_X170_APF_DISABLE)
	{
	pHwCfg->tiledModeSupport = 0;
	}
#endif /* DEC_X170_APF_DISABLE */

	pHwCfg->maxDecPicWidth = configReg & 0x07FFU;

	/* 2nd Config register */
	configReg = vc_os_mmio_read32(HX170DEC_SYNTH_CFG_2);
	if(pHwCfg->refBufSupport)
	{
	if((configReg >> DWL_REF_BUFF_ILACE_E) & 0x01U)
		pHwCfg->refBufSupport |= 2;
	if((configReg >> DWL_REF_BUFF_DOUBLE_E) & 0x01U)
		pHwCfg->refBufSupport |= 4;
	}

	pHwCfg->customMpeg4Support = (configReg >> DWL_MPEG4_CUSTOM_E) & 0x01U;
	pHwCfg->vp7Support = (configReg >> DWL_VP7_E) & 0x01U;
	pHwCfg->vp8Support = (configReg >> DWL_VP8_E) & 0x01U;
	pHwCfg->avsSupport = (configReg >> DWL_AVS_E) & 0x01U;

	/* JPEG extensions */
	if(((asicID >> 16) >= 0x8190U) || ((asicID >> 16) == 0x6731U) ||
	   ((asicID >> 16) == 0x6e64U))
	pHwCfg->jpegESupport = (configReg >> DWL_JPEG_EXT_E) & 0x01U;
	else
	pHwCfg->jpegESupport = JPEG_EXT_NOT_SUPPORTED;

	if(((asicID >> 16) >= 0x9170U) || ((asicID >> 16) == 0x6731U) ||
	   ((asicID >> 16) == 0x6e64U))
	pHwCfg->rvSupport = (configReg >> DWL_RV_E) & 0x03U;
	else
	pHwCfg->rvSupport = RV_NOT_SUPPORTED;

	pHwCfg->mvcSupport = (configReg >> DWL_MVC_E) & 0x03U;
	pHwCfg->webpSupport = (configReg >> DWL_WEBP_E) & 0x01U;
	pHwCfg->tiledModeSupport = (configReg >> DWL_DEC_TILED_L) & 0x03U;
	pHwCfg->maxDecPicWidth += (( configReg >> DWL_DEC_PIC_W_EXT) & 0x03U ) << 11;

	pHwCfg->ecSupport = (configReg >> DWL_EC_E) & 0x03U;
	pHwCfg->strideSupport = (configReg >> DWL_STRIDE_E) & 0x01U;
	pHwCfg->fieldDpbSupport = (configReg >> DWL_FIELD_DPB_E) & 0x01U;
	pHwCfg->avsPlusSupport = (configReg >> DWL_AVS_PLUS_E) & 0x01U;
	pHwCfg->addr64Support = (configReg >> DWL_64BIT_ENV_E) & 0x01U;

	if(pHwCfg->refBufSupport && (((asicID >> 16) == 0x6731U) ||
				 ((asicID >> 16) == 0x6e64U)))
	{
	pHwCfg->refBufSupport |= 8; /* enable HW support for offset */
	}

	/* Pp configuration */
	configReg = vc_os_mmio_read32(HX170PP_SYNTH_CFG);

	if((configReg >> DWL_PP_E) & 0x01U)
	{
	pHwCfg->ppSupport = 1;
	/* Theoretical max range 0...8191; actual 48...4096 */
	pHwCfg->maxPpOutPicWidth = configReg & 0x1FFFU;
	/*pHwCfg->ppConfig = (configReg >> DWL_CFG_E) & 0x0FU; */
	pHwCfg->ppConfig = configReg;
	}
	else
	{
	pHwCfg->ppSupport = 0;
	pHwCfg->maxPpOutPicWidth = 0;
	pHwCfg->ppConfig = 0;
	}

	/* check the HW version */
	if(((asicID >> 16) >= 0x8190U) || ((asicID >> 16) == 0x6731U) ||
	   ((asicID >> 16) == 0x6e64U))
	{
	u32 deInterlace;
	u32 alphaBlend;
	u32 deInterlaceFuse;
	u32 alphaBlendFuse;
	DWLHwFuseStatus_t hwFuseSts;

	/* check fuse status */
	ReadCoreFuse(&hwFuseSts);

	/* Maximum decoding width supported by the HW */
	if(pHwCfg->maxDecPicWidth > hwFuseSts.maxDecPicWidthFuse)
		pHwCfg->maxDecPicWidth = hwFuseSts.maxDecPicWidthFuse;
	/* Maximum output width of Post-Processor */
	if(pHwCfg->maxPpOutPicWidth > hwFuseSts.maxPpOutPicWidthFuse)
		pHwCfg->maxPpOutPicWidth = hwFuseSts.maxPpOutPicWidthFuse;
	/* h264 */
	if(!hwFuseSts.h264SupportFuse)
		pHwCfg->h264Support = H264_NOT_SUPPORTED;
	/* mpeg-4 */
	if(!hwFuseSts.mpeg4SupportFuse)
		pHwCfg->mpeg4Support = MPEG4_NOT_SUPPORTED;
	/* custom mpeg-4 */
	if(!hwFuseSts.customMpeg4SupportFuse)
		pHwCfg->customMpeg4Support = MPEG4_CUSTOM_NOT_SUPPORTED;
	/* jpeg (baseline && progressive) */
	if(!hwFuseSts.jpegSupportFuse)
		pHwCfg->jpegSupport = JPEG_NOT_SUPPORTED;
	if((pHwCfg->jpegSupport == JPEG_PROGRESSIVE) &&
		!hwFuseSts.jpegProgSupportFuse)
		pHwCfg->jpegSupport = JPEG_BASELINE;
	/* mpeg-2 */
	if(!hwFuseSts.mpeg2SupportFuse)
		pHwCfg->mpeg2Support = MPEG2_NOT_SUPPORTED;
	/* vc-1 */
	if(!hwFuseSts.vc1SupportFuse)
		pHwCfg->vc1Support = VC1_NOT_SUPPORTED;
	/* vp6 */
	if(!hwFuseSts.vp6SupportFuse)
		pHwCfg->vp6Support = VP6_NOT_SUPPORTED;
	/* vp7 */
	if(!hwFuseSts.vp7SupportFuse)
		pHwCfg->vp7Support = VP7_NOT_SUPPORTED;
	/* vp8 */
	if(!hwFuseSts.vp8SupportFuse)
		pHwCfg->vp8Support = VP8_NOT_SUPPORTED;
	/* webp */
	if(!hwFuseSts.vp8SupportFuse)
		pHwCfg->webpSupport = WEBP_NOT_SUPPORTED;
	/* pp */
	if(!hwFuseSts.ppSupportFuse)
		pHwCfg->ppSupport = PP_NOT_SUPPORTED;
	/* check the pp config vs fuse status */
	if((pHwCfg->ppConfig & 0xFC000000) &&
		((hwFuseSts.ppConfigFuse & 0xF0000000) >> 5))
	{
		/* config */
		deInterlace = ((pHwCfg->ppConfig & PP_DEINTERLACING) >> 25);
		alphaBlend = ((pHwCfg->ppConfig & PP_ALPHA_BLENDING) >> 24);
		/* fuse */
		deInterlaceFuse =
			(((hwFuseSts.ppConfigFuse >> 5) & PP_DEINTERLACING) >> 25);
		alphaBlendFuse =
			(((hwFuseSts.ppConfigFuse >> 5) & PP_ALPHA_BLENDING) >> 24);

		/* check if */
		if(deInterlace && !deInterlaceFuse)
		pHwCfg->ppConfig &= 0xFD000000;
		if(alphaBlend && !alphaBlendFuse)
		pHwCfg->ppConfig &= 0xFE000000;
	}
	/* sorenson */
	if(!hwFuseSts.sorensonSparkSupportFuse)
		pHwCfg->sorensonSparkSupport = SORENSON_SPARK_NOT_SUPPORTED;
	/* ref. picture buffer */
	if(!hwFuseSts.refBufSupportFuse)
		pHwCfg->refBufSupport = REF_BUF_NOT_SUPPORTED;

	/* rv */
	if(!hwFuseSts.rvSupportFuse)
		pHwCfg->rvSupport = RV_NOT_SUPPORTED;
	/* avs */
	if(!hwFuseSts.avsSupportFuse)
		pHwCfg->avsSupport = AVS_NOT_SUPPORTED;
	/* mvc */
	if(!hwFuseSts.mvcSupportFuse)
		pHwCfg->mvcSupport = MVC_NOT_SUPPORTED;
	}
#if 1 // Nuvoton
	pHwCfg->webpSupport = 1;
#endif
}

/*------------------------------------------------------------------------------
	Function name   : DWLReadAsicConfig
	Description     : Read HW configuration. Does not need a DWL instance to run

	Return type     : DWLHwConfig_t - structure with HW configuration
------------------------------------------------------------------------------*/
void DWLReadAsicConfig(DWLHwConfig_t *pHwCfg)
{
	/* config from shadow regs */
	if(dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG] != 0x00000000 &&
	   dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG_2] != 0x00000000 &&
	   dwlShadowConfigRegs[0][HX170PP_SYNTH_CFG] != 0x00000000)
	{
	ReadCoreConfig(pHwCfg);
	return;
	}

	/* Decoder configuration */
	memset(pHwCfg, 0, sizeof(*pHwCfg));

	ReadCoreConfig(pHwCfg);

	/* to shadow regs */
	dwlShadowConfigRegs[0][0] = vc_os_mmio_read32(0);
	dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG] = vc_os_mmio_read32(HX170DEC_SYNTH_CFG);
	dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG_2] = vc_os_mmio_read32(HX170DEC_SYNTH_CFG_2);
	dwlShadowConfigRegs[0][HX170PP_SYNTH_CFG] = vc_os_mmio_read32(HX170PP_SYNTH_CFG);
	dwlShadowConfigRegs[0][HX170DEC_FUSE_CFG] = vc_os_mmio_read32(HX170DEC_FUSE_CFG);
	dwlShadowConfigRegs[0][HX170PP_FUSE_CFG] = vc_os_mmio_read32(HX170PP_FUSE_CFG);
}
EXPORT_SYMBOL(DWLReadAsicConfig);

void DWLReadMCAsicConfig(DWLHwConfig_t pHwCfg[MAX_ASIC_CORES])
{
	unsigned int i;

	//int fd = (-1), fd_dec = (-1);

	/* config from shadow regs */
	if(dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG]   != 0x00000000 &&
	   dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG_2] != 0x00000000 &&
	   dwlShadowConfigRegs[0][HX170PP_SYNTH_CFG] != 0x00000000)
	{
	// for (i = 0; i < nCores; i++)
	{
		ReadCoreConfig(pHwCfg);
		dev_info(_vc8k_vpu->dev, "DWLReadMCAsicConfig config from shadow regs error!\n");
		return;
	}
	}

	/* Decoder configuration */
	memset(pHwCfg, 0, MAX_ASIC_CORES * sizeof(*pHwCfg));

	{
	ReadCoreConfig(pHwCfg + i);

	/* to shadow regs */
	dwlShadowConfigRegs[i][0] = vc_os_mmio_read32(0);
	dwlShadowConfigRegs[i][HX170DEC_SYNTH_CFG] = vc_os_mmio_read32(HX170DEC_SYNTH_CFG);
	dwlShadowConfigRegs[i][HX170DEC_SYNTH_CFG_2] = vc_os_mmio_read32(HX170DEC_SYNTH_CFG_2);
	dwlShadowConfigRegs[i][HX170PP_SYNTH_CFG] = vc_os_mmio_read32(HX170PP_SYNTH_CFG);
	dwlShadowConfigRegs[i][HX170DEC_FUSE_CFG] = vc_os_mmio_read32(HX170DEC_FUSE_CFG);
	dwlShadowConfigRegs[i][HX170PP_FUSE_CFG] = vc_os_mmio_read32(HX170PP_FUSE_CFG);
	}
}
EXPORT_SYMBOL(DWLReadMCAsicConfig);

/*------------------------------------------------------------------------------
	Function name   : DWLReadAsicFuseStatus
	Description     : Read HW fuse configuration. Does not need a DWL instance to run

	Returns     : DWLHwFuseStatus_t * pHwFuseSts - structure with HW fuse configuration
------------------------------------------------------------------------------*/
void DWLReadAsicFuseStatus(DWLHwFuseStatus_t * pHwFuseSts)
{
	//int fd = (-1), fd_dec = (-1);

	memset(pHwFuseSts, 0, sizeof(*pHwFuseSts));

	/* id from shadow regs */
	if(dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG] != 0x00000000 &&
	   dwlShadowConfigRegs[0][HX170DEC_FUSE_CFG] != 0x00000000 &&
	   dwlShadowConfigRegs[0][HX170PP_SYNTH_CFG] != 0x00000000 &&
	   dwlShadowConfigRegs[0][HX170PP_FUSE_CFG] != 0x00000000)
	{
	/* Decoder fuse configuration */
	ReadCoreFuse(pHwFuseSts);
	}

	/* Decoder fuse configuration */
	ReadCoreFuse(pHwFuseSts);

	dwlShadowConfigRegs[0][HX170DEC_SYNTH_CFG] = vc_os_mmio_read32(HX170DEC_SYNTH_CFG);
	dwlShadowConfigRegs[0][HX170DEC_FUSE_CFG] = vc_os_mmio_read32(HX170DEC_FUSE_CFG);
	dwlShadowConfigRegs[0][HX170PP_SYNTH_CFG] = vc_os_mmio_read32(HX170PP_SYNTH_CFG);
	dwlShadowConfigRegs[0][HX170PP_FUSE_CFG] = vc_os_mmio_read32(HX170PP_FUSE_CFG);
}
EXPORT_SYMBOL(DWLReadAsicFuseStatus);

struct vc8k_dma_blk {
	void		*owner;
	void		*vaddr;		/* memory block virtual address */
	dma_addr_t	paddr;		/* memory block physical address */
	unsigned char	used;		/* 1: used; 0: free */
	int		contig_bcnt;	/* number of contiguos blocks allocated */
};

static struct vc8k_dma_blk *_vc8k_mb_pool;
static int	_mb_total;	/* total number of memory blocks */
static int	_mb_alloc_cnt;
static struct mutex  _mb_lock;

void *VC8K_V4L2MemAlloc(void *owner, dma_addr_t *dma_addr, int size);
int VC8K_V4L2MemFree(void *owner, dma_addr_t dma_addr);

int VC8K_PreAllocDMABuff(struct device *dev, struct hantro_dev *vpu)
{
	struct vc8k_config  *vc8k_cfg;
	size_t  rmem_allocated;
	int   i;

	vc8k_cfg = &(vpu->vc8k_cfg);
	_mb_total = vc8k_cfg->res_mem_size / PA_BLK_SIZE;
	
	_vc8k_mb_pool = kmalloc(sizeof(struct vc8k_dma_blk) * _mb_total, GFP_KERNEL);
	if (_vc8k_mb_pool == NULL)
		return -ENOMEM;
	memset(_vc8k_mb_pool, 0, sizeof(struct vc8k_dma_blk) * _mb_total);

	_mb_alloc_cnt = 0;
	mutex_init(&_mb_lock);

	rmem_allocated = 0;
	for (i = 0; i < _mb_total; i++) {
		_vc8k_mb_pool[i].owner = NULL;
		_vc8k_mb_pool[i].used = 0;
		_vc8k_mb_pool[i].contig_bcnt = 0;
		_vc8k_mb_pool[i].paddr = vc8k_cfg->res_mem_base + rmem_allocated;
		_vc8k_mb_pool[i].vaddr = (void *)(vc8k_cfg->res_mem_virt + rmem_allocated);
		rmem_allocated += PA_BLK_SIZE;
	}
	printk("reserved %d memory block (%d KB)\n", _mb_total, PA_BLK_SIZE / 1024);
	return 0;
}
EXPORT_SYMBOL(VC8K_PreAllocDMABuff);

void  VC8K_ReleaseDMABuff(struct device *dev)
{
	kfree(_vc8k_mb_pool);
	mutex_destroy(&_mb_lock);
}
EXPORT_SYMBOL(VC8K_ReleaseDMABuff);

void *VC8K_MemAlloc(void *owner, dma_addr_t *dma_addr, int size);
void *VC8K_MemGetVaddr(void *owner, dma_addr_t dma_addr);
int VC8K_MemFree(void *owner, dma_addr_t dma_addr);

void *VC8K_MemAlloc(void *owner, dma_addr_t *dma_addr, int size)
{
	int i, start; 
	int found, wanted;
	
	mutex_lock(&_mb_lock);
	
	start = -1;
	found = 0;
	wanted = (size + PA_BLK_SIZE - 1) / PA_BLK_SIZE;
	for (i = 0; i < _mb_total - wanted + 1; i++) {
		if (_vc8k_mb_pool[i].used == 0) {
			if (found == 0)
				start = i;
			found++;
			if (found >= wanted)
				break;
		} else {
			found = 0;
		}
	}
	
	if (found < wanted) {
		mutex_unlock(&_mb_lock);
		printk("%s failed to allocate %d KB!!! (%d / %d)\n", __func__,
		       size / 1024, _mb_alloc_cnt, _mb_total);
		return NULL;
	}
	
	/* Go allocate it */
	for (i = start; found > 0; i++, found--) {
		_vc8k_mb_pool[i].owner = owner;
		_vc8k_mb_pool[i].used = 1;
		_vc8k_mb_pool[i].contig_bcnt = found;
	}
	_mb_alloc_cnt += wanted;
	
	mutex_unlock(&_mb_lock);
	
	printk("%s - allocate %d KB done. block %d, (%d / %d)", __func__,
	       size / 1024, start, _mb_alloc_cnt, _mb_total);
	*dma_addr = _vc8k_mb_pool[start].paddr;
	return _vc8k_mb_pool[start].vaddr;
}

void * VC8K_MemGetVaddr(void *owner, dma_addr_t dma_addr)
{
	int start;
	dma_addr_t  base;
	
	base = _vc8k_mb_pool[0].paddr;

	if ((dma_addr < base) || (dma_addr > base + _mb_total * PA_BLK_SIZE)) {
		printk("%s - invalid DMA address 0x%x!\n", __func__, (u32)dma_addr);
		return NULL;
	}

	start = (dma_addr - base) / PA_BLK_SIZE;
	if (_vc8k_mb_pool[start].paddr != dma_addr) {
		printk("%s dma_addr not block aligned: 0x%x\n", __func__, (u32)dma_addr);
		return NULL;
	}
	if (_vc8k_mb_pool[start].owner != owner) {
		printk("%s invalid owner on block %d.\n", __func__, start);
		return NULL;
	}
	return _vc8k_mb_pool[start].vaddr;
}

int VC8K_MemFree(void *owner, dma_addr_t dma_addr)
{
	int i, start, wanted;
	dma_addr_t  base;
	
	base = _vc8k_mb_pool[0].paddr;

	if ((dma_addr < base) || (dma_addr > base + _mb_total * PA_BLK_SIZE)) {
		printk("%s - invalid DMA address 0x%x!\n", __func__, (u32)dma_addr);
		return DWL_ERROR;
	}

	start = (dma_addr - base) / PA_BLK_SIZE;
	if (_vc8k_mb_pool[start].paddr != dma_addr) {
		printk("%s dma_addr not block aligned: 0x%x\n", __func__, (u32)dma_addr);
		return DWL_ERROR;
	}
	wanted = _vc8k_mb_pool[start].contig_bcnt;

	if ((dma_addr + wanted * PA_BLK_SIZE) > (base + _mb_total * PA_BLK_SIZE)) {
		printk("%s - invalid DMA address 0x%x!\n", __func__, (u32)dma_addr);
		return DWL_ERROR;
	}

	if (_vc8k_mb_pool[start].owner != owner) {
		printk("%s invalid owner on block %d.\n", __func__, start);
		return DWL_ERROR;
	}

	mutex_lock(&_mb_lock);
	
	for (i = start; i < start + wanted; i++) {
		_vc8k_mb_pool[i].owner = NULL;
		if (!_vc8k_mb_pool[i].used)
			printk("%s warning - try to free an unused block %d!\n", __func__, i); 
		_vc8k_mb_pool[i].used = 0;
		_vc8k_mb_pool[i].contig_bcnt = 0;
	}
	_mb_alloc_cnt -= wanted;
	mutex_unlock(&_mb_lock);

	printk("%s free %d KB done. block %d, (%d / %d)", __func__,
	       wanted * (PA_BLK_SIZE/1024), start, _mb_alloc_cnt, _mb_total);
	return DWL_OK;
}
EXPORT_SYMBOL(VC8K_MemFree);


/*------------------------------------------------------------------------------
	Function name   : DWLMallocRefFrm
	Description     : Allocate a frame buffer (contiguous linear RAM memory)

	Return type     : i32 - 0 for success or a negative error code

	Argument        : const void * instance - DWL instance
	Argument        : u32 size - size in bytes of the requested memory
	Argument        : void *info - place where the allocated memory buffer
			parameters are returned
------------------------------------------------------------------------------*/
i32 DWLMallocRefFrm(const void *instance, u32 size, DWLLinearMem_t * info)
{
	int    ret;
	void   *vaddr = NULL;

	if (_vc8k_vpu->vc8k_cfg.have_res_mem) {
		if (_vc8k_vpu->vc8k_cfg.use_dev_coherent) {
			if (_vc8k_vpu->vc8k_cfg.debug_level > 0)
				printk("%s - RC %u KB\n", __func__, size/1024);

			ret = vc_os_dma_alloc_from_coherent(size, &info->busAddress, &vaddr);
			if ((ret == 0) || (vaddr == NULL)) {
				printk("%s - dma_alloc_from_dev_coherent failed!!\n", __func__);
				return DWL_ERROR;
			}
			info->virtualAddress = vaddr;
		} else {
			if (_vc8k_vpu->vc8k_cfg.debug_level > 0)
				printk("%s - RV %u KB\n", __func__, size/1024);
			info->virtualAddress = VC8K_MemAlloc((void *)instance, &info->busAddress, size);
			if (info->virtualAddress == NULL) {
				return DWLMallocLinear(instance, size, info);
			}
		}
		info->size = size;
		return DWL_OK;
	} else {
		if (_vc8k_vpu->vc8k_cfg.debug_level > 0)
			printk("%s - NR %u KB\n", __func__, size/1024);
		return DWLMallocLinear(instance, size, info);
	}
}
EXPORT_SYMBOL(DWLMallocRefFrm);

/*------------------------------------------------------------------------------
	Function name   : DWLFreeRefFrm
	Description     : Release a frame buffer previously allocated with
			DWLMallocRefFrm.

	Return type     : void

	Argument        : const void * instance - DWL instance
	Argument        : void *info - frame buffer memory information
------------------------------------------------------------------------------*/
void DWLFreeRefFrm(const void *instance, DWLLinearMem_t * info)
{
	if (_vc8k_vpu->vc8k_cfg.have_res_mem) {
		if (_vc8k_vpu->vc8k_cfg.use_dev_coherent) {
			vc_os_dma_release_from_coherent(info->size, info->virtualAddress);
		} else {
			if (VC8K_MemFree((void *)instance, info->busAddress) != DWL_OK)
				DWLFreeLinear(instance, info);
		}
	} else {
		DWLFreeLinear(instance, info);
	}
}
EXPORT_SYMBOL(DWLFreeRefFrm);

#if 1  // NVT_PORT

/*------------------------------------------------------------------------------
	Function name   : DWLMallocLinear
	Description     : Allocate a contiguous, linear RAM  memory buffer

	Return type     : i32 - 0 for success or a negative error code

	Argument        : const void * instance - DWL instance
	Argument        : u32 size - size in bytes of the requested memory
	Argument        : void *info - place where the allocated memory buffer
			parameters are returned
------------------------------------------------------------------------------*/
i32 DWLMallocLinear(const void *instance, u32 size, DWLLinearMem_t * info)
{
	int    ret;
	void   *vaddr;

	if (_vc8k_vpu->vc8k_cfg.use_dev_coherent) {
		if (_vc8k_vpu->vc8k_cfg.debug_level > 0)
			printk("%s - RC %u KB\n", __func__, size/1024);
		ret = vc_os_dma_alloc_from_coherent(size, &info->busAddress, &vaddr);
		if ((ret == 0) || (vaddr == NULL)) {
			printk("DWLMallocLinear failed %d\n", __LINE__);
			return DWL_ERROR;
		}
		info->virtualAddress = vaddr;
	} else {
		if (_vc8k_vpu->vc8k_cfg.debug_level > 0)
			printk("%s - NR %u KB\n", __func__, size/1024);
		info->virtualAddress = vc_os_dma_alloc_writecombine(size, &info->busAddress);
		if (!info->virtualAddress) {
			dev_err(_vc8k_vpu->dev, "DWLMallocLinear - FAILED!\n");
			return DWL_ERROR;
		}
	}
#ifdef MEMORY_USAGE_TRACE
	if (_vc8k_vpu->vc8k_cfg.debug_level > 0)
		dev_info(_vc8k_vpu->dev, "DWLMallocLinear 0x%llx virtualAddress: 0x%lx\n",
			info->busAddress, (unsigned long) info->virtualAddress);
#endif
	info->size = size;
	return DWL_OK;
}
EXPORT_SYMBOL(DWLMallocLinear);

/*------------------------------------------------------------------------------
	Function name   : DWLFreeLinear
	Description     : Release a linera memory buffer, previously allocated with
			DWLMallocLinear.

	Return type     : void

	Argument        : const void * instance - DWL instance
	Argument        : void *info - linear buffer memory information
------------------------------------------------------------------------------*/
void DWLFreeLinear(const void *instance, DWLLinearMem_t * info)
{
	if (_vc8k_vpu->vc8k_cfg.use_dev_coherent) {
		vc_os_dma_release_from_coherent(info->size, info->virtualAddress);
		return;
	}
	vc_os_dma_free_writecombine(info->size, info->virtualAddress,
				 info->busAddress);
}
EXPORT_SYMBOL(DWLFreeLinear);

#else

/*------------------------------------------------------------------------------
	Function name   : DWLMallocLinear
	Description     : Allocate a contiguous, linear RAM  memory buffer

	Return type     : i32 - 0 for success or a negative error code

	Argument        : const void * instance - DWL instance
	Argument        : u32 size - size in bytes of the requested memory
	Argument        : void *info - place where the allocated memory buffer
			parameters are returned
------------------------------------------------------------------------------*/
i32 __DWLMallocLinear(const void *instance, u32 size, DWLLinearMem_t * info)
{
	hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

	u32 pgsize = 0x1000;  //getpagesize();
	MemallocParams params;

	//assert(dec_dwl != NULL);
	//assert(info != NULL);

#ifdef MEMORY_USAGE_TRACE
	dev_info(_vc8k_vpu->dev, "DWLMallocLinear\t%8d bytes \n", size);
#endif

	size = (size + (pgsize - 1)) & (~(pgsize - 1));

	info->size = size;
	info->virtualAddress = 0;
	info->busAddress = 0;

	params.size = size;

	/* get memory linear memory buffers */
	//ioctl(dec_dwl->fd_memalloc, MEMALLOC_IOCXGETBUFFER, &params);
	memalloc_ioctl(MEMALLOC_IOCXGETBUFFER, &params);
	if(params.busAddress == 0)
	{
	dev_err(_vc8k_vpu->dev, "ERROR! No linear buffer available\n");
	return DWL_ERROR;
	}

	// info->busAddress = params.busAddress;  // ychuang - old version
	info->busAddress = params.busAddress - params.translationOffset;

	/* Map the bus address to virtual address */
	//info->virtualAddress = (u32 *) mmap(0, info->size, PROT_READ | PROT_WRITE,
	//                                    MAP_SHARED, dec_dwl->fd_mem,
	//                                    params.busAddress);
	info->virtualAddress = (u32 *)((uint64_t)params.busAddress);

#ifdef MEMORY_USAGE_TRACE
	dev_info(_vc8k_vpu->dev, "DWLMallocLinear 0x%lx virtualAddress: 0x%lx\n",
	   info->busAddress, (unsigned long) info->virtualAddress);
#endif

	if(info->virtualAddress == 0)
	return DWL_ERROR;

	return DWL_OK;
}


i32 DWLMallocLinear(const void *instance, u32 size, DWLLinearMem_t * info)
{
	do {
		ret = __DWLMallocLinear(instance, size, info);
		if (ret != DWL_OK)
			dev_info(_vc8k_vpu->dev, "\n DWLMallocLinear %d failed!\n\n", size);
	}  while (ret != DWL_OK);
}

/*------------------------------------------------------------------------------
	Function name   : DWLFreeLinear
	Description     : Release a linera memory buffer, previously allocated with
			DWLMallocLinear.

	Return type     : void

	Argument        : const void * instance - DWL instance
	Argument        : void *info - linear buffer memory information
------------------------------------------------------------------------------*/
void DWLFreeLinear(const void *instance, DWLLinearMem_t * info)
{
	hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

	//assert(dec_dwl != NULL);
	//assert(info != NULL);

	if (info->busAddress != 0)
	//ioctl(dec_dwl->fd_memalloc, MEMALLOC_IOCSFREEBUFFER,
	//      &info->busAddress);
	memalloc_ioctl(MEMALLOC_IOCSFREEBUFFER, &info->busAddress);

	//if (info->virtualAddress != MAP_FAILED)
	//    munmap(info->virtualAddress, info->size);
}
#endif

/*------------------------------------------------------------------------------
	Function name   : DWLWriteReg
	Description     : Write a value to a hardware IO register

	Return type     : void

	Argument        : const void * instance - DWL instance
	Argument        : u32 offset - byte offset of the register to be written
	Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/

void DWLWriteReg(const void *instance, i32 coreID, u32 offset, u32 value)
{
	// hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

	/*some MSB regs added to support 64bit address accress,
	  so this assert should be ingored */

	offset = offset / 4;

	dwlShadowRegs[coreID][offset] = value;

#ifdef INTERNAL_TEST
	InternalTestDumpWriteSwReg(coreID, offset, value, dwlShadowRegs[coreID]);
#endif
}
EXPORT_SYMBOL(DWLWriteReg);

/*------------------------------------------------------------------------------
	Function name   : DWLReadReg
	Description     : Read the value of a hardware IO register

	Return type     : u32 - the value stored in the register

	Argument        : const void * instance - DWL instance
	Argument        : u32 offset - byte offset of the register to be read
------------------------------------------------------------------------------*/
u32 DWLReadReg(const void *instance, i32 coreID, u32 offset)
{
	// hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	u32 val;

	//assert(dec_dwl != NULL);
	/*some MSB regs added to support 64bit address accress,
	  so this assert should be ingored */
#ifndef USE_64BIT_ENV
	//assert((dec_dwl->clientType != DWL_CLIENT_TYPE_PP &&
	//        offset < HX170PP_REG_START) ||
	//        (dec_dwl->clientType == DWL_CLIENT_TYPE_PP &&
	//                offset >= HX170PP_REG_START) || (offset == 0) ||
	//                (offset == HX170PP_SYNTH_CFG));
#endif

	offset = offset / 4;

	val = dwlShadowRegs[coreID][offset];

#ifdef INTERNAL_TEST
	InternalTestDumpReadSwReg(coreID, offset, val, dwlShadowRegs[coreID]);
#endif

	return val;
}
EXPORT_SYMBOL(DWLReadReg);

/*------------------------------------------------------------------------------
	Function name   : DWLEnableHW
	Description     : Enable hw by writing to register
	Return type     : void
	Argument        : const void * instance - DWL instance
	Argument        : u32 offset - byte offset of the register to be written
	Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/
void DWLEnableHW(const void *instance, i32 coreID, u32 offset, u32 value)
{
	hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	struct vc_os_register_buffer core;
	int isPP;

	//assert(dec_dwl);
	isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

	DWLWriteReg(dec_dwl, coreID, offset, value);

	core.core_id = coreID;
	core.registers = dwlShadowRegs[coreID];
	/* If MSB registers added, the reg number of decoder and pp is not continuous
	   any more, so core.regs is all the same to unify the code */
//  core.regs += isPP ? 60 : 0;
#ifdef USE_64BIT_ENV
	/* 36 MSB address regs added in total, 9 regs for pp, 27 regs for decoder. */
	core.size = isPP ? (41 + 9) * 4 : (60 + 27) * 4;
#else
	core.size = isPP ? 41 * 4 : 60 * 4;
#endif

	//if(ioctl(dec_dwl->fd, ioctl_req, &core))
	//{
	//    DWL_DEBUG("ioctl HX170DEC_IOCS_*_PUSH_REG failed\n");
	//    assert(0);
	//}
	vc_os_hw_push_registers(isPP ? VC_OS_HW_POST_PROCESSOR :
				VC_OS_HW_DECODER, &core);
}
EXPORT_SYMBOL(DWLEnableHW);

/*------------------------------------------------------------------------------
	Function name   : DWLDisableHW
	Description     : Disable hw by writing to register
	Return type     : void
	Argument        : const void * instance - DWL instance
	Argument        : u32 offset - byte offset of the register to be written
	Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/
void DWLDisableHW(const void *instance, i32 coreID, u32 offset, u32 value)
{
	hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	struct vc_os_register_buffer core;
	int isPP;

	//assert(dec_dwl);

	isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

	DWLWriteReg(dec_dwl, coreID, offset, value);

	core.core_id = coreID;
	core.registers = dwlShadowRegs[coreID];
//  core.regs += isPP ? 60 : 0;
#ifdef USE_64BIT_ENV
	/* 36 MSB address regs added in total, 9 regs for pp, 27 regs for decoder. */
	core.size = isPP ? (41 + 9) * 4 : (60 + 27) * 4;
#else
	core.size = isPP ? 41 * 4 : 60 * 4;
#endif

	//if (ioctl(dec_dwl->fd, ioctl_req, &core))
	//{
	//    DWL_DEBUG("ioctl HX170DEC_IOCS_*_PUSH_REG failed\n");
	//    assert(0);
	//}
	vc_os_hw_push_registers(isPP ? VC_OS_HW_POST_PROCESSOR :
				VC_OS_HW_DECODER, &core);
}
EXPORT_SYMBOL(DWLDisableHW);

/*------------------------------------------------------------------------------
	Function name   : DWLWaitHwReady
	Description     : Wait until hardware has stopped running.
			  Used for synchronizing software runs with the hardware.
			  The wait could succed, timeout, or fail with an error.

	Return type     : i32 - one of the values DWL_HW_WAIT_OK
						  DWL_HW_WAIT_TIMEOUT
						  DWL_HW_WAIT_ERROR

	Argument        : const void * instance - DWL instance
------------------------------------------------------------------------------*/
i32 DWLWaitHwReady(const void *instance, i32 coreID, u32 timeout)
{
	const hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	struct vc_os_register_buffer core;
	int isPP;
	i32 ret = DWL_HW_WAIT_OK;

#ifndef DWL_USE_DEC_IRQ
	unsigned long  max_wait_time = 1000; /* 1000 ms */
	unsigned long   start_time;
#endif

	//assert(dec_dwl);

	isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

	core.core_id = coreID;
	core.registers = dwlShadowRegs[coreID];
//  core.regs += isPP ? 60 : 0;
#ifdef USE_64BIT_ENV
	core.size = isPP ? (41 + 9) * 4 : (60 + 27) * 4;
#else
	core.size = isPP ? 41 * 4 : 60 * 4;
#endif

#ifdef DWL_USE_DEC_IRQ
	if(isPP)
	{
	// dev_info(_vc8k_vpu->dev, "WAIT PP IRQ...\n");
	vc_os_hw_wait(VC_OS_HW_POST_PROCESSOR, &core);
	}
	else
	{
	//dev_info(_vc8k_vpu->dev, "WAIT DEC IRQ...\n");
	vc_os_hw_wait(VC_OS_HW_DECODER, &core);
	}

#else /* Polling */

	ret = DWL_HW_WAIT_TIMEOUT;

	start_time = jiffies;
	do
	{
	u32 irq_stats;

	dev_info(_vc8k_vpu->dev, "POLLING HW STATUS...\n");

	//if (ioctl(dec_dwl->fd, ioctl_req, &core))
	//{
	//    DWL_DEBUG("ioctl HX170DEC_IOCS_*_PULL_REG failed\n");
	//    ret = DWL_HW_WAIT_ERROR;
	//    break;
	//}
	vc_os_hw_pull_registers(isPP ? VC_OS_HW_POST_PROCESSOR :
				VC_OS_HW_DECODER, &core);

	irq_stats = isPP ? dwlShadowRegs[coreID][HX170_IRQ_STAT_PP] :
			   dwlShadowRegs[coreID][HX170_IRQ_STAT_DEC];

	irq_stats = (irq_stats >> 11) & 0xFF;

	if(irq_stats != 0)
	{
		ret = DWL_HW_WAIT_OK;
		break;
	}
	}
	while (jiffies - start_time < max_wait_time);

#endif

#ifdef _DWL_DEBUG
	{
	u32 irq_stats = isPP ? dwlShadowRegs[coreID][HX170_IRQ_STAT_PP] :
				   dwlShadowRegs[coreID][HX170_IRQ_STAT_DEC];

	PrintIrqType(isPP, coreID, irq_stats);
	}
#endif

#if 0   // dump register
{
	int  i;

	for (i = 0; i < 110; i++)
		dev->info(_vc8k_vpu->dev, "swreg%d = 0x%08x\n", i, vc_os_mmio_read32(i));
}
#endif

	return ret;
}
EXPORT_SYMBOL(DWLWaitHwReady);


/*------------------------------------------------------------------------------
	Function name   : DWLFakeTimeout
	Description     : Testing help function that changes HW stream errors info
			HW timeouts. You can check how the SW behaves or not.
	Return type     : void
	Argument        : void
------------------------------------------------------------------------------*/

#ifdef _DWL_FAKE_HW_TIMEOUT
void DWLFakeTimeout(u32 * status)
{

	if((*status) & DEC_IRQ_ERROR)
	{
	*status &= ~DEC_IRQ_ERROR;
	*status |= DEC_IRQ_TIMEOUT;
	dev_info(_vc8k_vpu->dev, "\nDWL: Change stream error to hw timeout\n");
	}
}
#endif
