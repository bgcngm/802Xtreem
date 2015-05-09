/* linux/arch/arm/mach-msm/display/tc2-panel.c
 *
 * Copyright (c) 2011 HTC.
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

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <asm/mach-types.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include <linux/msm_ion.h>
#include <mach/ion.h>
#include <linux/regulator/consumer.h>

#include "devices.h"
#include "board-tc2.h"
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include <asm/system_info.h>
#include <linux/leds.h>
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <mach/perflock.h>

#define RESOLUTION_WIDTH 544
#define RESOLUTION_HEIGHT 960

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE \
		(roundup((RESOLUTION_WIDTH * RESOLUTION_HEIGHT * 4), 4096) * 3) /* 4 bpp x 3 pages */
#else
#define MSM_FB_PRIM_BUF_SIZE \
		(roundup((RESOLUTION_WIDTH * RESOLUTION_HEIGHT * 4), 4096) * 2) /* 4 bpp x 2 pages */
#endif

/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1376 * 768 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

#define MDP_VSYNC_GPIO 0

#define MIPI_CMD_ORISE_QHD_PANEL_NAME	"mipi_cmd_orise_qhd"
//#define MIPI_CMD_ORISE_QHD_PANEL_NAME	"mipi_video_novatek_wvga"
#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	},
};

static int tc2_detect_panel(const char *name)
{
#if 0
	if (!strncmp(name, MIPI_CMD_ORISE_QHD_PANEL_NAME,
			strnlen(MIPI_CMD_ORISE_QHD_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
		PR_DISP_INFO("%s: Support (%s)\n",__func__, name);
		return 0;
	}


	PR_DISP_WARN("%s: not supported '%s'\n", __func__, name);
#endif
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = tc2_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init tc2_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));
}

#ifdef CONFIG_MSM_BUS_SCALING

static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
static struct msm_bus_vectors hdmi_as_primary_vectors[] = {
	/* If HDMI is used as primary */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 2000000000,
		.ib = 2000000000,
	},
};
static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
};
#else
static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};
#endif

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

#endif

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_max_clk = 200000000,
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_43,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.mdp_iommu_split_domain = 0,
	/* Command mode panel does not need this feature */
	.cont_splash_enabled = 0x00,
};

void __init tc2_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;
#endif
}

