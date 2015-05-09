/* drivers/input/touchscreen/max11871.c
 *
 * Copyright (c)2012 Maxim Integrated Products, Inc.
 *
 * Driver Version: 3.0.4.3
 * Release Date: Nov 29, 2012
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/suspend.h>
#endif
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/max11871.h>
#include <linux/kthread.h>
#include <linux/firmware.h>
#include <linux/crc16.h>
#include <linux/input/mt.h>
#include <asm/byteorder.h>  /* must include this header to get byte order */
#include <mach/board.h>

#define DEBUG_LOG(level, fmt, args...)  do{ if( ((1 << level)&debug_mask)>0 ) \
				printk( KERN_INFO MAX11871_LOG_NAME fmt "\n", ## args); } while(0)
#define WARNING(d, e...) printk(KERN_ERR MAX11871_LOG_NAME "[W](%s:%d): " d "\n", \
				__func__, __LINE__, ##e)
#define CHECKWI(a, d, e...) do {if (a) WARNING(d, ##e); } while (0)
#define CHECKW(a, b, c, d, e...) do {if (a) {b; WARNING(d, ##e); return c; }; } \
				while (0)
#define ERROR(d, e...) printk(KERN_ERR MAX11871_LOG_NAME "TOUCH_ERR:(%s:%d): " d "\n", \
				__func__, __LINE__, ##e)
#define CHECKI(a, d, e...) do {if (a) ERROR(d, ##e); } while (0)
#define CHECK(a, b, c, d, e...) do {if (a) {b; ERROR(d, ##e); return c; }; } \
				while (0)
#define CHECKB(a, b, d, e...) do {if (a) {b; ERROR(d, ##e); break; }; } \
				while (0)

#define PRINT(d, e...) printk(KERN_INFO MAX11871_LOG_NAME d "\n", ##e)
#define DEBUGL(a) ((1 << (a + 15)) & debug_mask)
#define DEBUG(a, d, e...) do {if DEBUGL(a) \
		printk(KERN_INFO MAX11871_LOG_NAME d "\n", ##e); } while (0)
#define DEBUGHD(a, d, e...) do {if DEBUGL(a) \
		printk(KERN_INFO MAX11871_LOG_NAME d, ##e); } while (0)
#define DEBUGNF(a, d, e...) do {if DEBUGL(a) printk(d, ##e); } while (0)

#define DEVFCA(n, imode, ishow, istore, a)                              \
do {                                                                    \
	int ii;                                                         \
	for (ii = 0; ii < n; ii++) {                                    \
		snprintf(dev_attr_##a##_name[ii],                       \
			sizeof(dev_attr_##a##_name[ii]), __stringify(a) \
			"%d", ii + 1);                                  \
		dev_attr_##a[ii].attr.name = dev_attr_##a##_name[ii];   \
		dev_attr_##a[ii].attr.mode = imode;                     \
		dev_attr_##a[ii].show = ishow;                          \
		dev_attr_##a[ii].store = istore;                        \
		CHECK(device_create_file(&client->dev,                  \
			&dev_attr_##a[ii]) < 0, , 0,                    \
			"failed to create sysfs file [%s]",             \
			dev_attr_##a##_name[ii]);                       \
		ts->sysfs_created++;                                    \
	}                                                               \
} while (0)
#define DEVFRA(n, a)                                                         \
do {                                                                         \
	int ii;                                                              \
	for (ii = 0; ii < n; ii++)                                           \
		if (ts->sysfs_created && ts->sysfs_created--)                \
			device_remove_file(&client->dev, &dev_attr_##a[ii]); \
} while (0)

#define ENABLE_IRQ()                            \
do {                                            \
	mutex_lock(&ts->irq_mutex);             \
	if (ts->irq_disabled) {                 \
		enable_irq(ts->client->irq);    \
		ts->irq_disabled = 0;           \
	}                                       \
	mutex_unlock(&ts->irq_mutex);           \
} while (0)

#define DISABLE_IRQ()                           \
do {                                            \
	mutex_lock(&ts->irq_mutex);             \
	if (ts->irq_disabled == 0) {            \
		disable_irq(ts->client->irq);   \
		ts->irq_disabled = 1;           \
	}                                       \
	mutex_unlock(&ts->irq_mutex);           \
} while (0)

#define NWORDS(a)    (sizeof(a) / sizeof(u16))
#define BYTE_SIZE(a) ((a) * sizeof(u16))
#define BYTEH(a)     ((a) >> 8)
#define BYTEL(a)     ((a) & 0xFF)

#define CONFIG(a)      (ts->config->a)
#define COORDINATES(a) ts->fw_config->coordinates.a
#define BUTTONXY(i, a) COORDINATES(button_xy[i]).a

#define MAXIM_TOUCH_REPORT_MODE 0x0001 /* 1=basic, 2=extended */
#define MAX_REPORT_READERS  5
#define RETRY_TIMES 3
#define SHIFT_BITS 10

#if !MAX11871_BOARD_CONFIG
struct max11871_config local_config = {};
#endif

struct report_reader {
	u16 report_id;
	u16 reports_passed;
	struct semaphore sem;
	int status;
};

struct report_point {
	u8 state;
	int x;
	int y;
	int z;
};

struct data {
	struct max11871_config  *config;
	struct max11871_board_config  *fw_config;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *key_input_dev;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	u8  early_suspend_registered;
#endif
	struct mutex irq_mutex;
	struct mutex i2c_mutex;
	struct mutex report_mutex;
	struct semaphore report_sem;
	struct report_reader report_readers[MAX_REPORT_READERS];
	u8 irq_disabled;
	u16 nbuttons_original;
	u8 report_readers_outstanding;
	u16 report[MAX_WORDS_REPORT + 1];  /* with header */
	u16 rx_report[MAX_WORDS_REPORT + 1];  /* with header */
	u32 irq_counter;
	u8 got_report;
	int fw_index;
	u16 fw_crc16;
	u16 fw_version[MAX_WORDS_REPORT];
	u16 vendor_pin;
	u16 touch_config[MAX_WORDS_COMMAND_ALL];
	char fw_ver[10];
	int buttondown;
	int nobutton;
	char phys[32];
	char key_phys[32];
	/* firmware download decision */
	u8  fw_responsive;
	u8  have_fw;
	u8  have_touchcfg;
	u16 config_id;
	u16 controller_id;
	u8  sysfs_created;
	u8  is_raw_mode;
	u16 max11871_Touch_Configuration_Data[44];
	u16 max11871_Calibration_Table_Data[52];
	u16 max11871_Private_Configuration_Data[25];
	u16 max11871_Lookup_Table_X_Data[11];
	u16 max11871_Lookup_Table_Y_Data[11];
	u16 max11871_Image_Factor_Table[460];
	struct report_point report_points[10];
	u32 width_factor;
	u32 height_factor;
	u16 x_channel;
	u16 y_channel;
	u16 frame_rate[2];
	u16 frame_count;
	u8  noise_level;
	u8  baseline_mode;
	u8  protocol_ver;
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void early_suspend(struct early_suspend *h);
static void late_resume(struct early_suspend *h);
#endif

static int device_init(struct i2c_client *client);
static int device_deinit(struct i2c_client *client);

static int bootloader_enter(struct data *ts);
static int bootloader_exit(struct data *ts);
static int bootloader_get_crc(struct data *ts, u16 *crc16, u16 len);
static int bootloader_set_byte_mode(struct data *ts);
static int bootloader_erase_flash(struct data *ts);
static int bootloader_write_flash(struct data *ts, u8 *image);

static int set_touch_frame(struct i2c_client *client,
			u16 idle_frame, u16 active_frame);
static int set_baseline_mode(struct i2c_client *client, u16 mode);
static int change_touch_rpt(struct i2c_client *client, u16 to);
static int sreset(struct i2c_client *client);
static int get_touch_config(struct i2c_client *client);
static int get_fw_version(struct i2c_client *client);
static void propagate_report(struct data *ts, int status, u16 *report);
static int get_report(struct data *ts, u16 report_id, ulong timeout);
static void release_report(struct data *ts);

extern unsigned int get_tamper_sf(void);

/*	Bit 0 - Raw Data
	Bit 1 - Finger Data
	Bit 2 - Touch Latency
	Bit 3 - Up/Down Log
	Bit 16 - I2C RX
	Bit 17 - I2C TX
	Bit 18 - I2C RX Header/TX Address
	Bit 19 - Touch and Key processing
	Bit 20 - Initialization */
static u32  debug_mask = 0x00100000;
static u8   bootloader;
static u8   init_state;
static struct kobject *android_touch_kobj;
static struct data *gl_ts;

static int i2c_rx_bytes(struct data *ts, u8 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 2) ? 3 : 1;

	do {ret = i2c_master_recv(ts->client, (char *)buf, (int)len);
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C RX fail (%d)", ret);

	if (DEBUGL(1)) {
		DEBUGHD(level, "(RX): ");
		for (i = 0; i < len; i++)
			DEBUGNF(level, "%02X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret;
}

static int i2c_rx_words(struct data *ts, u16 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 1) ? 3 : 1;

	do {ret = i2c_master_recv(ts->client, (char *)buf, (int)(len * 2));
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C RX fail (%d)", ret);
	CHECK((ret % 2) != 0, , -1, "I2C words RX fail: odd number of bytes "
		"(%d)", ret);

#ifdef __BIG_ENDIAN
	for (i = 0; i < len; i++)
		buf[i] = (buf[i] << 8) | (buf[i] >> 8);
#endif
	if (DEBUGL(1)) {
		DEBUGHD(level, "(RX): ");
		for (i = 0; i < len; i++)
			DEBUGNF(level, "%04X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret / 2;
}

static int i2c_tx_bytes(struct data *ts, u8 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 2) ? 3 : 2;

	do {ret = i2c_master_send(ts->client, (char *)buf, (int)len);
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C TX fail (%d)", ret);

	if (DEBUGL(2)) {
		DEBUGHD(level, "(TX): ");
		if (len >= 1)
			DEBUGNF(bootloader ? 2 : 3, "%02X ", buf[0]);
		if (len >= 2)
			DEBUGNF(bootloader ? 2 : 3, "%02X ", buf[1]);
		for (i = 2; i < len; i++)
			DEBUGNF(level, "%02X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret;
}

static int i2c_tx_words(struct data *ts, u16 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 1) ? 3 : 2;

#ifdef __BIG_ENDIAN
	for (i = 0; i < len; i++)
		buf[i] = (buf[i] << 8) | (buf[i] >> 8);
#endif
	do {ret = i2c_master_send(ts->client, (char *)buf, (int)(len * 2));
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C TX fail (%d)", ret);
	CHECK((ret % 2) != 0, , -1, "I2C words TX fail: odd number of bytes "
		"(%d)", ret);

	if (DEBUGL(2)) {
		DEBUGHD(level, "(TX): ");
		if (len >= 1)
			DEBUGNF(bootloader ? 2 : 3, "%04X ", buf[0]);
		for (i = 1; i < len; i++)
			DEBUGNF(level, "%04X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret / 2;
}

static int read_mtp_report(struct data *ts, u16 *buf)
{
	int  words = 1, words_rx, i, ret = 0, recover = 0, remainder = 0;
	u16  address = 0x000A;

	mutex_lock(&ts->i2c_mutex);
	/* read header, get size, read entire report */
	for (i = 1; i <= 2; i++) {
		if (!ts->got_report) {
			words = i2c_tx_words(ts, &address, 1);
			CHECK(words != 1, mutex_unlock(&ts->i2c_mutex), -1,
				"Report RX fail: failed to set address");
			ts->got_report = 1;
		}

		words_rx = i2c_rx_words(ts, buf, words);
		if (words_rx != words || BYTEH(buf[0]) != 0x11 ||
			BYTEL(buf[0]) > MAX_WORDS_REPORT ||
			(i == 2 && buf[1] < 0x0100)) {
			if (recover == 0) {
				ts->got_report = 0;
				i = 0;
				recover = 1;
				continue;
			}
			ret = -1;
			ERROR("Report RX fail: received (%d) expected (%d) "
				"words, header (%04X)", words_rx, words,
				buf[0]);
			break;
		}
		words = BYTEL(buf[0]) + 1;
		if (words > CONFIG(i2c_words))
			remainder = words - CONFIG(i2c_words);
		if (remainder > 0)
			words = CONFIG(i2c_words);
		if (i == 2 && remainder > 0) {
			ts->got_report = 0;
			address += CONFIG(i2c_words);
			words = i2c_tx_words(ts, &address, 1);
			CHECK(words != 1, mutex_unlock(&ts->i2c_mutex), -1,
				"Report RX fail: failed to set address 0x%X",
				address);
			words_rx = i2c_rx_words(ts, &buf[CONFIG(i2c_words)],
						remainder);
			CHECK(words_rx != remainder,
				mutex_unlock(&ts->i2c_mutex),
				-1, "Report RX fail 0x%X: received (%d) "
				"expected (%d) words", address, words_rx,
				remainder);
		}
	}
	mutex_unlock(&ts->i2c_mutex);
	return ret;
}

static int send_mtp_command(struct data *ts, u16 *buf, u16 len)
{
	u16  tx_buf[MAX_WORDS_COMMAND + 2]; /* with address and header */
	u16  packets, words, words_tx, csum = 0;
	int  i, ret = 0;

	/* check basics */
	CHECKW(len < 2, , -1, "Command too short (%d); 2 words minimum", len);
	CHECKW((buf[1] + 2) != len, , -1, "Inconsistent command length: "
		"expected (%d) given (%d)", (buf[1] + 2), len);
	CHECKW(len > MAX_WORDS_COMMAND_ALL, , -1, "Command too long (%d); "
		"maximum (%d) words", len, MAX_WORDS_COMMAND_ALL);

	/* calculate checksum for certain commands */
	if (buf[0] == 0x0001 || buf[0] == 0x0010 || buf[0] == 0x0003 ||
		buf[0] == 0x0030) {
		for (i = (buf[0] == 0x0030) ? 3 : 2; i < (len - 1); i++)
			csum += buf[i];
		buf[len - 1] = csum;
	}

	/* packetize and send */
	packets = len / MAX_WORDS_COMMAND;
	if (len % MAX_WORDS_COMMAND)
		packets++;
	tx_buf[0] = 0x0000;

	mutex_lock(&ts->i2c_mutex);
	for (i = 0; i < packets; i++) {
		words = (i == (packets - 1)) ? len : MAX_WORDS_COMMAND;
		tx_buf[1] = (packets << 12) | ((i + 1) << 8) | words;
		memcpy(&tx_buf[2], &buf[i * MAX_WORDS_COMMAND],
			BYTE_SIZE(words));
		words_tx = i2c_tx_words(ts, tx_buf, words + 2);
		CHECKB(words_tx != (words + 2), ret = -1, "Command TX fail: "
			"transmitted (%d) expected (%d) words, packet (%d)",
			words_tx, words + 2, i);
		len -= MAX_WORDS_COMMAND;
	}
	ts->got_report = 0;
	mutex_unlock(&ts->i2c_mutex);
	return ret;
}

static int report_button_xy(const struct data *ts, u16 x, u16 y)
{
	u16  i;

	if (!ts->key_input_dev)
		return -1;

	for (i = 0; i < CONFIG(buttons); i++) {
		if (x >= (BUTTONXY(i, x) - BUTTONXY(i, size_x) / 2) &&
		    x <= (BUTTONXY(i, x) + BUTTONXY(i, size_x) / 2) &&
		    y >= (BUTTONXY(i, y) - BUTTONXY(i, size_y) / 2) &&
		    y <= (BUTTONXY(i, y) + BUTTONXY(i, size_y) / 2)) {
			input_report_key(ts->key_input_dev,
					CONFIG(button_code[i]), 1);
			input_sync(ts->key_input_dev);
			return CONFIG(button_code[i]);
		}
	}

	return -1;
}

static void process_touch_report(struct data *ts, u16 *buf)
{
	int touch_count = 0;
	int i = 0;
	int x = 0, y = 0, z = 0, finger_id = 0;
	int data_size, swap;
	int state[10] = {0};
	u16 frame_count, noise_level;

	if (!ts->input_dev)
		return;

	switch (buf[1]) {
	case 0x0801:
		data_size = 4;  break;
	case 0x0802:
		data_size = 12; break;
	default:
		return;
	}

	DEBUG_LOG(0, "Touch:");
		for (i = 0; i < (buf[0]&0x00FF); i++)
			DEBUG_LOG(0, " %04x", buf[i]);
	DEBUG_LOG(0, "\n");

	touch_count = buf[3] & 0x000F;
	CHECK(touch_count < 0 || touch_count > 10, , , "Touch count == %i, "
		"out of bounds [0,10]!", touch_count);

	frame_count = buf[5];
	CHECKW(frame_count == ts->frame_count, , ,
		"Same frame count %d, Drop!", frame_count);
	ts->frame_count = frame_count;

	noise_level = (BYTEL(buf[3]) >> 4) & 0x000F;
	if(noise_level != ts->noise_level) {
		PRINT("Noise level %d -> %d", ts->noise_level, noise_level);
		ts->noise_level = (u8)noise_level;
	}

	if (!DEBUGL(1))
		DEBUG(4, "(TOUCH): -------------------------------");
	if (touch_count == 0) {
		if (ts->buttondown != -1) {
			input_report_key(ts->key_input_dev, ts->buttondown,
					0);
			input_sync(ts->key_input_dev);
			ts->buttondown = -1;
		}
		ts->nobutton = 0;
		for(i=0;i<CONFIG(max_touches);i++) {
			if(ts->report_points[i].state==1 && state[i]==0) {
				if(CONFIG(input_protocol) == MAX11871_PROTOCOL_B) {
					input_mt_slot(ts->input_dev, i);
					input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
				}
				ts->report_points[i].state = 0;
				if (debug_mask & BIT(3)) {
					if(ts->width_factor && ts->height_factor) {
						DEBUG_LOG(3, "Screen:F[%02d]:Up, X=%d, Y=%d, Z=%d",
							i+1, (ts->report_points[i].x*ts->width_factor)>>SHIFT_BITS,
							(ts->report_points[i].y*ts->height_factor)>>SHIFT_BITS,
							ts->report_points[i].z);
					}
					else {
						DEBUG_LOG(3, "Raw:F[%02d]:Down, X=%d, Y=%d, Z=%d",
							i+1, ts->report_points[i].x, ts->report_points[i].y,
							ts->report_points[i].z);
					}
				}
			}
		}
		switch (CONFIG(input_protocol)) {
		case MAX11871_PROTOCOL_A:
		case MAX11871_PROTOCOL_A_TRACK:
			input_mt_sync(ts->input_dev);
		case MAX11871_PROTOCOL_B:
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_CUSTOM1:
#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
			input_report_abs(ts->input_dev, ABS_MT_AMPLITUDE, 0);
			input_report_abs(ts->input_dev, ABS_MT_POSITION,
					1 << 31);
#endif
			break;
		default:
			break;
		}
		DEBUG(4, "(TOUCH): Fingers up, Frame(%d) Noise(%d)",
			frame_count, noise_level);
		DEBUG_LOG(1, "Finger leave, Noise:%d", noise_level);
	} else {
		for (i = 0; i < touch_count; i++) {
			x = buf[6 + i * data_size + 1] & 0x0FFF;
			y = buf[6 + i * data_size + 2] & 0x0FFF;
			if (ts->fw_config->coordinate_settings & MAX11871_SWAP_XY) {
				swap = x;
				x = y;
				y = swap;
			}
			z = BYTEH(buf[6 + i * data_size + 3]);
			if (z == 0)
				z++;
			finger_id = buf[6 + i * data_size] & 0x000F;

			DEBUG(4, "(TOUCH): Finger %d: X(%.3d) Y(%.3d) "
				"Z(%.3d) Frame(%d) Noise(%d)",
				finger_id, x, y, z, frame_count, noise_level);
			DEBUG_LOG(1, "Finger %d=> X:%d, Y:%d, Z:%d, Noise:%d",
				finger_id+1, x, y, z, noise_level);

			if (ts->nobutton == 0 && ts->buttondown == -1)
				ts->buttondown = report_button_xy(ts, x, y);
			if (ts->buttondown == -1) {
				ts->nobutton = 1;
				switch (CONFIG(input_protocol)) {
				case MAX11871_PROTOCOL_A_TRACK:
					input_report_abs(ts->input_dev,
							ABS_MT_TRACKING_ID,
							finger_id);
				case MAX11871_PROTOCOL_A:
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_X, x);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_Y, y);
					input_report_abs(ts->input_dev,
#ifdef ABS_MT_PRESSURE
							ABS_MT_PRESSURE,
#else
							ABS_MT_TOUCH_MAJOR,
#endif
							z);
					input_mt_sync(ts->input_dev);
					break;
				case MAX11871_PROTOCOL_B:
					input_mt_slot(ts->input_dev, finger_id);
					input_mt_report_slot_state(ts->input_dev,
						MT_TOOL_FINGER, 1);
					input_report_abs(ts->input_dev,
#ifdef ABS_MT_PRESSURE
							ABS_MT_PRESSURE,
#else
							ABS_MT_TOUCH_MAJOR,
#endif
							z);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_X, x);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_Y, y);
					break;
				case MAX11871_PROTOCOL_CUSTOM1:
#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
					input_report_abs(ts->input_dev,
							ABS_MT_AMPLITUDE,
							(z << 16) | 0x0A);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION,
							((i == (touch_count -
								1)) << 31) |
							 (x << 16) | y);
#endif
					break;
				}
				ts->report_points[finger_id].x = x;
				ts->report_points[finger_id].y = y;
				ts->report_points[finger_id].z = z;
				state[finger_id] = 1;
			}
		}
		for(i=0;i<CONFIG(max_touches);i++) {
			if(ts->report_points[i].state==1 && state[i]==0) {
				if(CONFIG(input_protocol) == MAX11871_PROTOCOL_B) {
					input_mt_slot(ts->input_dev, i);
					input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
				}
				ts->report_points[i].state = 0;
				if (debug_mask & BIT(3)) {
					if(ts->width_factor && ts->height_factor) {
						DEBUG_LOG(3, "Screen:F[%02d]:Up, X=%d, Y=%d, Z=%d",
							i+1, (ts->report_points[i].x*ts->width_factor)>>SHIFT_BITS,
							(ts->report_points[i].y*ts->height_factor)>>SHIFT_BITS,
							ts->report_points[i].z);
					}
					else {
						DEBUG_LOG(3, "Raw:F[%02d]:Up, X=%d, Y=%d, Z=%d",
							i+1, ts->report_points[i].x, ts->report_points[i].y,
							ts->report_points[i].z);
					}
				}
			}
			else if(ts->report_points[i].state ==0 && state[i]==1) {
				ts->report_points[i].state = 1;
				if(ts->width_factor && ts->height_factor) {
					DEBUG_LOG(3, "Screen:F[%02d]:Down, X=%d, Y=%d, Z=%d",
						i+1, (ts->report_points[i].x*ts->width_factor)>>SHIFT_BITS,
						(ts->report_points[i].y*ts->height_factor)>>SHIFT_BITS,
						ts->report_points[i].z);
				}
				else {
					DEBUG_LOG(3, "Raw:F[%02d]:Down, X=%d, Y=%d, Z=%d",
						i+1, ts->report_points[i].x, ts->report_points[i].y,
						ts->report_points[i].z);
				}
			}
		}
		switch (CONFIG(input_protocol)) {
		case MAX11871_PROTOCOL_A:
		case MAX11871_PROTOCOL_A_TRACK:
		case MAX11871_PROTOCOL_B:
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_CUSTOM1:
			break;
		}
	}
}

static irqreturn_t irq_handler(int irq, void *context)
{
	struct data *ts = (struct data *)context;
	struct timespec timeStart, timeEnd, timeDelta;

	if (debug_mask & BIT(2)) {
		getnstimeofday(&timeStart);
	}

	if (read_mtp_report(ts, ts->rx_report) == 0) {
		process_touch_report(ts, ts->rx_report);
		if (debug_mask & BIT(2)) {
			getnstimeofday(&timeEnd);
			timeDelta.tv_nsec = (timeEnd.tv_sec*1000000000+timeEnd.tv_nsec)
				-(timeStart.tv_sec*1000000000+timeStart.tv_nsec);
			DEBUG_LOG(2, "Touch latency = %ld us", timeDelta.tv_nsec/1000);
		}
		propagate_report(ts, 0, ts->rx_report);
	}
	ts->irq_counter++;

	return IRQ_HANDLED;
}

static ssize_t init_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", init_state);
}

static ssize_t init_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int value, ret;

	CHECK(sscanf(buf, "%d", &value) != 1, , -EINVAL, "bad parameter");
	switch (value) {
	case 0:
		if (init_state == 0)
			break;
		ret = device_deinit(to_i2c_client(dev));
		CHECK(ret != 0, , ret, "deinit error (%d)", ret);
		break;
	case 1:
		if (init_state == 1)
			break;
		ret = device_init(to_i2c_client(dev));
		CHECK(ret != 0, , ret, "init error (%d)", ret);
		break;
	case 2:
		if (init_state == 1) {
			ret = device_deinit(to_i2c_client(dev));
			CHECK(ret != 0, , ret, "deinit error (%d)", ret);
		}
		ret = device_init(to_i2c_client(dev));
		CHECK(ret != 0, , ret, "init error (%d)", ret);
		break;
	default:
		ERROR("bad value");
		return -EINVAL;
	}

	return count;
}

static ssize_t hreset_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);
	struct max11871_pdata *pdata = client->dev.platform_data;

	if (!pdata->reset)
		return count;

	DISABLE_IRQ();
	mutex_lock(&ts->i2c_mutex);
	pdata->reset(pdata, 0);
	usleep_range(10000, 11000);
	pdata->reset(pdata, 1);
	bootloader = 0;
	ts->got_report = 0;
	mutex_unlock(&ts->i2c_mutex);
	CHECK(get_report(ts, 0x01A0, 3000) != 0, , count, "Failed to receive "
		"system status report");
	release_report(ts);

	return count;
}

static ssize_t sreset_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);

	DISABLE_IRQ();
	CHECK(sreset(client) != 0, , count, "Failed to do soft reset.");
	CHECK(get_report(ts, 0x01A0, 3000) != 0, , count, "Failed to receive "
		"system status report");
	release_report(ts);
	return count;
}

static ssize_t irq_count_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u\n", ts->irq_counter);
}

static ssize_t irq_count_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);

	ts->irq_counter = 0;
	return count;
}

static ssize_t dflt_cfg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u %u %u\n", CONFIG(defaults_allow),
			CONFIG(default_chip_config), CONFIG(default_chip_id));
}

