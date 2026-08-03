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
--  Abstract : Decode Supplemental Enhancement Information (SEI) from the stream
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents
   
    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef H264HWD_SEI_H
#define H264HWD_SEI_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "h264hwd_seq_param_set.h"
#include "h264hwd_stream.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* enumerated sample aspect ratios, ASPECT_RATIO_M_N means M:N */
enum {
  SEI_BUFFERING_PERIOD = 0,
  SEI_PIC_TIMING,
  SEI_PAN_SCAN_RECT,
  SEI_FILLER_PAYLOAD,
  SEI_USER_DATA_REGISTERED_ITU_T_T35,
  SEI_USER_DATA_UNREGISTERED,
  SEI_RECOVERY_POINT,
  SEI_DEC_REF_PIC_MARKING_REPETITION,
  SEI_SPARE_PIC,
  SEI_SCENE_INFO,
  SEI_SUB_SEQ_INFO,
  SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
  SEI_SUB_SEQ_CHARACTERISTICS,
  SEI_FULL_FRAME_FREEZE,
  SEI_FULL_FRAME_FREEZE_RELEASE,
  SEI_FULL_FRAME_SNAPSHOT,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
  SEI_FILM_GRAIN_CHARACTERISTICS,
  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
  SEI_STEREO_VIDEO_INFO,
  SEI_POST_FILTER_HINTS,
  SEI_TONE_MAPPING,
  SEI_SCALABILITY_INFO,
  SEI_SUB_PIC_SCALABLE_LAYER,
  SEI_NON_REQUIRED_LAYER_REP,
  SEI_PRIORITY_LAYER_INFO,
  SEI_LAYERS_NOT_PRESENT,
  SEI_LAYER_DEPENDENCY_CHANGE,
  SEI_SCALABLE_NESTING,
  SEI_BASE_LAYER_TEMPORAL_HRD,
  SEI_QUALITY_LAYER_INTEGRITY_CHECK,
  SEI_REDUNDANT_PIC_PROPERTY,
  SEI_TL0_DEP_REP_INDEX,
  SEI_TL_SWITCHING_POINT,
  SEI_PARALLEL_DECODING_INFO,
  SEI_MVC_SCALABLE_NESTING,
  SEI_VIEW_SCALABILITY_INFO,
  SEI_MULTIVIEW_SCENE_INFO,
  SEI_MULTIVIEW_ACQUISITION_INFO,
  SEI_NON_REQUIRED_VIEW_COMPONENT,
  SEI_VIEW_DEPENDENCY_CHANGE,
  SEI_OPERATION_POINTS_NOT_PRESENT,
  SEI_BASE_VIEW_TEMPORAL_HRD,
  SEI_FRAME_PACKING_ARRANGEMENT,
  SEI_GREEN_METADATA=56,

  SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
};


/* structure to store Buffering period SEI parameters */
typedef struct
{
    u32 seqParameterSetId;
    u32 initialCpbRemovalDelay[32];
    u32 initialCpbRemovalDelayOffset[32];
    u32 existFlag;
} bufferingPeriodInfo_t;

/* storage for Picture timing SEI parameters */
typedef struct
{
    u32 cpbRemovalDelay;
    u32 dpbOutputDelay;
    u32 picStruct;
    u32 clockTimestampFlag[3];
    u32 ctType;
    u32 nuitFieldBasedFlag;
    u32 countingType;
    u32 fullTimestampFlag;
    u32 discontinuityFlag;
    u32 cntDroppedFlag;
    u32 nFrames;
    u32 secondsValue;
    u32 minutesValue;
    u32 hoursValue;
    u32 secondsFlag;
    u32 minutesFlag;
    u32 hoursFlag;
    u32 timeOffset;
    u32 existFlag;
} picTimingInfo_t;

#if 1  // NVT_PORT
typedef struct
{
    u32 clockTick;
    u32 initialArrivalTime;
    u32 prevNominalRemovalTime;
    u32 finalArrivalTime;
    u32 initialArrivalTimeEarliest;
    u32 nominalRemovalTime;
    u32 nominalRemovalTimeFirst;
    u32 cpbRemovalTime;
    u32 dpbOutputTime;
    u32 accessUnitSize;
    u32 isFirstAu;
    u32 hrdInitFlag;
    
} computeTimeInfo_t;
#else
typedef struct
{
    double clockTick;
    double initialArrivalTime;
    double prevNominalRemovalTime;
    double finalArrivalTime;
    double initialArrivalTimeEarliest;
    double nominalRemovalTime;
    double nominalRemovalTimeFirst;
    double cpbRemovalTime;
    double dpbOutputTime;
    double accessUnitSize;
    double isFirstAu;
    u32 hrdInitFlag;
    
} computeTimeInfo_t;
#endif

typedef struct
{
    u32 bumpingFlag;
    bufferingPeriodInfo_t bufferingPeriodInfo;
    picTimingInfo_t picTimingInfo;
    computeTimeInfo_t  computeTimeInfo;
} seiParameters_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeSeiParameters(seqParamSet_t **sps, strmData_t *pStrmData,
    seiParameters_t *pSeiParameters);

u32 h264bsdDecodeBufferingPeriodInfo(seqParamSet_t **sps, strmData_t *pStrmData,
                                bufferingPeriodInfo_t *pBufferingPeriodInfo);

u32 h264bsdDecodePicTimingInfo(seqParamSet_t **sps, strmData_t *pStrmData,
                            picTimingInfo_t *pPicTimingInfo, bufferingPeriodInfo_t *pBufferingPeriodInfo);

u32 h264bsdComputeTimes(seqParamSet_t *sps, seiParameters_t *pSeiParameters);
#endif /* #ifdef H264HWD_SEI_H */
