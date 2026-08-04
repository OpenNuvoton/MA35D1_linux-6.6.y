/* Copyright 2012 Google Inc. All Rights Reserved. */

#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>
#include "hantro.h"

#include "basetype.h"
#include "linux/vc_os_linux.h"

#include "hx170dec.h"
#include "../dwl/dwl_defs.h"


#define	HXDEC_MAX_CORES			1

#define	HANTRO_DEC_ORG_REGS		60
#define	HANTRO_PP_ORG_REGS		41

#define	HANTRO_DEC_EXT_REGS		27
#define	HANTRO_PP_EXT_REGS		9

#define	HANTRO_DEC_TOTAL_REGS		(HANTRO_DEC_ORG_REGS + HANTRO_DEC_EXT_REGS)
#define	HANTRO_PP_TOTAL_REGS		(HANTRO_PP_ORG_REGS + HANTRO_PP_EXT_REGS)

#define	HANTRO_TOTAL_REGS		119

#define	HANTRO_DEC_ORG_FIRST_REG	0
#define	HANTRO_DEC_ORG_LAST_REG		59
#define	HANTRO_DEC_EXT_FIRST_REG	119
#define	HANTRO_DEC_EXT_LAST_REG		145

#define	HANTRO_PP_ORG_FIRST_REG		60
#define	HANTRO_PP_ORG_LAST_REG		100
#define	HANTRO_PP_EXT_FIRST_REG		146
#define	HANTRO_PP_EXT_LAST_REG		154

#define	DEC_IO_SIZE			(HANTRO_TOTAL_REGS * 4) /* bytes */

wait_queue_head_t  wait_dec;
wait_queue_head_t  wait_pp;

struct hantro_dev *_vc8k_vpu;

EXPORT_SYMBOL(_vc8k_vpu);

/* here's all the must remember	stuff */
typedef	struct
{
    char *buffer;
    unsigned int iosize;
    //volatile u8 *hwregs[HXDEC_MAX_CORES];
    int	irq;
    int	cores;
//    struct fasync_struct *async_queue_dec;
//    struct fasync_struct *async_queue_pp;
} hx170dec_t;

static hx170dec_t hx170dec_data; /* dynamic allocation?	*/

long DecFlushRegs(hx170dec_t *dev, struct core_desc *core);
long DecRefreshRegs(hx170dec_t *dev, struct core_desc *core);

static void ResetAsic(hx170dec_t * dev);


static u32 dec_regs[HXDEC_MAX_CORES][DEC_IO_SIZE/4];

static atomic_t dec_irq;
static volatile int pp_irq = 0;


#define	DWL_CLIENT_TYPE_H264_DEC	 1U
#define	DWL_CLIENT_TYPE_MPEG4_DEC	 2U
#define	DWL_CLIENT_TYPE_JPEG_DEC	 3U
#define	DWL_CLIENT_TYPE_PP		 4U
#define	DWL_CLIENT_TYPE_VC1_DEC		 5U
#define	DWL_CLIENT_TYPE_MPEG2_DEC	 6U
#define	DWL_CLIENT_TYPE_VP6_DEC		 7U
#define	DWL_CLIENT_TYPE_AVS_DEC		 8U
#define	DWL_CLIENT_TYPE_RV_DEC		 9U
#define	DWL_CLIENT_TYPE_VP8_DEC		 10U

static u32 cfg[HXDEC_MAX_CORES];