static ssize_t dflt_cfg_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	(void)sscanf(buf, "%hu %hu %hu", &CONFIG(defaults_allow),
			&CONFIG(default_chip_config),
			&CONFIG(default_chip_id));
	return count;
}

static ssize_t panel_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u %u %u %u\n",
			COORDINATES(panel_min_x), COORDINATES(panel_max_x),
			COORDINATES(panel_min_y), COORDINATES(panel_max_y));
}

static ssize_t panel_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	(void)sscanf(buf, "%hu %hu %hu %hu",
			&COORDINATES(panel_min_x), &COORDINATES(panel_max_x),
			&COORDINATES(panel_min_y), &COORDINATES(panel_max_y));
	return count;
}

static ssize_t buttons_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u %u %u\n", CONFIG(buttons_enabled),
			CONFIG(buttons_type), CONFIG(buttons));
}

static ssize_t buttons_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	(void)sscanf(buf, "%hu %hu %hu", &CONFIG(buttons_enabled),
			&CONFIG(buttons_type), &CONFIG(buttons));
	CONFIG(buttons_enabled) = !!CONFIG(buttons_enabled);
	if (CONFIG(buttons_type) != MAX11871_BUTTONS_XY &&
	    CONFIG(buttons_type) != MAX11871_BUTTONS_SENSE)
		CONFIG(buttons_type) = MAX11871_BUTTONS_XY;
	if (CONFIG(buttons) > MAX11871_MAX_BUTTONS)
		CONFIG(buttons) = MAX11871_MAX_BUTTONS;
	return count;
}