static bool dsi_power_on;
static bool first_inited = true;
static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_l2;
	static struct regulator *reg_l10;

	int rc;

	PR_DISP_INFO("%s: power %s.\n", __func__, on ? "on" : "off");

	if (!dsi_power_on) {
		/* MIPI DSI power */
		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8038_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		//V_LCM_3V3_EN
		reg_l10 = regulator_get(&msm_mipi_dsi1_device.dev,
				"8038_l10");
		if (IS_ERR(reg_l10)) {
			pr_err("could not get 8038_l10, rc = %ld\n",
				PTR_ERR(reg_l10));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l10, 3000000, 3000000);
		if (rc) {
			pr_err("set_voltage l10 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		//MSM_LCD_ID0, need to set OUTPUT LOW to select MIPI mode
		gpio_tlmm_config(GPIO_CFG(MSM_LCD_ID0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, 0), GPIO_CFG_ENABLE);

		dsi_power_on = true;

		if (first_inited) {
			first_inited = false;

			rc = regulator_set_optimum_mode(reg_l2, 100000);
			if (rc < 0) {
				pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
				return -EINVAL;
			}
			rc = regulator_enable(reg_l2);
			if (rc) {
				pr_err("enable l2 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			//V_LCM_3V3_EN
			rc = regulator_set_optimum_mode(reg_l10, 100000);
			if (rc < 0) {
				pr_err("set_optimum_mode l10 failed, rc=%d\n", rc);
				return -EINVAL;
			}
			rc = regulator_enable(reg_l10);
			if (rc) {
				pr_err("enable l10 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			return 0;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l10, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l10 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		//V_LCM_3V3_EN
		rc = regulator_enable(reg_l10);
		if (rc) {
			pr_err("enable l10 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		hr_msleep(10);

		gpio_set_value(MSM_V_LCMIO_1V8_EN, 1);
		hr_msleep(5);

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(10);
		gpio_set_value(MSM_LCD_RSTz, 0);
		hr_msleep(1);
		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(30);
	} else {
		gpio_set_value(MSM_LCD_RSTz, 0);
		usleep(150);
		gpio_set_value(MSM_V_LCMIO_1V8_EN, 0);

		hr_msleep(2);
		rc = regulator_disable(reg_l10);
		if (rc) {
			pr_err("disable reg_l10 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l10, 100);
			if (rc < 0) {
				pr_err("set_optimum_mode l10 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
			if (rc < 0) {
				pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_VSYNC_GPIO,
	.dsi_power_save = mipi_dsi_panel_power,
};

struct dcs_cmd_req cmdreq;
static struct mipi_dsi_panel_platform_data *mipi_tc2_pdata;

static struct perf_lock tc2_perf_lock;

static struct dsi_buf tc2_panel_tx_buf;
static struct dsi_buf tc2_panel_rx_buf;
static int wled_trigger_initialized;
static atomic_t lcd_power_state;
static atomic_t lcd_backlight_off;

static char sony_orise_001[] ={0x00, 0x00}; /* DTYPE_DCS_WRITE1 : address shift*/
static char sony_orise_002[] = {
        0xFF, 0x96, 0x01, 0x01}; /* DTYPE_DCS_LWRITE : 0x9600:0x96, 0x9601:0x01, 0x9602:0x01*/
static char sony_orise_003[] ={0x00, 0x80}; /* DTYPE_DCS_WRITE1 : address shift*/
static char sony_orise_004[] = {
        0xFF, 0x96, 0x01};
static char sony_inv_01[] = {0x00, 0xB3};
static char sony_inv_02[] = {0xC0, 0x50};
static char sony_timing1_01[] = {0x00, 0x80};
static char sony_timing1_02[] = {0xF3, 0x04};
static char sony_timing2_01[] = {0x00, 0xC0};
static char sony_timing2_02[] = {0xC2, 0xB0};
static char sony_pwrctl2_01[] = {0x00, 0xA0};
static char sony_pwrctl2_02[] = {
	0xC5, 0x04, 0x3A, 0x56,
	0x44, 0x44, 0x44, 0x44};
static char sony_pwrctl3_01[] = {0x00, 0xB0};
static char sony_pwrctl3_02[] = {
	0xC5, 0x04, 0x3A, 0x56,
	0x44, 0x44, 0x44, 0x44};

static char sony_gamma28_00[] ={0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static char sony_gamma28_01[] = {
	0xe1, 0x07, 0x10, 0x16,
	0x0F, 0x08, 0x0F, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; /* DTYPE_DCS_LWRITE :0xE100:0x11, 0xE101:0x19, 0xE102: 0x1e, ..., 0xff are padding for 4 bytes*/

static char sony_gamma28_02[] ={0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static char sony_gamma28_03[] = {
	0xe2, 0x07, 0x10, 0x16,
	0x0F, 0x08, 0x0F, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static char sony_gamma28_04[] ={0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma28_05[] = {
	0xe3, 0x19, 0x1D, 0x20,
	0x0C, 0x04, 0x0B, 0x0B,
	0x0A, 0x03, 0x07, 0x12,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static char sony_gamma28_06[] ={0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static char sony_gamma28_07[] = {
	0xe4, 0x19, 0x1D, 0x20,
	0x0C, 0x04, 0x0B, 0x0B,
	0x0A, 0x03, 0x07, 0x12,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static char sony_gamma28_08[] ={0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static char sony_gamma28_09[] = {
	0xe5, 0x07, 0x0F, 0x15,
	0x0D, 0x06, 0x0E, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x09, 0x0D, 0x0D, 0x06,
	0x00
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static char sony_gamma28_10[] ={0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static char sony_gamma28_11[] = {
	0xe6, 0x07, 0x0F, 0x15,
	0x0D, 0x06, 0x0E, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x09, 0x0D, 0x0D, 0x06,
	0x00
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static char pwm_freq_sel_cmds1[] = {0x00, 0xB4}; /* address shift to pwm_freq_sel */
static char pwm_freq_sel_cmds2[] = {0xC6, 0x00}; /* CABC command with parameter 0 */

static char pwm_dbf_cmds1[] = {0x00, 0xB1}; /* address shift to PWM DBF */
static char pwm_dbf_cmds2[] = {0xC6, 0x04}; /* CABC command-- DBF: [2:1], force duty: [0] */

static char sony_ce_table1[] = {
	0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40};

static char sony_ce_table2[] = {
	0xD5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xfe, 0xfd, 0xfd, 0xfd, 0x4f, 0x4f, 0x4e,
	0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4d,
	0x4d, 0x4d, 0x4d, 0x4d, 0x4d, 0x4d, 0x4e, 0x4e,
	0x4e, 0x4f, 0x4f, 0x4f, 0x50, 0x50, 0x50, 0x51,
	0x51, 0x51, 0x52, 0x52, 0x52, 0x53, 0x53, 0x53,
	0x54, 0x54, 0x54, 0x54, 0x55, 0x55, 0x55, 0x56,
	0x56, 0x56, 0x56, 0x56, 0x56, 0x56, 0x56, 0x55,
	0x55, 0x55, 0x55, 0x54, 0x54, 0x54, 0x54, 0x54,
	0x53, 0x53, 0x54, 0x54, 0x54, 0x55, 0x55, 0x55,
	0x55, 0x56, 0x56, 0x56, 0x57, 0x57, 0x57, 0x58,
	0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x59, 0x59,
	0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x5a, 0x5a,
	0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
	0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x59,
	0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x58,
	0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58,
	0x58, 0x59, 0x59, 0x59, 0x59, 0x59, 0x5a, 0x5a,
	0x5a, 0x5a, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5a, 0x59, 0x58, 0x58, 0x57,
	0x56, 0x55, 0x54, 0x54, 0x53, 0x52, 0x51, 0x50,
	0x50};

static char sony_ce_01[] = {0x00, 0x00};
static char sony_ce_02[] = {0xD6, 0x08};

#define ORISE_CABC
#ifdef ORISE_CABC
static char dsi_orise_dim_on_1[] = {0x53, 0x04};/* DTYPE_DCS_WRITE1 *///bkl_ctrl off and dimming off
static char dsi_orise_dim_on_2[] = {0x53, 0x2C};/* DTYPE_DCS_WRITE1 *///bkl_ctrl on and dimming on
static char dsi_orise_dim_off_1[] = {0x53, 0x0C};/* DTYPE_DCS_WRITE1 *///bkl_ctrl off and dimming on
static char dsi_orise_dim_off_2[] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 *///bkl_ctrl on and dimming off
static char dsi_orise_pwm3[] = {0x55, 0x01};/* DTYPE_DCS_WRITE1 *///CABC on. set to UI mode
#else
static char dsi_orise_pwm3[] = {0x55, 0x00};/* DTYPE_DCS_WRITE1 *///CABC off
#endif
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */

static char led_pwm1[] = {0x51, 0x00}; /* DTYPE_DCS_WRITE1 */
static char dsi_orise_pwm2[] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 *///bkl on and no dim

#ifdef ORISE_CABC
static struct dsi_cmd_desc sony_orise_dim_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_dim_on_1), dsi_orise_dim_on_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_dim_on_2), dsi_orise_dim_on_2},
};

static struct dsi_cmd_desc sony_orise_dim_off_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_dim_off_1), dsi_orise_dim_off_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_dim_off_2), dsi_orise_dim_off_2},
};
#endif

static struct dsi_cmd_desc sony_orise_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_orise_color_enhance[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_ce_table1), sony_ce_table1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_ce_table2), sony_ce_table2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_02), sony_ce_02},
};

static struct dsi_cmd_desc sony_orise_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_001), sony_orise_001},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise_002), sony_orise_002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_003), sony_orise_003},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise_004), sony_orise_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_inv_01), sony_inv_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_inv_02), sony_inv_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing1_01), sony_timing1_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing1_02), sony_timing1_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing2_01), sony_timing2_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing2_02), sony_timing2_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_pwrctl2_01), sony_pwrctl2_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_pwrctl2_02), sony_pwrctl2_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_pwrctl3_01), sony_pwrctl3_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_pwrctl3_02), sony_pwrctl3_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_001), sony_orise_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_00), sony_gamma28_00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_01), sony_gamma28_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_02), sony_gamma28_02},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_03), sony_gamma28_03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_04), sony_gamma28_04},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_05), sony_gamma28_05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_06), sony_gamma28_06},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_07), sony_gamma28_07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_08), sony_gamma28_08},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_09), sony_gamma28_09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_10), sony_gamma28_10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_11), sony_gamma28_11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_001), sony_orise_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_freq_sel_cmds1), pwm_freq_sel_cmds1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_freq_sel_cmds2), pwm_freq_sel_cmds2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_dbf_cmds1), pwm_dbf_cmds1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_dbf_cmds2), pwm_dbf_cmds2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_ce_table1), sony_ce_table1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_ce_table2), sony_ce_table2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_02), sony_ce_02},
#if 0
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(orise_panel_Set_TE_Line), orise_panel_Set_TE_Line},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(orise_panel_TE_Enable), orise_panel_TE_Enable},
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(max_pktsize), max_pktsize},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_video_mode1), no_video_mode1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_video_mode2), no_video_mode2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_wait_te1), no_wait_te1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_wait_te2), no_wait_te2},
#endif
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_pwm2), dsi_orise_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_pwm3), dsi_orise_pwm3},
	{DTYPE_DCS_WRITE,  1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sony_orise_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};