static void ReadCoreConfig(hx170dec_t *dev)
{
    int	c = 0;
    u32	reg, mask, tmp;

    memset(cfg,	0, sizeof(cfg));

    // for(c = 0; c < dev->cores; c++)
    {
	/* Decoder configuration */
	//reg =	ioread32(dev->hwregs[c]	+ HX170DEC_SYNTH_CFG * 4);
	reg = vc_os_mmio_read32(HX170DEC_SYNTH_CFG);

	tmp = (reg >> DWL_H264_E) & 0x3U;
	cfg[c] |= tmp ?	1 << DWL_CLIENT_TYPE_H264_DEC :	0;

	tmp = (reg >> DWL_JPEG_E) & 0x01U;
	cfg[c] |= tmp ?	1 << DWL_CLIENT_TYPE_JPEG_DEC :	0;

	tmp = (reg >> DWL_MPEG4_E) & 0x3U;
	cfg[c] |= tmp ?	1 << DWL_CLIENT_TYPE_MPEG4_DEC : 0;

	//reg =	ioread32(dev->hwregs[c]	+ HX170DEC_SYNTH_CFG_2 * 4);
	reg = vc_os_mmio_read32(HX170DEC_SYNTH_CFG_2);

	/* VP7 and WEBP	is part	of VP8 */
	mask =	(1 << DWL_VP8_E) | (1 << DWL_VP7_E) | (1 << DWL_WEBP_E);
	tmp = (reg & mask);
	cfg[c] |= tmp ?	1 << DWL_CLIENT_TYPE_VP8_DEC : 0;

	tmp = (reg >> DWL_AVS_E) & 0x01U;
	cfg[c] |= tmp ?	1 << DWL_CLIENT_TYPE_AVS_DEC: 0;

	tmp = (reg >> DWL_RV_E)	& 0x03U;
	cfg[c] |= tmp ?	1 << DWL_CLIENT_TYPE_RV_DEC : 0;

	/* Post-processor configuration	*/
		//reg =	ioread32(dev->hwregs[c]	+ HX170PP_SYNTH_CFG * 4);
	reg = vc_os_mmio_read32(HX170PP_SYNTH_CFG);

	tmp = (reg >> DWL_PP_E)	& 0x01U;
	cfg[c] |= tmp ?	1 << DWL_CLIENT_TYPE_PP	: 0;
    }
}


long DecFlushRegs(hx170dec_t *dev, struct core_desc *core)
{
	long  i;
	
	u32	id = 0; // core->id;

	/* copy original dec regs to kernal space*/
	memcpy(dec_regs[id], core->regs, HANTRO_DEC_ORG_REGS*4);

#if 1  // Nuvoton, ychuang added
	dec_regs[id][2] &= ~0x80;
#endif
	
	/* write dec regs but the status reg[1] to hardware	*/
	/* both original and extended regs need to be written */
	for (i = 2; i <= HANTRO_DEC_ORG_LAST_REG; i++)
	{
		vc_os_mmio_write32(i, dec_regs[id][i]);
	}
	
	/* write the status register, which may start the decoder */
	vc_os_mmio_write32(1, dec_regs[id][1]);
	// dev_info(_vc8k_vpu->dev, "flushed registers on core %d\n", id);
	
	return 0;
}
EXPORT_SYMBOL(DecFlushRegs);

long DecRefreshRegs(hx170dec_t *dev, struct core_desc *core)
{
	long  i;
	u32	id = 0; //core->id;
	
	/* read all	registers from hardware	*/
	/* both original and extended regs need to be read */
	for(i = 0; i <= HANTRO_DEC_ORG_LAST_REG; i++)
		dec_regs[id][i]	= vc_os_mmio_read32(i);
	
	/* put registers to	user space*/
	//for(i = 0; i <= HANTRO_DEC_ORG_REGS; i++)
	//	core->regs[i] = dec_regs[id][i];
	memcpy(core->regs, dec_regs[id], HANTRO_DEC_ORG_REGS*4);
	
	return 0;
}
EXPORT_SYMBOL(DecRefreshRegs);


