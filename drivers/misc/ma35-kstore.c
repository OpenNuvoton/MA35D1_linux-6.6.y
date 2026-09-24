// SPDX-License-Identifier: GPL-2.0
/*
 * Nuvoton MA35 series key store driver
 *
 * Copyright (c) 2025 Nuvoton technology corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;version 2 of the License.
 *
 */

#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/of_platform.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/compat.h>
#include <linux/highmem.h>

#include <uapi/misc/ma35_kstore.h>

#define MISCDEV_NAME		"ksdev"

#define KS_BUSY_TIMEOUT		1000

#define OTP_NS_OFFSET_FROM_KS	0x10000
#define OTP_NS_REG_SIZE		0x14
#define OTP_BUSY_TIMEOUT_US	500000

#define OTP_CTL			0x00
#define OTP_CTL_START		BIT(0)
#define OTP_CTL_PROGRAM		BIT(4)
#define OTP_STS			0x04
#define OTP_STS_BUSY		BIT(0)
#define OTP_STS_PFF		BIT(1)
#define OTP_STS_ADDRFF		BIT(2)
#define OTP_STS_CMDFF		BIT(4)
#define OTP_ADDR		0x08
#define OTP_DATA		0x0C

#define OTP_ADDR_MIN		0x100
#define OTP_ADDR_END		0x1D0

struct ma35_ks_dev {
	struct device *dev;
	struct miscdevice miscdev;
	wait_queue_head_t waitq;
	void __iomem *reg_base;
	void __iomem *otp_base;
	struct mutex otp_lock; /* serializes OTP controller access */
	int irq;
	int int_err_sts;
};

static char  ks_miscdev_name[] = MISCDEV_NAME;

static uint16_t au8SRAMCntTbl[21] = {4, 6, 6, 7, 8, 8, 8, 9, 12, 13, 16, 17,
				     18, 0, 0, 0, 32, 48, 64, 96, 128};
static uint16_t au8OTPCntTbl[7] = {4, 6, 6, 7, 8, 8, 8};

static inline u32  ma35_read_reg(struct ma35_ks_dev *ks_dev, u32 offset)
{
	u32 value = readl_relaxed(ks_dev->reg_base + offset);

	dev_vdbg(ks_dev->dev, "reg read 0x%08x from <0x%x>\n", value, offset);
	return value;
}

static inline void ma35_write_reg(struct ma35_ks_dev *ks_dev, u32 offset, u32 value)
{
	dev_vdbg(ks_dev->dev, "write 0x%08x into <0x%x>\n", value, offset);
	writel_relaxed(value, ks_dev->reg_base + offset);
}

static inline int ma35_ks_wait_busy_clear(struct ma35_ks_dev *ks_dev)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(KS_BUSY_TIMEOUT);

	while (ma35_read_reg(ks_dev, KS_STS) & KS_STS_BUSY) {
		if (time_after(jiffies, timeout)) {
			pr_err("MA35 KeyStore is busy!\n");
			return -EBUSY;
		}
	}
	return 0;
}

static inline u32 ma35_otp_read_reg(struct ma35_ks_dev *ks_dev, u32 offset)
{
	return readl(ks_dev->otp_base + offset);
}

static inline void ma35_otp_write_reg(struct ma35_ks_dev *ks_dev,
				      u32 offset, u32 value)
{
	writel(value, ks_dev->otp_base + offset);
}

static int ma35_otp_wait_busy_clear(struct ma35_ks_dev *ks_dev)
{
	u32 status;
	int ret;

	ret = readl_poll_timeout(ks_dev->otp_base + OTP_STS, status,
				 !(status & OTP_STS_BUSY), 1,
				 OTP_BUSY_TIMEOUT_US);
	if (ret)
		dev_err(ks_dev->dev, "OTP controller is busy\n");

	return ret;
}

static int ma35_otp_read_word(struct ma35_ks_dev *ks_dev, u32 addr, u32 *data)
{
	u32 status;
	int ret;

	ret = ma35_otp_wait_busy_clear(ks_dev);
	if (ret)
		return ret;

	ma35_otp_write_reg(ks_dev, OTP_STS, OTP_STS_ADDRFF | OTP_STS_CMDFF);
	ma35_otp_write_reg(ks_dev, OTP_ADDR, addr);
	ma35_otp_write_reg(ks_dev, OTP_CTL, OTP_CTL_START);

	ret = ma35_otp_wait_busy_clear(ks_dev);
	if (ret)
		return ret;

	status = ma35_otp_read_reg(ks_dev, OTP_STS);
	if (status & (OTP_STS_ADDRFF | OTP_STS_CMDFF)) {
		dev_err(ks_dev->dev,
			"OTP read failed at 0x%x, status=0x%x\n", addr, status);
		ma35_otp_write_reg(ks_dev, OTP_STS,
				   OTP_STS_ADDRFF | OTP_STS_CMDFF);
		return -EIO;
	}

	*data = ma35_otp_read_reg(ks_dev, OTP_DATA);
	return 0;
}