static ssize_t button_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);
	u16                index;

	sscanf(attr->attr.name, "button%hu", &index);
	index--;
	if (CONFIG(buttons_type) == MAX11871_BUTTONS_SENSE)
		return snprintf(buf, PAGE_SIZE, "%u\n",
				CONFIG(button_code[index]));
	else
		return snprintf(buf, PAGE_SIZE, "%u %u %u %u %u\n",
				BUTTONXY(index, x), BUTTONXY(index, y),
				BUTTONXY(index, size_x),
				BUTTONXY(index, size_y),
				CONFIG(button_code[index]));
}

static ssize_t button_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);
	u16                index;

	sscanf(attr->attr.name, "button%hu", &index);
	index--;
	if (CONFIG(buttons_type) == MAX11871_BUTTONS_SENSE)
		(void)sscanf(buf, "%u", &CONFIG(button_code[index]));
	else
		(void)sscanf(buf, "%hu %hu %hu %hu %u",
				&BUTTONXY(index, x), &BUTTONXY(index, y),
				&BUTTONXY(index, size_x),
				&BUTTONXY(index, size_y),
				&CONFIG(button_code[index]));
	return count;
}

static ssize_t fw_ver_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);
	u16 build_number = 0;
	u8 branch = BYTEL(ts->fw_version[3]) >> 6;

	if (ts->fw_version[1] >= 3)
		build_number = ts->fw_version[4];
	return snprintf(buf, PAGE_SIZE, "%u.%u.%u p%u%c "
		"(CRC16 0x%04X=>0x%04X) Chip ID 0x%02X\n",
		BYTEH(ts->fw_version[2]), BYTEL(ts->fw_version[2]),
		build_number, BYTEL(ts->fw_version[3]) & 0x3F,
		(branch == 0) ? ' ' : (branch - 1 + 'a'), (ts->fw_index != -1)
			? CONFIG(fw_image[ts->fw_index]).config_boundary : 0,
		ts->fw_crc16, BYTEH(ts->fw_version[3]));
}

static ssize_t driver_ver_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "3.0.2: June 18, 2012\n");
}

static ssize_t debug_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%08X\n", debug_mask);
}

static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	CHECK(sscanf(buf, "%ix", &debug_mask) != 1, , -EINVAL,
		"bad parameter");
	return count;
}

static ssize_t command_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);
	u16 buffer[MAX_WORDS_COMMAND_ALL];
	char scan_buf[5];
	int i;

	count--;  /* ignore carriage return */
	CHECKW((count % 4) != 0, , -EINVAL, "words not properly defined");
	scan_buf[4] = '\0';
	for (i = 0; i < count; i += 4) {
		memcpy(scan_buf, &buf[i], 4);
		CHECK(sscanf(scan_buf, "%hx", &buffer[i / 4]) != 1, , -EINVAL,
			"bad word (%s)", scan_buf);
	}
	CHECKWI(send_mtp_command(ts, buffer, count / 4), "MTP command failed");
	return ++count;
}

static ssize_t report_read(struct file *file, struct kobject *kobj,
			struct bin_attribute *attr, char *buf, loff_t off,
			size_t count)
{
	struct i2c_client *client = kobj_to_i2c_client(kobj);
	struct data *ts = i2c_get_clientdata(client);
	int printed, i, offset = 0, payload;

	if (get_report(ts, 0xFFFF, 0xFFFFFFFF))
		return 0;

	payload = BYTEL(ts->report[0]);
	if (count < (4 * payload + 1))
		return -EIO;
	if (count > (4 * payload + 1))
		count = 4 * payload + 1;

	for (i = 1; i <= payload; i++) {
		printed = snprintf(&buf[offset], PAGE_SIZE, "%04X\n",
							ts->report[i]);
		if (printed <= 0)
			return -EIO;
		offset += printed - 1;
	}
	snprintf(&buf[offset], PAGE_SIZE, "\n");
	release_report(ts);

	return count;
}

static ssize_t touch_vendor_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct data *ts = gl_ts;

	return snprintf(buf, PAGE_SIZE, "Maxim-%s_p%u_chipID-0x%02X_twID-%02X\n",
		ts->fw_ver, ts->protocol_ver, BYTEH(ts->fw_version[3]), ts->vendor_pin);
}

