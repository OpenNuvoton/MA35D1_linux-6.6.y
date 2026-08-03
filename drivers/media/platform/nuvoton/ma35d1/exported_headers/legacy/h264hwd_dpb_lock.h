/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Google Inc.                                     --
--                                                                            --
--                   (C) COPYRIGHT 2012 GOOGLE INC.                           --
--                         ALL RIGHTS RESERVED                                --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/

#ifndef H264HIGH_H264HWD_DPB_LOCK_H_
#define H264HIGH_H264HWD_DPB_LOCK_H_

#include "basetype.h"
#include "h264decapi.h"

#include <dwlthread.h>

#define MAX_FRAME_BUFFER_NUMBER     34
#define FB_NOT_VALID_ID             ~0U

#define FB_HW_OUT_FIELD_TOP         0x10U
#define FB_HW_OUT_FIELD_BOT         0x20U
#define FB_HW_OUT_FRAME             (FB_HW_OUT_FIELD_TOP | FB_HW_OUT_FIELD_BOT)
#define ABORT_MARKER                 2
#define FLUSH_MARKER                 3

typedef struct FrameBufferStatus_
{
    u32 nRefCount;
    u32 bUsed;
    const void * data;
} FrameBufferStatus;

typedef struct OutElement_
{
    u32 memIdx;
    H264DecPicture pic;
} OutElement;

typedef struct FrameBufferList_
{
    int bInitialized;
    struct FrameBufferStatus_ fbStat[MAX_FRAME_BUFFER_NUMBER];
    struct OutElement_ outFifo[MAX_FRAME_BUFFER_NUMBER];
    int wrId;
    int rdId;
    int freeBuffers;
    int numOut;
#ifdef USE_OUTPUT_RELEASE
    u32 abort;
    u32 flushAll;
#endif
    struct
    {
        int id;
        const void * desc;
    } lastOut;

//    sem_t out_count_sem;
//    pthread_mutex_t out_count_mutex;
//    pthread_cond_t out_empty_cv;
//    pthread_mutex_t ref_count_mutex;
//    pthread_cond_t ref_count_cv;
//    pthread_cond_t hw_rdy_cv;
} FrameBufferList;

struct dpbStorage;
struct H264DecPicture_;

u32 InitList(FrameBufferList *fbList);
void ReleaseList(FrameBufferList *fbList);

u32 AllocateIdUsed(FrameBufferList *fbList, const void * data);
u32 AllocateIdFree(FrameBufferList *fbList, const void * data);
void ReleaseId(FrameBufferList *fbList, u32 id);
void * GetDataById(FrameBufferList *fbList, u32 id);
u32 GetIdByData(FrameBufferList *fbList, const void *data);

void IncrementRefUsage(FrameBufferList *fbList, u32 id);
void DecrementRefUsage(FrameBufferList *fbList, u32 id);

void IncrementDPBRefCount(struct dpbStorage *dpb);
void DecrementDPBRefCount(struct dpbStorage *dpb);

void MarkHWOutput(FrameBufferList *fbList, u32 id, u32 type);
void ClearHWOutput(FrameBufferList *fbList, u32 id, u32 type);

void MarkTempOutput(FrameBufferList *fbList, u32 id);
void ClearOutput(FrameBufferList *fbList, u32 id);

void FinalizeOutputAll(FrameBufferList *fbList);
void RemoveTempOutputAll(FrameBufferList *fbList);

#ifdef USE_OUTPUT_RELEASE
void RemoveOutputAll(FrameBufferList *fbList);
void SetAbortStatusInList(FrameBufferList *fbList);
void ClearAbortStatusInList(FrameBufferList *fbList);
void ResetOutFifoInList(FrameBufferList *fbList);
#endif

u32 GetFreePicBuffer(FrameBufferList * fbList, u32 *old_id, u32 *is_free);
void SetFreePicBuffer(FrameBufferList * fbList, u32 id);
u32 GetFreeBufferCount(FrameBufferList *fbList);

void PushOutputPic(FrameBufferList *fbList, const struct H264DecPicture_ *pic,
        u32 id);
u32 PeekOutputPic(FrameBufferList *fbList, struct H264DecPicture_ *pic);
u32 PopOutputPic(FrameBufferList *fbList, u32 id);

void MarkOutputPicCorrupt(FrameBufferList *fbList, u32 id, u32 errors);

u32 IsBufferReferenced(FrameBufferList *fbList, u32 id);
u32 IsOutputEmpty(FrameBufferList *fbList);
u32 IsBufferOutput(FrameBufferList *fbList, u32 id);

void WaitOutputEmpty(FrameBufferList *fbList);
void WaitListNotInUse(FrameBufferList *fbList);

void AbortList(FrameBufferList *fbList);
void MarkIdAllocated(FrameBufferList *fbList, u32 id);
void MarkIdFree(FrameBufferList *fbList, u32 id);
void ClearTempOut(FrameBufferList *fbList, u32 id);
#endif  /*  H264HIGH_H264HWD_DPB_LOCK_H_ */