static struct dsi_cmd_desc sony_orise_display_on_cmds[] = {  //brandon, check the value
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};


static struct dsi_cmd_desc sony_orise_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
};

static int tc2_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct msm_panel_info *pinfo;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;
	mipi  = &mfd->panel_info.mipi;

	if (mfd->init_mipi_lcd == 0) {
		PR_DISP_DEBUG("Display On - 1st time\n");

		/* Enable color enhancement for booting up */
		if (!is_perf_lock_active(&tc2_perf_lock))
			perf_lock(&tc2_perf_lock);
//		mipi_dsi_cmds_tx(&tc2_panel_tx_buf, sony_orise_color_enhance,
//			ARRAY_SIZE(sony_orise_color_enhance));
		cmdreq.cmds = sony_orise_color_enhance;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_orise_color_enhance);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		if (is_perf_lock_active(&tc2_perf_lock))
			perf_unlock(&tc2_perf_lock);

		mfd->init_mipi_lcd = 1;
		return 0;
	}

	PR_DISP_INFO("Display On \n");

	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmds_tx(&tc2_panel_tx_buf, sony_orise_video_on_cmds,
			ARRAY_SIZE(sony_orise_video_on_cmds));
	} else {
		if (!is_perf_lock_active(&tc2_perf_lock))
			perf_lock(&tc2_perf_lock);
		mipi_dsi_cmds_tx(&tc2_panel_tx_buf, sony_orise_cmd_on_cmds,
			ARRAY_SIZE(sony_orise_cmd_on_cmds));
		if (is_perf_lock_active(&tc2_perf_lock))
			perf_unlock(&tc2_perf_lock);
	}

	atomic_set(&lcd_power_state, 1);

	PR_DISP_DEBUG("Init done\n");

	return 0;
}