static long WaitDecReadyAndRefreshRegs(hx170dec_t *dev, struct	core_desc *core)
{
	u64   t0;

	wait_event_interruptible_timeout(wait_dec, atomic_read(&dec_irq), DEC_TIMEOUT);
	if (atomic_read(&dec_irq) == 0) {
		t0 = jiffies;
		while (1) {
			if (atomic_read(&dec_irq) != 0)
				break;
			if (jiffies - t0 > 1000) {  /* 2023.03.06, prevent form Ctrl^C case; let interrupt complete  */
				printk("%s - decode timeout!", __func__);
				break;
			}
		}
		if (atomic_read(&dec_irq) == 0)
			printk("%s - %d, %llu, %lu", __func__,
			       atomic_read(&dec_irq), t0, jiffies);
	}
	atomic_set(&dec_irq, 0);
	/* refresh registers */
	return DecRefreshRegs(dev, core);
}

static long PPFlushRegs(hx170dec_t *dev, struct	core_desc *core)
{
	u32	id = 0; // core->id;
	u32	i;
	
	/* copy original dec regs to kernal	space*/
	memcpy(dec_regs[id]	+ HANTRO_PP_ORG_FIRST_REG, core->regs +	HANTRO_PP_ORG_FIRST_REG, HANTRO_PP_ORG_REGS*4);
	    
	/* write all regs but the status reg[1] to hardware	*/
	/* both original and extended regs need to be written */
	for (i = HANTRO_PP_ORG_FIRST_REG + 1; i <= HANTRO_PP_ORG_LAST_REG; i++)
		vc_os_mmio_write32(i, dec_regs[id][i]);

#if 1  // ychuang - force disable tield
	vc_os_mmio_write32(91, 0);
#endif
	
	/* write the stat reg, which may start the PP */
	vc_os_mmio_write32(HANTRO_PP_ORG_FIRST_REG, dec_regs[id][HANTRO_PP_ORG_FIRST_REG]);
	
	return 0;
}

static long _PPRefreshRegs(hx170dec_t *dev, struct core_desc *core)
{
	long i;
	u32	id = 0; //core->id;
	
	/* read all	registers from hardware	*/
	/* both original and extended regs need to be read */
	for(i = HANTRO_PP_ORG_FIRST_REG; i <= HANTRO_PP_ORG_LAST_REG; i++) {
		dec_regs[id][i]	= vc_os_mmio_read32(i);
		core->regs[i] = dec_regs[id][i];
	}
	/* put registers to	user space*/
	/* put original registers to user space*/
	// memcpy(core->regs + HANTRO_PP_ORG_FIRST_REG, dec_regs[id] + HANTRO_PP_ORG_FIRST_REG, HANTRO_PP_ORG_REGS*4);
	
	return 0;
}

static long WaitPPReadyAndRefreshRegs(hx170dec_t *dev, struct core_desc *core)
{
	wait_event_interruptible_timeout(wait_pp, pp_irq, PP_TIMEOUT);
    	pp_irq = 0;

    	/* refresh registers */
    	return _PPRefreshRegs(dev, core);
}


/*------------------------------------------------------------------------------
 Function name	 : hx170dec_ioctl
 Description	 : communication method	to/from	the user space

 Return	type	 : long
------------------------------------------------------------------------------*/

