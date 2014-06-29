/* drivers/tty/smux_test.c
 *
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/termios.h>
#include <linux/smux.h>
#include <mach/subsystem_restart.h>
#include "smux_private.h"

#define DEBUG_BUFMAX 4096

/**
 * Unit test assertion for logging test cases.
 *
 * @a lval
 * @b rval
 * @cmp comparison operator
 *
 * Assertion fails if (@a cmp @b) is not true which then
 * logs the function and line number where the error occurred
 * along with the values of @a and @b.
 *
 * Assumes that the following local variables exist:
 * @buf - buffer to write failure message to
 * @i - number of bytes written to buffer
 * @max - maximum size of the buffer
 * @failed - set to true if test fails
 */
#define UT_ASSERT_INT(a, cmp, b) \
	{ \
	int a_tmp = (a); \
	int b_tmp = (b); \
	if (!((a_tmp)cmp(b_tmp))) { \
		i += scnprintf(buf + i, max - i, \
			"%s:%d Fail: " #a "(%d) " #cmp " " #b "(%d)\n", \
				__func__, __LINE__, \
				a_tmp, b_tmp); \
		failed = 1; \
		break; \
	} \
	}

#define UT_ASSERT_PTR(a, cmp, b) \
	{ \
	void *a_tmp = (a); \
	void *b_tmp = (b); \
	if (!((a_tmp)cmp(b_tmp))) { \
		i += scnprintf(buf + i, max - i, \
			"%s:%d Fail: " #a "(%p) " #cmp " " #b "(%p)\n", \
				__func__, __LINE__, \
				a_tmp, b_tmp); \
		failed = 1; \
		break; \
	} \
	}

#define UT_ASSERT_UINT(a, cmp, b) \
	{ \
	unsigned a_tmp = (a); \
	unsigned b_tmp = (b); \
	if (!((a_tmp)cmp(b_tmp))) { \
		i += scnprintf(buf + i, max - i, \
			"%s:%d Fail: " #a "(%u) " #cmp " " #b "(%u)\n", \
				__func__, __LINE__, \
				a_tmp, b_tmp); \
		failed = 1; \
		break; \
	} \
	}

/**
 * In-range unit test assertion for test cases.
 *
 * @a lval
 * @minv Minimum value
 * @maxv Maximum value
 *
 * Assertion fails if @a is not on the exclusive range minv, maxv
 * ((@a < @minv) or (@a > @maxv)).  In the failure case, the macro
 * logs the function and line number where the error occurred along
 * with the values of @a and @minv, @maxv.
 *
 * Assumes that the following local variables exist:
 * @buf - buffer to write failure message to
 * @i - number of bytes written to buffer
 * @max - maximum size of the buffer
 * @failed - set to true if test fails
 */
#define UT_ASSERT_INT_IN_RANGE(a, minv, maxv) \
	{ \
	int a_tmp = (a); \
	int minv_tmp = (minv); \
	int maxv_tmp = (maxv); \
	if (((a_tmp) < (minv_tmp)) || ((a_tmp) > (maxv_tmp))) { \
		i += scnprintf(buf + i, max - i, \
			"%s:%d Fail: " #a "(%d) < " #minv "(%d) or " \
				 #a "(%d) > " #maxv "(%d)\n", \
				__func__, __LINE__, \
				a_tmp, minv_tmp, a_tmp, maxv_tmp); \
		failed = 1; \
		break; \
	} \
	}


static unsigned char test_array[] = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55,
					89, 144, 233};

static int get_rx_buffer_mock_fail;


struct tiocm_test_vector {
	uint32_t input;
	uint32_t set_old;
	uint32_t set_new;
	uint32_t clr_old;
};

int get_rx_buffer(void *priv, void **pkt_priv, void **buffer, int size)
{
	void *rx_buf;

	rx_buf = kmalloc(size, GFP_KERNEL);
	*pkt_priv = (void *)0x1234;
	*buffer = rx_buf;

	return 0;
}

struct test_vector {
	const char *data;
	const unsigned len;
};

struct mock_read_event {
	struct list_head list;
	struct smux_meta_read meta;
};

struct mock_write_event {
	struct list_head list;
	struct smux_meta_write meta;
};

struct mock_get_rx_buff_event {
	struct list_head list;
	int size;
	unsigned long jiffies;
};

struct smux_mock_callback {
	int cb_count;
	struct completion cb_completion;
	spinlock_t lock;

	
	int event_connected;
	int event_disconnected;
	int event_disconnected_ssr;
	int event_low_wm;
	int event_high_wm;
	int event_rx_retry_high_wm;
	int event_rx_retry_low_wm;

	
	int event_tiocm;
	struct smux_meta_tiocm tiocm_meta;

	
	int event_read_done;
	int event_read_failed;
	struct list_head read_events;

	
	int get_rx_buff_retry_count;
	struct list_head get_rx_buff_retry_events;

	
	int event_write_done;
	int event_write_failed;
	struct list_head write_events;
};

static int get_rx_buffer_mock(void *priv, void **pkt_priv,
		void **buffer, int size);

void mock_cb_data_init(struct smux_mock_callback *cb)
{
	init_completion(&cb->cb_completion);
	spin_lock_init(&cb->lock);
	INIT_LIST_HEAD(&cb->read_events);
	INIT_LIST_HEAD(&cb->get_rx_buff_retry_events);
	INIT_LIST_HEAD(&cb->write_events);
}

