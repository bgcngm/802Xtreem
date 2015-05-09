/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/v4l2-subdev.h>


#include <mach/camera.h>

#include <media/msm_camera.h>
#include "msm.h"
#include <asm/mach-types.h>
#include "msm_ispif.h"
#include "msm_sensor.h"


#include <linux/slab.h>

//#include <media/msm_camera_sensor.h>
#include <mach/gpio.h>
//#include <mach/vreg.h>
#include "s5k6aafx.h"
#include <linux/slab.h>

DEFINE_MUTEX(s5k6aafx_mut);
int g_csi_if = 0;

#define SENSOR_NAME "s5k6aafx"
#define CHECK_STATE_TIME 100
struct s5k6aafx_work {
	struct work_struct work;
};

static struct s5k6aafx_work *s5k6aafx_sensorw;
static struct i2c_client *s5k6aafx_client;

struct s5k6aafx_format {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	u16 fmt;
	u16 order;
};

struct s5k6aafx_ctrl_t{
	const struct msm_camera_sensor_info *sensordata;
	uint32_t sensormode;
	uint32_t fps_divider;/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;/* init to 1 * 0x00000400 */
	uint16_t fps;
	uint16_t curr_lens_pos;
	uint16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;
	struct v4l2_subdev *sensor_dev;
			
	enum s5k6aafx_resolution_t prev_res;
	enum s5k6aafx_resolution_t pict_res;
	enum s5k6aafx_resolution_t curr_res;
	enum s5k6aafx_test_mode_t set_test;
	struct s5k6aafx_format *fmt;
};

static struct msm_sensor_output_info_t s5k6aafx_dimensions[] = {
	{
		.x_output = 0x500,
		.y_output = 0x2D0,
		.line_length_pclk = 0x8E9,
		.frame_length_lines = 0x57A,
		.vt_pixel_clk = 50000000,
		.op_pixel_clk = 50000000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x500,
		.y_output = 0x2D0,
		.line_length_pclk = 0x8E9,
		.frame_length_lines = 0x57A,
		.vt_pixel_clk = 50000000,
		.op_pixel_clk = 50000000,
		.binning_factor = 1,
	},

};

static struct msm_sensor_ctrl_t s5k6aafx_s_ctrl;
static struct s5k6aafx_ctrl_t s5k6aafx_ctrl;

static int op_mode;
static int qtr_size_mode;
static DECLARE_WAIT_QUEUE_HEAD(s5k6aafx_wait_queue);
DEFINE_SEMAPHORE(s5k6aafx_sem);

static int sensor_probe_node = 0;
static enum frontcam_t previous_mirror_mode;
static int32_t config_csi=0;

#define MAX_I2C_RETRIES 10
static int i2c_transfer_retry(struct i2c_adapter *adap,
			struct i2c_msg *msgs,
			int len)
{
	int i2c_retry = 0;
	int ns; /* number sent */

	while (i2c_retry++ < MAX_I2C_RETRIES) {
		ns = i2c_transfer(adap, msgs, len);
		if (ns == len)
			break;
		pr_err("%s: try %d/%d: i2c_transfer sent: %d, len %d\n",
			__func__,
			i2c_retry, MAX_I2C_RETRIES, ns, len);
		msleep(10);
	}

	return ns == len ? 0 : -EIO;
}


static int s5k6aafx_i2c_txdata(unsigned short saddr,
				  unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr >> 1,	/* To fit new i2c device in msm_sensor */
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer_retry(s5k6aafx_client->adapter, msg, 1) < 0) {
		pr_info("%s failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int s5k6aafx_i2c_write(unsigned short saddr,
				 unsigned short waddr, unsigned short wdata)
{
	int rc = -EIO;
	unsigned char buf[4];
	memset(buf, 0, sizeof(buf));

	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = (wdata & 0xFF00) >> 8;
	buf[3] = (wdata & 0x00FF);
	rc = s5k6aafx_i2c_txdata(saddr, buf, 4);
	if (rc < 0)
		pr_info("%s i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		     __func__, waddr, wdata);

	return rc;
}

static int s5k6aafx_i2c_write_table(struct s5k6aafx_i2c_reg_conf const
				       *reg_conf_tbl, int num_of_items_in_table)
{
	int i;
	int rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = s5k6aafx_i2c_write(s5k6aafx_client->addr,
		       reg_conf_tbl->waddr, reg_conf_tbl->wdata);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}
	return rc;
}

static int s5k6aafx_i2c_rxdata(unsigned short saddr,
			      unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = saddr >> 1,	/* To fit new i2c device in msm_sensor */
		 .flags = 0,
		 .len = 2,
		 .buf = rxdata,
		 },
		{
		 .addr = saddr >> 1,	/* To fit new i2c device in msm_sensor */
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxdata,
		 },
	};

	if (i2c_transfer_retry(s5k6aafx_client->adapter, msgs, 2) < 0) {
		pr_info("%s failed!\n", __func__);
		return -EIO;
	}

	return 0;
}