//long _ioctl(unsigned int cmd,	 unsigned long arg)
long hx170dec_ioctl(unsigned int cmd, void * arg)
{
	struct core_desc 	core;
	
	// dev_info(_vc8k_vpu->dev, "ioctl cmd	0x%08x\n", cmd);
	
	switch (cmd)
	{
 	case HX170DEC_IOCS_DEC_PUSH_REG:
		atomic_set(&dec_irq, 0);
		memcpy(&core, (void*)arg, sizeof(struct	core_desc));
		DecFlushRegs(&hx170dec_data, &core);
		//printk("+");
		break;

	case HX170DEC_IOCS_PP_PUSH_REG:
		pp_irq = 0;
		memcpy(&core, (void*)arg, sizeof(struct	core_desc));
		PPFlushRegs(&hx170dec_data, &core);
		break;

	case HX170DEC_IOCS_DEC_PULL_REG:
		memcpy(&core, (void*)arg, sizeof(struct	core_desc));
		return DecRefreshRegs(&hx170dec_data, &core);

	case HX170DEC_IOCS_PP_PULL_REG:
		memcpy(&core, (void*)arg, sizeof(struct	core_desc));
		return _PPRefreshRegs(&hx170dec_data, &core);

	case HX170DEC_IOCH_DEC_RESERVE:
		return 0;  // ReserveDecoder(&hx170dec_data, filp, arg);

	case HX170DEC_IOCT_DEC_RELEASE:
		return 0;
	
	case HX170DEC_IOCQ_PP_RESERVE:
		return 0;  // ReservePostProcessor(&hx170dec_data, filp);
	
	case HX170DEC_IOCT_PP_RELEASE:
		return 0;
	
	case HX170DEC_IOCX_DEC_WAIT:
		memcpy(&core, (void*)arg, sizeof(struct	core_desc));
		return WaitDecReadyAndRefreshRegs(&hx170dec_data, &core);

	case HX170DEC_IOCX_PP_WAIT:
		memcpy(&core, (void*)arg, sizeof(struct	core_desc));
		return WaitPPReadyAndRefreshRegs(&hx170dec_data, &core);

	default:
	return -1; //-ENOTTY;
	}
	
	return 0;
}
EXPORT_SYMBOL(hx170dec_ioctl);


/*------------------------------------------------------------------------------
 Function name	 : hx170dec_init
 Description	 : Initialize the driver

 Return	type	 : int
------------------------------------------------------------------------------*/

int  hx170dec_init(struct hantro_dev *vpu)
{
	_vc8k_vpu = vpu;
	
	init_waitqueue_head(&wait_dec);
	init_waitqueue_head(&wait_pp);
	
	hx170dec_data.iosize = DEC_IO_SIZE;
	
	/* read configuration fo all cores */
	ReadCoreConfig(&hx170dec_data);
	
	/* reset hardware */
	ResetAsic(&hx170dec_data);
	
	return 0;
}
EXPORT_SYMBOL(hx170dec_init);

/*------------------------------------------------------------------------------
 Function name	 : hx170dec_isr
 Description	 : interrupt handler

 Return	type	 : irqreturn_t
------------------------------------------------------------------------------*/
void hx170dec_isr(void)
{
	unsigned int handled = 0;
	u32	irq_status_dec;
	u32	irq_status_pp;
	
	irq_status_dec = vc_os_mmio_read32(HX170_IRQ_STAT_DEC);
	
	if (irq_status_dec & HX170_DEC_IRQ)
	{
		//printk("$");	
		//dev_info(_vc8k_vpu->dev, "DEC IRQ received!\n");
		atomic_set(&dec_irq, 1);
		handled++;
		wake_up_interruptible_all(&wait_dec);
	}
	
	/* check PP also */
	irq_status_pp = vc_os_mmio_read32(HX170_IRQ_STAT_PP);
	if (irq_status_pp & HX170_PP_IRQ)
	{
		/* clear pp IRQ	*/
		dev_dbg(_vc8k_vpu->dev, "PP IRQ received!\n");
		pp_irq = 1;
		handled++;
		wake_up_interruptible_all(&wait_pp);
	}
	
	if (!handled)
	{
		dev_info(_vc8k_vpu->dev, "IRQ received, but not x170's!\n");
	}
}
EXPORT_SYMBOL(hx170dec_isr);

/*------------------------------------------------------------------------------
 Function name	 : ResetAsic
 Description	 : reset asic

 Return	type	 :
------------------------------------------------------------------------------*/
void ResetAsic(hx170dec_t * dev)
{
	int	i;
	u32	status;
	
	status = vc_os_mmio_read32(HX170_IRQ_STAT_DEC);
	
	if( status & HX170_DEC_E)
	{
		/* abort with IRQ disabled */
		status = HX170_DEC_ABORT | HX170_DEC_IRQ_DISABLE;
		vc_os_mmio_write32(HX170_IRQ_STAT_DEC, status);
	}
	
	for (i = 4; i <	dev->iosize; i += 4)
	{
		vc_os_mmio_write32(i, 0);
	}
}