static int tc2_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	return 0;
}

static int __devinit tc2_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (pdev->id == 0) {
		mipi_tc2_pdata = pdev->dev.platform_data;

		if (mipi_tc2_pdata
			&& mipi_tc2_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_tc2_pdata->phy_ctrl_settings);
		}

		if (mipi_tc2_pdata
			&& mipi_tc2_pdata->dlane_swap) {
			dlane_swap = (mipi_tc2_pdata->dlane_swap);
		}

		perf_lock_init(&tc2_perf_lock, TYPE_PERF_LOCK, PERF_LOCK_HIGHEST, "tc2");
		return 0;
	}

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) {
		mfd = platform_get_drvdata(current_pdev);
		if (!mfd)
			return -ENODEV;
		if (mfd->key != MFD_KEY)
			return -EINVAL;

		mipi  = &mfd->panel_info.mipi;

		if (phy_settings != NULL)
			mipi->dsi_phy_db = phy_settings;

		if (dlane_swap)
			mipi->dlane_swap = dlane_swap;
	}
	return 0;
}

static void tc2_display_on(struct msm_fb_data_type *mfd)
{
    /* The Orise-Sony panel need to set display on after first frame sent */

	mipi_dsi_op_mode_config(DSI_CMD_MODE);

	cmdreq.cmds = sony_orise_display_on_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_orise_display_on_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

DEFINE_LED_TRIGGER(bkl_led_trigger);

static void tc2_display_off(struct msm_fb_data_type *mfd)
{
	if (!is_perf_lock_active(&tc2_perf_lock))
		perf_lock(&tc2_perf_lock);

	cmdreq.cmds = sony_orise_display_off_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_orise_display_off_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	/* workaround : framework set backlight as 0 too late when suspend, it will cause wled can't be disabled */
	if (wled_trigger_initialized)
		led_trigger_event(bkl_led_trigger, 0);
	else
		PR_DISP_ERR("%s: wled trigger is not initialized!\n", __func__);

	if (is_perf_lock_active(&tc2_perf_lock))
		perf_unlock(&tc2_perf_lock);

	atomic_set(&lcd_power_state, 0);

	PR_DISP_INFO("%s\n", __func__);
}

#ifdef ORISE_CABC
static void tc2_dim_on(struct msm_fb_data_type *mfd)
{
	if (atomic_read(&lcd_backlight_off)) {
		PR_DISP_DEBUG("%s: backlight is off. Skip dimming setting\n", __func__);
		return;
	}

	PR_DISP_DEBUG("%s\n",  __FUNCTION__);

	mipi_dsi_op_mode_config(DSI_CMD_MODE);

	if (!is_perf_lock_active(&tc2_perf_lock))
		perf_lock(&tc2_perf_lock);
	cmdreq.cmds = sony_orise_dim_on_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_orise_dim_on_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
	if (is_perf_lock_active(&tc2_perf_lock))
		perf_unlock(&tc2_perf_lock);

}
#endif

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static unsigned char tc2_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	pwm_min = 7;
	pwm_default = 86;
	pwm_max = 255;


	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
			shrink_br = pwm_min;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
			shrink_br = (val - BRI_SETTING_MIN) * (pwm_default - pwm_min) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + pwm_min;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
			shrink_br = (val - BRI_SETTING_DEF) * (pwm_max - pwm_default) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + pwm_default;
	} else if (val > BRI_SETTING_MAX)
			shrink_br = pwm_max;

	PR_DISP_INFO("brightness orig=%d, transformed=%d\n", val, shrink_br);

	return shrink_br;
}