static int s5k6aafx_i2c_read(unsigned short saddr,
				unsigned short raddr, unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = s5k6aafx_i2c_rxdata(saddr, buf, 2);
	if (rc < 0) {
		pr_err("%s failed!\n", __func__);
		return rc;
	}

	*rdata = buf[0] << 8 | buf[1];

	return rc;
}
static int s5k6aafx_gpio_pull(int gpio_pin, int pull_mode)
{
	int rc = 0;
	rc = gpio_request(gpio_pin, "s5k6aafx");
	if (!rc)
		gpio_direction_output(gpio_pin, pull_mode);
	else
		pr_err("%s GPIO(%d) request failed\n", __func__, gpio_pin);
	gpio_free(gpio_pin);
	return rc;
}

static int s5k6aafx_set_qtr_size_mode(enum qtr_size_mode qtr_size_mode_value)
{
	int rc = 0;

	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;
	pr_info("%s", __func__);
	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_I2C_MODE_GENERAL);
	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_W_ADDH, S5K6AAFX_ADDH_SW_REG_INT);

	qtr_size_mode = qtr_size_mode_value;
	
	/* separate mipi/parallel */
	if (g_csi_if) {
		switch (qtr_size_mode_value) {
		case NORMAL_QTR_SIZE_MODE:
			pr_info("%s NORMAL_QTR_SIZE_MODE\n", __func__);
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.prev_mode_switch_VGA[0],
				s5k6aafx_regs.prev_mode_switch_VGA_size);
			mdelay(150);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0224);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0226);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			/*********** APPLY PREVIEW CONFIGURATION & RUN PREVIEW ***********/
			/* REG_TC_GP_ActivePrevConfig-Select preview configuration_3 */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x021C);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0003);
			/* REG_TC_GP_PrevOpenAfterChange */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0220);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			/* REG_TC_GP_NewConfigSync-Update preview configuration */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x01F8);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			/* REG_TC_GP_PrevConfigChanged */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x021E);
			/* Enable output after config change */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x01F0);
			/* REG_TC_GP_EnablePreview - Start preview */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			//mdelay(150); //mark for moving it to set preview mode
			/* REG_TC_GP_EnablePreviewChanged */
			//s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			break;

		case LARGER_QTR_SIZE_MODE:/*HD*/
			pr_info("%s LARGER_QTR_SIZE_MODE\n", __func__);
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.prev_HD[0],
			s5k6aafx_regs.prev_HD_size);
			mdelay(150); // reduce from 200 to 150 by vendor's suggestion
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0250);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0254);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x029A);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x01DE);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x021E);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			//mdelay(200); //mark for moving it to set preview mode

			break;
		default:
			break;
		}
	} else {
		switch (qtr_size_mode_value) {
		case NORMAL_QTR_SIZE_MODE:
			pr_info("%s NORMAL_QTR_SIZE_MODE\n", __func__);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021C);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0003);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0224);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021E);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0226);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);

			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x020A);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_ADJ_FULL_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_ADJ_FULL_SIZE_HEIGHT);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0212);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_FULL_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_FULL_SIZE_HEIGHT);

			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x01FA);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_ADJ_FULL_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_ADJ_FULL_SIZE_HEIGHT);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR,  (S5K6AAFX_FULL_SIZE_WIDTH-S5K6AAFX_ADJ_FULL_SIZE_WIDTH)/2);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR,  (S5K6AAFX_FULL_SIZE_HEIGHT-S5K6AAFX_ADJ_FULL_SIZE_HEIGHT)/2);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_FULL_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_FULL_SIZE_HEIGHT);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR,  0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR,  0x0000);

			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021A);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);

			mdelay(100);

			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021E);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0226);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);

			mdelay(600);

			break;
		case LARGER_QTR_SIZE_MODE:
			pr_info("%s LARGER_QTR_SIZE_MODE\n", __func__);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021C);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0224);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021E);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0226);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);

			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x020A);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_HEIGHT);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0212);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_HEIGHT);

			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x01FA);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_HEIGHT);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, (S5K6AAFX_FULL_SIZE_WIDTH-S5K6AAFX_720P_SIZE_WIDTH)/2);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, (S5K6AAFX_FULL_SIZE_HEIGHT-S5K6AAFX_720P_SIZE_HEIGHT)/2);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_WIDTH);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, S5K6AAFX_720P_SIZE_HEIGHT);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, (S5K6AAFX_FULL_SIZE_WIDTH-S5K6AAFX_720P_SIZE_WIDTH)/2);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, (S5K6AAFX_FULL_SIZE_HEIGHT-S5K6AAFX_720P_SIZE_HEIGHT)/2);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021A);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);

			mdelay(100);

			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x021E);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_W_ADDL, 0x0226);
			s5k6aafx_i2c_write(s5k6aafx_client->addr,
				S5K6AAFX_REG_WR, 0x0001);

			mdelay(200);

			break;
		default:
			break;
		}
	}
	return 0;
}