static ssize_t config_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct data *ts = gl_ts;
	int i, ret;
	size_t count = 0;
	u16 mtpdata[]={0x0000, 0x0000, 0x0000};

	for(i=0; i<RETRY_TIMES; i++) {
		DISABLE_IRQ();
		ret = get_touch_config(ts->client);
		CHECKI(ret < 0, "Failed to retrieve touch config");
		if (ret == 0) {
			ret = get_report(ts, 0x0102, 150);
			CHECKWI(ret != 0, "Get touch config time out-%d, retry", i);
			if (ret == 0) {
				count += snprintf(buf + count, PAGE_SIZE, "Touch config:\n");
				for (i = 3; i < 45; i++) {
					count += snprintf(buf + count, PAGE_SIZE, "%04X ", ts->report[i]);
					if (((i-3) % 16) == (16 - 1))
						count += snprintf(buf + count, PAGE_SIZE, "\n");
				}
				count += snprintf(buf + count, PAGE_SIZE, "\n");
				release_report(ts);
				break;
			}
		}
	}
	CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive touch config report");

	for(i=0; i<RETRY_TIMES; i++) {
		DISABLE_IRQ();
		//Get calibration table
		mtpdata[0]=0x0011;
		mtpdata[1]=0x0000;
		ret = send_mtp_command(ts, mtpdata, 2);
		CHECKI(ret < 0, "Failed to retrieve calibration table");
		if (ret == 0) {
			ret = get_report(ts, 0x0111, 150);
			CHECKWI(ret != 0, "Get calibration table time out-%d, retry", i);
			if (ret == 0) {
				count += snprintf(buf + count, PAGE_SIZE, "Calibration Table:\n");
				for (i = 3; i < 53; i++) {
					count += snprintf(buf + count, PAGE_SIZE, "%04X ", ts->report[i]);
					if (((i-3) % 16) == (16 - 1))
						count += snprintf(buf + count, PAGE_SIZE, "\n");
				}
				count += snprintf(buf + count, PAGE_SIZE, "\n");
				release_report(ts);
				break;
			}
		}
	}
	CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive calibration table report");

	for(i=0; i<RETRY_TIMES; i++) {
		DISABLE_IRQ();
		//Get private configuration
		mtpdata[0]=0x0004;
		mtpdata[1]=0x0000;
		ret = send_mtp_command(ts, mtpdata, 2);
		CHECKI(ret < 0, "Failed to retrieve private config");
		if (ret == 0) {
			ret = get_report(ts, 0x0104, 150);
			CHECKWI(ret != 0, "Get private config time out-%d, retry", i);
			if (ret == 0) {
				count += snprintf(buf + count, PAGE_SIZE, "Private Config:\n");
				for (i = 3; i < 26; i++) {
					count += snprintf(buf + count, PAGE_SIZE, "%04X ", ts->report[i]);
					if (((i-3) % 16) == (16 - 1))
						count += snprintf(buf + count, PAGE_SIZE, "\n");
				}
				count += snprintf(buf + count, PAGE_SIZE, "\n");
				release_report(ts);
				break;
			}
		}
	}
	CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive private config report");

	for(i=0; i<RETRY_TIMES; i++) {
		DISABLE_IRQ();
		//Get Lookup table X
		mtpdata[0]=0x0031;
		mtpdata[1]=0x0001;
		mtpdata[2]=0x0000;
		ret = send_mtp_command(ts, mtpdata, 3);
		CHECKI(ret < 0, "Failed to retrieve Lookup table X");
		if (ret == 0) {
			ret = get_report(ts, 0x0131, 150);
			CHECKWI(ret != 0, "Get Lookup table X time out-%d, retry", i);
			if (ret == 0) {
				count += snprintf(buf + count, PAGE_SIZE, "Lookup Table X:\n");
				for (i = 3; i < 11; i++) {
					count += snprintf(buf + count, PAGE_SIZE, "%04X ", ts->report[i]);
					if (((i-3) % 16) == (16 - 1))
						count += snprintf(buf + count, PAGE_SIZE, "\n");
				}
				count += snprintf(buf + count, PAGE_SIZE, "\n");
				release_report(ts);
				break;
			}
		}
	}
	CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive Lookup table X report");

	for(i=0; i<RETRY_TIMES; i++) {
		DISABLE_IRQ();
		//Get Lookup table Y
		mtpdata[0]=0x0031;
		mtpdata[1]=0x0001;
		mtpdata[2]=0x0001;
		ret = send_mtp_command(ts, mtpdata, 3);
		CHECKI(ret < 0, "Failed to retrieve Lookup table Y");
		if (ret == 0) {
			ret = get_report(ts, 0x0131, 150);
			CHECKWI(ret != 0, "Get Lookup table Y time out-%d, retry", i);
			if (ret == 0) {
				count += snprintf(buf + count, PAGE_SIZE, "Lookup Table Y:\n");
				for (i = 3; i < 11; i++) {
					count += snprintf(buf + count, PAGE_SIZE, "%04X ", ts->report[i]);
					if (((i-3) % 16) == (16 - 1))
						count += snprintf(buf + count, PAGE_SIZE, "\n");
				}
				count += snprintf(buf + count, PAGE_SIZE, "\n");
				release_report(ts);
				break;
			}
		}
	}
	CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive Lookup table Y report");

	return count;
}

static ssize_t diag_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct data *ts = gl_ts;
	size_t count = 0;
	uint16_t i, j;
	int ret;
	int16_t report[MAX_WORDS_REPORT + 1];

	if(ts->baseline_mode != MAX11871_AUTO_BASELINE)
		CHECK(set_baseline_mode(ts->client, ts->baseline_mode) < 0, , -1,
			"Failed to set up baseline mode");
	CHECK(set_touch_frame(ts->client, ts->frame_rate[1], 0x0A) < 0, , -1,
		"Failed to set up frame rate");

	DISABLE_IRQ();
	CHECK(change_touch_rpt(ts->client, 0) < 0, , -1,
		"Failed to set up raw data report");
	ret = get_report(ts, 0x0800, 500);
	CHECKI(ret != 0, "Failed to receive raw data report");
	memcpy(report, &ts->report[5], BYTE_SIZE(ts->report[2] - 2));

	if(ret==0) {
		count += sprintf(buf + count, "Channel: %dx%d\n", ts->x_channel, ts->y_channel);
		for (i = 0; i < ts->y_channel; i++) {
			for (j = 0; j < ts->x_channel; j++) {
				count += sprintf(buf + count, "%6d", report[i*ts->x_channel + j]);
			}
			count += sprintf(buf + count, "\n");
		}
		release_report(ts);
	}

	DISABLE_IRQ();
	CHECK(change_touch_rpt(ts->client, 1) < 0, , -1,
		"Failed to set up raw data report");
	CHECK(set_touch_frame(ts->client, ts->frame_rate[1], ts->frame_rate[0]) < 0, , -1,
		"Failed to set up frame rate");
	if(ts->baseline_mode != MAX11871_AUTO_BASELINE)
		CHECK(set_baseline_mode(ts->client, 2) < 0, , -1,
			"Failed to set up baseline mode");
	ENABLE_IRQ();

	return count;
}

static ssize_t diag_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct data *ts = gl_ts;
	if (buf[0] == '1')
		ts->baseline_mode = MAX11871_AUTO_BASELINE;
	else if (buf[0] == '2')
		ts->baseline_mode = MAX11871_NO_BASELINE;
	else if (buf[0] == '3')
		ts->baseline_mode = MAX11871_FIX_BASELINE;

	return count;
}

static ssize_t gpio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct data *ts = gl_ts;
	struct max11871_pdata *pdata = ts->client->dev.platform_data;

	if (!pdata->tirq)
		return ret;

	ret = pdata->tirq(pdata);
	printk(KERN_DEBUG "[TP] GPIO_TP_INT_N=%d\n", ret);
	sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(init,         (S_IWUSR|S_IRUGO), init_show,         init_store);
static DEVICE_ATTR(hreset,       S_IWUSR,           NULL,              hreset_store);
static DEVICE_ATTR(sreset,       S_IWUSR,           NULL,              sreset_store);
static DEVICE_ATTR(irq_count,    (S_IWUSR|S_IRUGO), irq_count_show,    irq_count_store);
static DEVICE_ATTR(dflt_cfg,     (S_IWUSR|S_IRUGO), dflt_cfg_show,     dflt_cfg_store);
static DEVICE_ATTR(panel,        (S_IWUSR|S_IRUGO), panel_show,        panel_store);
static DEVICE_ATTR(buttons,      (S_IWUSR|S_IRUGO), buttons_show,      buttons_store);
static DEVICE_ATTR(fw_ver,       S_IRUGO,           fw_ver_show,       NULL);
static DEVICE_ATTR(driver_ver,   S_IRUGO,           driver_ver_show,   NULL);
static DEVICE_ATTR(debug,        (S_IWUSR|S_IRUGO), debug_show,        debug_store);
static DEVICE_ATTR(command,      S_IWUSR,           NULL,              command_store);
static struct bin_attribute dev_attr_report = {
	.attr = {.name = "report", .mode = S_IRUGO}, .read = report_read};
static struct device_attribute dev_attr_button[MAX11871_MAX_BUTTONS];
static char                    dev_attr_button_name[MAX11871_MAX_BUTTONS][10];

static struct device_attribute  *dev_attrs[] = {
	&dev_attr_hreset,
	&dev_attr_sreset,
	&dev_attr_irq_count,
	&dev_attr_dflt_cfg,
	&dev_attr_panel,
	&dev_attr_buttons,
	&dev_attr_fw_ver,
	&dev_attr_driver_ver,
	&dev_attr_debug,
	&dev_attr_command,
	NULL
};

static DEVICE_ATTR(debug_level, (S_IWUSR|S_IRUGO), debug_show, debug_store);
static DEVICE_ATTR(vendor, S_IRUGO, touch_vendor_show, NULL);
static DEVICE_ATTR(config, S_IRUGO, config_show, NULL);
static DEVICE_ATTR(diag, (S_IWUSR|S_IRUGO), diag_show, diag_store);
static DEVICE_ATTR(gpio, S_IRUGO, gpio_show, NULL);

static void collect_chip_data(struct data *ts)
{
	int  ret, i, build_number = 0;

	ret = get_report(ts, 0x01A0, 3000);
	if (ret != 0) {
		ERROR("Failed to receive system status report");
		if (CONFIG(defaults_allow) == 0)
			msleep(5000);
	} else {
		ts->vendor_pin = BYTEH(ts->report[3]) & CONFIG(tw_mask);
		DEBUG(5, "(INIT): vendor_pin=%x", ts->vendor_pin);
		release_report(ts);
		ts->fw_responsive = 1;
	}
	for(i=0;i<RETRY_TIMES;i++) {
		DISABLE_IRQ();
		DEBUG(5, "(INIT): collect_chip_data: Get FW version");
		ret = get_fw_version(ts->client);
		CHECKI(ret < 0, "Failed to retrieve firmware version");
		if (ret == 0) {
			ret = get_report(ts, 0x0140, 150);
			CHECKWI(ret != 0, "Get firmware version time out-%d, retry", i);
			if (ret == 0) {
				memcpy(ts->fw_version, &ts->report[1],
					BYTE_SIZE(ts->report[2] + 2));
				release_report(ts);
				ts->have_fw = 1;
				break;
			}
		}
	}
	CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive firmware version report");
	for(i=0;i<RETRY_TIMES;i++) {
		DISABLE_IRQ();
		DEBUG(5, "(INIT): collect_chip_data: Get touch config");
		ret = get_touch_config(ts->client);
		CHECKI(ret < 0, "Failed to retrieve touch config");
		if (ret == 0) {
			ret = get_report(ts, 0x0102, 150);
			CHECKWI(ret != 0, "Get touch config time out-%d, retry", i);
			if (ret == 0) {
				memcpy(ts->touch_config, &ts->report[1],
					BYTE_SIZE(ts->report[2] + 2));
				release_report(ts);
				ts->have_touchcfg = 1;
				break;
			}
		}
	}
	CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive touch config report");
	ENABLE_IRQ();
	DEBUG(5, "(INIT): firmware responsive: (%u)", ts->fw_responsive);
	if (ts->fw_responsive) {
		if (ts->have_fw) {
			if (ts->fw_version[1] >= 3)
				build_number = ts->fw_version[4];
			sprintf(ts->fw_ver, "%u.%u.%u", BYTEH(ts->fw_version[2]),
						BYTEL(ts->fw_version[2]), build_number);
			ts->protocol_ver = BYTEL(ts->fw_version[3]) & 0x3F;
			DEBUG(5, "(INIT): firmware version: %u.%u.%u_p%u Chip ID: "
				"0x%02X", BYTEH(ts->fw_version[2]),
				BYTEL(ts->fw_version[2]),
				build_number,
				BYTEL(ts->fw_version[3]) & 0x3F,
				BYTEH(ts->fw_version[3]));
		}
		else
			sprintf(ts->fw_ver, "Bootloader");
		if (ts->have_touchcfg) {
			DEBUG(5, "(INIT): configuration ID: 0x%04X",
				ts->touch_config[2]);
			ts->x_channel = BYTEH(ts->touch_config[3]);
			ts->y_channel = BYTEL(ts->touch_config[3]);
			DEBUG(5, "(INIT): Channel=(%d,%d)", ts->x_channel, ts->y_channel);
			ts->frame_rate[0] = ts->touch_config[4];
			ts->frame_rate[1] = ts->touch_config[5];
			DEBUG(5, "(INIT): Frame Rate=(%d,%d)", ts->frame_rate[0], ts->frame_rate[1]);
		}
	}
	else
		sprintf(ts->fw_ver, "Failed");
}