static void tc2_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;
///HTC:
	led_pwm1[1] = tc2_shrink_pwm((unsigned char)(mfd->bl_level));
///:HTC

	if (mipi_tc2_pdata && (mipi_tc2_pdata->enable_wled_bl_ctrl)
	    && (wled_trigger_initialized)) {
		led_trigger_event(bkl_led_trigger, led_pwm1[1]);
		return;
	}
	mipi  = &mfd->panel_info.mipi;
	pr_debug("%s+:bl=%d \n", __func__, mfd->bl_level);

	/* Check LCD power state */
	if (atomic_read(&lcd_power_state) == 0) {
		PR_DISP_DEBUG("%s: LCD is off. Skip backlight setting\n", __func__);
		return;
	}

	/* mdp4_dsi_cmd_busy_wait: will turn on dsi clock also */

	if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
	}
#ifdef ORISE_CABC
	/* Turn off dimming before suspend */
	if (led_pwm1[1] == 0) {
		atomic_set(&lcd_backlight_off, 1);
		cmdreq.cmds = sony_orise_dim_off_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_orise_dim_off_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
	} else
		atomic_set(&lcd_backlight_off, 0);
#endif

	cmdreq.cmds = sony_orise_cmd_backlight_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_orise_cmd_backlight_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

///HTC:
#ifdef CONFIG_BACKLIGHT_WLED_CABC
	/* For WLED CABC, To switch on/off WLED module */
	if (wled_trigger_initialized) {
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
	}
#endif
///:HTC
	return;
}

static struct mipi_dsi_panel_platform_data tc2_pdata = {
	.dlane_swap = 1,
#ifdef CONFIG_BACKLIGHT_WLED_CABC
	.enable_wled_bl_ctrl = 0x0,
#else
	.enable_wled_bl_ctrl = 0x1,
#endif
};

static struct platform_device mipi_dsi_tc2_panel_device = {
	.name = "mipi_tc2",
	.id = 0,
	.dev = {
		.platform_data = &tc2_pdata,
	}
};