static int s5k6aafx_set_front_camera_mode(enum frontcam_t frontcam_value)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE || previous_mirror_mode == frontcam_value)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (frontcam_value) {
	case CAMERA_MIRROR:
		/*mirror and flip*/
		if (!s5k6aafx_ctrl.sensordata->full_size_preview &&
			!s5k6aafx_ctrl.sensordata->power_down_disable &&
			!s5k6aafx_ctrl.sensordata->mirror_mode) {
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D4, 0x0002);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D6, 0x0002);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0262, 0x0002);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0264, 0x0002);
		} else { /* for flyer */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D4, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D6, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0262, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0264, 0x0001);
		}
		break;
	case CAMERA_REVERSE:
		/*reverse mode*/
		if (!s5k6aafx_ctrl.sensordata->full_size_preview &&
			!s5k6aafx_ctrl.sensordata->power_down_disable &&
			!s5k6aafx_ctrl.sensordata->mirror_mode) {
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D4, 0x0003);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D6, 0x0003);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0262, 0x0003);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0264, 0x0003);
		} else { /* for flyer */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D4, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D6, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0262, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0264, 0x0000);
		}
		break;

	case CAMERA_PORTRAIT_REVERSE:
		/*portrait reverse mode*/
		if (!s5k6aafx_ctrl.sensordata->full_size_preview &&
			!s5k6aafx_ctrl.sensordata->power_down_disable &&
			!s5k6aafx_ctrl.sensordata->mirror_mode) {
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D4, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D6, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0262, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0264, 0x0000);
		} else { /* for flyer */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D4, 0x0003);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x02D6, 0x0003);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0262, 0x0003);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0264, 0x0003);
		}
		break;

	default:
		break;
	}

	s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_TC_GP_PrevConfigChanged, 0x0001);

	previous_mirror_mode = frontcam_value;

	msleep(150);

	return 0;
}
#if 1
static void s5k6aafx_mipi_setting(void){
		/*raise up driving strength*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0xFCFC, 0xD000);/*FCFC page*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0xB0A2, 0x001E);
		/*Dphy CTS 1.0 timing match to Qualocmm 8660*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDH, 0xD000);

		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0xB0CC);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x000B);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0xB0A2);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x001E);

		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0xB0C0);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x2EE0);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0960);/*B0C2*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0xB0C8);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0005);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0003);/*B0CA*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x000A);/*B0CC*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0xB0EA);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0004);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);/*B0EC*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0xB0E6);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0004);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);/*B0E8*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0xB0F0);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0004);
}
#endif

static int s5k6aafx_set_sensor_mode(struct msm_sensor_ctrl_t *s_ctrl, int mode, enum qtr_size_mode qtr_size_mode_value)
{

	pr_info("%s: sinfo->csi_if = %d, mode = %d, qtr_size_mode_value: %d",
		__func__, g_csi_if, mode, qtr_size_mode_value);

	if (config_csi == 0) {
		if (g_csi_if) {
			s_ctrl->curr_frame_length_lines =
				s_ctrl->msm_sensor_reg->output_settings[mode].frame_length_lines;

			s_ctrl->curr_line_length_pclk =
				s_ctrl->msm_sensor_reg->output_settings[mode].line_length_pclk;

			pr_info("%s: set csi config\n", __func__);
			v4l2_subdev_notify(&(s5k6aafx_s_ctrl.sensor_v4l2_subdev),
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_OFF_IMMEDIATELY));

			pr_info("%s subdev name: %s\n",
				__func__, s5k6aafx_s_ctrl.sensor_v4l2_subdev.name);

			s5k6aafx_s_ctrl.curr_csi_params = s5k6aafx_s_ctrl.csi_params[mode];
			v4l2_subdev_notify(&(s5k6aafx_s_ctrl.sensor_v4l2_subdev),
					NOTIFY_CSID_CFG, &s5k6aafx_s_ctrl.curr_csi_params->csid_params);
			v4l2_subdev_notify(&(s5k6aafx_s_ctrl.sensor_v4l2_subdev),
					NOTIFY_CID_CHANGE, NULL);
			dsb();

			msleep(1);
			v4l2_subdev_notify(&(s5k6aafx_s_ctrl.sensor_v4l2_subdev),
					NOTIFY_CSIPHY_CFG, &s5k6aafx_s_ctrl.curr_csi_params->csiphy_params);
			dsb();
			config_csi = 1;

			msleep(1);
			v4l2_subdev_notify(&(s5k6aafx_s_ctrl.sensor_v4l2_subdev),
				NOTIFY_PCLK_CHANGE, &s5k6aafx_dimensions[0].op_pixel_clk);
			v4l2_subdev_notify(&(s5k6aafx_s_ctrl.sensor_v4l2_subdev),
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_ON_FRAME_BOUNDARY));
		}
	}

	pr_info("%s\n", __func__);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		pr_info("%s: sensor set mode: preview\n", __func__);
		op_mode = SENSOR_PREVIEW_MODE;

		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_I2C_MODE, S5K6AAFX_I2C_MODE_GENERAL);

		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_W_ADDH, S5K6AAFX_ADDH_SW_REG_INT);
		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_W_ADDL, 0x01F4);
		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_WR, 0x0000); /* REG_TC_GP_EnableCapture */
		/* REG_TC_GP_EnableCaptureChanged */
		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_WR, 0x0001);

		if (qtr_size_mode != qtr_size_mode_value)
			s5k6aafx_set_qtr_size_mode(qtr_size_mode_value);
		mdelay(150);
		break;

	case SENSOR_SNAPSHOT_MODE:
		pr_info("%s: sensor set mode: snapshot\n", __func__);
		op_mode = SENSOR_SNAPSHOT_MODE;

		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_I2C_MODE, S5K6AAFX_I2C_MODE_GENERAL);

		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_W_ADDH, S5K6AAFX_ADDH_SW_REG_INT);
		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_W_ADDL, 0x01F4);
		/* REG_TC_GP_EnableCapture */
		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_WR, 0x0001);
		/* REG_TC_GP_EnableCaptureChanged */
		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_WR, 0x0001);
		mdelay(150);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int s5k6aafx_set_FPS(struct fps_cfg *fps)
{
	/* Avoid duplicate action to set FPS */
	static struct fps_cfg pre_fps_cfg = {
		.fps_div = -1,
	};

	static int pre_op_mode = -1;
	if (pre_op_mode != op_mode) {
		pre_fps_cfg.fps_div = -1;
		pre_op_mode = op_mode;
	}

	if (pre_fps_cfg.fps_div == fps->fps_div)
		return 0;
	else
		pre_fps_cfg.fps_div = fps->fps_div;

	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;
	pr_info("%s fps->fps_div=%d", __func__, fps->fps_div);

	s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);
	/* s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usFrTimeType, 1); */

	if (fps->fps_div == 0) {
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMaxFrTimeMsecMult10, 0x0535);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMinFrTimeMsecMult10, 0x0168);
	}	else if (fps->fps_div == 15) {
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMaxFrTimeMsecMult10, 0x029A);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMinFrTimeMsecMult10, 0x029A);
	}	else if (fps->fps_div == 10) {
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMaxFrTimeMsecMult10, 0x03E8);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMinFrTimeMsecMult10, 0x03E8);
	}	else if (fps->fps_div == 1015) {
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMaxFrTimeMsecMult10, 0x03E8);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_3TC_PCFG_usMinFrTimeMsecMult10, 0x029A);
	}
	s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_TC_GP_PrevConfigChanged, 0x0001);

	return 0;
}
static int32_t s5k6aafx_get_output_info(struct sensor_output_info_t *sensor_output_info)
{
	int rc = 0;
	sensor_output_info->num_info = 2;
	if (copy_to_user((void *)sensor_output_info->output_info,
		s5k6aafx_dimensions,
		sizeof(struct msm_sensor_output_info_t) * 2))
		rc = -EFAULT;

	return rc;
}