static int device_fw_load(struct data *ts, const struct firmware *fw,
			u16 fw_index, int tagLen)
{
	u16 fw_crc16, chip_crc16;
	int retry=0, ret, retry_times=5;

	fw_crc16 = crc16(0, fw->data+tagLen,
			CONFIG(fw_image[fw_index]).config_boundary);
	DEBUG(5, "(INIT): firmware size (%d) CRC16(0x%04X)", fw->size-tagLen,
		fw_crc16);
	ret = bootloader_enter(ts);
	CHECKI(ret!=0, "Failed to enter bootloader");
	if (ret==0) {
		ret = bootloader_get_crc(ts, &chip_crc16,
			CONFIG(fw_image[fw_index]).config_boundary);
		CHECKI(ret!=0, "Failed to get CRC16 from the chip");
		DEBUG(5, "(INIT): chip CRC16(0x%04X)", chip_crc16);
		ts->fw_index = fw_index;
		ts->fw_crc16 = chip_crc16;
		CHECKI(bootloader_exit(ts), "Failed to exit bootloader");
	}

	if (fw_crc16 != chip_crc16) {
		for (retry=0; retry<retry_times; retry++) {
			ret = bootloader_enter(ts);
			CHECKI(ret!=0, "Failed to enter bootloader");
			if (ret==0) {
				DEBUG(5, "(INIT): will reprogram chip");
				ret = bootloader_erase_flash(ts);
				CHECKI(ret!=0, "Failed to erase chip flash");
			}
			if (ret==0) {
				DEBUG(5, "(INIT): flash erase OK");
				ret = bootloader_set_byte_mode(ts);
				CHECKI(ret!=0, "Failed to set byte mode");
			}
			if (ret==0) {
				DEBUG(5, "(INIT): byte mode OK");
				ret = bootloader_write_flash(ts, (u8 *)fw->data+tagLen);
				CHECKI(ret!=0, "Failed to write flash");
			}
			if (ret==0) {
				DEBUG(5, "(INIT): flash write OK");
				fw_crc16 = crc16(0, fw->data+tagLen,
						CONFIG(fw_image[fw_index]).length);
				ret = bootloader_get_crc(ts, &chip_crc16,
					CONFIG(fw_image[fw_index]).length);
				CHECKI(ret!=0, "Failed to get CRC16 from the chip");
				if (fw_crc16 != chip_crc16) {
					ERROR("Failed to verify programming! (0x%04X)", chip_crc16);
					ret = -1;
				} else {
					DEBUG(5, "(INIT): chip programmed successfully");
					CHECKI(bootloader_get_crc(ts, &chip_crc16,
						CONFIG(fw_image[fw_index]).config_boundary),
						"Failed to get CRC16 from the chip");
					DEBUG(5, "(INIT): new chip CRC16(0x%04X)", chip_crc16);
					ts->fw_crc16 = chip_crc16;
					ret = 0;
				}
			}
			CHECKI(bootloader_exit(ts), "Failed to exit bootloader");
			if (ret==0)
				break;
		}
	}

	CHECK(retry==retry_times, , -1, "Failed to update firmware");
	/* wait for chip to send proof of life */
	collect_chip_data(ts);
	CHECK(ts->have_fw == 0 || ts->have_touchcfg == 0, , -1, "firmware is "
		"unresponsive or inconsistent and no valid configuration is "
		"present");

	return 0;
}

static int is_booting(void)
{
	unsigned long long t;
	unsigned long nanosec_rem;

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	return (t < 30) ? 1 : 0;
}

static int compare_u16_arrays(u16 *buf1, u16 *buf2, u16 n)
{
	int i;
	for (i=0; i<n;i++)
	{
		if (buf1[i]!=buf2[i]) return -1;
	}
	return 0;
}

u16 calculate_checksum(u16 *buf, u16 n)
{
	u16 i, cs=0;
	for (i=0;i<n;i++)
	{
		cs += buf[i];
	}
	return cs;
}

static void update_config(struct data *ts)
{
	int i, ret;
	u16 reload_touch_config=0, reload_calib_table=0, reload_private_config=0;
	u16 reload_lookup_x=0, reload_lookup_y=0, reload_imagefactor_table=0;
	u16 mtpdata[]={0x0000, 0x0000, 0x0000};
	u16 imagefactor_data[104];

	/* configure the chip */
	if (ts->max11871_Touch_Configuration_Data) {
		for (i=0; i<RETRY_TIMES; i++) {
			DISABLE_IRQ();
			ret = get_touch_config(ts->client);
			CHECKI(ret < 0, "Failed to retrieve touch config");
			if (ret == 0) {
				ret = get_report(ts, 0x0102, 150);
				CHECKWI(ret != 0, "Get touch config time out-%d, retry", i);
				if (ret == 0) {
					if (compare_u16_arrays(&ts->report[2],
						&ts->max11871_Touch_Configuration_Data[1], 43)!=0) {
						DEBUG(5, "(Config): Touch Configuration Data mismatch");
						reload_touch_config = 1;
					} else {
						DEBUG(5, "(Config): Touch Configuration Data okay");
					}
					release_report(ts);
					break;
				}
			}
		}
		CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive touch config report");
	}

	if (ts->max11871_Calibration_Table_Data) {
		for (i=0; i<RETRY_TIMES; i++) {
			DISABLE_IRQ();
			//Get calibration table
			mtpdata[0]=0x0011;
			mtpdata[1]=0x0000;
			ret = send_mtp_command(ts, mtpdata, 2);
			CHECKI(ret < 0, "Failed to retrieve calibration table");
			if (ret == 0) {
				ret = get_report(ts, 0x0111, 150);
				CHECKWI(ret != 0, "Get calibration table time out-%d, retry", i);
				if (ret == 0) {
					if (compare_u16_arrays(&ts->report[2],
						&ts->max11871_Calibration_Table_Data[1], 51)!=0) {
						DEBUG(5, "(Config): Calibration Table Data mismatch");
						reload_calib_table = 1;
					} else {
						DEBUG(5, "(Config): Calibration Table Data okay");
					}
					release_report(ts);
					break;
				}
			}
		}
		CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive calibration table report");
	}

	if (ts->max11871_Private_Configuration_Data) {
		for (i=0; i<RETRY_TIMES; i++) {
			DISABLE_IRQ();
			//Get private configuration
			mtpdata[0]=0x0004;
			mtpdata[1]=0x0000;
			ret = send_mtp_command(ts, mtpdata, 2);
			CHECKI(ret < 0, "Failed to retrieve private config");
			if (ret == 0) {
				ret = get_report(ts, 0x0104, 150);
				CHECKWI(ret != 0, "Get private config time out-%d, retry", i);
				if (ret == 0) {
					if (compare_u16_arrays(&ts->report[2],
						&ts->max11871_Private_Configuration_Data[1], 24)!=0) {
						DEBUG(5, "(Config): Private Configuration Data mismatch");
						reload_private_config = 1;
					} else {
						DEBUG(5, "(Config): Private Configuration Data okay");
					}
					release_report(ts);
					break;
				}
			}
		}
		CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive private config report");
	}

	if (ts->max11871_Lookup_Table_X_Data) {
		for (i=0; i<RETRY_TIMES; i++) {
			DISABLE_IRQ();
			//Get Lookup table X
			mtpdata[0]=0x0031;
			mtpdata[1]=0x0001;
			mtpdata[2]=0x0000;
			ret = send_mtp_command(ts, mtpdata, 3);
			CHECKI(ret < 0, "Failed to retrieve Lookup table X");
			if (ret == 0) {
				ret = get_report(ts, 0x0131, 150);
				CHECKWI(ret != 0, "Get Lookup table X time out-%d, retry", i);
				if (ret == 0) {
					if (compare_u16_arrays(&ts->report[3],
						&ts->max11871_Lookup_Table_X_Data[3], 8)!=0) {
						DEBUG(5, "(Config): Lookup Table X Data mismatch");
						reload_lookup_x = 1;
					} else {
						DEBUG(5, "(Config): Lookup Table X Data okay");
					}
					release_report(ts);
					break;
				}
			}
		}
		CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive Lookup table X report");
	}

	if (ts->max11871_Lookup_Table_Y_Data) {
		for (i=0; i<RETRY_TIMES; i++) {
			DISABLE_IRQ();
			//Get Lookup table Y
			mtpdata[0]=0x0031;
			mtpdata[1]=0x0001;
			mtpdata[2]=0x0001;
			ret = send_mtp_command(ts, mtpdata, 3);
			CHECKI(ret < 0, "Failed to retrieve Lookup table Y");
			if (ret == 0) {
				ret = get_report(ts, 0x0131, 150);
				CHECKWI(ret != 0, "Get Lookup table Y time out-%d, retry", i);
				if (ret == 0) {
					if (compare_u16_arrays(&ts->report[3],
						&ts->max11871_Lookup_Table_Y_Data[3], 8)!=0) {
						DEBUG(5, "(Config): Lookup Table Y Data mismatch");
						reload_lookup_y = 1;
					} else {
						DEBUG(5, "(Config): Lookup Table Y Data okay");
					}
					release_report(ts);
					break;
				}
			}
		}
		CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive Lookup table Y report");
	}

	if (ts->max11871_Image_Factor_Table) {
		for (i=0; i<RETRY_TIMES; i++) {
			DISABLE_IRQ();
			//Get Image Factor Table
			mtpdata[0]=0x0047;
			mtpdata[1]=0x0000;
			ret = send_mtp_command(ts, mtpdata, 2);
			CHECKI(ret < 0, "Failed to retrieve Image Factor Table");
			if (ret == 0) {
				ret = get_report(ts, 0x0147, 150);
				CHECKWI(ret != 0, "Get Image Factor Table time out-%d, retry", i);
				if (ret == 0) {
					if (ts->report[3] !=
						calculate_checksum(ts->max11871_Image_Factor_Table, 460)) {
						DEBUG(5, "(Config): Image Factor Table mismatch");
						reload_imagefactor_table = 1;
					} else {
						DEBUG(5, "(Config): Image Factor Table okay");
					}
					release_report(ts);
					break;
				}
			}
		}
		CHECKI(i==RETRY_TIMES && ret!=0, "Failed to receive Image Factor Table report");
	}

	//Configuration check has been done
	//Now download correct configurations if required
	if (reload_touch_config) {
		DEBUG(5, "(Config): Update Configuration Table");
		DISABLE_IRQ();
		ret = send_mtp_command(ts, ts->max11871_Touch_Configuration_Data, 44);
		CHECKI(ret < 0, "Failed to send Touch Config");
		msleep(100);
		ENABLE_IRQ();
	}
	if (reload_calib_table) {
		DEBUG(5, "(Config): Update Calibration Table");
		DISABLE_IRQ();
		ret = send_mtp_command(ts, ts->max11871_Calibration_Table_Data, 52);
		CHECKI(ret < 0, "Failed to send Calib Table");
		msleep(100);
		ENABLE_IRQ();
	}
	if (reload_private_config) {
		DEBUG(5, "(Config): Update Private Configuration Table");
		DISABLE_IRQ();
		ret = send_mtp_command(ts, ts->max11871_Private_Configuration_Data, 25);
		CHECKI(ret < 0, "Failed to send Private Config");
		msleep(100);
		ENABLE_IRQ();
	}
	if (reload_lookup_x) {
		DEBUG(5, "(Config): Update Lookup Table X");
		DISABLE_IRQ();
		ret = send_mtp_command(ts, ts->max11871_Lookup_Table_X_Data, 11);
		CHECKI(ret < 0, "Failed to send Lookup Table X");
		msleep(100);
		ENABLE_IRQ();
	}
	if (reload_lookup_y) {
		DEBUG(5, "(Config): Update Lookup Table Y");
		DISABLE_IRQ();
		ret = send_mtp_command(ts, ts->max11871_Lookup_Table_Y_Data, 11);
		CHECKI(ret < 0, "Failed to send Lookup Table Y");
		msleep(100);
		ENABLE_IRQ();
	}
	if (reload_imagefactor_table) {
		DEBUG(5, "(Config): Update Image Factor Table");
		DISABLE_IRQ();
		//0-59 words
		imagefactor_data[0] = 0x0046;
		imagefactor_data[1] = 0x003E;
		imagefactor_data[2] = 0x0000;
		memcpy(imagefactor_data+3, ts->max11871_Image_Factor_Table, 60 << 1);
		imagefactor_data[63] = calculate_checksum(imagefactor_data+2,61);
		send_mtp_command(ts, imagefactor_data, 64);
		msleep(100);
		//60-159 words
		imagefactor_data[0] = 0x0046;
		imagefactor_data[1] = 0x0066;
		imagefactor_data[2] = 0x003C;
		memcpy(imagefactor_data+3, ts->max11871_Image_Factor_Table+60, 100 << 1);
		imagefactor_data[103] = calculate_checksum(imagefactor_data+2,101);
		send_mtp_command(ts, imagefactor_data, 104);
		msleep(100);
		//160-259 words
		imagefactor_data[0] = 0x0046;
		imagefactor_data[1] = 0x0066;
		imagefactor_data[2] = 0x00A0;
		memcpy(imagefactor_data+3, ts->max11871_Image_Factor_Table+160, 100 << 1);
		imagefactor_data[103] = calculate_checksum(imagefactor_data+2,101);
		send_mtp_command(ts, imagefactor_data, 104);
		msleep(100);
		//260-359 words
		imagefactor_data[0] = 0x0046;
		imagefactor_data[1] = 0x0066;
		imagefactor_data[2] = 0x0104;
		memcpy(imagefactor_data+3, ts->max11871_Image_Factor_Table+260, 100 << 1);
		imagefactor_data[103] = calculate_checksum(imagefactor_data+2,101);
		send_mtp_command(ts, imagefactor_data, 104);
		msleep(100);
		//360-459 words
		imagefactor_data[0] = 0x0046;
		imagefactor_data[1] = 0x0066;
		imagefactor_data[2] = 0x8168;
		memcpy(imagefactor_data+3, ts->max11871_Image_Factor_Table+360, 100 << 1);
		imagefactor_data[103] = calculate_checksum(imagefactor_data+2,101);
		send_mtp_command(ts, imagefactor_data, 104);
		msleep(100);
		ENABLE_IRQ();
	}
	if (reload_touch_config || reload_calib_table || reload_private_config ||
		reload_lookup_x || reload_lookup_y || reload_imagefactor_table) {
		DISABLE_IRQ();
		CHECK(sreset(ts->client) != 0, , , "Failed to do soft reset.");
		collect_chip_data(ts);
		CHECK(ts->have_fw == 0 || ts->have_touchcfg == 0, , , "firmware is "
		"unresponsive or inconsistent and no valid configuration is "
		"present");
		DEBUG(5, "(INIT): Update config complete");
	}
}