static int ma35_otp_program_word(struct ma35_ks_dev *ks_dev, u32 addr, u32 data)
{
	u32 status;
	int ret;

	ret = ma35_otp_wait_busy_clear(ks_dev);
	if (ret)
		return ret;

	ma35_otp_write_reg(ks_dev, OTP_STS,
			   OTP_STS_PFF | OTP_STS_ADDRFF | OTP_STS_CMDFF);
	ma35_otp_write_reg(ks_dev, OTP_ADDR, addr);
	ma35_otp_write_reg(ks_dev, OTP_DATA, data);
	ma35_otp_write_reg(ks_dev, OTP_CTL, OTP_CTL_PROGRAM | OTP_CTL_START);

	ret = ma35_otp_wait_busy_clear(ks_dev);
	if (ret)
		return ret;

	status = ma35_otp_read_reg(ks_dev, OTP_STS);
	if (status & (OTP_STS_PFF | OTP_STS_ADDRFF | OTP_STS_CMDFF)) {
		dev_err(ks_dev->dev,
			"OTP program failed at 0x%x, status=0x%x\n",
			addr, status);
		ma35_otp_write_reg(ks_dev, OTP_STS,
				   OTP_STS_PFF | OTP_STS_ADDRFF |
				   OTP_STS_CMDFF);
		return -EIO;
	}

	return 0;
}

static int ma35_otp_read(struct ma35_ks_dev *ks_dev, void __user *arg)
{
	struct ks_read_args r_args;
	int i;
	int ret;

	if (copy_from_user(&r_args, arg, sizeof(r_args)))
		return -EFAULT;

	if (r_args.word_cnt <= 0 ||
	    r_args.word_cnt > (int)ARRAY_SIZE(r_args.key) ||
	    r_args.key_idx < OTP_ADDR_MIN || r_args.key_idx >= OTP_ADDR_END ||
	    (r_args.key_idx & 3) ||
	    r_args.word_cnt > (OTP_ADDR_END - r_args.key_idx) / 4)
		return -EINVAL;

	mutex_lock(&ks_dev->otp_lock);
	for (i = 0; i < r_args.word_cnt; i++) {
		ret = ma35_otp_read_word(ks_dev, r_args.key_idx + i * 4,
					 &r_args.key[i]);
		if (ret)
			goto out_unlock;
	}
	mutex_unlock(&ks_dev->otp_lock);

	return copy_to_user(arg, &r_args, sizeof(r_args)) ? -EFAULT : 0;

out_unlock:
	mutex_unlock(&ks_dev->otp_lock);
	return ret;
}

static int ma35_otp_program(struct ma35_ks_dev *ks_dev, void __user *arg)
{
	struct ks_read_args p_args;
	u32 data;
	int ret;

	if (copy_from_user(&p_args, arg, sizeof(p_args)))
		return -EFAULT;

	data = p_args.key[0];
	if (p_args.word_cnt != 1 ||
	    p_args.key_idx < OTP_ADDR_MIN || p_args.key_idx >= OTP_ADDR_END ||
	    (p_args.key_idx & 3) || !data || (data & (data - 1)))
		return -EINVAL;

	mutex_lock(&ks_dev->otp_lock);
	ret = ma35_otp_program_word(ks_dev, p_args.key_idx, data);
	mutex_unlock(&ks_dev->otp_lock);

	return ret;
}