void mock_cb_data_reset(struct smux_mock_callback *cb)
{
	cb->cb_count = 0;
	INIT_COMPLETION(cb->cb_completion);
	cb->event_connected = 0;
	cb->event_disconnected = 0;
	cb->event_disconnected_ssr = 0;
	cb->event_low_wm = 0;
	cb->event_high_wm = 0;
	cb->event_rx_retry_high_wm = 0;
	cb->event_rx_retry_low_wm = 0;
	cb->event_tiocm = 0;
	cb->tiocm_meta.tiocm_old = 0;
	cb->tiocm_meta.tiocm_new = 0;

	cb->event_read_done = 0;
	cb->event_read_failed = 0;
	while (!list_empty(&cb->read_events)) {
		struct mock_read_event *meta;
		meta = list_first_entry(&cb->read_events,
				struct mock_read_event,
				list);
		kfree(meta->meta.buffer);
		list_del(&meta->list);
		kfree(meta);
	}

	cb->get_rx_buff_retry_count = 0;
	while (!list_empty(&cb->get_rx_buff_retry_events)) {
		struct mock_get_rx_buff_event *meta;
		meta = list_first_entry(&cb->get_rx_buff_retry_events,
				struct mock_get_rx_buff_event,
				list);
		list_del(&meta->list);
		kfree(meta);
	}

	cb->event_write_done = 0;
	cb->event_write_failed = 0;
	while (!list_empty(&cb->write_events)) {
		struct mock_write_event *meta;
		meta = list_first_entry(&cb->write_events,
				struct mock_write_event,
				list);
		list_del(&meta->list);
		kfree(meta);
	}
}

static int mock_cb_data_print(const struct smux_mock_callback *cb,
		char *buf, int max)
{
	int i = 0;

	i += scnprintf(buf + i, max - i,
		"\tcb_count=%d\n"
		"\tcb_completion.done=%d\n"
		"\tevent_connected=%d\n"
		"\tevent_disconnected=%d\n"
		"\tevent_disconnected_ssr=%d\n"
		"\tevent_low_wm=%d\n"
		"\tevent_high_wm=%d\n"
		"\tevent_rx_retry_high_wm=%d\n"
		"\tevent_rx_retry_low_wm=%d\n"
		"\tevent_tiocm=%d\n"
		"\tevent_read_done=%d\n"
		"\tevent_read_failed=%d\n"
		"\tread_events empty=%d\n"
		"\tget_rx_retry=%d\n"
		"\tget_rx_retry_events empty=%d\n"
		"\tevent_write_done=%d\n"
		"\tevent_write_failed=%d\n"
		"\twrite_events empty=%d\n",
		cb->cb_count,
		cb->cb_completion.done,
		cb->event_connected,
		cb->event_disconnected,
		cb->event_disconnected_ssr,
		cb->event_low_wm,
		cb->event_high_wm,
		cb->event_rx_retry_high_wm,
		cb->event_rx_retry_low_wm,
		cb->event_tiocm,
		cb->event_read_done,
		cb->event_read_failed,
		list_empty(&cb->read_events),
		cb->get_rx_buff_retry_count,
		list_empty(&cb->get_rx_buff_retry_events),
		cb->event_write_done,
		cb->event_write_failed,
		list_empty(&cb->write_events)
		);

	return i;
}

void smux_mock_cb(void *priv, int event, const void *metadata)
{
	struct smux_mock_callback *cb_data_ptr;
	struct mock_write_event *write_event_meta;
	struct mock_read_event *read_event_meta;
	unsigned long flags;

	cb_data_ptr = (struct smux_mock_callback *)priv;
	if (cb_data_ptr == NULL) {
		pr_err("%s: invalid private data\n", __func__);
		return;
	}

	switch (event) {
	case SMUX_CONNECTED:
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_connected;
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_DISCONNECTED:
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_disconnected;
		cb_data_ptr->event_disconnected_ssr =
			((struct smux_meta_disconnected *)metadata)->is_ssr;
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_READ_DONE:
		read_event_meta = kmalloc(sizeof(struct mock_read_event),
						GFP_KERNEL);
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_read_done;
		if (read_event_meta) {
			read_event_meta->meta =
				*(struct smux_meta_read *)metadata;
			list_add_tail(&read_event_meta->list,
						&cb_data_ptr->read_events);
		}
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_READ_FAIL:
		read_event_meta = kmalloc(sizeof(struct mock_read_event),
						GFP_KERNEL);
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_read_failed;
		if (read_event_meta) {
			if (metadata)
				read_event_meta->meta =
					*(struct smux_meta_read *)metadata;
			else
				memset(&read_event_meta->meta, 0x0,
						sizeof(struct smux_meta_read));
			list_add_tail(&read_event_meta->list,
					&cb_data_ptr->read_events);
		}
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_WRITE_DONE:
		write_event_meta = kmalloc(sizeof(struct mock_write_event),
						GFP_KERNEL);
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_write_done;
		if (write_event_meta) {
			write_event_meta->meta =
					*(struct smux_meta_write *)metadata;
			list_add_tail(&write_event_meta->list,
					&cb_data_ptr->write_events);
		}
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_WRITE_FAIL:
		write_event_meta = kmalloc(sizeof(struct mock_write_event),
						GFP_KERNEL);
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_write_failed;
		if (write_event_meta) {
			write_event_meta->meta =
				*(struct smux_meta_write *)metadata;
			list_add_tail(&write_event_meta->list,
					&cb_data_ptr->write_events);
		}
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_LOW_WM_HIT:
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_low_wm;
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_HIGH_WM_HIT:
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_high_wm;
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_RX_RETRY_HIGH_WM_HIT:
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_rx_retry_high_wm;
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	case SMUX_RX_RETRY_LOW_WM_HIT:
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_rx_retry_low_wm;
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;


	case SMUX_TIOCM_UPDATE:
		spin_lock_irqsave(&cb_data_ptr->lock, flags);
		++cb_data_ptr->event_tiocm;
		cb_data_ptr->tiocm_meta = *(struct smux_meta_tiocm *)metadata;
		spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
		break;

	default:
		pr_err("%s: unknown event %d\n", __func__, event);
	};

	spin_lock_irqsave(&cb_data_ptr->lock, flags);
	++cb_data_ptr->cb_count;
	complete(&cb_data_ptr->cb_completion);
	spin_unlock_irqrestore(&cb_data_ptr->lock, flags);
}