static int check_bin_version(const struct firmware *fw, int *tagLen, char *fw_ver)
{
	char tag[40];
	int i = 0;

	if (fw->data[0] == 'T' && fw->data[1] == 'P') {
		while ((tag[i] = fw->data[i]) != '\n')
			i++;
		tag[i] = '\0';
		*tagLen = i+1;
		DEBUG(5, "(INIT): tag=%s", tag);
		if (strstr(tag, fw_ver) != NULL) {
			DEBUG(5, "(INIT): Update Bypass");
			return 0; /* bypass */
		}
	}

	DEBUG(5, "(INIT): Need Update");
	return 1;
}

static void check_fw_and_config(struct data *ts, u16 request_slept)
{
	u16 config_id, chip_id;
	int i, j, ret;
	const struct firmware *fw;
	int tagLen = 0;

	DEBUG(5, "(INIT): Chip Reset");
	sreset(ts->client);
	collect_chip_data(ts);
	if (ts->fw_responsive == 0 || ts->have_fw == 0 || ts->have_touchcfg == 0) {
		ERROR("firmware is unresponsive.");
		CONFIG(update_feature) |= MAX11871_UPDATE_BIN;
	}
	/*CHECK((ts->have_fw == 0 || ts->have_touchcfg == 0) &&
		CONFIG(defaults_allow) == 0, , , "firmware is unresponsive "
		"or inconsistent and default selections are disabled");*/
	config_id = ts->have_touchcfg ? ts->touch_config[2] :
						CONFIG(default_chip_config);
	chip_id = ts->have_fw ? BYTEH(ts->fw_version[3]) :
						CONFIG(default_chip_id);

	if (CONFIG(update_feature)&MAX11871_UPDATE_BIN) {
		for (i = 0; i < CONFIG(fw_mappings); i++)
			if (CONFIG(fw_mapping[i]).chip_id == chip_id)
				break;
		CHECK(i == CONFIG(fw_mappings), , , "firmware image is"
			" not found for configuration 0x%04X and chip 0x%02X",
			config_id, chip_id);
		j = CONFIG(fw_mapping[i]).fw_index;
		if (request_slept == 0 && is_booting() &&
			CONFIG(cfgfw_request_delay) > 0)
			msleep(CONFIG(cfgfw_request_delay));
		DEBUG(5, "(INIT): firmware file (%s)", CONFIG(fw_image[j]).file_name);
		ret = request_firmware(&fw, CONFIG(fw_image[j]).file_name,
					&ts->client->dev);
		CHECK(ret || fw == NULL, , , "firmware request failed (%d,%p)", ret,
			fw);
		CHECK(fw->size < CONFIG(fw_image[j]).length, release_firmware(fw), ,
			"firmware size %d is different from expected %d", fw->size,
			CONFIG(fw_image[j]).length);
		if (check_bin_version(fw, &tagLen, ts->fw_ver))
			CHECK(device_fw_load(ts, fw, j, tagLen), release_firmware(fw), ,
				"firmware download failed");
		release_firmware(fw);
		DEBUG(5, "(INIT): firmware download OK");
	}

	if (CONFIG(update_feature)&MAX11871_UPDATE_CONFIG) {
		while(ts->fw_config->config_id != 0) {
			if(ts->fw_config->protocol_ver != ts->protocol_ver) {
				ts->fw_config++;
				continue;
			}
			if(ts->fw_config->major_ver > BYTEH(ts->fw_version[2])) {
				ts->fw_config++;
				continue;
			}
			if(ts->fw_config->minor_ver > BYTEL(ts->fw_version[2])) {
				ts->fw_config++;
				continue;
			}
			if(ts->fw_config->vendor_pin != ts->vendor_pin) {
				ts->fw_config++;
				continue;
			}
			if(ts->fw_config->config_touch) {
				ts->max11871_Touch_Configuration_Data[0] = 0x0001;
				ts->max11871_Touch_Configuration_Data[1] = 0x002A;
				memcpy(ts->max11871_Touch_Configuration_Data+2,
					ts->fw_config->config_touch, BYTE_SIZE(42));
			}
			if(ts->fw_config->config_cal) {
				ts->max11871_Calibration_Table_Data[0] = 0x0010;
				ts->max11871_Calibration_Table_Data[1] = 0x0032;
				memcpy(ts->max11871_Calibration_Table_Data+2,
					ts->fw_config->config_cal, BYTE_SIZE(50));
			}
			if(ts->fw_config->config_private) {
				ts->max11871_Private_Configuration_Data[0] = 0x0003;
				ts->max11871_Private_Configuration_Data[1] = 0x0017;
				memcpy(ts->max11871_Private_Configuration_Data+2,
					ts->fw_config->config_private, BYTE_SIZE(23));
			}
			if(ts->fw_config->config_lin_x) {
				ts->max11871_Lookup_Table_X_Data[0] = 0x0030;
				ts->max11871_Lookup_Table_X_Data[1] = 0x0009;
				ts->max11871_Lookup_Table_X_Data[2] = 0x0000;
				memcpy(ts->max11871_Lookup_Table_X_Data+3,
					ts->fw_config->config_lin_x, BYTE_SIZE(8));
			}
			if(ts->fw_config->config_lin_y) {
				ts->max11871_Lookup_Table_Y_Data[0] = 0x0030;
				ts->max11871_Lookup_Table_Y_Data[1] = 0x0009;
				ts->max11871_Lookup_Table_Y_Data[2] = 0x0001;
				memcpy(ts->max11871_Lookup_Table_Y_Data+3,
					ts->fw_config->config_lin_y, BYTE_SIZE(8));
			}
			if(ts->fw_config->config_ifactor) {
				memcpy(ts->max11871_Image_Factor_Table,
					ts->fw_config->config_ifactor, BYTE_SIZE(460));
			}
			break;
		}
		if(ts->fw_config->config_id != 0) {
			update_config(ts);
			DEBUG(5, "(INIT): Check config finish");
		}
	}

	ENABLE_IRQ();
	CHECK(change_touch_rpt(ts->client, MAXIM_TOUCH_REPORT_MODE) < 0, , ,
		"Failed to set up touch report mode");
}

static int device_init_thread(void *arg)
{
	return device_init((struct i2c_client *)arg);
}

static int device_init(struct i2c_client *client)
{
	struct data *ts = NULL;
	struct max11871_pdata *pdata = client->dev.platform_data;
	struct device_attribute  **dev_attr = dev_attrs;
	const struct firmware *fw;
	u16 request_slept = 0, panel_x = 0, panel_y = 0;
	int ret;

	init_state = 1;
	PRINT("(INIT): Start");

	/* if I2C functionality is not present we are done */
	/*CHECK(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C), , 0,
		"I2C core driver does not support I2C functionality");
	DEBUG(5, "(INIT): I2C functionality OK");*/

	/* if platform data is missing we are also done */
	CHECK(!pdata, , 0, "Platform data is missing");
	DEBUG(5, "(INIT): platform data OK");

	/* allocate control block; nothing more to do if we can't */
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	CHECK(!ts, , 0, "Failed to allocate control block memory");
#if 1
	ts->config = pdata->config;
	ts->fw_config = pdata->fw_config;
	DEBUG(5, "(INIT): use platform data");
	CHECK(!ts->config, , 0, "Configuration data is missing");
#else
	ts->config = &local_config;
	DEBUG(5, "(INIT): use local config");
#endif
	ts->client = client;
	i2c_set_clientdata(client, ts);
	mutex_init(&ts->irq_mutex);
	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->report_mutex);
	sema_init(&ts->report_sem, 1);
	ts->nbuttons_original = CONFIG(buttons);
	ts->fw_index = -1;
	ts->buttondown = -1;
	ts->noise_level = 0;
	ts->frame_count = -1;
	ts->baseline_mode = MAX11871_AUTO_BASELINE;
	DEBUG(5, "(INIT): memory allocation OK");

	/* request configuration from file */
	if (CONFIG(cfg_request) && CONFIG(cfg_file_name)) {
		if (is_booting() && CONFIG(cfgfw_request_delay) > 0) {
			msleep(CONFIG(cfgfw_request_delay));
			request_slept = 1;
		}
		ret = request_firmware(&fw, CONFIG(cfg_file_name),
					&ts->client->dev);
		if (ret || fw == NULL) {
			ERROR("configuration file [%s] cannot be acquired, "
				"%d %p", CONFIG(cfg_file_name), ret, fw);
		} else {
			if (fw->size >= 10) {
				/* parse configuration here */
				DEBUG(5, "(INIT): configuration parsed OK");
			} else
				DEBUG(5, "configuration file size is small "
					"(%d bytes)", fw->size);
			release_firmware(fw);
		}
	}
	/* need to validate configuration here */

	if (get_tamper_sf()==0) {
		debug_mask |= BIT(3);
		DEBUG_LOG(20, "(INIT): Debug level=0x%08X", debug_mask);
	}

	/* initialize GPIO pins */
	if (pdata->init)
		CHECK(pdata->init(pdata, 1) < 0, , 0, "GPIO init failed");
	DEBUG(5, "(INIT): chip init OK");
	/* set up IRQ and handler */
	CHECK(request_threaded_irq(client->irq, NULL, irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ts) != 0, ,
		0, "Failed to set up IRQ");
	DEBUG(5, "(INIT): IRQ handler OK");
	/* start with known state - reset the chip */
	/*CHECK(bootloader_enter(ts), , 0, "Failed to enter bootloader");
	CHECK(bootloader_exit(ts), , 0, "Failed to exit bootloader");
	DEBUG(5, "(INIT): chip reset OK");*/

	/* evaluate configuration parameters */
	while (ts->fw_config->config_id != 0) {
		if (CONFIG(default_chip_config) == ts->fw_config->config_id) {
			panel_x = COORDINATES(panel_max_x) - COORDINATES(panel_min_x);
			panel_y = COORDINATES(panel_max_y) - COORDINATES(panel_min_y);
			ts->x_channel = BYTEH(ts->fw_config->config_touch[1]);
			ts->y_channel = BYTEH(ts->fw_config->config_touch[1]);
			ts->frame_rate[0] = ts->fw_config->config_touch[2];
			ts->frame_rate[1] = ts->fw_config->config_touch[3];
			break;
		}
		else
			ts->fw_config++;
	}
	if (ts->fw_config->config_id == 0) {
		panel_x = 719;
		panel_y = 1279;
		ts->x_channel = 12;
		ts->x_channel = 20;
		ts->frame_rate[0] = 0x78;
		ts->frame_rate[1] = 0x1E;
	}
	/* collect controller ID and configuration ID data from firmware   */
	/* and perform firmware comparison/download if we have valid image */
	check_fw_and_config(ts, request_slept);

	/* allocate and register touch device */
	ts->input_dev = input_allocate_device();
	CHECK(!ts->input_dev, , 0, "Failed to allocate touch input device");
	snprintf(ts->phys, sizeof(ts->phys), "%s/input0",
		dev_name(&ts->client->dev));
	ts->input_dev->name = MAX11871_TOUCH;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	__set_bit(EV_SYN, ts->input_dev->evbit);
	__set_bit(EV_ABS, ts->input_dev->evbit);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
				COORDINATES(panel_min_x),
				COORDINATES(panel_max_x), 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
				COORDINATES(panel_min_y),
				COORDINATES(panel_max_y), 0, 0);