static int ma35_ks_read(struct ma35_ks_dev *ks_dev, void __user *arg)
{
	struct ks_read_args r_args;
	int err, remain_cnt;
	int offset, i, cnt;
	u32 cont_msk;

	err = copy_from_user(&r_args, arg, sizeof(r_args));
	if (err)
		return -EFAULT;

	/* Just return when key store is in busy */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Specify the key address */
	ma35_write_reg(ks_dev, KS_METADATA, (r_args.type <<
			KS_METADATA_DST_Pos) | KS_TOMETAKEY(r_args.key_idx));

	/* Clear error flag */
	ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF);
	offset = 0;
	cont_msk = 0;
	remain_cnt = r_args.word_cnt;

	do {
		/* Clear Status */
		ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF | KS_STS_IF);

		/* Trigger to read the key */
		ma35_write_reg(ks_dev, KS_CTL, cont_msk | KS_OP_READ | KS_CTL_START |
				(ma35_read_reg(ks_dev, KS_CTL) & KS_CLT_FUNC_MASK));

		/* Waiting for key store processing */
		if (ma35_ks_wait_busy_clear(ks_dev) != 0)
			return -EBUSY;

		/* Read the key to key buffer */
		cnt = remain_cnt;
		if (cnt > 8)
			cnt = 8;
		for (i = 0; i < cnt; i++)
			r_args.key[offset+i] = ma35_read_reg(ks_dev, KS_KEY(i));

		cont_msk = KS_CTL_CONT;
		remain_cnt -= 8;
		offset += 8;

	} while (remain_cnt > 0);

	/* Check error flag */
	if (ma35_read_reg(ks_dev, KS_STS) & KS_STS_EIF) {
		pr_err("KS EIF set on reading keys!\n");
		return -EIO;
	}

	err = copy_to_user(arg, &r_args, sizeof(r_args));
	if (err)
		err = -EFAULT;

	return err;
}

static int ma35_ks_write_sram(struct ma35_ks_dev *ks_dev, void __user *arg)
{
	struct ks_write_args w_args;
	int err, remain_cnt;
	int offset, i, cnt;
	u32 cont_msk;

	err = copy_from_user(&w_args, arg, sizeof(w_args));
	if (err)
		return -EFAULT;

	/* Just return when key store is in busy */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Specify the key address */
	ma35_write_reg(ks_dev, KS_METADATA,
		       (KS_SRAM << KS_METADATA_DST_Pos) | w_args.meta_data);

	/* Get key size by indexing to size table */
	i = ((w_args.meta_data & KS_METADATA_SIZE_Msk) >> KS_METADATA_SIZE_Pos);
	remain_cnt = au8SRAMCntTbl[i];

	/* Invalid key length */
	if (remain_cnt == 0) {
		pr_err("Invalid key length!\n");
		return -EINVAL;
	}

	/* Clear error flag */
	ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF);
	offset = 0;
	cont_msk = 0;
	do {
		/* Prepare the key to write */
		cnt = remain_cnt;
		if (cnt > 8)
			cnt = 8;
		for (i = 0; i < cnt; i++) {
			ma35_write_reg(ks_dev, KS_KEY(i), w_args.key[offset + i]);
		}

		/* Clear Status */
		ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF | KS_STS_IF);

		/* Write the key */
		ma35_write_reg(ks_dev, KS_CTL,	cont_msk | KS_OP_WRITE | KS_CTL_START |
			       (ma35_read_reg(ks_dev, KS_CTL) & KS_CLT_FUNC_MASK));

		cont_msk = KS_CTL_CONT;
		remain_cnt -= 8;
		offset += 8;

		/* Waiting for key store processing */
		if (ma35_ks_wait_busy_clear(ks_dev) != 0)
			return -EBUSY;

	} while (remain_cnt > 0);

	/* Check error flag */
	if (ma35_read_reg(ks_dev, KS_STS) & KS_STS_EIF) {
		pr_err("KS EIF set on writing SRAM keys!\n");
		return -EIO;
	}

	return KS_TOKEYIDX(ma35_read_reg(ks_dev, KS_METADATA));
}

static int ma35_ks_write_otp(struct ma35_ks_dev *ks_dev, void __user *arg)
{
	struct ks_write_args w_args;
	int err, remain_cnt;
	int sidx, offset, i, cnt;
	u32 cont_msk;

	err = copy_from_user(&w_args, arg, sizeof(w_args));
	if (err)
		return -EFAULT;

	/* Just return when key store is in busy */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Specify the key address */
	ma35_write_reg(ks_dev, KS_METADATA, (KS_OTP << KS_METADATA_DST_Pos) |
			w_args.meta_data | KS_TOMETAKEY(w_args.key_idx));

	/* Get size index */
	sidx = ((w_args.meta_data & KS_METADATA_SIZE_Msk) >> KS_METADATA_SIZE_Pos);

	/* OTP only support maximum 256 bits */
	if (sidx >= 7)
		return -1;

	remain_cnt = au8OTPCntTbl[sidx];

	/* Clear error flag */
	ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF);
	offset = 0;
	cont_msk = 0;
	do  {
		/* Prepare the key to write */
		cnt = remain_cnt;
		if (cnt > 8)
			cnt = 8;
		for (i = 0; i < cnt; i++)
			ma35_write_reg(ks_dev, KS_KEY(i), w_args.key[offset + i]);

		/* Clear Status */
		ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF | KS_STS_IF);

		/* Write the key */
		ma35_write_reg(ks_dev, KS_CTL,	cont_msk | KS_OP_WRITE | KS_CTL_START |
			       (ma35_read_reg(ks_dev, KS_CTL) & KS_CLT_FUNC_MASK));

		cont_msk = KS_CTL_CONT;
		remain_cnt -= 8;
		offset += 8;

		/* Waiting for key store processing */
		if (ma35_ks_wait_busy_clear(ks_dev) != 0)
			return -EBUSY;

	} while (remain_cnt > 0);

	/* Check error flag */
	if (ma35_read_reg(ks_dev, KS_STS) & KS_STS_EIF) {
		pr_err("KS EIF set on writing OTP keys!\n");
		return -EIO;
	}
	return 0;
}