static int smux_ut_basic_core(char *buf, int max,
	const struct test_vector *vectors,
	const char *name)
{
	int i = 0;
	int failed = 0;
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	int ret;

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	while (!failed) {
		struct mock_write_event *write_event;
		struct mock_read_event *read_event;

		
		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data, smux_mock_cb,
					get_rx_buffer);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		for (; vectors->data != NULL; ++vectors) {
			const char *test_data = vectors->data;
			const unsigned test_len = vectors->len;

			i += scnprintf(buf + i, max - i,
					"Writing vector %p len %d\n",
					test_data, test_len);

			
			msm_smux_write(SMUX_TEST_LCID, (void *)0xCAFEFACE,
					test_data, test_len);
			UT_ASSERT_INT(ret, ==, 0);
			UT_ASSERT_INT(
					(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);

			
			INIT_COMPLETION(cb_data.cb_completion);
			if (cb_data.cb_count < 2)
				UT_ASSERT_INT(
					(int)wait_for_completion_timeout(
						&cb_data.cb_completion, HZ),
					>, 0);

			UT_ASSERT_INT(cb_data.cb_count, >=, 1);
			UT_ASSERT_INT(cb_data.event_write_done, ==, 1);
			UT_ASSERT_INT(list_empty(&cb_data.write_events), ==, 0);

			write_event = list_first_entry(&cb_data.write_events,
					struct mock_write_event, list);
			UT_ASSERT_PTR(write_event->meta.pkt_priv, ==,
							(void *)0xCAFEFACE);
			UT_ASSERT_PTR(write_event->meta.buffer, ==,
							(void *)test_data);
			UT_ASSERT_INT(write_event->meta.len, ==, test_len);

			
			UT_ASSERT_INT(cb_data.event_read_done, ==, 1);
			UT_ASSERT_INT(list_empty(&cb_data.read_events), ==, 0);
			read_event = list_first_entry(&cb_data.read_events,
					struct mock_read_event, list);
			UT_ASSERT_PTR(read_event->meta.pkt_priv, ==,
							(void *)0x1234);
			UT_ASSERT_PTR(read_event->meta.buffer, !=, NULL);

			if (read_event->meta.len != test_len ||
				memcmp(read_event->meta.buffer,
						test_data, test_len)) {
				
				char linebuff[80];

				hex_dump_to_buffer(test_data, test_len,
					16, 1, linebuff, sizeof(linebuff), 1);
				i += scnprintf(buf + i, max - i,
						"Expected:\n%s\n\n", linebuff);

				hex_dump_to_buffer(read_event->meta.buffer,
					read_event->meta.len,
					16, 1, linebuff, sizeof(linebuff), 1);
				i += scnprintf(buf + i, max - i,
						"Actual:\n%s\n", linebuff);
				failed = 1;
				break;
			}
			mock_cb_data_reset(&cb_data);
		}

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 0);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", name);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}

	mock_cb_data_reset(&cb_data);
	return i;
}

static int smux_ut_basic(char *buf, int max)
{
	const struct test_vector test_data[] = {
		{"hello\0world\n", sizeof("hello\0world\n")},
		{0, 0},
	};
	int i = 0;
	int failed = 0;
	int ret;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_LOCAL_LOOPBACK, 0);
		UT_ASSERT_INT(ret, ==, 0);

		i += smux_ut_basic_core(buf + i, max - i, test_data, __func__);
		break;
	}

	if (failed) {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
	}
	return i;
}

static int smux_ut_remote_basic(char *buf, int max)
{
	const struct test_vector test_data[] = {
		{"hello\0world\n", sizeof("hello\0world\n")},
		{0, 0},
	};
	int i = 0;
	int failed = 0;
	int ret;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_REMOTE_LOOPBACK, 0);
		UT_ASSERT_INT(ret, ==, 0);

		i += smux_ut_basic_core(buf + i, max - i, test_data, __func__);
		break;
	}

	if (failed) {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
	}
	return i;
}

static int smux_ut_remote_ssr_basic(char *buf, int max)
{
	const struct test_vector test_data[] = {
		{"hello\0world\n", sizeof("hello\0world\n")},
		{0, 0},
	};
	int i = 0;
	int failed = 0;
	int retry_count = 0;
	int ret;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_REMOTE_LOOPBACK, 0);
		UT_ASSERT_INT(ret, ==, 0);

		i += smux_ut_basic_core(buf + i, max - i, test_data, __func__);
		subsystem_restart("external_modem");

		do {
			msleep(500);
			++retry_count;
			UT_ASSERT_INT(retry_count, <, 20);
		} while (!smux_remote_is_active() && !failed);

		i += smux_ut_basic_core(buf + i, max - i, test_data, __func__);
		break;
	}

	if (failed) {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
	}
	return i;
}