static int s5k6aafx_set_effect(int effect)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (effect) {
	case CAMERA_EFFECT_OFF:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EE, 0x0000);
		break;
	case CAMERA_EFFECT_MONO:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EE, 0x0001);
		break;
	case CAMERA_EFFECT_NEGATIVE:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EE, 0x0002);
		break;
	case CAMERA_EFFECT_SEPIA:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EE, 0x0003);
		break;
	case CAMERA_EFFECT_AQUA:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EE, 0x0004);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static int s5k6aafx_set_antibanding(enum antibanding_mode antibanding_value)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (antibanding_value) {
	case CAMERA_ANTI_BANDING_50HZ:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DC, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DE, 0x0001);
		break;
	case CAMERA_ANTI_BANDING_60HZ:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DC, 0x0002);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DE, 0x0001);
		break;
	case CAMERA_ANTI_BANDING_AUTO:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DC, 0x0002);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DE, 0x0001);
		break;
	}
	return 0;
}


static int s5k6aafx_set_brightness(enum brightness_t brightness_value)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (brightness_value) {
	case CAMERA_BRIGHTNESS_N4:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0xFF81);
		break;
	case CAMERA_BRIGHTNESS_N3:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0xFFA1);
		break;
	case CAMERA_BRIGHTNESS_N2:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0xFFC1);
		break;
	case CAMERA_BRIGHTNESS_N1:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0xFFE1);
		break;
	case CAMERA_BRIGHTNESS_D:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0x0000);
		break;
	case CAMERA_BRIGHTNESS_P1:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0x001F);
		break;
	case CAMERA_BRIGHTNESS_P2:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0x003F);
		break;
	case CAMERA_BRIGHTNESS_P3:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0x005F);
		break;
	case CAMERA_BRIGHTNESS_P4:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E4, 0x007F);
		break;
	default:
		 break;
	}
	return 0;
}

static int s5k6aafx_set_wb(enum wb_mode wb_value)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (wb_value) {
	case CAMERA_AWB_AUTO: /*auto*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0400, 0x007F);
		break;
	case CAMERA_AWB_CLOUDY: /*cloudy*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0400, 0x0077);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D0, 0x0185);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D2, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D4, 0x0100);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D6, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D8, 0x0150);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DA, 0x0001);
		break;
	case CAMERA_AWB_INDOOR_HOME: /*Fluorescent*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0400, 0x0077);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D0, 0x0110);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D2, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D4, 0x0100);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D6, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D8, 0x0235);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DA, 0x0001);
		break;
	case CAMERA_AWB_INDOOR_OFFICE: /*Incandescent*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0400, 0x0077);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D0, 0x0100);//100 130 150 0x01B3 R gain//
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D2, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D4, 0x0100);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D6, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D8, 0x0238);//210 180 0x0150 B gain//
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DA, 0x0001);
		break;
	case CAMERA_AWB_SUNNY: /*outdoor*/
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x0400, 0x0077);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D0, 0x0175);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D2, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D4, 0x0100);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D6, 0x0001);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03D8, 0x0160);
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x03DA, 0x0001);
		break;
	default:
		break;
	}
	return 0;
}


