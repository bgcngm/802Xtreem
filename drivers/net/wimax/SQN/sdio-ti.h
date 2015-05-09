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

#ifndef _SQN_TI_SDIO_H
#define _SQN_TI_SDIO_H


#define sdio_claim_host(func)		({ 0; })
#define sdio_release_host(func)		({ 0; })
#define sdio_enable_func(func)		({ 0; })
#define sdio_disable_func(func)		({ 0; })
#define sdio_claim_irq(func, sqn_sdio_interrupt)	({ 0; })
#define sdio_release_irq(func)		({ 0; })


struct sdio_func;
struct mmc_card;


unsigned char sdio_readb(struct sdio_func *func,
	unsigned int addr, int *err_ret);

unsigned short sdio_readw(struct sdio_func *func,
	unsigned int addr, int *err_ret);

unsigned long sdio_readl(struct sdio_func *func,
	unsigned int addr, int *err_ret);


void sdio_writeb(struct sdio_func *func, unsigned char b,
	unsigned int addr, int *err_ret);

void sdio_writew(struct sdio_func *func, unsigned short b,
	unsigned int addr, int *err_ret);

void sdio_writel(struct sdio_func *func, unsigned long b,
	unsigned int addr, int *err_ret);


int sdio_readsb(struct sdio_func *func, void *dst,
	unsigned int addr, int count);

int sdio_writesb(struct sdio_func *func, unsigned int addr,
	void *src, int count);

int sdio_memcpy_fromio(struct sdio_func *func, void *dst,
	unsigned int addr, int count);

int sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,
	void *src, int count);


int sdio_start_it_thread(struct mmc_card *card);
int sdio_stop_it_thread(void);

#endif /* _SQN_TI_SDIO_H */