static int smux_ut_remote_ssr_open(char *buf, int max)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	int ret;
	int retry_count;
	int i = 0;
	int failed = 0;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	while (!failed) {
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_REMOTE_LOOPBACK, 0);
		UT_ASSERT_INT(ret, ==, 0);

		
		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data, smux_mock_cb,
					get_rx_buffer);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		subsystem_restart("external_modem");

		
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, 5*HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);

		
		retry_count = 0;
		do {
			msleep(500);
			++retry_count;
			UT_ASSERT_INT(retry_count, <, 20);
		} while (!smux_remote_is_active() && !failed);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}

	mock_cb_data_reset(&cb_data);

	return i;
}

/**
 * Verify get_rx_buffer callback retry doesn't livelock SSR
 * until all RX Bufffer Retries have timed out.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_remote_ssr_rx_buff_retry(char *buf, int max)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	int i = 0;
	int failed = 0;
	int retry_count;
	int ret;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	pr_err("%s", buf);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_REMOTE_LOOPBACK,
				0);
		UT_ASSERT_INT(ret, ==, 0);

		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data,
				smux_mock_cb, get_rx_buffer_mock);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		get_rx_buffer_mock_fail = 1;
		ret = msm_smux_write(SMUX_TEST_LCID, (void *)1,
					test_array, sizeof(test_array));
		UT_ASSERT_INT(ret, ==, 0);
		while (!cb_data.get_rx_buff_retry_count) {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ),
				>, 0);
			INIT_COMPLETION(cb_data.cb_completion);
		}
		if (failed)
			break;
		mock_cb_data_reset(&cb_data);

		
		subsystem_restart("external_modem");

		
		retry_count = 0;
		while (cb_data.event_disconnected_ssr == 0) {
			(void)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ);
			INIT_COMPLETION(cb_data.cb_completion);
			++retry_count;
			UT_ASSERT_INT(retry_count, <, 10);
		}
		if (failed)
			break;
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);

		
		retry_count = 0;
		do {
			msleep(500);
			++retry_count;
			UT_ASSERT_INT(retry_count, <, 20);
		} while (!smux_remote_is_active() && !failed);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}
	mock_cb_data_reset(&cb_data);
	return i;
}
uint8_t *test_pattern_fill(char *buf, int len, int redzone)
{
	void *ret;
	uint8_t ch;

	ret = buf;
	if (redzone) {
		memset((char *)buf, 0xAB, 16);
		memset((char *)buf + len, 0xBA, 16);
		ret += 16;
	}

	
	for (ch = 0; len > 0; --len, ++ch)
		*buf++ = (char)ch;

	return ret;
}

unsigned test_pattern_verify(char **buf_ptr, int len, int redzone,
					char *errmsg, int errmsg_max)
{
	int n;
	int i = 0;
	char linebuff[80];

	if (redzone) {
		*buf_ptr -= 16;

		
		for (n = 0; n < 16; ++n) {
			if (*buf_ptr[n] != 0xAB) {
				hex_dump_to_buffer(*buf_ptr, 16,
					16, 1, linebuff, sizeof(linebuff), 1);
				i += scnprintf(errmsg + i, errmsg_max - i,
					"Redzone violation: %s\n", linebuff);
				break;
			}
		}

		
		for (n = 0; n < 16; ++n) {
			if (*buf_ptr[len + n] != 0xBA) {
				hex_dump_to_buffer(&(*buf_ptr)[len], 16,
					16, 1, linebuff, sizeof(linebuff), 1);
				i += scnprintf(errmsg + i, errmsg_max - i,
					"Redzone violation: %s\n", linebuff);
				break;
			}
		}
	}
	return i;
}

/**
 * Write a multiple packets in ascending size and verify packet is received
 * correctly.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 * @name Name of the test for error reporting
 *
 * @returns Number of bytes written to @buf
 *
 * Requires that the port already be opened and loopback mode is
 * configured correctly (if required).
 */
static int smux_ut_loopback_big_pkt(char *buf, int max, const char *name)
{
	struct test_vector test_data[] = {
		{0, 64},
		{0, 128},
		{0, 256},
		{0, 512},
		{0, 1024},
		{0, 2048},
		{0, 4096},
		{0, 0},
	};
	int i = 0;
	int failed = 0;
	struct test_vector *tv;

	
	for (tv = test_data; tv->len > 0; ++tv) {
		tv->data = kmalloc(tv->len + 32, GFP_KERNEL);
		pr_err("%s: allocating %p len %d\n",
				__func__, tv->data, tv->len);
		if (!tv->data) {
			i += scnprintf(buf + i, max - i,
					"%s: Unable to allocate %d bytes\n",
					__func__, tv->len);
			failed = 1;
			goto out;
		}
		test_pattern_fill((uint8_t *)tv->data, tv->len, 1);
	}

	
	i += scnprintf(buf + i, max - i, "Running %s\n", name);
	while (!failed) {
		i += smux_ut_basic_core(buf + i, max - i, test_data, name);
		break;
	}

out:
	if (failed) {
		pr_err("%s: Failed\n", name);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
	}

	for (tv = test_data; tv->len > 0; ++tv) {
		if (!tv->data) {
			i += test_pattern_verify((char **)&tv->data,
						tv->len, 1, buf + i, max - i);
			pr_err("%s: freeing %p len %d\n", __func__,
							tv->data, tv->len);
			kfree(tv->data);
		}
	}

	return i;
}

/**
 * Verify Large-packet Local Loopback Support.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 *
 * Open port in local loopback mode and write a multiple packets in ascending
 * size and verify packet is received correctly.
 */
static int smux_ut_local_big_pkt(char *buf, int max)
{
	int i = 0;
	int ret;

	ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
			SMUX_CH_OPTION_LOCAL_LOOPBACK, 0);

	if (ret == 0) {
		smux_byte_loopback = SMUX_TEST_LCID;
		i += smux_ut_loopback_big_pkt(buf, max, __func__);
		smux_byte_loopback = 0;
	} else {
		i += scnprintf(buf + i, max - i,
				"%s: Unable to set loopback mode\n",
				__func__);
	}

	return i;
}