static int s5k6aafx_set_sharpness(enum sharpness_mode sharpness_value)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (sharpness_value) {
	case CAMERA_SHARPNESS_X0:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EA, 0xFF81);
		break;
	case CAMERA_SHARPNESS_X1:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EA, 0xFFC1);
		break;
	case CAMERA_SHARPNESS_X2:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EA, 0x0000);
		break;
	case CAMERA_SHARPNESS_X3:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EA, 0x003F);
		break;
	case CAMERA_SHARPNESS_X4:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01EA, 0x007F);
		break;
	default:
		break;
	}
	return 0;
}


static int s5k6aafx_set_saturation(enum saturation_mode saturation_value)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (saturation_value) {
	case CAMERA_SATURATION_X0:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E8, 0xFF81);
		break;
	case CAMERA_SATURATION_X05:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E8, 0xFFC1);
		break;
	case CAMERA_SATURATION_X1:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E8, 0x0000);
		break;
	case CAMERA_SATURATION_X15:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E8, 0x003F);
		break;
	case CAMERA_SATURATION_X2:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E8, 0x007F);
		break;
	default:
		break;
	}
	return 0;
}

static int s5k6aafx_set_contrast(enum contrast_mode contrast_value)
{
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_ADDH_SW_REG_INT);

	switch (contrast_value) {
	case CAMERA_CONTRAST_N2:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E6, 0xFF81);
		break;
	case CAMERA_CONTRAST_N1:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E6, 0xFFC1);
		break;
	case CAMERA_CONTRAST_D:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E6, 0x0000);
		break;
	case CAMERA_CONTRAST_P1:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E6, 0x0029/*0x0028*/);
		break;
	case CAMERA_CONTRAST_P2:
		s5k6aafx_i2c_write(s5k6aafx_client->addr, 0x01E6, 0x0058/*0x0050*/);
		break;
	default:
		break;
	}
	return 0;
}

#if 0
static int s5k6aafx_set_metering_mode(enum aec_metering_mode metering_value)
{
	uint16_t weight_table[32];
	uint8_t i;

	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	for (i = 0; i < 32; i++)
		weight_table[i] = 0x0101;

	if (metering_value == CAMERA_METERING_CENTERING) {
		weight_table[9] = 0x0303;
		weight_table[10] = 0x0303;
		weight_table[13] = 0x0303; /* 0x0305 */
		weight_table[14] = 0x0303; /* 0x0503 */
		weight_table[17] = 0x0303; /* 0x0305 */
		weight_table[18] = 0x0303; /* 0x0503 */
		weight_table[21] = 0x0303;
		weight_table[22] = 0x0303;
	} else if (metering_value == CAMERA_METERING_SPOT) {
		weight_table[13] = 0x0501;
		weight_table[14] = 0x0105;
		weight_table[17] = 0x0501;
		weight_table[18] = 0x0105;
	} else if (metering_value >= CAMERA_METERING_ZONE1 &&
		metering_value <= CAMERA_METERING_ZONE16) {
		i = metering_value - CAMERA_METERING_ZONE1;
		i += (i & 0xFC); /* i=i+((int)(i/4))*4 */
		weight_table[i] = 0x0505;
		weight_table[i+4] = 0x0505;
	}

	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_I2C_MODE, S5K6AAFX_I2C_MODE_GENERAL);
	s5k6aafx_i2c_write(s5k6aafx_client->addr,
		S5K6AAFX_REG_W_ADDH, S5K6AAFX_ADDH_SW_REG_INT);
	s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x100E);

	for (i = 0; i < 32; i++) {
		CDBG("[CAM] write table[%d]=%x\n", i, weight_table[i]);
		s5k6aafx_i2c_write(s5k6aafx_client->addr,
			S5K6AAFX_REG_WR, weight_table[i]);
	}

	return 0;
}
#endif

static int s5k6aafx_sensor_read_id(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id;
	int rc = 0;

	pr_info("%s \n", __func__);
	/* Read the Model ID of the sensor */
	rc = s5k6aafx_i2c_write(s5k6aafx_client->addr,
	       S5K6AAFX_REG_I2C_MODE, S5K6AAFX_I2C_MODE_GENERAL);
	if (rc < 0)
		goto init_probe_fail;
	rc = s5k6aafx_i2c_write(s5k6aafx_client->addr,
	       S5K6AAFX_REG_R_ADDH, S5K6AAFX_ADDH_SW_REG_INT);
	if (rc < 0)
		goto init_probe_fail;
	rc = s5k6aafx_i2c_write(s5k6aafx_client->addr,
	       S5K6AAFX_REG_R_ADDL, S5K6AAFX_REG_MODEL_ID);
	if (rc < 0)
		goto init_probe_fail;
	rc = s5k6aafx_i2c_read(s5k6aafx_client->addr,
		S5K6AAFX_REG_WR, &model_id);
	if (rc < 0)
		goto init_probe_fail;

	pr_info("%s: model_id = 0x%x\n", __func__, model_id);
	/* Check if it matches it with the value in Datasheet */
	if (model_id != S5K6AAFX_MODEL_ID) {
		pr_info("%s invalid model id\n", __func__);
		rc = -EINVAL;
		goto init_probe_fail;
	}

init_probe_fail:
	return rc;

}

