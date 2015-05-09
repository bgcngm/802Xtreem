/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __RMNET_USB_CTRL_H
#define __RMNET_USB_CTRL_H

#include <linux/mutex.h>
#include <linux/usb.h>
#include <linux/cdev.h>
#include <linux/usb/ch9.h>
#include <linux/usb/cdc.h>

#define CTRL_DEV_MAX_LEN 10

//HTC_DBG+++
#define HTC_LOG_RMNET_USB_CTRL
#define HTC_DEBUG_QMI_STUCK
#define HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
#define RMNET_OPEN_TIMEOUT_MS	30000
//HTC_DBG---

struct rmnet_ctrl_dev {

	/*for debugging purpose*/
	char			name[CTRL_DEV_MAX_LEN];

	struct cdev		cdev;
	struct device		*devicep;

	struct usb_interface	*intf;
	unsigned int		int_pipe;
	struct urb		*rcvurb;
	struct urb		*inturb;
	struct usb_anchor	tx_submitted;
	struct usb_anchor	rx_submitted;
	void			*rcvbuf;
	void			*intbuf;
	struct usb_ctrlrequest	*in_ctlreq;

//--------------------------------------------------------
#ifdef HTC_DEBUG_QMI_STUCK
	struct timer_list rcv_timer;
#endif	//HTC_DEBUG_QMI_STUCK
//--------------------------------------------------------

	spinlock_t		rx_lock;
	struct mutex		dev_lock;
	struct list_head	rx_list;
	wait_queue_head_t	read_wait_queue;
	wait_queue_head_t	open_wait_queue;

	struct workqueue_struct	*wq;
	struct work_struct	get_encap_work;

	unsigned		is_opened;

	bool			is_connected;

//--------------------------------------------------------
#ifdef HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
	unsigned long  connected_jiffies;
#endif	//HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
//--------------------------------------------------------

	/*input control lines (DSR, CTS, CD, RI)*/
	unsigned int		cbits_tolocal;

	/*output control lines (DTR, RTS)*/
	unsigned int		cbits_tomdm;

	/*
	 * track first resp available from mdm when it boots up
	 * to avoid bigger  timeout value used by qmuxd
	 */
	bool			resp_available;

	unsigned int		mdm_wait_timeout;

	/*counters*/
	unsigned int		snd_encap_cmd_cnt;
	unsigned int		get_encap_resp_cnt;
	unsigned int		resp_avail_cnt;
	unsigned int		get_encap_failure_cnt;
	unsigned int		set_ctrl_line_state_cnt;
	unsigned int		tx_ctrl_err_cnt;
	unsigned int		zlp_cnt;
};

extern struct rmnet_ctrl_dev *ctrl_dev[];

/* ++SSD_RIL */
/*extern int rmnet_usb_ctrl_start(struct rmnet_ctrl_dev *);*/
extern int rmnet_usb_ctrl_start_rx(struct rmnet_ctrl_dev *);
/* --SSD_RIL */
extern int rmnet_usb_ctrl_suspend(struct rmnet_ctrl_dev *dev);
extern int rmnet_usb_ctrl_init(void);
extern void rmnet_usb_ctrl_exit(void);
extern int rmnet_usb_ctrl_probe(struct usb_interface *intf,
		struct usb_host_endpoint *status,
		struct rmnet_ctrl_dev *dev);
extern void rmnet_usb_ctrl_disconnect(struct rmnet_ctrl_dev *);

#endif /* __RMNET_USB_H*/