/**
 * Verify Large-packet Remote Loopback Support.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 *
 * Open port in remote loopback mode and write a multiple packets in ascending
 * size and verify packet is received correctly.
 */
static int smux_ut_remote_big_pkt(char *buf, int max)
{
	int i = 0;
	int ret;

	ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
			SMUX_CH_OPTION_REMOTE_LOOPBACK, 0);
	if (ret == 0) {
		i += smux_ut_loopback_big_pkt(buf, max, __func__);
	} else {
		i += scnprintf(buf + i, max - i,
				"%s: Unable to set loopback mode\n",
				__func__);
	}

	return i;
}

/**
 * Verify set and get operations for each TIOCM bit.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 * @name Name of the test for error reporting
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_tiocm(char *buf, int max, const char *name)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	static const struct tiocm_test_vector tiocm_vectors[] = {
		
		{TIOCM_DTR, TIOCM_DTR, TIOCM_DTR | TIOCM_DSR, TIOCM_DSR},
		{TIOCM_RTS, TIOCM_RTS, TIOCM_RTS | TIOCM_CTS, TIOCM_CTS},
		{TIOCM_RI, 0x0, TIOCM_RI, TIOCM_RI},
		{TIOCM_CD, 0x0, TIOCM_CD, TIOCM_CD},
	};
	int i = 0;
	int failed = 0;
	int n;
	int ret;

	i += scnprintf(buf + i, max - i, "Running %s\n", name);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	while (!failed) {
		
		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data, smux_mock_cb,
								get_rx_buffer);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		for (n = 0; n < ARRAY_SIZE(tiocm_vectors) && !failed; ++n) {
			
			ret = msm_smux_tiocm_set(SMUX_TEST_LCID,
						tiocm_vectors[n].input, 0x0);
			UT_ASSERT_INT(ret, ==, 0);
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
			UT_ASSERT_INT(cb_data.cb_count, ==, 1);
			UT_ASSERT_INT(cb_data.event_tiocm, ==, 1);
			UT_ASSERT_INT(cb_data.tiocm_meta.tiocm_old, ==,
						tiocm_vectors[n].set_old);
			UT_ASSERT_INT(cb_data.tiocm_meta.tiocm_new, ==,
						tiocm_vectors[n].set_new);
			mock_cb_data_reset(&cb_data);

			
			ret = msm_smux_tiocm_set(SMUX_TEST_LCID, 0x0,
						tiocm_vectors[n].input);
			UT_ASSERT_INT(ret, ==, 0);
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ),
				>, 0);
			UT_ASSERT_INT(cb_data.cb_count, ==, 1);
			UT_ASSERT_INT(cb_data.event_tiocm, ==, 1);
			UT_ASSERT_INT(cb_data.tiocm_meta.tiocm_old, ==,
						tiocm_vectors[n].clr_old);
			UT_ASSERT_INT(cb_data.tiocm_meta.tiocm_new, ==, 0x0);
			mock_cb_data_reset(&cb_data);
		}
		if (failed)
			break;

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 0);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", name);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}

	mock_cb_data_reset(&cb_data);
	return i;
}

/**
 * Verify TIOCM Status Bits for local loopback.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_local_tiocm(char *buf, int max)
{
	int i = 0;
	int ret;

	ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
			SMUX_CH_OPTION_LOCAL_LOOPBACK, 0);

	if (ret == 0) {
		smux_byte_loopback = SMUX_TEST_LCID;
		i += smux_ut_tiocm(buf, max, __func__);
		smux_byte_loopback = 0;
	} else {
		i += scnprintf(buf + i, max - i,
				"%s: Unable to set loopback mode\n",
				__func__);
	}

	return i;
}

/**
 * Verify TIOCM Status Bits for remote loopback.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_remote_tiocm(char *buf, int max)
{
	int i = 0;
	int ret;

	ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
			SMUX_CH_OPTION_REMOTE_LOOPBACK, 0);
	if (ret == 0) {
		i += smux_ut_tiocm(buf, max, __func__);
	} else {
		i += scnprintf(buf + i, max - i,
				"%s: Unable to set loopback mode\n",
				__func__);
	}

	return i;
}

/**
 * Verify High/Low Watermark notifications.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_local_wm(char *buf, int max)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	int i = 0;
	int failed = 0;
	int ret;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	pr_err("%s", buf);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	smux_byte_loopback = SMUX_TEST_LCID;
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_LOCAL_LOOPBACK
				| SMUX_CH_OPTION_REMOTE_TX_STOP,
				0);
		UT_ASSERT_INT(ret, ==, 0);

		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data, smux_mock_cb,
								get_rx_buffer);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		ret = 0;
		ret |= msm_smux_write(SMUX_TEST_LCID, (void *)1,
					test_array, sizeof(test_array));
		ret |= msm_smux_write(SMUX_TEST_LCID, (void *)2,
					test_array, sizeof(test_array));
		ret |= msm_smux_write(SMUX_TEST_LCID, (void *)3,
					test_array, sizeof(test_array));
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 0);
		UT_ASSERT_INT(cb_data.event_high_wm, ==, 0);

		ret = msm_smux_write(SMUX_TEST_LCID, (void *)4,
					test_array, sizeof(test_array));
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.event_high_wm, ==, 1);
		UT_ASSERT_INT(cb_data.event_low_wm, ==, 0);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_write(SMUX_TEST_LCID, (void *)5,
					test_array, sizeof(test_array));
		UT_ASSERT_INT(ret, ==, -EAGAIN);

		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				0, SMUX_CH_OPTION_REMOTE_TX_STOP);
		UT_ASSERT_INT(ret, ==, 0);
		while (cb_data.cb_count < 9) {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ),
				>, 0);
			INIT_COMPLETION(cb_data.cb_completion);
		}
		if (failed)
			break;

		UT_ASSERT_INT(cb_data.event_high_wm, ==, 0);
		UT_ASSERT_INT(cb_data.event_low_wm, ==, 1);
		UT_ASSERT_INT(cb_data.event_write_done, ==, 4);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 0);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}
	smux_byte_loopback = 0;
	mock_cb_data_reset(&cb_data);
	return i;
}

/**
 * Verify smuxld_receive_buf regular and error processing.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_local_smuxld_receive_buf(char *buf, int max)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	struct mock_read_event *meta;
	int i = 0;
	int failed = 0;
	int ret;
	char data[] = {SMUX_UT_ECHO_REQ,
		SMUX_UT_ECHO_REQ, SMUX_UT_ECHO_REQ,
	};
	char flags[] = {0x0, 0x1, 0x0,};


	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	smux_byte_loopback = SMUX_TEST_LCID;
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_LOCAL_LOOPBACK, 0);
		UT_ASSERT_INT(ret, ==, 0);

		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data, smux_mock_cb,
								get_rx_buffer);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		smuxld_receive_buf(0, data, flags, sizeof(data));

		
		do {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
			INIT_COMPLETION(cb_data.cb_completion);
		} while (cb_data.cb_count < 3);
		UT_ASSERT_INT(cb_data.cb_count, ==, 3);
		UT_ASSERT_INT(cb_data.event_read_done, ==, 3);

		meta = list_first_entry(&cb_data.read_events,
				struct mock_read_event, list);
		UT_ASSERT_INT((int)meta->meta.pkt_priv, ==,
				SMUX_UT_ECHO_ACK_OK);
		list_del(&meta->list);

		meta = list_first_entry(&cb_data.read_events,
				struct mock_read_event, list);
		UT_ASSERT_INT((int)meta->meta.pkt_priv, ==,
				SMUX_UT_ECHO_ACK_FAIL);
		list_del(&meta->list);

		meta = list_first_entry(&cb_data.read_events,
				struct mock_read_event, list);
		UT_ASSERT_INT((int)meta->meta.pkt_priv, ==,
				SMUX_UT_ECHO_ACK_OK);
		list_del(&meta->list);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 0);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}
	smux_byte_loopback = 0;
	mock_cb_data_reset(&cb_data);
	return i;
}

static int get_rx_buffer_mock(void *priv, void **pkt_priv,
		void **buffer, int size)
{
	void *rx_buf;
	unsigned long flags;
	struct smux_mock_callback *cb_ptr;

	cb_ptr = (struct smux_mock_callback *)priv;
	if (!cb_ptr) {
		pr_err("%s: no callback data\n", __func__);
		return -ENXIO;
	}

	if (get_rx_buffer_mock_fail) {
		
		struct mock_get_rx_buff_event *meta;
		meta = kmalloc(sizeof(struct mock_get_rx_buff_event),
				GFP_KERNEL);
		if (!meta) {
			pr_err("%s: unable to allocate metadata\n", __func__);
			return -ENOMEM;
		}
		INIT_LIST_HEAD(&meta->list);
		meta->size = size;
		meta->jiffies = jiffies;

		spin_lock_irqsave(&cb_ptr->lock, flags);
		++cb_ptr->get_rx_buff_retry_count;
		list_add_tail(&meta->list, &cb_ptr->get_rx_buff_retry_events);
		++cb_ptr->cb_count;
		complete(&cb_ptr->cb_completion);
		spin_unlock_irqrestore(&cb_ptr->lock, flags);
		return -EAGAIN;
	} else {
		rx_buf = kmalloc(size, GFP_KERNEL);
		*pkt_priv = (void *)0x1234;
		*buffer = rx_buf;
		return 0;
	}
	return 0;
}

/**
 * Verify get_rx_buffer callback retry.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_local_get_rx_buff_retry(char *buf, int max)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	int i = 0;
	int failed = 0;
	char try_two[] = "try 2";
	int ret;
	unsigned long start_j;
	struct mock_get_rx_buff_event *event;
	struct mock_read_event *read_event;
	int try;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	pr_err("%s", buf);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	smux_byte_loopback = SMUX_TEST_LCID;
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_LOCAL_LOOPBACK,
				SMUX_CH_OPTION_AUTO_REMOTE_TX_STOP);
		UT_ASSERT_INT(ret, ==, 0);

		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data,
				smux_mock_cb, get_rx_buffer_mock);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		get_rx_buffer_mock_fail = 1;
		start_j = jiffies;
		ret = msm_smux_write(SMUX_TEST_LCID, (void *)1,
					test_array, sizeof(test_array));
		UT_ASSERT_INT(ret, ==, 0);
		ret = msm_smux_write(SMUX_TEST_LCID, (void *)2,
					try_two, sizeof(try_two));
		UT_ASSERT_INT(ret, ==, 0);

		
		while (cb_data.event_read_failed == 0) {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, 2*HZ),
				>, 0);
			INIT_COMPLETION(cb_data.cb_completion);
		}
		if (failed)
			break;

		
		UT_ASSERT_INT(cb_data.get_rx_buff_retry_count, ==, 12);
		event = list_first_entry(&cb_data.get_rx_buff_retry_events,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				0, 0 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				1, 1 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				2, 2 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				4, 4 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				8, 8 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				16, 16 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				32 - 20, 32 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				64 - 20, 64 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				128 - 20, 128 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				256 - 20, 256 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				512 - 20, 512 + 20);
		start_j = event->jiffies;

		event = list_first_entry(&event->list,
				struct mock_get_rx_buff_event, list);
		pr_err("%s: event->jiffies = %d (ms)\n", __func__,
				jiffies_to_msecs(event->jiffies - start_j));
		UT_ASSERT_INT_IN_RANGE(
				jiffies_to_msecs(event->jiffies - start_j),
				1024 - 20, 1024 + 20);
		mock_cb_data_reset(&cb_data);

		
		get_rx_buffer_mock_fail = 0;
		INIT_COMPLETION(cb_data.cb_completion);
		if (cb_data.event_read_done == 0)
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ),
				>, 0);
		UT_ASSERT_INT(cb_data.event_read_done, ==, 1);
		UT_ASSERT_INT(list_empty(&cb_data.read_events), ==, 0);
		read_event = list_first_entry(&cb_data.read_events,
				struct mock_read_event, list);
		UT_ASSERT_PTR(read_event->meta.pkt_priv, ==, (void *)0x1234);
		UT_ASSERT_PTR(read_event->meta.buffer, !=, NULL);
		UT_ASSERT_INT(0, ==, memcmp(read_event->meta.buffer, try_two,
				sizeof(try_two)));
		mock_cb_data_reset(&cb_data);

		
		get_rx_buffer_mock_fail = 1;
		for (try = 0; try < (SMUX_RX_RETRY_MAX_PKTS + 1); ++try) {
			mock_cb_data_reset(&cb_data);
			ret = msm_smux_write(SMUX_TEST_LCID, (void *)1,
						test_array, sizeof(test_array));
			UT_ASSERT_INT(ret, ==, 0);
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ),
				>, 0);
		}

		
		while (cb_data.event_read_failed == 0) {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, 2*HZ),
				>, 0);
			INIT_COMPLETION(cb_data.cb_completion);
		}
		if (failed)
			break;

		get_rx_buffer_mock_fail = 0;
		while (cb_data.event_read_done < SMUX_RX_RETRY_MAX_PKTS) {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, 2*HZ),
				>, 0);
			INIT_COMPLETION(cb_data.cb_completion);
		}
		if (failed)
			break;

		UT_ASSERT_INT(1, ==, cb_data.event_read_failed);
		UT_ASSERT_INT(SMUX_RX_RETRY_MAX_PKTS, ==,
				cb_data.event_read_done);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 0);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}
	smux_byte_loopback = 0;
	mock_cb_data_reset(&cb_data);
	return i;
}

/**
 * Verify get_rx_buffer callback retry for auto-rx flow control.
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_local_get_rx_buff_retry_auto(char *buf, int max)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	int i = 0;
	int failed = 0;
	int ret;
	int try;
	int try_rx_retry_wm;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	pr_err("%s", buf);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	smux_byte_loopback = SMUX_TEST_LCID;
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_LOCAL_LOOPBACK
				| SMUX_CH_OPTION_AUTO_REMOTE_TX_STOP,
				0);
		UT_ASSERT_INT(ret, ==, 0);

		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data,
				smux_mock_cb, get_rx_buffer_mock);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		get_rx_buffer_mock_fail = 1;
		try_rx_retry_wm = 0;
		for (try = 0; try < SMUX_RX_RETRY_MAX_PKTS; ++try) {
			pr_err("%s: try %d\n", __func__, try);
			ret = msm_smux_write(SMUX_TEST_LCID, (void *)1,
						test_array, sizeof(test_array));
			UT_ASSERT_INT(ret, ==, 0);
			if (failed)
				break;

			if (!try_rx_retry_wm &&
					cb_data.event_rx_retry_high_wm) {
				
				try_rx_retry_wm = try + 1;
				break;
			}

			while (cb_data.event_write_done <= try) {
				UT_ASSERT_INT(
					(int)wait_for_completion_timeout(
						&cb_data.cb_completion, HZ),
					>, 0);
				INIT_COMPLETION(cb_data.cb_completion);
			}
			if (failed)
				break;
		}
		if (failed)
			break;

		
		UT_ASSERT_INT(cb_data.event_rx_retry_high_wm, ==, 1);
		UT_ASSERT_INT(try_rx_retry_wm, ==, SMUX_RX_WM_HIGH);

		get_rx_buffer_mock_fail = 0;
		while (cb_data.event_read_done < SMUX_RX_WM_HIGH) {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, 2*HZ),
				>, 0);
			INIT_COMPLETION(cb_data.cb_completion);
		}
		if (failed)
			break;

		UT_ASSERT_INT(0, ==, cb_data.event_read_failed);
		UT_ASSERT_INT(SMUX_RX_WM_HIGH, ==,
				cb_data.event_read_done);
		UT_ASSERT_INT(cb_data.event_rx_retry_low_wm, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 0);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_close(SMUX_TEST_LCID);
	}
	smux_byte_loopback = 0;
	mock_cb_data_reset(&cb_data);
	return i;
}

/**
 * Verify remote flow control (remote TX stop).
 *
 * @buf  Buffer for status message
 * @max  Size of buffer
 *
 * @returns Number of bytes written to @buf
 */