static int ma35_ks_erase(struct ma35_ks_dev *ks_dev, void __user *arg)
{
	struct ks_kidx_args k_args;
	int err;

	err = copy_from_user(&k_args, arg, sizeof(k_args));
	if (err)
		return -EFAULT;

	/* Just return when key store is in busy */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Specify the key address */
	ma35_write_reg(ks_dev, KS_METADATA,
		       (k_args.type << KS_METADATA_DST_Pos) | KS_TOMETAKEY(k_args.key_idx));

	/* Clear Status */
	ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF | KS_STS_IF);

	/* Erase the key */
	ma35_write_reg(ks_dev, KS_CTL, KS_OP_ERASE | KS_CTL_START |
		       (ma35_read_reg(ks_dev, KS_CTL) & KS_CLT_FUNC_MASK));

	/* Waiting for processing */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Check error flag */
	if (ma35_read_reg(ks_dev, KS_STS) & KS_STS_EIF) {
		pr_err("KS EIF set on erasing a key!\n");
		return -EIO;
	}
	return 0;
}

static int ma35_ks_erase_all(struct ma35_ks_dev *ks_dev)
{
	/* Just return when key store is in busy */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Specify the key address */
	ma35_write_reg(ks_dev, KS_METADATA, (KS_SRAM << KS_METADATA_DST_Pos));

	/* Clear Status */
	ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF | KS_STS_IF);

	/* Erase the key */
	ma35_write_reg(ks_dev, KS_CTL, KS_OP_ERASE_ALL | KS_CTL_START |
		       (ma35_read_reg(ks_dev, KS_CTL) & KS_CLT_FUNC_MASK));

	/* Waiting for processing */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Check error flag */
	if (ma35_read_reg(ks_dev, KS_STS) & KS_STS_EIF) {
		pr_err("KS EIF set on erase-all!\n");
		return -EIO;
	}
	return 0;
}

static int ma35_ks_revoke(struct ma35_ks_dev *ks_dev, void __user *arg)
{
	struct ks_kidx_args k_args;
	int err;

	err = copy_from_user(&k_args, arg, sizeof(k_args));
	if (err)
		return -EFAULT;

	/* Just return when key store is in busy */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Specify the key address */
	ma35_write_reg(ks_dev, KS_METADATA, (k_args.type <<
		       KS_METADATA_DST_Pos) | KS_TOMETAKEY(k_args.key_idx));

	/* Clear Status */
	ma35_write_reg(ks_dev, KS_STS, KS_STS_EIF | KS_STS_IF);

	/* Revoke the key */
	ma35_write_reg(ks_dev, KS_CTL,	KS_OP_REVOKE | KS_CTL_START |
		       (ma35_read_reg(ks_dev, KS_CTL) & KS_CLT_FUNC_MASK));

	/* Waiting for processing */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	/* Check error flag */
	if (ma35_read_reg(ks_dev, KS_STS) & KS_STS_EIF) {
		pr_err("KS EIF set on revoking a key!\n");
		return -EIO;
	}
	return 0;
}

static int ma35_ks_remain(struct ma35_ks_dev *ks_dev)
{
	u32 reg_data, sram_remain;

	reg_data = ma35_read_reg(ks_dev, KS_REMAIN);
	sram_remain = (reg_data & KS_REMAIN_RRMNG_Msk) >> KS_REMAIN_RRMNG_Pos;

	return sram_remain;
}

static int ks_dev_open(struct inode *iptr, struct file *fptr)
{
	struct ma35_ks_dev *ks_dev;
	unsigned long timeout;

	ks_dev = container_of(fptr->private_data, struct ma35_ks_dev, miscdev);

	/* Start Key Store Initial */
	ma35_write_reg(ks_dev, KS_CTL, KS_CTL_INIT | KS_CTL_START);

	/* Waiting for KeyStore initilization done */
	timeout = jiffies + msecs_to_jiffies(KS_BUSY_TIMEOUT);
	while ((ma35_read_reg(ks_dev, KS_STS) & KS_STS_INITDONE) == 0) {
		if (time_after(jiffies, timeout))
			return -EIO;
	}

	/* Waiting for processing */
	if (ma35_ks_wait_busy_clear(ks_dev) != 0)
		return -EBUSY;

	return 0;
}