int s5k6aafx_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	g_csi_if = data->csi_if;

	pr_info("%s s5k6aafx_regs.reset_init after 0x%X\n",
		__func__, (unsigned int)s5k6aafx_regs.reset_init);
	if (data == NULL) {
		pr_err("%s sensor data is NULL\n", __func__);
		return -EINVAL;
	}
	s5k6aafx_ctrl.sensordata = data;

	mdelay(1);

	/*MCLK ENABLE*/
	rc = msm_camio_clk_enable(sdata,CAMIO_CAM_MCLK_CLK);
	
	msm_camio_clk_rate_set(24000000);
	mdelay(1);

	if (s5k6aafx_gpio_pull(data->sensor_reset, 1) < 0)
		goto init_fail;
	/*sugest by samsung change from 100ms to 10ms to pass CTS 2nd cam launch time*/
	mdelay(10);

	/*reset sensor*/
	rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.reset_init[0],
		s5k6aafx_regs.reset_init_size);
	if (rc < 0)
		goto init_fail;
	mdelay(10);

	/*T&P setting*/
	rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.TP_init[0],
		s5k6aafx_regs.TP_init_size);
	if (rc < 0)
		goto init_fail;

	/*analog setting*/
	rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.analog_setting_init[0],
		s5k6aafx_regs.analog_setting_init_size);
	if (rc < 0)
		goto init_fail;
	mdelay(10);

	/*set initial register*/
	rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.register_init[0],
		s5k6aafx_regs.register_init_size);
	if (rc < 0)
		goto init_fail;

	/*set clock*/
	if (data->csi_if) {/*mipi*/
		pr_info("%s set mipi sensor clk\n", __func__);
		rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.mipi_clk_init[0],
			s5k6aafx_regs.mipi_clk_init_size);
		if (rc < 0)
			goto init_fail;
		/*sugest by samsung to pass CTS 2nd cam launch time*/
		/*mdelay(134);*/

		/*init setting*/
		pr_info("%s: pre_snap_conf_init\n", __func__);
		rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.prev_snap_conf_init[0],
			s5k6aafx_regs.prev_snap_conf_init_size);

		/* preview configuration */
		if (!data->full_size_preview) {/*VGA*/
			pr_info("%s NORMAL_QTR_SIZE_MODE\n", __func__);
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.prev_mode_switch_VGA[0],
			s5k6aafx_regs.prev_mode_switch_VGA_size);
			mdelay(150);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0224);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0226);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			/*********** APPLY PREVIEW CONFIGURATION & RUN PREVIEW ***********/
			/* REG_TC_GP_ActivePrevConfig-Select preview configuration_3 */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x021C);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0003);
			/* REG_TC_GP_PrevOpenAfterChange */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0220);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			/* REG_TC_GP_NewConfigSync-Update preview configuration */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x01F8);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			/* REG_TC_GP_PrevConfigChanged */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x021E);
			/* Enable output after config change */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x01F0);
			/* REG_TC_GP_EnablePreview - Start preview */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
			/* REG_TC_GP_EnablePreviewChanged */
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
		} else {/*HD*/
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.prev_HD[0],
			s5k6aafx_regs.prev_HD_size);
			mdelay(150);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0250);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0000);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x0254);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x029A);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x01DE);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_W_ADDL, 0x021E);
			s5k6aafx_i2c_write(s5k6aafx_client->addr, S5K6AAFX_REG_WR, 0x0001);
		}

		if (rc < 0)
			goto init_fail;

	} else { /*parallel*/
		if (!data->full_size_preview) {
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.clk_init[0],
				s5k6aafx_regs.clk_init_size);
		} else { /* for flyer */
			pr_info("%s: clk_init_tb2\n", __func__);
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.clk_init_tb2[0],
				s5k6aafx_regs.clk_init_tb2_size);
		}
		if (rc < 0)
			goto init_fail;
		/*sugest by samsung to pass CTS 2nd cam launch time*/
		/*mdelay(100);*/

		/* preview configuration */
		if (!data->full_size_preview) {
			pr_info("%s: pre_snap_conf_init\n", __func__);
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.prev_snap_conf_init[0],
				s5k6aafx_regs.prev_snap_conf_init_size);
		} else {
			pr_info("%s: pre_snap_conf_init_tb2\n", __func__);
			rc = s5k6aafx_i2c_write_table(&s5k6aafx_regs.prev_snap_conf_init_tb2[0],
				s5k6aafx_regs.prev_snap_conf_init_tb2_size);
		}

		if (rc < 0)
			goto init_fail;

	}

	if (data->csi_if) {/*mipi*/
		mdelay(200); // set 200 as vendor's suggestion
		/*MIPI Non_continous enable*/
		s5k6aafx_mipi_setting();
	}

	/* set default AWB */
	rc = s5k6aafx_set_wb(CAMERA_AWB_AUTO);
	if (rc < 0)
		goto init_fail;

	rc = s5k6aafx_sensor_read_id(data);
	
	s5k6aafx_ctrl.fps_divider = 1*0x00000400;
	s5k6aafx_ctrl.pict_fps_divider = 1* 0x00000400;
	s5k6aafx_ctrl.set_test = TEST_OFF;
	s5k6aafx_ctrl.prev_res = QTR_SIZE;
	s5k6aafx_ctrl.pict_res = FULL_SIZE;
	if (rc < 0)
		goto init_fail;
	config_csi = 0;
	qtr_size_mode = data->full_size_preview;
	previous_mirror_mode = -1;
	return rc;