static int smux_ut_remote_tx_stop(char *buf, int max)
{
	static struct smux_mock_callback cb_data;
	static int cb_initialized;
	int i = 0;
	int failed = 0;
	int ret;

	i += scnprintf(buf + i, max - i, "Running %s\n", __func__);
	pr_err("%s", buf);

	if (!cb_initialized)
		mock_cb_data_init(&cb_data);

	mock_cb_data_reset(&cb_data);
	while (!failed) {
		
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_REMOTE_LOOPBACK, 0);
		UT_ASSERT_INT(ret, ==, 0);

		ret = msm_smux_open(SMUX_TEST_LCID, &cb_data, smux_mock_cb,
								get_rx_buffer);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ), >, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_connected, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_write(SMUX_TEST_LCID, (void *)1,
					test_array, sizeof(test_array));
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.event_write_done, ==, 1);

		INIT_COMPLETION(cb_data.cb_completion);
		if (!cb_data.event_read_done) {
			UT_ASSERT_INT(
				(int)wait_for_completion_timeout(
					&cb_data.cb_completion, HZ),
				>, 0);
		}
		UT_ASSERT_INT(cb_data.event_read_done, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		UT_ASSERT_INT(smux_lch[SMUX_TEST_LCID].tx_flow_control, ==, 0);
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				SMUX_CH_OPTION_REMOTE_TX_STOP, 0);
		UT_ASSERT_INT(ret, ==, 0);

		
		msleep(500);
		UT_ASSERT_INT(smux_lch[SMUX_TEST_LCID].tx_flow_control, ==, 1);
		smux_lch[SMUX_TEST_LCID].tx_flow_control = 0;

		
		ret = msm_smux_write(SMUX_TEST_LCID, (void *)2,
					test_array, sizeof(test_array));
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
				>, 0);
		INIT_COMPLETION(cb_data.cb_completion);
		UT_ASSERT_INT(cb_data.event_write_done, ==, 1);
		UT_ASSERT_INT(cb_data.event_read_done, ==, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);

		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, 1*HZ),
			==, 0);
		UT_ASSERT_INT(cb_data.event_read_done, ==, 0);
		mock_cb_data_reset(&cb_data);

		
		UT_ASSERT_INT(cb_data.event_read_done, ==, 0);
		ret = msm_smux_set_ch_option(SMUX_TEST_LCID,
				0, SMUX_CH_OPTION_REMOTE_TX_STOP);
		UT_ASSERT_INT(ret, ==, 0);

		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.event_read_done, ==, 1);
		mock_cb_data_reset(&cb_data);

		
		ret = msm_smux_close(SMUX_TEST_LCID);
		UT_ASSERT_INT(ret, ==, 0);
		UT_ASSERT_INT(
			(int)wait_for_completion_timeout(
				&cb_data.cb_completion, HZ),
			>, 0);
		UT_ASSERT_INT(cb_data.cb_count, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected, ==, 1);
		UT_ASSERT_INT(cb_data.event_disconnected_ssr, ==, 0);
		break;
	}

	if (!failed) {
		i += scnprintf(buf + i, max - i, "\tOK\n");
	} else {
		pr_err("%s: Failed\n", __func__);
		i += scnprintf(buf + i, max - i, "\tFailed\n");
		i += mock_cb_data_print(&cb_data, buf + i, max - i);
		msm_smux_set_ch_option(SMUX_TEST_LCID,
			0, SMUX_CH_OPTION_REMOTE_TX_STOP);
		msm_smux_close(SMUX_TEST_LCID);
	}
	mock_cb_data_reset(&cb_data);
	return i;
}