static struct platform_driver this_driver = {
	.probe  = tc2_lcd_probe,
	.driver = {
		.name   = "mipi_tc2",
	},
};

static struct msm_fb_panel_data tc2_panel_data = {
	.on		= tc2_lcd_on,
	.off		= tc2_lcd_off,
	.set_backlight  = tc2_set_backlight,
	.display_on	= tc2_display_on,
	.display_off	= tc2_display_off,
#ifdef ORISE_CABC
	.dimming_on	= tc2_dim_on,
#endif
};

static struct msm_panel_info pinfo;
static int ch_used[3];

int mipi_tc2_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_tc2", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	tc2_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &tc2_panel_data,
		sizeof(tc2_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static struct mipi_dsi_phy_ctrl dsi_cmd_mode_phy_db = {
#if 0
/* DSI_BIT_CLK at 507MHz, 4 lane, RGB888 */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0x8c, 0x34, 0x15, 0x00, 0x46, 0x50, 0x1a, 0x38,
	0x24, 0x03, 0x04, 0xa0},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
		{0x0, 0xf9, 0x30, 0xda, 0x00, 0x40, 0x03, 0x62,
	0x40, 0x07, 0x03,
	0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
#else
	/* DSI_BIT_CLK at 482MHz, 2 lane, RGB888 */
	/* regulator *//* off=0x0500 */
	{0x09, 0x08, 0x05, 0x00, 0x20},
	/* timing *//* off=0x0440 */
	//clk_rate:600MHz
	{0xb9, 0x2A, 0x20, 0x00, 0x24, 0x50, 0x1D, 0x2A, 0x24,
	0x03, 0x04, 0xa0},
	/* phy ctrl *//* off=0x0470 */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength *//* off=0x0480 */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control *//* off=0x0200 */
	{0x0, 0xe, 0x30, 0xda, 0x00, 0x10, 0x0f, 0x61,
	0x40, 0x07, 0x03,
	0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02},
#endif
};

static int __init mipi_cmd_sony_orise_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 56;
	pinfo.height = 99;

	pinfo.lcdc.h_back_porch = 22;
	pinfo.lcdc.h_front_porch = 22;
	pinfo.lcdc.h_pulse_width = 1;
	pinfo.lcdc.v_back_porch = 3;
	pinfo.lcdc.v_front_porch = 3;
	pinfo.lcdc.v_pulse_width = 1;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;

	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.camera_backlight = 185;
	pinfo.clk_rate = 482000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6096; /* adjust refx100 to prevent tearing */

	pinfo.lcd.v_back_porch = 2;
	pinfo.lcd.v_front_porch = 2;
	pinfo.lcd.v_pulse_width = 2;

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.esc_byte_ratio = 4;

	pinfo.mipi.t_clk_post = 0x0a;
	pinfo.mipi.t_clk_pre = 0x21;
	pinfo.mipi.stream = 0;	/* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; /* TE from vsycn gpio */
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.tx_eot_append = 1; /* to prevent flicker and color shift */
	pinfo.mipi.dsi_phy_db = &dsi_cmd_mode_phy_db;

	ret = mipi_tc2_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	return ret;
}

void __init tc2_init_fb(void)
{
	platform_device_register(&msm_fb_device);

	if(panel_type != PANEL_ID_NONE && board_mfg_mode() != 5) {
		PR_DISP_DEBUG("system_rev = %d\n", system_rev);
		/* XA (system_rev = 0): PMIC,
		   XB (system_rev = 1): External IC */
		if (system_rev >= 1)
			tc2_pdata.enable_wled_bl_ctrl = 0x0;

		platform_device_register(&mipi_dsi_tc2_panel_device);
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	}
}

static int __init tc2_init_panel(void)
{
	if(panel_type == PANEL_ID_NONE || board_mfg_mode() == 5) {
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	led_trigger_register_simple("bkl_trigger", &bkl_led_trigger);
	pr_info("%s: SUCCESS (WLED TRIGGER)\n", __func__);
	wled_trigger_initialized = 1;
	atomic_set(&lcd_power_state, 1);

	mipi_dsi_buf_alloc(&tc2_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&tc2_panel_rx_buf, DSI_BUF_SIZE);

	mipi_cmd_sony_orise_init();

	return platform_driver_register(&this_driver);
}

device_initcall_sync(tc2_init_panel);