#ifdef ABS_MT_PRESSURE
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 0xFF, 0,
#else
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0,
#endif
				0);
	if (CONFIG(input_protocol) == MAX11871_PROTOCOL_B) {
		input_mt_init_slots(ts->input_dev, CONFIG(max_touches));
	} else {
		ts->input_dev->mtsize = 10;
		input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0,
			CONFIG(max_touches), 0, 0);
	}
	if (CONFIG(input_protocol) == MAX11871_PROTOCOL_CUSTOM1) {
#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
		input_set_abs_params(ts->input_dev, ABS_MT_AMPLITUDE, 0,
					0xFF14, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION, 0,
					((1 << 31) | (999 << 16) | 1003), 0,
					0);
#endif
	}
	CHECK(input_register_device(ts->input_dev),
		{input_free_device(ts->input_dev); ts->input_dev = NULL; }, 0,
		"Failed to register touch input device");
	DEBUG(5, "(INIT): input touch device OK");

	/* allocate and register key device */
	if (CONFIG(buttons_enabled)) {
		ts->key_input_dev = input_allocate_device();
		CHECK(!ts->key_input_dev, , 0, "Failed to allocate key input "
			"device");
		snprintf(ts->key_phys, sizeof(ts->phys), "%s/input1",
			dev_name(&client->dev));
		ts->key_input_dev->name = MAX11871_KEY;
		ts->key_input_dev->phys = ts->key_phys;
		ts->key_input_dev->id.bustype = BUS_I2C;
		__set_bit(EV_KEY,    ts->key_input_dev->evbit);
		set_bit(KEY_HOME, ts->key_input_dev->keybit);
		set_bit(KEY_MENU, ts->key_input_dev->keybit);
		set_bit(KEY_BACK, ts->key_input_dev->keybit);
		set_bit(KEY_SEARCH, ts->key_input_dev->keybit);
		CHECK(input_register_device(ts->key_input_dev),
			{input_free_device(ts->key_input_dev);
			ts->key_input_dev = NULL; }, 0,
			"Failed to register key input device");
		DEBUG(5, "(INIT): input key device OK");
	}

	if(panel_x!=0 && panel_y!=0) {
		ts->width_factor = (CONFIG(lcd_x)<<SHIFT_BITS) / panel_x;
		ts->height_factor = (CONFIG(lcd_y)<<SHIFT_BITS) / panel_y;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	/* configure suspend/resume */
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1;
	ts->early_suspend.suspend = early_suspend;
	ts->early_suspend.resume = late_resume;
	register_early_suspend(&ts->early_suspend);
	ts->early_suspend_registered = 1;
	DEBUG(5, "(INIT): suspend/resume registration OK");
#endif

	gl_ts = ts;
	/* set up debug interface */
	CHECK(sysfs_create_file(android_touch_kobj, &dev_attr_debug_level.attr) < 0, ,
		0, "failed to create sysfs file [debug_level]");
	CHECK(sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr) < 0, ,
		0, "failed to create sysfs file [vendor]");
	CHECK(sysfs_create_file(android_touch_kobj, &dev_attr_config.attr) < 0, ,
		0, "failed to create sysfs file [config]");
	CHECK(sysfs_create_file(android_touch_kobj, &dev_attr_diag.attr) < 0, ,
		0, "failed to create sysfs file [diag]");
	CHECK(sysfs_create_file(android_touch_kobj, &dev_attr_gpio.attr) < 0, ,
		0, "failed to create sysfs file [gpio]");
	while (*dev_attr) {
		CHECK(device_create_file(&client->dev, *dev_attr) < 0,
			, 0, "failed to create sysfs file [hreset]");
		ts->sysfs_created++;
		dev_attr++;
	}
	DEVFCA(CONFIG(buttons), (S_IWUSR|S_IRUGO), button_show, button_store, button);
	CHECK(device_create_bin_file(&client->dev, &dev_attr_report) < 0, , 0,
		"failed to create sysfs file [report]");
	ts->sysfs_created++;

	PRINT("(INIT): Done");
	return 0;
}

static int device_deinit(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
	struct max11871_pdata *pdata = client->dev.platform_data;
	struct device_attribute  **dev_attr = dev_attrs;

	if (ts == NULL)
		return 0;

	propagate_report(ts, -1, NULL);

	init_state = 0;
	sysfs_remove_file(android_touch_kobj, &dev_attr_debug_level.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_config.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_diag.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_gpio.attr);
	while (*dev_attr) {
		if (ts->sysfs_created && ts->sysfs_created--)
			device_remove_file(&client->dev, *dev_attr);
		dev_attr++;
	}
	DEVFRA(ts->nbuttons_original, button);
	if (ts->sysfs_created && ts->sysfs_created--)
		device_remove_bin_file(&client->dev, &dev_attr_report);

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (ts->early_suspend_registered)
		unregister_early_suspend(&ts->early_suspend);
#endif
	if (ts->input_dev)
		input_unregister_device(ts->input_dev);
	if (ts->key_input_dev)
		input_unregister_device(ts->key_input_dev);

	if (client->irq)
		free_irq(client->irq, ts);
	kfree(ts);
	if (pdata && pdata->init)
		(void)pdata->init(pdata, 0);

	PRINT("(INIT): Deinitialized");
	return 0;
}

static int check_chip_exist(struct i2c_client *client)
{
	char buf[32];
	int read_len = 0, i;
	/* if I2C functionality is not present we are done */
	CHECK(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C), , -1,
		"I2C core driver does not support I2C functionality");

	for (i=0; i<RETRY_TIMES; i++) {
		if (i != 0)
			msleep(20);
		buf[0] = 0x0A;
		buf[1] = 0x00;
		if (i2c_master_send(client, buf, 2) <= 0)
			continue;
		if (i2c_master_recv(client, buf, 2) <= 0)
			continue;
		read_len = (buf[0] + 1) << 1;
		if (read_len>10)
			read_len = 10;
		if (i2c_master_recv(client, buf, read_len) <= 0)
			continue;
		break;
	}
	if (i == RETRY_TIMES) {
		PRINT("(PROBE) No Maxim chip");
		return -1;
	}

	PRINT("(INIT): I2C functionality OK");
	PRINT("(INIT): Chip exist");
	return 0;
}

static void off_mode_suspend(struct i2c_client *client)
{
	char data[] = {0x00, 0x00, 0x03, 0x11, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00};
	int ret;
	PRINT("(PROBE): Offmode charging. Set touch chip to sleep mode and skip touch driver probe");
	do {ret = i2c_master_send(client, data, 10);
	} while (ret == -EAGAIN);
	CHECKI(ret <= 0, "Send Suspend Commamd Failed");
}

static int probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	PRINT("(PROBE): max11871_%s Enter", __func__);
	if (check_chip_exist(client) < 0)
		return -1;

	if (board_mfg_mode() == 5) {
		off_mode_suspend(client);
		return -1;
	}

	android_touch_kobj = kobject_create_and_add("android_touch", NULL);
	CHECK(android_touch_kobj == NULL,
			, 0, "failed to create kobj");
	CHECK(device_create_file(&client->dev, &dev_attr_init) < 0, , 0,
		"failed to create sysfs file [init]");
	CHECK(sysfs_create_link(android_touch_kobj,
		&client->dev.kobj, "maxim11871") < 0,
		, 0, "failed to create link");
	if (!is_booting())
		return device_init(client);
	CHECK(IS_ERR(kthread_run(device_init_thread, (void *)client,
		MAX11871_NAME)), , -1, "failed to start kernel thread");
	return 0;
}

static int remove(struct i2c_client *client)
{
	int ret = device_deinit(client);

	sysfs_remove_link(android_touch_kobj, "maxim11871");
	device_remove_file(&client->dev, &dev_attr_init);
	kobject_del(android_touch_kobj);
	return ret;
}

/*
	COMMANDS
*/
static int sreset(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x00E9, 0x0000};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int get_touch_config(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0002, 0x0000};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int get_fw_version(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0040, 0x0000};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int change_touch_rpt(struct i2c_client *client, u16 to)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0018, 0x0001, to & 0x0003};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int set_touch_frame(struct i2c_client *client,
			u16 idle_frame, u16 active_frame)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0026, 0x0001,
		(((idle_frame & 0xFF) << 8) | (active_frame & 0xFF))};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int set_baseline_mode(struct i2c_client *client, u16 mode)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0028, 0x0001, mode & 0x0003};
	return send_mtp_command(ts, data, NWORDS(data));
}

static void propagate_report(struct data *ts, int status, u16 *report)
{
	int i;

	down(&ts->report_sem);
	if (report)
		memcpy(ts->report, report, BYTE_SIZE(BYTEL(report[0]) + 1));

	mutex_lock(&ts->report_mutex);
	for (i = 0; i < MAX_REPORT_READERS; i++) {
		if (status == 0) {
			if (ts->report_readers[i].report_id == 0xFFFF ||
			    ts->report_readers[i].report_id ==
							ts->report[1]) {
				up(&ts->report_readers[i].sem);
				ts->report_readers[i].reports_passed++;
				ts->report_readers_outstanding++;
			}
		} else {
			if (ts->report_readers[i].report_id != 0) {
				ts->report_readers[i].status = status;
				up(&ts->report_readers[i].sem);
			}
		}
	}
	if (ts->report_readers_outstanding == 0)
		up(&ts->report_sem);
	mutex_unlock(&ts->report_mutex);
}