init_fail:
	mdelay(5);
	s5k6aafx_gpio_pull(data->sensor_reset, 0);
	mdelay(1);
	msm_camio_clk_disable(data,CAMIO_CAM_MCLK_CLK);
	mdelay(1);	

	memset(&s5k6aafx_ctrl, 0, sizeof(s5k6aafx_ctrl));
	return rc;
}

int32_t s5k6aafx_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;

	pr_info("%s\n", __func__);
	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("%s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_on == NULL) {
		pr_err("%s camera_power_on is NULL\n", __func__);
		return -EIO;
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	/*
		use s5k6aafx_sensor_open_init instead of msm_sensor_set_power_up
		It will do sensor reset and mclk enable
	*/
	rc = s5k6aafx_sensor_open_init(sdata);
	if (rc < 0) {
		goto enable_power_on_failed;		
	}
	pr_info("%s end\n", __func__);

	return rc;

enable_power_on_failed:
	return rc;
}

int32_t s5k6aafx_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("%s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	s5k6aafx_gpio_pull(s5k6aafx_ctrl.sensordata->sensor_reset, 0);

	if (sdata->camera_power_off == NULL) {
		pr_err("%s sensor platform_data didn't register\n", __func__);
		return -EIO;
	}

	rc = sdata->camera_power_off();
	if (rc < 0)
		pr_err("%s failed to disable power\n", __func__);

	if (!sdata->use_rawchip) {
		pr_info("%s MCLK disable clk\n", __func__);
		msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_err("[%s: msm_camio_sensor_clk_off failed:%d\n",
				 __func__, rc);
	}

	memset(&s5k6aafx_ctrl, 0, sizeof(s5k6aafx_ctrl));
	return rc;
}

void s5k6aafx_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("%s\n", __func__);
	return;
}

static int s5k6aafx_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k6aafx_wait_queue);
	return 0;
}

