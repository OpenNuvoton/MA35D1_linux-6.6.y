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
--  Description : 
--
------------------------------------------------------------------------------*/

#ifndef REGDRV_H
#define REGDRV_H

/*------------------------------------------------------------------------------
    Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

#define DEC_8170_IRQ_RDY            0x02
#define DEC_8170_IRQ_BUS            0x04
#define DEC_8170_IRQ_BUFFER         0x08
#define DEC_8170_IRQ_ASO            0x10
#define DEC_8170_IRQ_ERROR          0x20
#define DEC_8170_IRQ_SLICE          0x40
#define DEC_8170_IRQ_TIMEOUT        0x80

#define DEC_8190_IRQ_ABORT          0x01
#define DEC_8190_IRQ_RDY            DEC_8170_IRQ_RDY
#define DEC_8190_IRQ_BUS            DEC_8170_IRQ_BUS
#define DEC_8190_IRQ_BUFFER         DEC_8170_IRQ_BUFFER
#define DEC_8190_IRQ_ASO            DEC_8170_IRQ_ASO
#define DEC_8190_IRQ_ERROR          DEC_8170_IRQ_ERROR
#define DEC_8190_IRQ_SLICE          DEC_8170_IRQ_SLICE
#define DEC_8190_IRQ_TIMEOUT        DEC_8170_IRQ_TIMEOUT

#define DEC_IRQ_DISABLE             0x10
#define DEC_ABORT                   0x20

#define PP_REG_START                60

#ifdef USE_64BIT_ENV

#define SET_ADDR_REG(reg_base, REGBASE, addr) do {\
    SetDecRegister((reg_base), REGBASE, (u32)(addr));  \
    SetDecRegister((reg_base), REGBASE##_MSB, (u32)((addr) >> 32)); \
  } while (0)

#define SET_ADDR_REG2(reg_base, lsb, msb, addr) do {\
    SetDecRegister((reg_base), (lsb), (u32)(addr));  \
    SetDecRegister((reg_base), (msb), (u32)((addr) >> 32)); \
  } while (0)

#define SET_PP_ADDR_REG(reg_base, REGBASE, addr) do {\
    SetPpRegister((reg_base), REGBASE, (u32)(addr));  \
    SetPpRegister((reg_base), REGBASE##_MSB, (u32)((addr) >> 32)); \
  } while (0)

#define SET_PP_ADDR_REG2(reg_base, lsb, msb, addr) do {\
    SetPpRegister((reg_base), (lsb), (u32)(addr));  \
    SetPpRegister((reg_base), (msb), (u32)((addr) >> 32)); \
  } while (0)

#define GET_ADDR_REG(reg_base, REGBASE)  \
  (((g1_addr_t)GetDecRegister((reg_base), REGBASE)) |  \
  (((g1_addr_t)GetDecRegister((reg_base), REGBASE##_MSB)) << 32))

#define GET_ADDR_REG2(reg_base, lsb, msb)  \
  (((g1_addr_t)GetDecRegister((reg_base), (lsb))) |  \
  (((g1_addr_t)GetDecRegister((reg_base), (msb))) << 32))

#define GET_PP_ADDR_REG(reg_base, REGBASE)  \
  (((g1_addr_t)GetPpRegister((reg_base), REGBASE)) |  \
  (((g1_addr_t)GetPpRegister((reg_base), REGBASE##_MSB)) << 32))

#define GET_PP_ADDR_REG2(reg_base, lsb, msb)  \
  (((g1_addr_t)GetPpRegister((reg_base), (lsb))) |  \
  (((g1_addr_t)GetPpRegister((reg_base), (msb))) << 32))

#else

#define SET_ADDR_REG(reg_base, REGBASE, addr) do {\
    SetDecRegister((reg_base), REGBASE, (u32)(addr));  \
  } while (0)

#define SET_ADDR_REG2(reg_base, lsb, msb, addr) do {\
    SetDecRegister((reg_base), (lsb), (u32)(addr));  \
    SetDecRegister((reg_base), (msb), 0); \
  } while (0)

#define SET_PP_ADDR_REG(reg_base, REGBASE, addr) do {\
    SetPpRegister((reg_base), REGBASE, (u32)(addr));  \
  } while (0)

#define SET_PP_ADDR_REG2(reg_base, lsb, msb, addr) do {\
    SetPpRegister((reg_base), (lsb), (u32)(addr));  \
    SetPpRegister((reg_base), (msb), 0); \
  } while (0)

#define GET_ADDR_REG(reg_base, REGID)  \
  ((g1_addr_t)GetDecRegister((reg_base), REGID))

#define GET_ADDR_REG2(reg_base, lsb, msb)  \
  (((g1_addr_t)GetDecRegister((reg_base), (lsb))) |  \
  (((g1_addr_t)GetDecRegister((reg_base), (msb))) & 0))

#define GET_PP_ADDR_REG(reg_base, REGID)  \
  ((g1_addr_t)GetDecRegister((reg_base), REGID))

#define GET_PP_ADDR_REG2(reg_base, lsb, msb)  \
  ((g1_addr_t)GetPpRegister((reg_base), (lsb)))

#endif

typedef enum
{
/* include script-generated part */
#include "8170enum.h"
    HWIF_DEC_IRQ_STAT,
    HWIF_PP_IRQ_STAT,
    HWIF_LAST_REG,

    /* aliases */
    HWIF_MPEG4_DC_BASE = HWIF_I4X4_OR_DC_BASE,
    HWIF_MPEG4_DC_BASE_MSB = HWIF_I4X4_OR_DC_BASE_MSB,
    HWIF_INTRA_4X4_BASE = HWIF_I4X4_OR_DC_BASE,
    HWIF_INTRA_4X4_BASE_MSB = HWIF_I4X4_OR_DC_BASE_MSB,
    /* VP6 */
    HWIF_VP6HWGOLDEN_BASE = HWIF_REFER4_BASE,
    HWIF_VP6HWGOLDEN_BASE_MSB = HWIF_REFER4_BASE_MSB,
    HWIF_VP6HWPART1_BASE = HWIF_REFER13_BASE,
    HWIF_VP6HWPART1_BASE_MSB = HWIF_REFER13_BASE_MSB,
    HWIF_VP6HWPART2_BASE = HWIF_RLC_VLC_BASE,
    HWIF_VP6HWPART2_BASE_MSB = HWIF_RLC_VLC_BASE_MSB,
    HWIF_VP6HWPROBTBL_BASE = HWIF_QTABLE_BASE,
    HWIF_VP6HWPROBTBL_BASE_MSB = HWIF_QTABLE_BASE_MSB,
    /* progressive JPEG */
    HWIF_PJPEG_COEFF_BUF = HWIF_DIR_MV_BASE,
    HWIF_PJPEG_COEFF_BUF_MSB = HWIF_DIR_MV_BASE_MSB,

    /* MVC */
    HWIF_INTER_VIEW_BASE = HWIF_REFER15_BASE,
    HWIF_INTER_VIEW_BASE_MSB = HWIF_REFER15_BASE_MSB

} hwIfName_e;

/*------------------------------------------------------------------------------
    Data types
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Function prototypes
------------------------------------------------------------------------------*/

void SetDecRegister(u32 *regBase, u32 id, u32 value);
u32 GetDecRegister(const u32 *regBase, u32 id);

void SetPpRegister(u32 *ppRegBase, u32 id, u32 value);
u32 GetPpRegister(const u32 *ppRegBase, u32 id);

#endif /* #ifndef REGDRV_H */