static int get_report(struct data *ts, u16 report_id, ulong timeout)
{
	int i, ret, status;

	mutex_lock(&ts->report_mutex);
	for (i = 0; i < MAX_REPORT_READERS; i++)
		if (ts->report_readers[i].report_id == 0)
			break;
	CHECK(i == MAX_REPORT_READERS, {mutex_unlock(&ts->report_mutex);
			ENABLE_IRQ(); }, -1, "maximum readers reached");

	ts->report_readers[i].report_id = report_id;
	sema_init(&ts->report_readers[i].sem, 1);
	down(&ts->report_readers[i].sem);

	ts->report_readers[i].status = 0;
	ts->report_readers[i].reports_passed = 0;
	mutex_unlock(&ts->report_mutex);
	ENABLE_IRQ();

	if (timeout == 0xFFFFFFFF)
		ret = down_interruptible(&ts->report_readers[i].sem);
	else
		ret = down_timeout(&ts->report_readers[i].sem,
			(timeout * HZ) / 1000);
	mutex_lock(&ts->report_mutex);

	if (ret && ts->report_readers[i].reports_passed > 0)
		if (--ts->report_readers_outstanding == 0)
			up(&ts->report_sem);
	status = ts->report_readers[i].status;
	ts->report_readers[i].report_id = 0;
	mutex_unlock(&ts->report_mutex);
	return (status == 0) ? ret : status;
}

static void release_report(struct data *ts)
{
	mutex_lock(&ts->report_mutex);
	if (--ts->report_readers_outstanding == 0)
		up(&ts->report_sem);
	mutex_unlock(&ts->report_mutex);
}

static void suspend(struct i2c_client *client, pm_message_t mesg)
{
	u16 data[] = {0x0020, 0x0001, 0x0000};
	struct data *ts = i2c_get_clientdata(client);

	PRINT("max11871_%s", __func__);
	DISABLE_IRQ();
	(void)send_mtp_command(ts, data, NWORDS(data));
}

static void resume(struct i2c_client *client)
{
	u16 data[] = {0x0020, 0x0001, 0x0002};
	struct data *ts = i2c_get_clientdata(client);

	PRINT("max11871_%s", __func__);
	/* previous_fingers = current_fingers = 0; */
	(void)send_mtp_command(ts, data, NWORDS(data));
	ENABLE_IRQ();

	(void)change_touch_rpt(ts->client, MAXIM_TOUCH_REPORT_MODE);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void early_suspend(struct early_suspend *h)
{
	struct data *ts;
	ts = container_of(h, struct data, early_suspend);

	suspend(ts->client, PMSG_SUSPEND);
}

static void late_resume(struct early_suspend *h)
{
	struct data *ts;
	ts = container_of(h, struct data, early_suspend);

	resume(ts->client);
}
#endif

#define STATUS_ADDR_H 0x00
#define STATUS_ADDR_L 0xFF
#define DATA_ADDR_H   0x00
#define DATA_ADDR_L   0xFE
#define STATUS_READY_H 0xAB
#define STATUS_READY_L 0xCC
#define RXTX_COMPLETE_H 0x54
#define RXTX_COMPLETE_L 0x32
static int bootloader_read_status_reg(struct data *ts, const u8 byteL,
					const u8 byteH)
{
	u8 buffer[] = {STATUS_ADDR_L, STATUS_ADDR_H}, i;

	for (i = 0; i < 3; i++) {
		CHECK(i2c_tx_bytes(ts, buffer, 2) != 2, , -1, "TX fail");
		CHECK(i2c_rx_bytes(ts, buffer, 2) != 2, , -1, "RX fail");
		if (buffer[0] == byteL && buffer[1] == byteH)
			break;
	}
	CHECK(i == 3, , -1, "Unexpected status => %02X%02X vs %02X%02X",
		buffer[0], buffer[1], byteL, byteH);

	return 0;
}

static int bootloader_write_status_reg(struct data *ts, const u8 byteL,
					const u8 byteH)
{
	u8 buffer[] = {STATUS_ADDR_L, STATUS_ADDR_H, byteL, byteH};

	CHECK(i2c_tx_bytes(ts, buffer, 4) != 4, , -1, "TX fail");
	return 0;
}

static int bootloader_rxtx_complete(struct data *ts)
{
	return bootloader_write_status_reg(ts, RXTX_COMPLETE_L,
						RXTX_COMPLETE_H);
}

static int bootloader_read_data_reg(struct data *ts, u8* byteL, u8* byteH)
{
	u8 buffer[] = {DATA_ADDR_L, DATA_ADDR_H, 0x00, 0x00};

	CHECK(i2c_tx_bytes(ts, buffer, 2) != 2, , -1, "TX fail");
	CHECK(i2c_rx_bytes(ts, buffer, 4) != 4, , -1, "RX fail");
	CHECK(buffer[2] != 0xCC && buffer[3] != 0xAB, , -1,
		"Status is not ready");

	*byteL = buffer[0];
	*byteH = buffer[1];
	return bootloader_rxtx_complete(ts);
}

static int bootloader_write_data_reg(struct data *ts, const u8 byteL,
					const u8 byteH)
{
	u8 buffer[6] = {DATA_ADDR_L, DATA_ADDR_H, byteL, byteH,
			RXTX_COMPLETE_L, RXTX_COMPLETE_H};

	CHECK(bootloader_read_status_reg(ts, STATUS_READY_L,
		STATUS_READY_H) < 0, , -1, "read status register fail");
	CHECK(i2c_tx_bytes(ts, buffer, 6) != 6, , -1, "TX fail");
	return 0;
}

static int bootloader_rxtx(struct data *ts, u8* byteL, u8* byteH, const int tx)
{
	if (tx > 0) {
		CHECK(bootloader_write_data_reg(ts, *byteL, *byteH) < 0, , -1,
			"write data register fail");
		return 0;
	}

	CHECK(bootloader_read_data_reg(ts, byteL, byteH) < 0, , -1, "read "
		"data register fail");
	return 0;
}

static int bootloader_get_cmd_conf(struct data *ts, int retries)
{
	u8 byteL, byteH;

	do {
		if (bootloader_read_data_reg(ts, &byteL, &byteH) >= 0) {
			if (byteH == 0x00 && byteL == 0x3E)
				return 0;
		}
		retries--;
	} while (retries > 0);

	return -1;
}

static int bootloader_write_buffer(struct data *ts, u8* buffer, int size)
{
	u8 byteH = 0x00;
	int k;

	for (k = 0; k < size; k++) {
		CHECK(bootloader_rxtx(ts, &buffer[k], &byteH, 1) < 0, , -1,
			"bootloader RX-TX fail");
	}
	return 0;
}

static int bootloader_enter(struct data *ts)
{
	int i;
	u16 enter[3][2] = {{0x7F00, 0x0047}, {0x7F00, 0x00C7},
			{0x7F00, 0x0007} };

	DISABLE_IRQ();
	mutex_lock(&ts->i2c_mutex);
	for (i = 0; i < 3; i++) {
		CHECK(i2c_tx_words(ts, enter[i], 2) != 2,
			{mutex_unlock(&ts->i2c_mutex); ENABLE_IRQ(); }, -1,
			"Failed to enter bootloader");
	}

	CHECK(bootloader_get_cmd_conf(ts, 5) < 0,
		{mutex_unlock(&ts->i2c_mutex); ENABLE_IRQ(); }, -1,
		"Failed to enter bootloader mode");
	bootloader = 1;
	return 0;
}

static int bootloader_exit(struct data *ts)
{
	u16 exit[] = {0x00FE, 0x0001, 0x5432};

	bootloader = 0;
	ts->got_report = 0;
	CHECK(i2c_tx_words(ts, exit, NWORDS(exit)) != NWORDS(exit),
		mutex_unlock(&ts->i2c_mutex), -1,
		"Failed to exit bootloader");
	mutex_unlock(&ts->i2c_mutex);
	return 0;
}

static int bootloader_get_crc(struct data *ts, u16 *crc16, u16 len)
{
	u8 crc_command[] = {0x30, 0x02, 0x00, 0x00, BYTEL(len), BYTEH(len)};
	u8 byteL = 0, byteH = 0;
	u16 rx_crc16 = 0;

	CHECK(bootloader_write_buffer(ts, crc_command, 6) < 0, , -1,
		"write buffer fail");
	msleep(200); /* wait 200ms for it to get done */

	/* reads low 8bits (crcL) */
	CHECK(bootloader_rxtx(ts, &byteL, &byteH, 0) < 0, , -1,
		"Failed to read low byte of crc response!");
	rx_crc16 = (u16)byteL;

	/* reads high 8bits (crcH) */
	CHECK(bootloader_rxtx(ts, &byteL, &byteH, 0) < 0, , -1,
		"Failed to read high byte of crc response!");
	rx_crc16 = (u16)(byteL << 8) | rx_crc16;

	CHECK(bootloader_get_cmd_conf(ts, 5) < 0, , -1, "CRC get failed");
	*crc16 = rx_crc16;

	return 0;
}

static int bootloader_set_byte_mode(struct data *ts)
{
	u8 buffer[2] = {0x0A, 0x00};

	CHECK(bootloader_write_buffer(ts, buffer, 2) < 0, , -1,
		"write buffer fail");
	CHECK(bootloader_get_cmd_conf(ts, 10) < 0, , -1,
		"command confirm fail");
	return 0;
}

static int bootloader_erase_flash(struct data *ts)
{
	u8 byteL = 0x02, byteH = 0x00;
	int i, verify = 0;

	CHECK(bootloader_rxtx(ts, &byteL, &byteH, 1) < 0, , -1,
		"bootloader RX-TX fail");

	for (i = 0; i < 10; i++) {
		msleep(60); /* wait 60ms */

		if (bootloader_get_cmd_conf(ts, 0) < 0)
			continue;

		verify = 1;
		break;
	}

	CHECK(verify != 1, , -1, "Flash Erase failed");

	return 0;
}

static int bootloader_write_flash(struct data *ts, u8 *image)
{
	u8 command[] = {0xF0, 0x00, 0x80, 0x00, 0x00}, buffer[130];
	int i, j;

	CHECK(bootloader_write_buffer(ts, command, 5) < 0, , -1,
		"write buffer fail");

	for (i = 0; i < 256; i++) {
		for (j = 0; j < 100; j++) {
			usleep_range(1500, 2000);
			if (bootloader_read_status_reg(ts, STATUS_READY_L,
						STATUS_READY_H) == 0)
				break;
		}
		CHECK(j == 100, , -1, "Failed to read Status register!");

		buffer[0] = ((i % 2) == 0) ? 0x00 : 0x40;
		buffer[1] = 0x00;
		memcpy(buffer + 2, image + i * 128, 128);

		CHECK(i2c_tx_bytes(ts, buffer, 130) != 130, , -1,
			"Failed to write data (%d)", i);
		CHECK(bootloader_rxtx_complete(ts) < 0, , -1,
			"Transfer failure (%d)", i);
	}

	usleep_range(10000, 11000);
	CHECK(bootloader_get_cmd_conf(ts, 5) < 0, , -1,
		"Flash programming failed");
	return 0;
}

/****************************************
 *
 * Standard Driver Structures/Functions
 *
 ****************************************/
static const struct i2c_device_id id[] = {
	{ MAX11871_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, id);

static struct i2c_driver driver = {
	.probe          = probe,
	.remove         = remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend        = suspend,
	.resume         = resume,
#endif
	.id_table       = id,
	.driver = {
		.name   = MAX11871_NAME,
	},
};

static int __devinit max11871_init(void)
{
	return i2c_add_driver(&driver);
}

static void __exit max11871_exit(void)
{
	i2c_del_driver(&driver);
}

module_init(max11871_init);
module_exit(max11871_exit);

MODULE_AUTHOR("Maxim Integrated Products, Inc.");
MODULE_DESCRIPTION("MAX11871 Touchscreen Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("3.0.4.3");

