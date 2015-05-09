/*
 * This is part of the Sequans SQN1130 driver.
 * Copyright 2008 SEQUANS Communications
 * Written by Dmitriy Chumak <chumakd@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/kthread.h>

#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include "sdio-ti.h"
#include "msg.h"
#include "version.h"
#include "sdio-netdev.h"
#include "sdio-sqn.h"


#define BYTE_MODE 0
#define BLOCK_MODE 1
#define FIXED_OPCODE 0
#define INC_OPCODE 1


int io_rw_direct(int rwFlag, int funcNum, int rawFlag, unsigned long regAddr,
	int value);

int io_rw_extended(struct mmc_request *mrq, int rwFlag, int funcNum,
	int blkMode, int opcode, unsigned long regAddr, int bytec);


unsigned char sdio_readb(struct sdio_func *func,
	unsigned int addr, int *err_ret)
{
	int ret = 0;

	if (err_ret)
		*err_ret = 0;

	ret = io_rw_direct(READ, FUNCTION_1, 0, addr, 0);

	return ret;

}


unsigned short sdio_readw(struct sdio_func *func,
	unsigned int addr, int *err_ret)
{
	int ret = 0;
	struct mmc_request mrq = { 0 };
	struct mmc_data data = { 0 };

	if (err_ret)
		*err_ret = 0;

	data.sdio_buffer_virt = kmalloc(4, GFP_KERNEL|GFP_DMA);
	mrq.data = &data;
	data.mrq = &mrq;

	ret = io_rw_extended(&mrq, READ, FUNCTION_1, BYTE_MODE, INC_OPCODE, addr, 2);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		ret = 0xFFFF;
	} else {
		ret = le16_to_cpu(*(u16 *)data.sdio_buffer_virt);
	}

	kfree(data.sdio_buffer_virt);
	return ret;
}


unsigned long sdio_readl(struct sdio_func *func,
	unsigned int addr, int *err_ret)
{
	int ret = 0;
	struct mmc_request mrq = { 0 };
	struct mmc_data data = { 0 };

	if (err_ret)
		*err_ret = 0;

	data.sdio_buffer_virt = kmalloc(4, GFP_KERNEL|GFP_DMA);
	mrq.data = &data;
	data.mrq = &mrq;

	ret = io_rw_extended(&mrq, READ, FUNCTION_1, BYTE_MODE, INC_OPCODE, addr, 4);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		ret = 0xFFFFFFFF;
	} else {
		ret = le32_to_cpu(*(u32 *)data.sdio_buffer_virt);
	}

	kfree(data.sdio_buffer_virt);
	return ret;

}



void sdio_writeb(struct sdio_func *func, unsigned char b,
	unsigned int addr, int *err_ret)
{
	int ret;

	ret = io_rw_direct(WRITE, FUNCTION_1, 0, addr, b);
	if (err_ret)
		*err_ret = ret;
}


void sdio_writew(struct sdio_func *func, unsigned short b,
	unsigned int addr, int *err_ret)
{
	int ret = 0;
	struct mmc_request mrq = { 0 };
	struct mmc_data data = { 0 };

	data.sdio_buffer_virt = kmalloc(4, GFP_KERNEL|GFP_DMA);
	mrq.data = &data;
	data.mrq = &mrq;

	*(u16 *)data.sdio_buffer_virt = cpu_to_le16(b);

	ret = io_rw_extended(&mrq, WRITE, FUNCTION_1, BYTE_MODE, INC_OPCODE , addr, 2);
	if (err_ret)
		*err_ret = ret;

	kfree(data.sdio_buffer_virt);
}


void sdio_writel(struct sdio_func *func, unsigned long b,
	unsigned int addr, int *err_ret)
{
	int ret = 0;
	struct mmc_request mrq = { 0 };
	struct mmc_data data = { 0 };

	data.sdio_buffer_virt = kmalloc(4, GFP_KERNEL|GFP_DMA);
	mrq.data = &data;
	data.mrq = &mrq;

	*(u32 *)data.sdio_buffer_virt = cpu_to_le32(b);

	ret = io_rw_extended(&mrq, WRITE, FUNCTION_1, BYTE_MODE, INC_OPCODE , addr, 4);
	if (err_ret)
		*err_ret = ret;

	kfree(data.sdio_buffer_virt);
}


#define CMD53_BYTE_MODE_MAX_BYTES	512

static int sdio_io_helper(int addr, void *data, int count, int rw_flag,
	int mode, int opcode)
{
	int ret = 0;
	struct mmc_request mrq = { 0 };
	struct mmc_data mmcdata = { 0 };

	mmcdata.sdio_buffer_virt = data;
	mrq.data = &mmcdata;
	mmcdata.mrq = &mrq;

	while (count > 0) {
		ret = io_rw_extended(&mrq, rw_flag, FUNCTION_1, mode, opcode
			, addr, count > CMD53_BYTE_MODE_MAX_BYTES ?
				CMD53_BYTE_MODE_MAX_BYTES : count);
		if (ret)
			break;
		count -= CMD53_BYTE_MODE_MAX_BYTES;
		mmcdata.sdio_buffer_virt += CMD53_BYTE_MODE_MAX_BYTES;
	}

	return ret;
}


int sdio_readsb(struct sdio_func *func, void *dst,
	unsigned int addr, int count)
{
	return sdio_io_helper(addr, dst, count, READ, BYTE_MODE, FIXED_OPCODE);
}


int sdio_writesb(struct sdio_func *func, unsigned int addr,
	void *src, int count)
{
	return sdio_io_helper(addr, src, count, WRITE, BYTE_MODE, FIXED_OPCODE);
}


int sdio_memcpy_fromio(struct sdio_func *func, void *dst,
	unsigned int addr, int count)
{
	return sdio_io_helper(addr, dst, count, READ, BYTE_MODE, INC_OPCODE);
}


int sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,
	void *src, int count)
{
	return sdio_io_helper(addr, src, count, WRITE, BYTE_MODE, INC_OPCODE);
}



#define SDIO_CCCR_IENx		0x04	/* Function/Master Interrupt Enable */
#define SDIO_CCCR_INTx		0x05	/* Function Interrupt Pending */


