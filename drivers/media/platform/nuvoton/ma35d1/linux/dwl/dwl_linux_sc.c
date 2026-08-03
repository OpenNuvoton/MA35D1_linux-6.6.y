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
--  Description : System Wrapper Layer for Linux using IRQs
--
------------------------------------------------------------------------------*/

#include "hantro.h"
#include "basetype.h"
#include "linux/vc_os_linux.h"
#include "dwl_linux.h"
#include "dwl.h"
#include "dwlthread.h"
//#include "memalloc.h"

//#include <assert.h>
//#include <errno.h>
//#include <fcntl.h>
//#include <signal.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <sys/syscall.h>
//#include <sys/types.h>
//#include <unistd.h>

/* the decoder device driver nod */
const char *dec_dev = DEC_MODULE_PATH;
/* the memalloc device driver nod */
const char *mem_dev = MEMALLOC_MODULE_PATH;

/*------------------------------------------------------------------------------
    Function name   : DWLInit
    Description     : Initialize a DWL instance

    Return type     : const void * - pointer to a DWL instance

    Argument        : void * param - not in use, application passes NULL
------------------------------------------------------------------------------*/
const void *DWLInit(DWLInitParam_t * param)
{
    hX170dwl_t *dec_dwl;

    dec_dwl = (hX170dwl_t *)kmalloc(sizeof(hX170dwl_t), GFP_KERNEL);

    if(dec_dwl == NULL)
    {
        dev_err(_vc8k_vpu->dev, "failed to alloc hX170dwl_t struct\n");
        return NULL;
    }

    dec_dwl->clientType = param->clientType;
    dec_dwl->fd = -1;
    dec_dwl->fd_mem = -1;
    dec_dwl->fd_memalloc = -1;

    /* open the device */
    //dec_dwl->fd = open(dec_dev, O_RDWR);
    //if(dec_dwl->fd == -1)
    //{
    //    DWL_DEBUG("failed to open '%s'\n", dec_dev);
    //    goto err;
    //}

    /* Linear memories not needed in pp */
    //if(dec_dwl->clientType != DWL_CLIENT_TYPE_PP)
    //{
    //    /* open memalloc for linear memory allocation */
    //    dec_dwl->fd_memalloc = open(mem_dev, O_RDWR | O_SYNC);

    //    if(dec_dwl->fd_memalloc == -1)
    //    {
    //        DWL_DEBUG("failed to open: %s\n", mem_dev);
    //        goto err;
    //    }
    //}

    //dec_dwl->fd_mem = open("/dev/mem", O_RDWR | O_SYNC);

    //if(dec_dwl->fd_mem == -1)
    //{
    //    DWL_DEBUG("failed to open: %s\n", "/dev/mem");
    //    goto err;
    //}

    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
    case DWL_CLIENT_TYPE_PP:
    {
        break;
    }
    default:
    {
        dev_err(_vc8k_vpu->dev, "Unknown client type no. %d\n", dec_dwl->clientType);
        goto err;
    }
    }
    return dec_dwl;

    err:

    DWLRelease(dec_dwl);
    return NULL;
}
EXPORT_SYMBOL(DWLInit);

/*------------------------------------------------------------------------------
    Function name   : DWLRelease
    Description     : Release a DWl instance

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - instance to be released
------------------------------------------------------------------------------*/
i32 DWLRelease(const void *instance)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    // assert(dec_dwl != NULL);

    //if(dec_dwl->fd_mem != -1)
    //    close(dec_dwl->fd_mem);

    //if(dec_dwl->fd != -1)
    //    close(dec_dwl->fd);

    /* linear memory allocator */
    //if(dec_dwl->fd_memalloc != -1)
    //    close(dec_dwl->fd_memalloc);

    kfree(dec_dwl);
    return (DWL_OK);
}
EXPORT_SYMBOL(DWLRelease);