static int ks_dev_release(struct inode *iptr, struct file *fptr)
{
	return 0;
}

static long ks_dev_ioctl(struct file *fptr, unsigned int cmd, unsigned long data)
{
	struct ma35_ks_dev *ks_dev;
	char __user *argp = (char __user *)data;
	int rval = -EINVAL;

	ks_dev = container_of(fptr->private_data, struct ma35_ks_dev, miscdev);

	if (_IOC_TYPE(cmd) != MA35_KS_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case NU_KS_IOCTL_READ:
		rval = ma35_ks_read(ks_dev, argp);
		break;

	case NU_KS_IOCTL_WRITE_SRAM:
		rval = ma35_ks_write_sram(ks_dev, argp);
		break;

	case NU_KS_IOCTL_WRITE_OTP:
		rval = ma35_ks_write_otp(ks_dev, argp);
		break;

	case NU_KS_IOCTL_ERASE:
		rval = ma35_ks_erase(ks_dev, argp);
		break;

	case NU_KS_IOCTL_ERASE_ALL:
		rval = ma35_ks_erase_all(ks_dev);
		break;

	case NU_KS_IOCTL_REVOKE:
		rval = ma35_ks_revoke(ks_dev, argp);
		break;

	case NU_KS_IOCTL_GET_REMAIN:
		rval = ma35_ks_remain(ks_dev);
		break;
	case NU_KS_IOCTL_OTP_READ:
		rval = ma35_otp_read(ks_dev, argp);
		break;

	case NU_KS_IOCTL_OTP_WRITE:
		rval = ma35_otp_program(ks_dev, argp);
		break;

	default:
		/* Should not get here */
		break;
	}
	return rval;
}

static const struct file_operations ma35_ks_fops = {
	.owner = THIS_MODULE,
	.open = ks_dev_open,
	.release = ks_dev_release,
	.unlocked_ioctl = ks_dev_ioctl,
	.compat_ioctl = ks_dev_ioctl,
};

static int ma35_ks_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ma35_ks_dev *ks_dev;
	struct resource *res;
	int ret;

	ks_dev = devm_kzalloc(&pdev->dev, sizeof(*ks_dev), GFP_KERNEL);
	if (!ks_dev)
		return -ENOMEM;

	ks_dev->dev = dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ks_dev->reg_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(ks_dev->reg_base))
		return PTR_ERR(ks_dev->reg_base);

	ks_dev->otp_base = devm_ioremap(dev,
					res->start + OTP_NS_OFFSET_FROM_KS,
					OTP_NS_REG_SIZE);
	if (!ks_dev->otp_base)
		return -ENOMEM;

	ks_dev->irq = platform_get_irq(pdev, 0);
	if (ks_dev->irq < 0) {
		dev_dbg(dev, "platform_get_irq failed");
		return -EINVAL;
	}
	mutex_init(&ks_dev->otp_lock);

	/* Save driver private data */
	platform_set_drvdata(pdev, ks_dev);

	ks_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ks_dev->miscdev.name = ks_miscdev_name;
	ks_dev->miscdev.fops = &ma35_ks_fops;
	ks_dev->miscdev.parent = dev;
	ret = misc_register(&ks_dev->miscdev);
	if (ret) {
		dev_err(dev, "error:%d. Unable to register device", ret);
		return ret;
	}

	printk("ma35 kstore initialized.\n");
	return 0;
}

static int ma35_ks_remove(struct platform_device *pdev)
{
	struct ma35_ks_dev *ks_dev;

	ks_dev = platform_get_drvdata(pdev);
	misc_deregister(&ks_dev->miscdev);
	return 0;
}

static const struct of_device_id ma35_ks_of_match[] = {
	{ .compatible = "nuvoton,ma35d0-kstore" },
	{ .compatible = "nuvoton,ma35h0-kstore" },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, ma35_ks_of_match);

static struct platform_driver ma35_ks_driver = {
	.probe = ma35_ks_probe,
	.remove = ma35_ks_remove,
	.driver = {
		.name = "ma35-kstore",
		.of_match_table = ma35_ks_of_match,
	},
};

module_platform_driver(ma35_ks_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nuvoton, Inc");
MODULE_DESCRIPTION("Nuvoton MA35 Key Store Driver");