void sqn_sdio_interrupt(struct sdio_func *func);


static int sdio_it_thread(void *_card)
{
	struct mmc_card *card = _card;
	struct sqn_sdio_card *sqn_card = mmc_get_drvdata(card);
	int ret = 0;
	unsigned long irq_flags = 0;

	sqn_pr_enter();
	/*
	 * Set PF_NOFREEZE to prevent kernel to freeze this thread
	 * when going to suspend. We will manually stop it from
	 * driver's suspend handler.
	 */
	current->flags |= PF_NOFREEZE;

	/* card->host->card_busy = -1; */

	do {
		/* mmc_claim_host(card->host); */
		/* ret = __mmc_claim_host(card->host, card); */
		/* if (ret) */
			/* break; */

		u8 pending = 0;

		sqn_pr_dbg("waiting for irq notification\n");
		ret = wait_event_interruptible(card->host->sdio_irq_waitq
			, card->host->sdio_irq_pending || kthread_should_stop()
			|| sqn_card->it_thread_should_stop);
		if (0 != ret) {
			sqn_pr_dbg("got a signal from kernel %d, exit\n", ret);
			break;
		} else {
			sqn_pr_dbg("got irq\n");
		}

		pending = sdio_readb(sqn_card->func, SDIO_CCCR_INTx, &ret);

		if (pending & (1 << FUNCTION_1))
			sqn_sdio_interrupt(sqn_card->func);

		spin_lock_irqsave(&sqn_card->priv->drv_lock, irq_flags);
		if (sqn_card->it_thread_should_stop) {
			spin_unlock_irqrestore(&sqn_card->priv->drv_lock, irq_flags);
			sqn_pr_dbg("got stop request from driver, exit\n");
			break;
		}
		spin_unlock_irqrestore(&sqn_card->priv->drv_lock, irq_flags);

		/* mmc_release_host(card->host); */
	} while (!kthread_should_stop());

	sqn_pr_leave();

	return 0;
}


struct task_struct	*it_thread = 0;

int sdio_start_it_thread(struct mmc_card *card)
{
	int rc = 0;
	struct sdio_func *func = 0;
	u8 reg = 0;

	sqn_pr_enter();

	sqn_pr_dbg("read IEN\n");
	reg = sdio_readb(func, SDIO_CCCR_IENx, &rc);

	reg |= 1 << FUNCTION_1;
	reg |= 1; /* Master interrupt enable */

	sqn_pr_dbg("write IEN %x\n", reg);
	sdio_writeb(func, reg, SDIO_CCCR_IENx, &rc);

	it_thread = kthread_run(sdio_it_thread, card, "ksdioirqd");
	if (IS_ERR(it_thread)) {
		int err = PTR_ERR(it_thread);
		sqn_pr_dbg("can't create sqn_irq_thread\n");
		return err;
	}

	sqn_pr_leave();

	return rc;
}


int sdio_stop_it_thread(void)
{
	int rc = 0;
	/* struct sdio_func *func = 0; */

	sqn_pr_enter();

	rc = kthread_stop(it_thread);
	/* sdio_writeb(func, 0, SDIO_CCCR_IENx, &rc); */

	sqn_pr_leave();

	return rc;
}