/* HW locking */

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHwPipe
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHwPipe(const void *instance, i32 *coreID)
{
    i32 ret;
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

//    assert(dec_dwl != NULL);
//    assert(dec_dwl->clientType != DWL_CLIENT_TYPE_PP);

    /* reserve decoder */
    //*coreID = ioctl(dec_dwl->fd, HX170DEC_IOCH_DEC_RESERVE,
    //        dec_dwl->clientType);
    *coreID = vc_os_hw_reserve(VC_OS_HW_DECODER, dec_dwl->clientType);
          

    if (*coreID != 0)
    {
        return DWL_ERROR;
    }

    /* reserve PP */
    // ret = ioctl(dec_dwl->fd, HX170DEC_IOCQ_PP_RESERVE);
    ret = vc_os_hw_reserve(VC_OS_HW_POST_PROCESSOR, 0);
    

    /* for pipeline we expect same core for both dec and PP */
    if (ret != *coreID)
    {
        /* release the decoder */
        //ioctl(dec_dwl->fd, HX170DEC_IOCT_DEC_RELEASE, coreID);
        vc_os_hw_release(VC_OS_HW_DECODER, *coreID);
        return DWL_ERROR;
    }

    dec_dwl->bPPReserved = 1;
    return DWL_OK;
}
EXPORT_SYMBOL(DWLReserveHwPipe);


/*------------------------------------------------------------------------------
    Function name   : DWLReserveHw
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHw(const void *instance, i32 *coreID)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;

    //assert(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    // dev_info(_vc8k_vpu->dev, " %s\n", isPP ? "PP" : "DEC");

    if (isPP)
    {
        // *coreID = ioctl(dec_dwl->fd, HX170DEC_IOCQ_PP_RESERVE);
        *coreID = vc_os_hw_reserve(VC_OS_HW_POST_PROCESSOR, 0);

        /* PP is single core so we expect a zero return value */
        if (*coreID != 0)
        {
            return DWL_ERROR;
        }
    }
    else
    {
        //*coreID = ioctl(dec_dwl->fd, HX170DEC_IOCH_DEC_RESERVE,
        //        dec_dwl->clientType);
        *coreID = vc_os_hw_reserve(VC_OS_HW_DECODER, dec_dwl->clientType);
    }

    /* negative value signals an error */
    if (*coreID < 0)
    {
        dev_err(_vc8k_vpu->dev,  "ioctl HX170DEC_IOCS_%s_RESERVE failed\n", isPP ? "PP" : "DEC");
        return DWL_ERROR;
    }
    return DWL_OK;
}
EXPORT_SYMBOL(DWLReserveHw);

/*------------------------------------------------------------------------------
    Function name   : DWLReleaseHw
    Description     :
    Return type     : void
    Argument        : const void *instance
------------------------------------------------------------------------------*/
void DWLReleaseHw(const void *instance, i32 coreID)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;

    //assert((u32)coreID < dec_dwl->numCores);
    //assert(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    if ((u32) coreID >= dec_dwl->numCores)
        return;

    if (isPP)
    {
        // assert(coreID == 0);

        // ioctl(dec_dwl->fd, HX170DEC_IOCT_PP_RELEASE, coreID);
        vc_os_hw_release(VC_OS_HW_POST_PROCESSOR, coreID);
    }
    else
    {
        if (dec_dwl->bPPReserved)
        {
            /* decoder has reserved PP also => release it */
            dec_dwl->bPPReserved = 0;
            //assert(coreID == 0);
            // ioctl(dec_dwl->fd, HX170DEC_IOCT_PP_RELEASE, coreID);
            vc_os_hw_release(VC_OS_HW_POST_PROCESSOR, coreID);
        }

        // ioctl(dec_dwl->fd, HX170DEC_IOCT_DEC_RELEASE, coreID);
        vc_os_hw_release(VC_OS_HW_DECODER, coreID);
    }
}
EXPORT_SYMBOL(DWLReleaseHw);

#if 0
void DWLSetIRQCallback(const void *instance, i32 coreID,
        DWLIRQCallbackFn *pCallbackFn, void* arg)
{
    /* not in use with single core only control code */
    UNUSED(instance);
    UNUSED(coreID);
    UNUSED(pCallbackFn);
    UNUSED(arg);

    // assert(0);
}
#endif