static char debug_buffer[DEBUG_BUFMAX];

static ssize_t debug_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	int (*fill)(char *buf, int max) = file->private_data;
	int bsize;

	if (*ppos != 0)
		return 0;

	bsize = fill(debug_buffer, DEBUG_BUFMAX);
	return simple_read_from_buffer(buf, count, ppos, debug_buffer, bsize);
}

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations debug_ops = {
	.read = debug_read,
	.open = debug_open,
};

static void debug_create(const char *name, mode_t mode,
			 struct dentry *dent,
			 int (*fill)(char *buf, int max))
{
	debugfs_create_file(name, mode, dent, fill, &debug_ops);
}

static int __init smux_debugfs_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("n_smux_test", 0);
	if (IS_ERR(dent))
		return PTR_ERR(dent);

	debug_create("ut_local_basic", 0444, dent, smux_ut_basic);
	debug_create("ut_remote_basic", 0444, dent,	smux_ut_remote_basic);
	debug_create("ut_local_big_pkt", 0444, dent, smux_ut_local_big_pkt);
	debug_create("ut_remote_big_pkt", 0444, dent, smux_ut_remote_big_pkt);
	debug_create("ut_local_tiocm", 0444, dent, smux_ut_local_tiocm);
	debug_create("ut_remote_tiocm", 0444, dent,	smux_ut_remote_tiocm);
	debug_create("ut_local_wm", 0444, dent, smux_ut_local_wm);
	debug_create("ut_local_smuxld_receive_buf", 0444, dent,
			smux_ut_local_smuxld_receive_buf);
	debug_create("ut_local_get_rx_buff_retry", 0444, dent,
			smux_ut_local_get_rx_buff_retry);
	debug_create("ut_local_get_rx_buff_retry_auto", 0444, dent,
			smux_ut_local_get_rx_buff_retry_auto);
	debug_create("ut_remote_ssr_basic", 0444, dent,
			smux_ut_remote_ssr_basic);
	debug_create("ut_remote_ssr_open", 0444, dent,
			smux_ut_remote_ssr_open);
	debug_create("ut_remote_ssr_rx_buff_retry", 0444, dent,
			smux_ut_remote_ssr_rx_buff_retry);
	debug_create("ut_remote_tx_stop", 0444, dent,
			smux_ut_remote_tx_stop);

	return 0;
}

late_initcall(smux_debugfs_init);