int s5k6aafx_sensor_config(struct msm_sensor_ctrl_t *s_ctrl, void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long rc = 0;
	
	if (copy_from_user(&cfg_data,
			   (void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	switch (cfg_data.cfgtype) {
	case CFG_GET_OUTPUT_INFO:
		rc = s5k6aafx_get_output_info(&cfg_data.cfg.output_info);
		if (copy_to_user((void *)argp,
			&cfg_data,
			sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_SET_MODE:
		rc = s5k6aafx_set_sensor_mode(s_ctrl, cfg_data.mode, cfg_data.cfg.qtr_size_mode_value);
		break;
	case CFG_SET_EFFECT:
		rc = s5k6aafx_set_effect(cfg_data.cfg.effect);
		break;
	case CFG_SET_FPS:
		rc = s5k6aafx_set_FPS(&(cfg_data.cfg.fps));
		break;
	case CFG_SET_ANTIBANDING:
		rc = s5k6aafx_set_antibanding
				(cfg_data.cfg.antibanding_value);
		break;
	case CFG_SET_BRIGHTNESS:
		rc = s5k6aafx_set_brightness
				(cfg_data.cfg.brightness_value);
		break;
	case CFG_SET_WB:
		rc = s5k6aafx_set_wb(cfg_data.cfg.wb_value);
		break;
	case CFG_SET_SHARPNESS:
		rc = s5k6aafx_set_sharpness
			(cfg_data.cfg.sharpness_value);
		break;
	case CFG_SET_SATURATION:
		rc = s5k6aafx_set_saturation
			(cfg_data.cfg.saturation_value);
		break;
	case CFG_SET_CONTRAST:
		rc = s5k6aafx_set_contrast(cfg_data.cfg.contrast_value);
		break;
	case CFG_SET_FRONT_CAMERA_MODE:
		rc = s5k6aafx_set_front_camera_mode(cfg_data.cfg.frontcam_value);
		break;
	case CFG_SET_QTR_SIZE_MODE:
		rc = s5k6aafx_set_qtr_size_mode(cfg_data.cfg.qtr_size_mode_value);
		break;	
	case CFG_I2C_IOCTL_R_OTP:
		rc = 0;
		break;
#if 0
	case CFG_SET_EXPOSURE_MODE:
		rc = s5k6aafx_set_metering_mode
			(cfg_data.cfg.metering_value);
		break;
#endif
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static const char *S5K6AAFXVendor = "Samsung";
static const char *S5K6AAFXNAME = "s5k6aafx";
static const char *S5K6AAFXSize = "1M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", S5K6AAFXVendor, S5K6AAFXNAME, S5K6AAFXSize);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t sensor_read_node(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t length;
	length = sprintf(buf, "%d\n", sensor_probe_node);
	return length;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);
static DEVICE_ATTR(node, 0444, sensor_read_node, NULL);

static struct kobject *android_s5k6aafx;

static int s5k6aafx_sysfs_init(void)
{
	int ret ;
	pr_info("%s: kobject creat and add\n", __func__);
	android_s5k6aafx = kobject_create_and_add("android_camera2", NULL);
	if (android_s5k6aafx == NULL) {
		pr_info("%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("%s: sysfs_create_file\n", __func__);
	ret = sysfs_create_file(android_s5k6aafx, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("%s: sysfs_create_file failed\n", __func__);
		kobject_del(android_s5k6aafx);
	}

    ret = sysfs_create_file(android_s5k6aafx, &dev_attr_node.attr);
	if (ret) {
		pr_info("%s: dev_attr_node failed\n", __func__);
		kobject_del(android_s5k6aafx);
	}

	return 0 ;
}

int32_t s5k6aafx_i2c_probe_2(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	pr_info("%s: sensor_i2c_probe called - name: %s\n", __func__, client->name);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		pr_err("%s: failed to sensor_i2c_client is NULL\n", __func__);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl->sensordata = client->dev.platform_data;
	if (s_ctrl->sensordata == NULL) {
		pr_err("%s: failed to sensor data is NULL\n", __func__);
		return -EFAULT;
	}

	msm_camio_probe_on_bootup(s_ctrl);	//HTC_START steven 20120619 fix display dark screen on bootup stage

	if (s_ctrl->func_tbl && s_ctrl->func_tbl->sensor_power_up)
		rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);

	if (rc < 0)
		goto probe_fail;

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);

	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	goto power_down;
probe_fail:
	pr_info("%s_i2c_probe failed\n", client->name);
power_down:
	if (rc > 0)
		rc = 0;

	if (s_ctrl->func_tbl && s_ctrl->func_tbl->sensor_power_down)
		s_ctrl->func_tbl->sensor_power_down(s_ctrl);

	msm_camio_probe_off_bootup(s_ctrl);	//HTC_START steven 20120619 fix display dark screen on bootup stage

	return rc;
}

static int s5k6aafx_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	int rc = 0;

	pr_info("%s\n", __func__);
	
	s5k6aafx_sensorw = kzalloc(sizeof(struct s5k6aafx_work), GFP_KERNEL);

	if (!s5k6aafx_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k6aafx_sensorw);
	s5k6aafx_init_client(client);
	s5k6aafx_client = client;

	rc = s5k6aafx_i2c_probe_2(client, id);	/* It has special read ID sequence*/

	if (rc >= 0)
		s5k6aafx_sysfs_init();
	else
		goto probe_failure;

	pr_info("%s succeeded!\n", __func__);

	return rc;

probe_failure:
	kfree(s5k6aafx_sensorw);
	s5k6aafx_sensorw = NULL;
	pr_info("%s failed!\n", __func__);

	return rc;
}

static struct msm_camera_i2c_client s5k6aafx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static struct msm_sensor_id_info_t s5k6aafx_id_info = {
	.sensor_id_reg_addr = S5K6AAFX_REG_MODEL_ID,
	.sensor_id = S5K6AAFX_MODEL_ID,
};

static struct msm_camera_csid_vc_cfg s5k6aafx_cid_cfg[] = {
	{0, CSI_YUV422_8, CSI_DECODE_8BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
	{2, CSI_RAW8, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k6aafx_csi_params = {
	.csid_params = {
		.lane_cnt = 1,
		.lane_assign = 0xE4,
		.lut_params = {
			.num_cid = ARRAY_SIZE(s5k6aafx_cid_cfg),
			.vc_cfg = s5k6aafx_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 20,
		.lane_mask = 1,
	},
};

static struct msm_camera_csi2_params *s5k6aafx_csi_params_array[] = {
	&s5k6aafx_csi_params,
	&s5k6aafx_csi_params,
};

static struct v4l2_subdev_info s5k6aafx_subdev_info[] = {
	{
		.code		= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt		= 1,
		.order		= 0,
	},
	/* more can be supported, to be added later */
};

static struct v4l2_subdev_core_ops s5k6aafx_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k6aafx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k6aafx_subdev_ops = {
	.core = &s5k6aafx_subdev_core_ops,
	.video  = &s5k6aafx_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k6aafx_func_tbl = {
	.sensor_stop_stream = s5k6aafx_stop_stream,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = s5k6aafx_sensor_config,
	.sensor_power_up = s5k6aafx_power_up,
	.sensor_power_down = s5k6aafx_power_down,
};

static const struct i2c_device_id s5k6aafx_i2c_id[] = {
	{"s5k6aafx", (kernel_ulong_t)&s5k6aafx_s_ctrl},
	{},
};

static struct i2c_driver s5k6aafx_i2c_driver = {
	.id_table = s5k6aafx_i2c_id,
	.probe  = s5k6aafx_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_sensor_reg_t s5k6aafx_sensor_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.output_settings = &s5k6aafx_dimensions[0],
};


static struct msm_sensor_ctrl_t s5k6aafx_s_ctrl = {
	.msm_sensor_reg = &s5k6aafx_sensor_regs,
	.sensor_i2c_client = &s5k6aafx_sensor_i2c_client,
	.sensor_i2c_addr = 0x5a,
	.sensor_id_info = &s5k6aafx_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &s5k6aafx_csi_params_array[0],
	.msm_sensor_mutex = &s5k6aafx_mut,
	.sensor_i2c_driver = &s5k6aafx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k6aafx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k6aafx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k6aafx_subdev_ops,
	.func_tbl = &s5k6aafx_func_tbl,
};

static int __init s5k6aafx_init(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&s5k6aafx_i2c_driver);
}

module_init(s5k6aafx_init);
