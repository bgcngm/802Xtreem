/* linux/arch/arm/mach-msm/display/zip_cl-panel.c
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
#include "board-zip_cl.h"
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include <asm/system_info.h>
#include <linux/leds.h>
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <mach/perflock.h>

#define RESOLUTION_WIDTH 768
#define RESOLUTION_HEIGHT 1376

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

#define MIPI_CMD_HIMAX_720P_PANEL_NAME	"mipi_cmd_himax_720p"
#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	},
};

static int zip_cl_detect_panel(const char *name)
{
#if 0
	if (!strncmp(name, MIPI_CMD_HIMAX_720P_PANEL_NAME,
			strnlen(MIPI_CMD_HIMAX_720P_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
		PR_DISP_INFO("%s: Support (%s)\n",__func__, name);
		return 0;
	}


	PR_DISP_WARN("%s: not supported '%s'\n", __func__, name);
#endif
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = zip_cl_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init zip_cl_allocate_fb_region(void)
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

void __init zip_cl_mdp_writeback(struct memtype_reserve* reserve_table)
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

		gpio_set_value(MSM_LCD_RSTz, 0);
		hr_msleep(5);

		gpio_set_value(MSM_LCMIO_1V8_EN, 1);
		hr_msleep(1);

		//V_LCM_3V3_EN
		rc = regulator_enable(reg_l10);
		if (rc) {
			pr_err("enable l10 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		hr_msleep(12);

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(1);
		gpio_set_value(MSM_LCD_RSTz, 0);
		hr_msleep(1);
		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(25);
	} else {
		gpio_set_value(MSM_LCD_RSTz, 0);
		hr_msleep(12);

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

		hr_msleep(5);
		gpio_set_value(MSM_LCMIO_1V8_EN, 0);

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
static struct mipi_dsi_panel_platform_data *mipi_zip_cl_pdata;

static struct perf_lock zip_cl_perf_lock;

static struct dsi_buf zip_cl_panel_tx_buf;
static struct dsi_buf zip_cl_panel_rx_buf;
static struct dsi_cmd_desc *init_on_cmds = NULL;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *backlight_cmds = NULL;
static int init_on_cmds_count = 0;
static int display_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static int backlight_cmds_count = 0;
static int wled_trigger_initialized;
static atomic_t lcd_power_state;

/* mipi commands */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */

static char led_pwm1[] = {0x51, 0xff}; /* backlight value */
static char led_pwm2[] = {0x53, 0x24}; /* backlight control */
static char pwm_off[]  = {0x51, 0x00};

static char himax_b9[] = {0xB9, 0xFF, 0x83, 0x92}; /* initial setting  */
static char himax_d4[] = {0xD4, 0x00}; /* EQ function enable */
static char himax_ba[] = {0xBA, 0x12, 0x83, 0x00, 0xD6, 0xC6, 0x00, 0x0A}; /* dsi setting */
static char himax_c0[] = {0xC0, 0x01, 0x94}; /* STBA setting */
static char himax_c6[] = {0xC6, 0x35, 0x00, 0x00, 0x04}; /* flash issue */
static char himax_d5[] = {0xD5, 0x00, 0x00, 0x02}; /* EQ delay */
static char himax_bf[] = {0xBF, 0x05, 0x60, 0x02}; /* PTBF setting */
//static char himax_b2[] = {0xB2, 0x0F, 0xC8, 0x04, 0x0C, 0x04}; /* 60fps */
static char himax_35[] = {0x35, 0x00}; /* TE enable */
static char himax_c2[] = {0xC2, 0x08}; /* display mode */
//static char himax_36[] = {0x36, 0x03}; //some panel needed, correct it in feture
static char himax_55[] = {0x55, 0x03};
static char cabc_UI[] = {
	0xCA, 0x2D, 0x27, 0x26,
	0x25, 0x25, 0x25, 0x21,
	0x20, 0x20};
#ifdef CONFIG_MSM_CABC_VIDEO_ENHANCE
static char cabc_moving[] = {
	0xCA, 0x40, 0x3C, 0x38,
	0x34, 0x33, 0x32, 0x2D,
	0x24, 0x20};
#endif
static char himax_e3[] = {0xE3, 0x01};
static char himax_e5[] = {
	0xE5, 0x00, 0x04, 0x0B,
	0x05, 0x05, 0x00, 0x80,
	0x20, 0x80, 0x10, 0x00,
	0x07, 0x07, 0x07, 0x07,
	0x07, 0x80, 0x0A};
static char himax_c9[] = {0xC9, 0x1F, 0x00, 0x1E, 0x3F, 0x00, 0x80}; /* pwm setting */

/* AUO panel with Novatek SY35590 */

static char cmd2_page0_0[] = {0xFF, 0x01};
static char cmd2_page0_1[] = {0xFB, 0x01};
static char cmd2_page0_data00[] = {0x00, 0x4A};
static char cmd2_page0_data01[] = {0x01, 0x33};
static char cmd2_page0_data02[] = {0x02, 0x53};
static char cmd2_page0_data03[] = {0x03, 0x55};
static char cmd2_page0_data04[] = {0x04, 0x55};
static char cmd2_page0_data05[] = {0x05, 0x33};
static char cmd2_page0_data06[] = {0x06, 0x22};
static char cmd2_page0_data07[] = {0x08, 0x56};
static char cmd2_page0_data08[] = {0x09, 0x8F};
static char cmd2_page0_data09[] = {0x0B, 0x97};
static char cmd2_page0_data10[] = {0x0C, 0x97};
static char cmd2_page0_data11[] = {0x0D, 0x2F};
static char cmd2_page0_data12[] = {0x0E, 0x24};
static char cmd2_page0_data13[] = {0x11, 0x80};
static char cmd2_page0_data14[] = {0x12, 0x03};
static char cmd2_page0_data15[] = {0x36, 0x73};
static char cmd2_page0_data16[] = {0x0F, 0x04};

static char cmd3_0[] = {0xFF, 0xEE};
static char cmd3_1[] = {0xFB, 0x01};
static char cmd3_data00[] = {0x04, 0xAD};
static char cmd3_data01[] = {0xFF, 0x00};

static char cmd2_page4_0[] = {0xFF, 0x05};
static char cmd2_page4_1[] = {0xFB, 0x01};
static char cmd2_page4_data00[] = {0x01, 0x00};
static char cmd2_page4_data01[] = {0x02, 0x82};
static char cmd2_page4_data02[] = {0x03, 0x82};
static char cmd2_page4_data03[] = {0x04, 0x82};
static char cmd2_page4_data04[] = {0x05, 0x30};
static char cmd2_page4_data05[] = {0x06, 0x33};
static char cmd2_page4_data06[] = {0x07, 0x01};
static char cmd2_page4_data07[] = {0x08, 0x00};
static char cmd2_page4_data08[] = {0x09, 0x46};
static char cmd2_page4_data09[] = {0x0A, 0x46};
static char cmd2_page4_data10[] = {0x0D, 0x0B};
static char cmd2_page4_data11[] = {0x0E, 0x1D};
static char cmd2_page4_data12[] = {0x0F, 0x08};
static char cmd2_page4_data13[] = {0x10, 0x53};
static char cmd2_page4_data14[] = {0x11, 0x00};
static char cmd2_page4_data15[] = {0x12, 0x00};
static char cmd2_page4_data16[] = {0x14, 0x01};
static char cmd2_page4_data17[] = {0x15, 0x00};
static char cmd2_page4_data18[] = {0x16, 0x05};
static char cmd2_page4_data19[] = {0x17, 0x00};
static char cmd2_page4_data20[] = {0x19, 0x7F};
static char cmd2_page4_data21[] = {0x1A, 0xFF};
static char cmd2_page4_data22[] = {0x1B, 0x0F};
static char cmd2_page4_data23[] = {0x1C, 0x00};
static char cmd2_page4_data24[] = {0x1D, 0x00};
static char cmd2_page4_data25[] = {0x1E, 0x00};
static char cmd2_page4_data26[] = {0x1F, 0x07};
static char cmd2_page4_data27[] = {0x20, 0x00};
static char cmd2_page4_data28[] = {0x21, 0x00};
static char cmd2_page4_data29[] = {0x22, 0x55};
static char cmd2_page4_data30[] = {0x23, 0x4D};
static char cmd2_page4_data31[] = {0x6C, 0x00};
static char cmd2_page4_data32[] = {0x6D, 0x00};
static char cmd2_page4_data33[] = {0x2D, 0x02};
static char cmd2_page4_data34[] = {0x83, 0x02};
static char cmd2_page4_data35[] = {0x9E, 0x58};
static char cmd2_page4_data36[] = {0x9F, 0x6A};
static char cmd2_page4_data37[] = {0xA0, 0x41};
static char cmd2_page4_data38[] = {0xA2, 0x10};
static char cmd2_page4_data39[] = {0xBB, 0x0A};
static char cmd2_page4_data40[] = {0xBC, 0x0A};
static char cmd2_page4_data41[] = {0x28, 0x01};
static char cmd2_page4_data42[] = {0x2F, 0x02};
static char cmd2_page4_data43[] = {0x32, 0x08};
static char cmd2_page4_data44[] = {0x33, 0xB8};
static char cmd2_page4_data45[] = {0x36, 0x02};
static char cmd2_page4_data46[] = {0x37, 0x00};
static char cmd2_page4_data47[] = {0x43, 0x00};
static char cmd2_page4_data48[] = {0x4B, 0x21};
static char cmd2_page4_data49[] = {0x4C, 0x03};
static char cmd2_page4_data50[] = {0x50, 0x21};
static char cmd2_page4_data51[] = {0x51, 0x03};
static char cmd2_page4_data52[] = {0x58, 0x21};
static char cmd2_page4_data53[] = {0x59, 0x03};
static char cmd2_page4_data54[] = {0x5D, 0x21};
static char cmd2_page4_data55[] = {0x5E, 0x03};
#if 0
static char cmd2_page4_data56[] = {0xFF, 0x03};
static char cmd2_page4_data57[] = {0x25, 0x26};
static char cmd2_page4_data58[] = {0x00, 0x05};
static char cmd2_page4_data59[] = {0x01, 0x05};
static char cmd2_page4_data60[] = {0x02, 0x10};
static char cmd2_page4_data61[] = {0x03, 0x17};
static char cmd2_page4_data62[] = {0x04, 0x22};
static char cmd2_page4_data63[] = {0x05, 0x26};
static char cmd2_page4_data64[] = {0x06, 0x29};
static char cmd2_page4_data65[] = {0x07, 0x29};
static char cmd2_page4_data66[] = {0x08, 0x26};
static char cmd2_page4_data67[] = {0x09, 0x23};
static char cmd2_page4_data68[] = {0x0A, 0x17};
static char cmd2_page4_data69[] = {0x0B, 0x12};
static char cmd2_page4_data70[] = {0x0C, 0x06};
static char cmd2_page4_data71[] = {0x0D, 0x02};
static char cmd2_page4_data72[] = {0x0E, 0x01};
static char cmd2_page4_data73[] = {0x0F, 0x00};
#endif

#if 0
static char cmd2_page0_data000[] = {0x75, 0x00};
static char cmd2_page0_data001[] = {0x76, 0xFA};
static char cmd2_page0_data002[] = {0x77, 0x00};
static char cmd2_page0_data003[] = {0x78, 0xFE};
static char cmd2_page0_data004[] = {0x79, 0x01};
static char cmd2_page0_data005[] = {0x7A, 0x07};
static char cmd2_page0_data006[] = {0x7B, 0x01};
static char cmd2_page0_data007[] = {0x7C, 0x10};
static char cmd2_page0_data008[] = {0x7D, 0x01};
static char cmd2_page0_data009[] = {0x7E, 0x19};
static char cmd2_page0_data010[] = {0x7F, 0x01};
static char cmd2_page0_data011[] = {0x80, 0x21};
static char cmd2_page0_data012[] = {0x81, 0x01};
static char cmd2_page0_data013[] = {0x82, 0x28};
static char cmd2_page0_data014[] = {0x83, 0x01};
static char cmd2_page0_data015[] = {0x84, 0x30};
static char cmd2_page0_data016[] = {0x85, 0x01};
static char cmd2_page0_data017[] = {0x86, 0x37};
static char cmd2_page0_data018[] = {0x87, 0x01};
static char cmd2_page0_data019[] = {0x88, 0x52};
static char cmd2_page0_data020[] = {0x89, 0x01};
static char cmd2_page0_data021[] = {0x8A, 0x6A};
static char cmd2_page0_data022[] = {0x8B, 0x01};
static char cmd2_page0_data023[] = {0x8C, 0x93};
static char cmd2_page0_data024[] = {0x8D, 0x01};
static char cmd2_page0_data025[] = {0x8E, 0xB7};
static char cmd2_page0_data026[] = {0x8F, 0x02};
static char cmd2_page0_data027[] = {0x90, 0xF2};
static char cmd2_page0_data028[] = {0x91, 0x02};
static char cmd2_page0_data029[] = {0x92, 0x23};
static char cmd2_page0_data030[] = {0x93, 0x02};
static char cmd2_page0_data031[] = {0x94, 0x24};
static char cmd2_page0_data032[] = {0x95, 0x02};
static char cmd2_page0_data033[] = {0x96, 0x53};
static char cmd2_page0_data034[] = {0x97, 0x02};
static char cmd2_page0_data035[] = {0x98, 0x89};
static char cmd2_page0_data036[] = {0x99, 0x02};
static char cmd2_page0_data037[] = {0x9A, 0xAC};
static char cmd2_page0_data038[] = {0x9B, 0x02};
static char cmd2_page0_data039[] = {0x9C, 0xDD};
static char cmd2_page0_data040[] = {0x9D, 0x02};
static char cmd2_page0_data041[] = {0x9E, 0xFD};
static char cmd2_page0_data042[] = {0x9F, 0x03};
static char cmd2_page0_data043[] = {0xA0, 0x26};
static char cmd2_page0_data044[] = {0xA2, 0x03};
static char cmd2_page0_data045[] = {0xA3, 0x32};
static char cmd2_page0_data046[] = {0xA4, 0x03};
static char cmd2_page0_data047[] = {0xA5, 0x3F};
static char cmd2_page0_data048[] = {0xA6, 0x03};
static char cmd2_page0_data049[] = {0xA7, 0x4C};
static char cmd2_page0_data050[] = {0xA9, 0x03};
static char cmd2_page0_data051[] = {0xAA, 0x5B};
static char cmd2_page0_data052[] = {0xAB, 0x03};
static char cmd2_page0_data053[] = {0xAC, 0x68};
static char cmd2_page0_data054[] = {0xAD, 0x03};
static char cmd2_page0_data055[] = {0xAE, 0x70};
static char cmd2_page0_data056[] = {0xAF, 0x03};
static char cmd2_page0_data057[] = {0xB0, 0x73};
static char cmd2_page0_data058[] = {0xB1, 0x03};
static char cmd2_page0_data059[] = {0xB2, 0xFF};
static char cmd2_page0_data060[] = {0xB3, 0x00};
static char cmd2_page0_data061[] = {0xB4, 0xFA};
static char cmd2_page0_data062[] = {0xB5, 0x00};
static char cmd2_page0_data063[] = {0xB6, 0xFE};
static char cmd2_page0_data064[] = {0xB7, 0x01};
static char cmd2_page0_data065[] = {0xB8, 0x07};
static char cmd2_page0_data066[] = {0xB9, 0x01};
static char cmd2_page0_data067[] = {0xBA, 0x10};
static char cmd2_page0_data068[] = {0xBB, 0x01};
static char cmd2_page0_data069[] = {0xBC, 0x19};
static char cmd2_page0_data070[] = {0xBD, 0x01};
static char cmd2_page0_data071[] = {0xBE, 0x21};
static char cmd2_page0_data072[] = {0xBF, 0x01};
static char cmd2_page0_data073[] = {0xC0, 0x28};
static char cmd2_page0_data074[] = {0xC1, 0x01};
static char cmd2_page0_data075[] = {0xC2, 0x30};
static char cmd2_page0_data076[] = {0xC3, 0x01};
static char cmd2_page0_data077[] = {0xC4, 0x37};
static char cmd2_page0_data078[] = {0xC5, 0x01};
static char cmd2_page0_data079[] = {0xC6, 0x52};
static char cmd2_page0_data080[] = {0xC7, 0x01};
static char cmd2_page0_data081[] = {0xC8, 0x6A};
static char cmd2_page0_data082[] = {0xC9, 0x01};
static char cmd2_page0_data083[] = {0xCA, 0x93};
static char cmd2_page0_data084[] = {0xCB, 0x01};
static char cmd2_page0_data085[] = {0xCC, 0xB7};
static char cmd2_page0_data086[] = {0xCD, 0x01};
static char cmd2_page0_data087[] = {0xCE, 0xF2};
static char cmd2_page0_data088[] = {0xCF, 0x02};
static char cmd2_page0_data089[] = {0xD0, 0x23};
static char cmd2_page0_data090[] = {0xD1, 0x02};
static char cmd2_page0_data091[] = {0xD2, 0x24};
static char cmd2_page0_data092[] = {0xD3, 0x02};
static char cmd2_page0_data093[] = {0xD4, 0x53};
static char cmd2_page0_data094[] = {0xD5, 0x02};
static char cmd2_page0_data095[] = {0xD6, 0x89};
static char cmd2_page0_data096[] = {0xD7, 0x02};
static char cmd2_page0_data097[] = {0xD8, 0xAC};
static char cmd2_page0_data098[] = {0xD9, 0x02};
static char cmd2_page0_data099[] = {0xDA, 0xDD};
static char cmd2_page0_data100[] = {0xDB, 0x02};
static char cmd2_page0_data101[] = {0xDC, 0xFD};
static char cmd2_page0_data102[] = {0xDD, 0x03};
static char cmd2_page0_data103[] = {0xDE, 0x26};
static char cmd2_page0_data104[] = {0xDF, 0x03};
static char cmd2_page0_data105[] = {0xE0, 0x32};
static char cmd2_page0_data106[] = {0xE1, 0x03};
static char cmd2_page0_data107[] = {0xE2, 0x3F};
static char cmd2_page0_data108[] = {0xE3, 0x03};
static char cmd2_page0_data109[] = {0xE4, 0x4C};
static char cmd2_page0_data110[] = {0xE5, 0x03};
static char cmd2_page0_data111[] = {0xE6, 0x5B};
static char cmd2_page0_data112[] = {0xE7, 0x03};
static char cmd2_page0_data113[] = {0xE8, 0x68};
static char cmd2_page0_data114[] = {0xE9, 0x03};
static char cmd2_page0_data115[] = {0xEA, 0x70};
static char cmd2_page0_data116[] = {0xEB, 0x03};
static char cmd2_page0_data117[] = {0xEC, 0x73};
static char cmd2_page0_data118[] = {0xED, 0x03};
static char cmd2_page0_data119[] = {0xEE, 0xFF};
static char cmd2_page0_data120[] = {0xEF, 0x00};
static char cmd2_page0_data121[] = {0xF0, 0xD6};
static char cmd2_page0_data122[] = {0xF1, 0x00};
static char cmd2_page0_data123[] = {0xF2, 0xDB};
static char cmd2_page0_data124[] = {0xF3, 0x00};
static char cmd2_page0_data125[] = {0xF4, 0xE7};
static char cmd2_page0_data126[] = {0xF5, 0x00};
static char cmd2_page0_data127[] = {0xF6, 0xF1};
static char cmd2_page0_data128[] = {0xF7, 0x00};
static char cmd2_page0_data129[] = {0xF8, 0xFB};
static char cmd2_page0_data130[] = {0xF9, 0x01};
static char cmd2_page0_data131[] = {0xFA, 0x04};

static char cmd2_page1_0[] = {0xFF, 0x02};
static char cmd2_page1_1[] = {0xFB, 0x01};
static char cmd2_page1_data000[] = {0x00, 0x01};
static char cmd2_page1_data001[] = {0x01, 0x0D};
static char cmd2_page1_data002[] = {0x02, 0x01};
static char cmd2_page1_data003[] = {0x03, 0x16};
static char cmd2_page1_data004[] = {0x04, 0x01};
static char cmd2_page1_data005[] = {0x05, 0x1F};
static char cmd2_page1_data006[] = {0x06, 0x01};
static char cmd2_page1_data007[] = {0x07, 0x3D};
static char cmd2_page1_data008[] = {0x08, 0x01};
static char cmd2_page1_data009[] = {0x09, 0x5B};
static char cmd2_page1_data010[] = {0x0A, 0x01};
static char cmd2_page1_data011[] = {0x0B, 0x86};
static char cmd2_page1_data012[] = {0x0C, 0x01};
static char cmd2_page1_data013[] = {0x0D, 0xAC};
static char cmd2_page1_data014[] = {0x0E, 0x01};
static char cmd2_page1_data015[] = {0x0F, 0xEB};
static char cmd2_page1_data016[] = {0x10, 0x02};
static char cmd2_page1_data017[] = {0x11, 0x1F};
static char cmd2_page1_data018[] = {0x12, 0x02};
static char cmd2_page1_data019[] = {0x13, 0x20};
static char cmd2_page1_data020[] = {0x14, 0x02};
static char cmd2_page1_data021[] = {0x15, 0x51};
static char cmd2_page1_data022[] = {0x16, 0x02};
static char cmd2_page1_data023[] = {0x17, 0x87};
static char cmd2_page1_data024[] = {0x18, 0x02};
static char cmd2_page1_data025[] = {0x19, 0xAB};
static char cmd2_page1_data026[] = {0x1A, 0x02};
static char cmd2_page1_data027[] = {0x1B, 0xDC};
static char cmd2_page1_data028[] = {0x1C, 0x02};
static char cmd2_page1_data029[] = {0x1D, 0xFD};
static char cmd2_page1_data030[] = {0x1E, 0x03};
static char cmd2_page1_data031[] = {0x1F, 0x28};
static char cmd2_page1_data032[] = {0x20, 0x03};
static char cmd2_page1_data033[] = {0x21, 0x34};
static char cmd2_page1_data034[] = {0x22, 0x03};
static char cmd2_page1_data035[] = {0x23, 0x42};
static char cmd2_page1_data036[] = {0x24, 0x03};
static char cmd2_page1_data037[] = {0x25, 0x50};
static char cmd2_page1_data038[] = {0x26, 0x03};
static char cmd2_page1_data039[] = {0x27, 0x5F};
static char cmd2_page1_data040[] = {0x28, 0x03};
static char cmd2_page1_data041[] = {0x29, 0x6F};
static char cmd2_page1_data042[] = {0x2A, 0x03};
static char cmd2_page1_data043[] = {0x2B, 0x7C};
static char cmd2_page1_data044[] = {0x2D, 0x03};
static char cmd2_page1_data045[] = {0x2F, 0x84};
static char cmd2_page1_data046[] = {0x30, 0x03};
static char cmd2_page1_data047[] = {0x31, 0xFF};
static char cmd2_page1_data048[] = {0x32, 0x00};
static char cmd2_page1_data049[] = {0x33, 0xD6};
static char cmd2_page1_data050[] = {0x34, 0x00};
static char cmd2_page1_data051[] = {0x35, 0xDB};
static char cmd2_page1_data052[] = {0x36, 0x00};
static char cmd2_page1_data053[] = {0x37, 0xE7};
static char cmd2_page1_data054[] = {0x38, 0x00};
static char cmd2_page1_data055[] = {0x39, 0xF1};
static char cmd2_page1_data056[] = {0x3A, 0x00};
static char cmd2_page1_data057[] = {0x3B, 0xFB};
static char cmd2_page1_data058[] = {0x3D, 0x01};
static char cmd2_page1_data059[] = {0x3F, 0x04};
static char cmd2_page1_data060[] = {0x40, 0x01};
static char cmd2_page1_data061[] = {0x41, 0x0D};
static char cmd2_page1_data062[] = {0x42, 0x01};
static char cmd2_page1_data063[] = {0x43, 0x16};
static char cmd2_page1_data064[] = {0x44, 0x01};
static char cmd2_page1_data065[] = {0x45, 0x1F};
static char cmd2_page1_data066[] = {0x46, 0x01};
static char cmd2_page1_data067[] = {0x47, 0x3D};
static char cmd2_page1_data068[] = {0x48, 0x01};
static char cmd2_page1_data069[] = {0x49, 0x58};
static char cmd2_page1_data070[] = {0x4A, 0x01};
static char cmd2_page1_data071[] = {0x4B, 0x86};
static char cmd2_page1_data072[] = {0x4C, 0x01};
static char cmd2_page1_data073[] = {0x4D, 0xAC};
static char cmd2_page1_data074[] = {0x4E, 0x01};
static char cmd2_page1_data075[] = {0x4F, 0xEB};
static char cmd2_page1_data076[] = {0x50, 0x02};
static char cmd2_page1_data077[] = {0x51, 0x1F};
static char cmd2_page1_data078[] = {0x52, 0x02};
static char cmd2_page1_data079[] = {0x53, 0x20};
static char cmd2_page1_data080[] = {0x54, 0x02};
static char cmd2_page1_data081[] = {0x55, 0x51};
static char cmd2_page1_data082[] = {0x56, 0x02};
static char cmd2_page1_data083[] = {0x58, 0x87};
static char cmd2_page1_data084[] = {0x59, 0x02};
static char cmd2_page1_data085[] = {0x5A, 0xAB};
static char cmd2_page1_data086[] = {0x5B, 0x02};
static char cmd2_page1_data087[] = {0x5C, 0xDC};
static char cmd2_page1_data088[] = {0x5D, 0x02};
static char cmd2_page1_data089[] = {0x5E, 0xFD};
static char cmd2_page1_data090[] = {0x5F, 0x03};
static char cmd2_page1_data091[] = {0x60, 0x28};
static char cmd2_page1_data092[] = {0x61, 0x03};
static char cmd2_page1_data093[] = {0x62, 0x34};
static char cmd2_page1_data094[] = {0x63, 0x03};
static char cmd2_page1_data095[] = {0x64, 0x42};
static char cmd2_page1_data096[] = {0x65, 0x03};
static char cmd2_page1_data097[] = {0x66, 0x50};
static char cmd2_page1_data098[] = {0x67, 0x03};
static char cmd2_page1_data099[] = {0x68, 0x5F};
static char cmd2_page1_data100[] = {0x69, 0x03};
static char cmd2_page1_data101[] = {0x6A, 0x6F};
static char cmd2_page1_data102[] = {0x6B, 0x03};
static char cmd2_page1_data103[] = {0x6C, 0x7C};
static char cmd2_page1_data104[] = {0x6D, 0x03};
static char cmd2_page1_data105[] = {0x6E, 0x84};
static char cmd2_page1_data106[] = {0x6F, 0x03};
static char cmd2_page1_data107[] = {0x70, 0xFF};
static char cmd2_page1_data108[] = {0x71, 0x00};
static char cmd2_page1_data109[] = {0x72, 0x00};
static char cmd2_page1_data110[] = {0x73, 0x00};
static char cmd2_page1_data111[] = {0x74, 0x19};
static char cmd2_page1_data112[] = {0x75, 0x00};
static char cmd2_page1_data113[] = {0x76, 0x41};
static char cmd2_page1_data114[] = {0x77, 0x00};
static char cmd2_page1_data115[] = {0x78, 0x5F};
static char cmd2_page1_data116[] = {0x79, 0x00};
static char cmd2_page1_data117[] = {0x7A, 0x77};
static char cmd2_page1_data118[] = {0x7B, 0x00};
static char cmd2_page1_data119[] = {0x7C, 0x8C};
static char cmd2_page1_data120[] = {0x7D, 0x00};
static char cmd2_page1_data121[] = {0x7E, 0xA0};
static char cmd2_page1_data122[] = {0x7F, 0x00};
static char cmd2_page1_data123[] = {0x80, 0xB0};
static char cmd2_page1_data124[] = {0x81, 0x00};
static char cmd2_page1_data125[] = {0x82, 0xBF};
static char cmd2_page1_data126[] = {0x83, 0x00};
static char cmd2_page1_data127[] = {0x84, 0xF4};
static char cmd2_page1_data128[] = {0x85, 0x01};
static char cmd2_page1_data129[] = {0x86, 0x1C};
static char cmd2_page1_data130[] = {0x87, 0x01};
static char cmd2_page1_data131[] = {0x88, 0x5C};
static char cmd2_page1_data132[] = {0x89, 0x01};
static char cmd2_page1_data133[] = {0x8A, 0xBD};
static char cmd2_page1_data134[] = {0x8B, 0x01};
static char cmd2_page1_data135[] = {0x8C, 0xD8};
static char cmd2_page1_data136[] = {0x8D, 0x02};
static char cmd2_page1_data137[] = {0x8E, 0x12};
static char cmd2_page1_data138[] = {0x8F, 0x02};
static char cmd2_page1_data139[] = {0x90, 0x14};
static char cmd2_page1_data140[] = {0x91, 0x02};
static char cmd2_page1_data141[] = {0x92, 0x43};
static char cmd2_page1_data142[] = {0x93, 0x02};
static char cmd2_page1_data143[] = {0x94, 0x7D};
static char cmd2_page1_data144[] = {0x95, 0x02};
static char cmd2_page1_data145[] = {0x96, 0xA0};
static char cmd2_page1_data146[] = {0x97, 0x02};
static char cmd2_page1_data147[] = {0x98, 0xCE};
static char cmd2_page1_data148[] = {0x99, 0x02};
static char cmd2_page1_data149[] = {0x9A, 0xFC};
static char cmd2_page1_data150[] = {0x9B, 0x03};
static char cmd2_page1_data151[] = {0x9C, 0x1A};
static char cmd2_page1_data152[] = {0x9D, 0x03};
static char cmd2_page1_data153[] = {0x9E, 0x20};
static char cmd2_page1_data154[] = {0x9F, 0x03};
static char cmd2_page1_data155[] = {0xA0, 0x34};
static char cmd2_page1_data156[] = {0xA2, 0x03};
static char cmd2_page1_data157[] = {0xA3, 0x4A};
static char cmd2_page1_data158[] = {0xA4, 0x03};
static char cmd2_page1_data159[] = {0xA5, 0x63};
static char cmd2_page1_data160[] = {0xA6, 0x03};
static char cmd2_page1_data161[] = {0xA7, 0x80};
static char cmd2_page1_data162[] = {0xA9, 0x03};
static char cmd2_page1_data163[] = {0xAA, 0xA2};
static char cmd2_page1_data164[] = {0xAB, 0x03};
static char cmd2_page1_data165[] = {0xAC, 0xC9};
static char cmd2_page1_data166[] = {0xAD, 0x03};
static char cmd2_page1_data167[] = {0xAE, 0xFF};
static char cmd2_page1_data168[] = {0xAF, 0x00};
static char cmd2_page1_data169[] = {0xB0, 0x00};
static char cmd2_page1_data170[] = {0xB1, 0x00};
static char cmd2_page1_data171[] = {0xB2, 0x19};
static char cmd2_page1_data172[] = {0xB3, 0x00};
static char cmd2_page1_data173[] = {0xB4, 0x41};
static char cmd2_page1_data174[] = {0xB5, 0x00};
static char cmd2_page1_data175[] = {0xB6, 0x5F};
static char cmd2_page1_data176[] = {0xB7, 0x00};
static char cmd2_page1_data177[] = {0xB8, 0x77};
static char cmd2_page1_data178[] = {0xB9, 0x00};
static char cmd2_page1_data179[] = {0xBA, 0x8C};
static char cmd2_page1_data180[] = {0xBB, 0x00};
static char cmd2_page1_data181[] = {0xBC, 0xA0};
static char cmd2_page1_data182[] = {0xBD, 0x00};
static char cmd2_page1_data183[] = {0xBE, 0xB0};
static char cmd2_page1_data184[] = {0xBF, 0x00};
static char cmd2_page1_data185[] = {0xC0, 0xBF};
static char cmd2_page1_data186[] = {0xC1, 0x00};
static char cmd2_page1_data187[] = {0xC2, 0xF4};
static char cmd2_page1_data188[] = {0xC3, 0x01};
static char cmd2_page1_data189[] = {0xC4, 0x1C};
static char cmd2_page1_data190[] = {0xC5, 0x01};
static char cmd2_page1_data191[] = {0xC6, 0x5C};
static char cmd2_page1_data192[] = {0xC7, 0x01};
static char cmd2_page1_data193[] = {0xC8, 0x8D};
static char cmd2_page1_data194[] = {0xC9, 0x01};
static char cmd2_page1_data195[] = {0xCA, 0xD8};
static char cmd2_page1_data196[] = {0xCB, 0x02};
static char cmd2_page1_data197[] = {0xCC, 0x12};
static char cmd2_page1_data198[] = {0xCD, 0x02};
static char cmd2_page1_data199[] = {0xCE, 0x14};
static char cmd2_page1_data200[] = {0xCF, 0x02};
static char cmd2_page1_data201[] = {0xD0, 0x43};
static char cmd2_page1_data202[] = {0xD1, 0x02};
static char cmd2_page1_data203[] = {0xD2, 0x7D};
static char cmd2_page1_data204[] = {0xD3, 0x02};
static char cmd2_page1_data205[] = {0xD4, 0xA0};
static char cmd2_page1_data206[] = {0xD5, 0x02};
static char cmd2_page1_data207[] = {0xD6, 0xCE};
static char cmd2_page1_data208[] = {0xD7, 0x02};
static char cmd2_page1_data209[] = {0xD8, 0xFC};
static char cmd2_page1_data210[] = {0xD9, 0x03};
static char cmd2_page1_data211[] = {0xDA, 0x1A};
static char cmd2_page1_data212[] = {0xDB, 0x03};
static char cmd2_page1_data213[] = {0xDC, 0x20};
static char cmd2_page1_data214[] = {0xDD, 0x03};
static char cmd2_page1_data215[] = {0xDE, 0x34};
static char cmd2_page1_data216[] = {0xDF, 0x03};
static char cmd2_page1_data217[] = {0xE0, 0x4A};
static char cmd2_page1_data218[] = {0xE1, 0x03};
static char cmd2_page1_data219[] = {0xE2, 0x63};
static char cmd2_page1_data220[] = {0xE3, 0x03};
static char cmd2_page1_data221[] = {0xE4, 0x80};
static char cmd2_page1_data222[] = {0xE5, 0x03};
static char cmd2_page1_data223[] = {0xE6, 0xA2};
static char cmd2_page1_data224[] = {0xE7, 0x03};
static char cmd2_page1_data225[] = {0xE8, 0xC9};
static char cmd2_page1_data226[] = {0xE9, 0x03};
static char cmd2_page1_data227[] = {0xEA, 0xFF};
#endif

static char cmd2_page2_0[] = {0xFF, 0x03};
static char cmd2_page2_1[] = {0xFE, 0x08};
static char cmd2_page2_data00[] = {0x25, 0x26};
static char cmd2_page2_data01[] = {0x00, 0x00};
static char cmd2_page2_data02[] = {0x01, 0x05};
static char cmd2_page2_data03[] = {0x02, 0x10};
static char cmd2_page2_data04[] = {0x03, 0x14};
static char cmd2_page2_data05[] = {0x04, 0x16};
static char cmd2_page2_data06[] = {0x05, 0x18};
static char cmd2_page2_data07[] = {0x06, 0x20};
static char cmd2_page2_data08[] = {0x07, 0x20};
static char cmd2_page2_data09[] = {0x08, 0x18};
static char cmd2_page2_data10[] = {0x09, 0x16};
static char cmd2_page2_data11[] = {0x0A, 0x14};
static char cmd2_page2_data12[] = {0x0B, 0x12};
static char cmd2_page2_data13[] = {0x0C, 0x06};
static char cmd2_page2_data14[] = {0x0D, 0x02};
static char cmd2_page2_data15[] = {0x0E, 0x01};
static char cmd2_page2_data16[] = {0x0F, 0x00};
static char cmd2_page2_data17[] = {0xFB, 0x01};
static char cmd2_page2_data18[] = {0xFF, 0x00};
static char cmd2_page2_data19[] = {0xFE, 0x01};

static char cmd2_page3_0[] = {0xFF, 0x04};
static char cmd2_page3_1[] = {0xFB, 0x01};
static char cmd2_page3_data00[] = {0x0A, 0x03};
static char cmd2_page3_data01[] = {0x05, 0x2D};
static char cmd2_page3_data02[] = {0x21, 0xFF};
static char cmd2_page3_data03[] = {0x22, 0xF7};
static char cmd2_page3_data04[] = {0x23, 0xEF};
static char cmd2_page3_data05[] = {0x24, 0xE7};
static char cmd2_page3_data06[] = {0x25, 0xDF};
static char cmd2_page3_data07[] = {0x26, 0xD7};
static char cmd2_page3_data08[] = {0x27, 0xCF};
static char cmd2_page3_data09[] = {0x28, 0xC7};
static char cmd2_page3_data10[] = {0x29, 0xBF};
static char cmd2_page3_data11[] = {0x2A, 0xB7};

static char cmd1_0[] = {0xFF, 0x00};
static char cmd1_1[] = {0xFB, 0x01};
static char pwm_duty[] = {0x51, 0x07}; /* PWM Duty 3% */
static char bl_ctl[4] = {0x53, 0x2C}; /* BCTRL on, DD on, BL on */
static char power_save[4] = {0x55, 0x83}; /* CABC_COND on, IMAGE_ENHANCEMENT off */
static char power_save2[4] = {0x5E, 0x06};
static char set_display_mode[4] = {0xC2, 0x08}; /* 0x80: Via RAM, 0x00: Bypass RAM */
static char set_mipi_lane[4] = {0xBA, 0x02}; /* 0x01: 2 lane, 0x02: 3 lane */
static char set_te_on[4] = {0x35, 0x00};

#if 0 //some panel needed, correct it in feture
static struct dsi_cmd_desc sharp_hx_cmd_on_rotation_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_b9), himax_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_d4), himax_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_ba), himax_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c0), himax_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c6), himax_c6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_d5), himax_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_bf), himax_bf},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_b2), himax_b2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_e3), himax_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_e5), himax_e5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_35), himax_35},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_c2), himax_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_36), himax_36},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_55), himax_55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(cabc_UI), cabc_UI},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c9), himax_c9},
};
#endif

static struct dsi_cmd_desc sharp_hx_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_b9), himax_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_d4), himax_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_ba), himax_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c0), himax_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c6), himax_c6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_d5), himax_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_bf), himax_bf},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c2), himax_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5,  sizeof(himax_e3), himax_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_e5), himax_e5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_35), himax_35},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5,  sizeof(himax_55), himax_55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(cabc_UI), cabc_UI},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c9), himax_c9},
};

static struct dsi_cmd_desc auo_nt_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_0), cmd2_page0_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_1), cmd2_page0_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data00), cmd2_page0_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data01), cmd2_page0_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data02), cmd2_page0_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data03), cmd2_page0_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data04), cmd2_page0_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data05), cmd2_page0_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data06), cmd2_page0_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data07), cmd2_page0_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data08), cmd2_page0_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data09), cmd2_page0_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data10), cmd2_page0_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data11), cmd2_page0_data11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data12), cmd2_page0_data12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data13), cmd2_page0_data13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data14), cmd2_page0_data14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data15), cmd2_page0_data15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data16), cmd2_page0_data16},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_0), cmd3_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_1), cmd3_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_data00), cmd3_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_data01), cmd3_data01},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_0), cmd2_page4_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_1), cmd2_page4_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data00), cmd2_page4_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data01), cmd2_page4_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data02), cmd2_page4_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data03), cmd2_page4_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data04), cmd2_page4_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data05), cmd2_page4_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data06), cmd2_page4_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data07), cmd2_page4_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data08), cmd2_page4_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data09), cmd2_page4_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data10), cmd2_page4_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data11), cmd2_page4_data11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data12), cmd2_page4_data12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data13), cmd2_page4_data13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data14), cmd2_page4_data14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data15), cmd2_page4_data15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data16), cmd2_page4_data16},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data17), cmd2_page4_data17},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data18), cmd2_page4_data18},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data19), cmd2_page4_data19},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data20), cmd2_page4_data20},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data21), cmd2_page4_data21},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data22), cmd2_page4_data22},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data23), cmd2_page4_data23},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data24), cmd2_page4_data24},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data25), cmd2_page4_data25},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data26), cmd2_page4_data26},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data27), cmd2_page4_data27},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data28), cmd2_page4_data28},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data29), cmd2_page4_data29},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data30), cmd2_page4_data30},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data31), cmd2_page4_data31},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data32), cmd2_page4_data32},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data33), cmd2_page4_data33},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data34), cmd2_page4_data34},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data35), cmd2_page4_data35},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data36), cmd2_page4_data36},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data37), cmd2_page4_data37},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data38), cmd2_page4_data38},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data39), cmd2_page4_data39},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data40), cmd2_page4_data40},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data41), cmd2_page4_data41},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data42), cmd2_page4_data42},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data43), cmd2_page4_data43},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data44), cmd2_page4_data44},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data45), cmd2_page4_data45},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data46), cmd2_page4_data46},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data47), cmd2_page4_data47},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data48), cmd2_page4_data48},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data49), cmd2_page4_data49},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data50), cmd2_page4_data50},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data51), cmd2_page4_data51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data52), cmd2_page4_data52},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data53), cmd2_page4_data53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data54), cmd2_page4_data54},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data55), cmd2_page4_data55},
#if 0
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data56), cmd2_page4_data56},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data57), cmd2_page4_data57},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data58), cmd2_page4_data58},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data59), cmd2_page4_data59},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data60), cmd2_page4_data60},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data61), cmd2_page4_data61},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data62), cmd2_page4_data62},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data63), cmd2_page4_data63},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data64), cmd2_page4_data64},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data65), cmd2_page4_data65},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data66), cmd2_page4_data66},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data67), cmd2_page4_data67},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data68), cmd2_page4_data68},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data69), cmd2_page4_data69},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data70), cmd2_page4_data70},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data71), cmd2_page4_data71},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data72), cmd2_page4_data72},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data73), cmd2_page4_data73},
#endif

#if 0
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_0), cmd2_page0_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_1), cmd2_page0_1},
	/* Red Positive Gamma setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data000), cmd2_page0_data000},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data001), cmd2_page0_data001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data002), cmd2_page0_data002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data003), cmd2_page0_data003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data004), cmd2_page0_data004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data005), cmd2_page0_data005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data006), cmd2_page0_data006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data007), cmd2_page0_data007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data008), cmd2_page0_data008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data009), cmd2_page0_data009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data010), cmd2_page0_data010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data011), cmd2_page0_data011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data012), cmd2_page0_data012},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data013), cmd2_page0_data013},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data014), cmd2_page0_data014},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data015), cmd2_page0_data015},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data016), cmd2_page0_data016},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data017), cmd2_page0_data017},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data018), cmd2_page0_data018},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data019), cmd2_page0_data019},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data020), cmd2_page0_data020},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data021), cmd2_page0_data021},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data022), cmd2_page0_data022},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data023), cmd2_page0_data023},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data024), cmd2_page0_data024},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data025), cmd2_page0_data025},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data026), cmd2_page0_data026},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data027), cmd2_page0_data027},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data028), cmd2_page0_data028},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data029), cmd2_page0_data029},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data030), cmd2_page0_data030},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data031), cmd2_page0_data031},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data032), cmd2_page0_data032},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data033), cmd2_page0_data033},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data034), cmd2_page0_data034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data035), cmd2_page0_data035},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data036), cmd2_page0_data036},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data037), cmd2_page0_data037},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data038), cmd2_page0_data038},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data039), cmd2_page0_data039},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data040), cmd2_page0_data040},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data041), cmd2_page0_data041},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data042), cmd2_page0_data042},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data043), cmd2_page0_data043},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data044), cmd2_page0_data044},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data045), cmd2_page0_data045},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data046), cmd2_page0_data046},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data047), cmd2_page0_data047},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data048), cmd2_page0_data048},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data049), cmd2_page0_data049},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data050), cmd2_page0_data050},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data051), cmd2_page0_data051},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data052), cmd2_page0_data052},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data053), cmd2_page0_data053},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data054), cmd2_page0_data054},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data055), cmd2_page0_data055},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data056), cmd2_page0_data056},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data057), cmd2_page0_data057},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data058), cmd2_page0_data058},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data059), cmd2_page0_data059},
	/* Red Negative Gamma setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data060), cmd2_page0_data060},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data061), cmd2_page0_data061},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data062), cmd2_page0_data062},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data063), cmd2_page0_data063},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data064), cmd2_page0_data064},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data065), cmd2_page0_data065},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data066), cmd2_page0_data066},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data067), cmd2_page0_data067},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data068), cmd2_page0_data068},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data069), cmd2_page0_data069},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data070), cmd2_page0_data070},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data071), cmd2_page0_data071},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data072), cmd2_page0_data072},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data073), cmd2_page0_data073},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data074), cmd2_page0_data074},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data075), cmd2_page0_data075},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data076), cmd2_page0_data076},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data077), cmd2_page0_data077},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data078), cmd2_page0_data078},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data079), cmd2_page0_data079},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data080), cmd2_page0_data080},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data081), cmd2_page0_data081},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data082), cmd2_page0_data082},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data083), cmd2_page0_data083},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data084), cmd2_page0_data084},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data085), cmd2_page0_data085},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data086), cmd2_page0_data086},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data087), cmd2_page0_data087},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data088), cmd2_page0_data088},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data089), cmd2_page0_data089},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data090), cmd2_page0_data090},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data091), cmd2_page0_data091},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data092), cmd2_page0_data092},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data093), cmd2_page0_data093},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data094), cmd2_page0_data094},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data095), cmd2_page0_data095},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data096), cmd2_page0_data096},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data097), cmd2_page0_data097},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data098), cmd2_page0_data098},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data099), cmd2_page0_data099},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data100), cmd2_page0_data100},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data101), cmd2_page0_data101},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data102), cmd2_page0_data102},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data103), cmd2_page0_data103},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data104), cmd2_page0_data104},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data105), cmd2_page0_data105},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data106), cmd2_page0_data106},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data107), cmd2_page0_data107},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data108), cmd2_page0_data108},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data109), cmd2_page0_data109},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data110), cmd2_page0_data110},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data111), cmd2_page0_data111},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data112), cmd2_page0_data112},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data113), cmd2_page0_data113},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data114), cmd2_page0_data114},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data115), cmd2_page0_data115},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data116), cmd2_page0_data116},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data117), cmd2_page0_data117},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data118), cmd2_page0_data118},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data119), cmd2_page0_data119},
	/* Green Positive Gamma setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data120), cmd2_page0_data120},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data121), cmd2_page0_data121},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data122), cmd2_page0_data122},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data123), cmd2_page0_data123},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data124), cmd2_page0_data124},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data125), cmd2_page0_data125},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data126), cmd2_page0_data126},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data127), cmd2_page0_data127},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data128), cmd2_page0_data128},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data129), cmd2_page0_data129},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data130), cmd2_page0_data130},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data131), cmd2_page0_data131},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_0), cmd2_page1_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_1), cmd2_page1_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data000), cmd2_page1_data000},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data001), cmd2_page1_data001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data002), cmd2_page1_data002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data003), cmd2_page1_data003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data004), cmd2_page1_data004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data005), cmd2_page1_data005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data006), cmd2_page1_data006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data007), cmd2_page1_data007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data008), cmd2_page1_data008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data009), cmd2_page1_data009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data010), cmd2_page1_data010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data011), cmd2_page1_data011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data012), cmd2_page1_data012},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data013), cmd2_page1_data013},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data014), cmd2_page1_data014},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data015), cmd2_page1_data015},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data016), cmd2_page1_data016},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data017), cmd2_page1_data017},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data018), cmd2_page1_data018},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data019), cmd2_page1_data019},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data020), cmd2_page1_data020},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data021), cmd2_page1_data021},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data022), cmd2_page1_data022},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data023), cmd2_page1_data023},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data024), cmd2_page1_data024},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data025), cmd2_page1_data025},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data026), cmd2_page1_data026},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data027), cmd2_page1_data027},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data028), cmd2_page1_data028},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data029), cmd2_page1_data029},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data030), cmd2_page1_data030},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data031), cmd2_page1_data031},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data032), cmd2_page1_data032},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data033), cmd2_page1_data033},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data034), cmd2_page1_data034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data035), cmd2_page1_data035},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data036), cmd2_page1_data036},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data037), cmd2_page1_data037},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data038), cmd2_page1_data038},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data039), cmd2_page1_data039},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data040), cmd2_page1_data040},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data041), cmd2_page1_data041},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data042), cmd2_page1_data042},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data043), cmd2_page1_data043},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data044), cmd2_page1_data044},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data045), cmd2_page1_data045},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data046), cmd2_page1_data046},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data047), cmd2_page1_data047},
	/* Green Negative Gamma setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data048), cmd2_page1_data048},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data049), cmd2_page1_data049},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data050), cmd2_page1_data050},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data051), cmd2_page1_data051},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data052), cmd2_page1_data052},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data053), cmd2_page1_data053},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data054), cmd2_page1_data054},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data055), cmd2_page1_data055},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data056), cmd2_page1_data056},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data057), cmd2_page1_data057},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data058), cmd2_page1_data058},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data059), cmd2_page1_data059},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data060), cmd2_page1_data060},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data061), cmd2_page1_data061},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data062), cmd2_page1_data062},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data063), cmd2_page1_data063},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data064), cmd2_page1_data064},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data065), cmd2_page1_data065},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data066), cmd2_page1_data066},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data067), cmd2_page1_data067},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data068), cmd2_page1_data068},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data069), cmd2_page1_data069},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data070), cmd2_page1_data070},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data071), cmd2_page1_data071},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data072), cmd2_page1_data072},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data073), cmd2_page1_data073},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data074), cmd2_page1_data074},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data075), cmd2_page1_data075},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data076), cmd2_page1_data076},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data077), cmd2_page1_data077},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data078), cmd2_page1_data078},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data079), cmd2_page1_data079},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data080), cmd2_page1_data080},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data081), cmd2_page1_data081},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data082), cmd2_page1_data082},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data083), cmd2_page1_data083},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data084), cmd2_page1_data084},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data085), cmd2_page1_data085},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data086), cmd2_page1_data086},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data087), cmd2_page1_data087},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data088), cmd2_page1_data088},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data089), cmd2_page1_data089},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data090), cmd2_page1_data090},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data091), cmd2_page1_data091},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data092), cmd2_page1_data092},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data093), cmd2_page1_data093},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data094), cmd2_page1_data094},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data095), cmd2_page1_data095},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data096), cmd2_page1_data096},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data097), cmd2_page1_data097},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data098), cmd2_page1_data098},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data099), cmd2_page1_data099},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data100), cmd2_page1_data100},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data101), cmd2_page1_data101},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data102), cmd2_page1_data102},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data103), cmd2_page1_data103},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data104), cmd2_page1_data104},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data105), cmd2_page1_data105},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data106), cmd2_page1_data106},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data107), cmd2_page1_data107},
	/* Blue Positive Gamma setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data108), cmd2_page1_data108},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data109), cmd2_page1_data109},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data110), cmd2_page1_data110},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data111), cmd2_page1_data111},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data112), cmd2_page1_data112},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data113), cmd2_page1_data113},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data114), cmd2_page1_data114},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data115), cmd2_page1_data115},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data116), cmd2_page1_data116},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data117), cmd2_page1_data117},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data118), cmd2_page1_data118},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data119), cmd2_page1_data119},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data120), cmd2_page1_data120},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data121), cmd2_page1_data121},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data122), cmd2_page1_data122},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data123), cmd2_page1_data123},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data124), cmd2_page1_data124},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data125), cmd2_page1_data125},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data126), cmd2_page1_data126},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data127), cmd2_page1_data127},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data128), cmd2_page1_data128},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data129), cmd2_page1_data129},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data130), cmd2_page1_data130},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data131), cmd2_page1_data131},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data132), cmd2_page1_data132},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data133), cmd2_page1_data133},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data134), cmd2_page1_data134},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data135), cmd2_page1_data135},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data136), cmd2_page1_data136},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data137), cmd2_page1_data137},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data138), cmd2_page1_data138},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data139), cmd2_page1_data139},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data140), cmd2_page1_data140},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data141), cmd2_page1_data141},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data142), cmd2_page1_data142},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data143), cmd2_page1_data143},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data144), cmd2_page1_data144},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data145), cmd2_page1_data145},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data146), cmd2_page1_data146},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data147), cmd2_page1_data147},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data148), cmd2_page1_data148},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data149), cmd2_page1_data149},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data150), cmd2_page1_data150},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data151), cmd2_page1_data151},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data152), cmd2_page1_data152},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data153), cmd2_page1_data153},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data154), cmd2_page1_data154},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data155), cmd2_page1_data155},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data156), cmd2_page1_data156},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data157), cmd2_page1_data157},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data158), cmd2_page1_data158},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data159), cmd2_page1_data159},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data160), cmd2_page1_data160},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data161), cmd2_page1_data161},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data162), cmd2_page1_data162},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data163), cmd2_page1_data163},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data164), cmd2_page1_data164},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data165), cmd2_page1_data165},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data166), cmd2_page1_data166},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data167), cmd2_page1_data167},
	/* Blue Negative Gamma setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data168), cmd2_page1_data168},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data169), cmd2_page1_data169},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data170), cmd2_page1_data170},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data171), cmd2_page1_data171},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data172), cmd2_page1_data172},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data173), cmd2_page1_data173},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data174), cmd2_page1_data174},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data175), cmd2_page1_data175},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data176), cmd2_page1_data176},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data177), cmd2_page1_data177},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data178), cmd2_page1_data178},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data179), cmd2_page1_data179},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data180), cmd2_page1_data180},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data181), cmd2_page1_data181},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data182), cmd2_page1_data182},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data183), cmd2_page1_data183},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data184), cmd2_page1_data184},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data185), cmd2_page1_data185},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data186), cmd2_page1_data186},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data187), cmd2_page1_data187},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data188), cmd2_page1_data188},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data189), cmd2_page1_data189},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data190), cmd2_page1_data190},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data191), cmd2_page1_data191},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data192), cmd2_page1_data192},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data193), cmd2_page1_data193},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data194), cmd2_page1_data194},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data195), cmd2_page1_data195},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data196), cmd2_page1_data196},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data197), cmd2_page1_data197},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data198), cmd2_page1_data198},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data199), cmd2_page1_data199},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data200), cmd2_page1_data200},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data201), cmd2_page1_data201},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data202), cmd2_page1_data202},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data203), cmd2_page1_data203},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data204), cmd2_page1_data204},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data205), cmd2_page1_data205},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data206), cmd2_page1_data206},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data207), cmd2_page1_data207},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data208), cmd2_page1_data208},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data209), cmd2_page1_data209},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data210), cmd2_page1_data210},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data211), cmd2_page1_data211},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data212), cmd2_page1_data212},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data213), cmd2_page1_data213},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data214), cmd2_page1_data214},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data215), cmd2_page1_data215},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data216), cmd2_page1_data216},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data217), cmd2_page1_data217},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data218), cmd2_page1_data218},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data219), cmd2_page1_data219},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data220), cmd2_page1_data220},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data221), cmd2_page1_data221},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data222), cmd2_page1_data222},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data223), cmd2_page1_data223},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data224), cmd2_page1_data224},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data225), cmd2_page1_data225},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data226), cmd2_page1_data226},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page1_data227), cmd2_page1_data227},
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_0), cmd2_page2_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_1), cmd2_page2_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data00), cmd2_page2_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data01), cmd2_page2_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data02), cmd2_page2_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data03), cmd2_page2_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data04), cmd2_page2_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data05), cmd2_page2_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data06), cmd2_page2_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data07), cmd2_page2_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data08), cmd2_page2_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data09), cmd2_page2_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data10), cmd2_page2_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data11), cmd2_page2_data11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data12), cmd2_page2_data12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data13), cmd2_page2_data13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data14), cmd2_page2_data14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data15), cmd2_page2_data15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data16), cmd2_page2_data16},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data17), cmd2_page2_data17},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data18), cmd2_page2_data18},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data19), cmd2_page2_data19},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_0), cmd2_page3_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_1), cmd2_page3_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data00), cmd2_page3_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data01), cmd2_page3_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data02), cmd2_page3_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data03), cmd2_page3_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data04), cmd2_page3_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data05), cmd2_page3_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data06), cmd2_page3_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data07), cmd2_page3_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data08), cmd2_page3_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data09), cmd2_page3_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data10), cmd2_page3_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data11), cmd2_page3_data11},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd1_0), cmd1_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd1_1), cmd1_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwm_duty), pwm_duty},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(bl_ctl), bl_ctl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(power_save), power_save},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(power_save2), power_save2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_display_mode), set_display_mode},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(set_mipi_lane), set_mipi_lane},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_te_on), set_te_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pwm_off), pwm_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 130,
		sizeof(enter_sleep), enter_sleep},
};

static struct dsi_cmd_desc sharp_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sharp_hx_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
};

#ifdef CONFIG_MSM_CABC_VIDEO_ENHANCE
static struct dsi_cmd_desc cabc_UI_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(cabc_UI), cabc_UI},
};
static struct dsi_cmd_desc cabc_moving_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(cabc_moving), cabc_moving},
};
#endif

static int zip_cl_lcd_on(struct platform_device *pdev)
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
		mfd->init_mipi_lcd = 1;
		return 0;
	}

	PR_DISP_INFO("Display On \n");

	mipi_dsi_cmds_tx(&zip_cl_panel_tx_buf, init_on_cmds,
				init_on_cmds_count);

	atomic_set(&lcd_power_state, 1);

	PR_DISP_DEBUG("Init done\n");

	return 0;
}

static int zip_cl_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	return 0;
}

static int __devinit zip_cl_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (pdev->id == 0) {
		mipi_zip_cl_pdata = pdev->dev.platform_data;

		if (mipi_zip_cl_pdata
			&& mipi_zip_cl_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_zip_cl_pdata->phy_ctrl_settings);
		}

		if (mipi_zip_cl_pdata
			&& mipi_zip_cl_pdata->dlane_swap) {
			dlane_swap = (mipi_zip_cl_pdata->dlane_swap);
		}

		perf_lock_init(&zip_cl_perf_lock, TYPE_PERF_LOCK, PERF_LOCK_HIGHEST, "zip_cl");
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

static void zip_cl_display_on(struct msm_fb_data_type *mfd)
{
    /* The Orise-Sony panel need to set display on after first frame sent */

	mipi_dsi_op_mode_config(DSI_CMD_MODE);

	cmdreq.cmds = display_on_cmds;
	cmdreq.cmds_cnt = display_on_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

DEFINE_LED_TRIGGER(bkl_led_trigger);

static void zip_cl_display_off(struct msm_fb_data_type *mfd)
{
	if (!is_perf_lock_active(&zip_cl_perf_lock))
		perf_lock(&zip_cl_perf_lock);

	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	if (wled_trigger_initialized)
		led_trigger_event(bkl_led_trigger, 0);
	else
		PR_DISP_ERR("%s: wled trigger is not initialized!\n", __func__);

	if (is_perf_lock_active(&zip_cl_perf_lock))
		perf_unlock(&zip_cl_perf_lock);

	atomic_set(&lcd_power_state, 0);

	PR_DISP_INFO("%s\n", __func__);
}

#ifdef CONFIG_MSM_CABC_VIDEO_ENHANCE
static void zip_cl_set_cabc(struct msm_fb_data_type *mfd, int mode)
{
	PR_DISP_DEBUG("%s: mode=%d\n",  __FUNCTION__, mode);

	if (mode == 0) {
		cmdreq.cmds = cabc_UI_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(cabc_UI_cmds);
	} else {
		cmdreq.cmds = cabc_moving_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(cabc_moving_cmds);
	}
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
}
#endif

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char zip_cl_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	pwm_min = 12;
	if (panel_type == PANEL_ID_ANDROMEDA_AUO_NT)
		pwm_default = 88;
	else
		pwm_default = 79;
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

static void zip_cl_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;
///HTC:
	led_pwm1[1] = zip_cl_shrink_pwm((unsigned char)(mfd->bl_level));
///:HTC

	if (mipi_zip_cl_pdata && (mipi_zip_cl_pdata->enable_wled_bl_ctrl)
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

	cmdreq.cmds = backlight_cmds;
	cmdreq.cmds_cnt = backlight_cmds_count;
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

static struct mipi_dsi_panel_platform_data zip_cl_pdata = {
	.dlane_swap = 0,
#ifdef CONFIG_BACKLIGHT_WLED_CABC
	.enable_wled_bl_ctrl = 0x0,
#else
	.enable_wled_bl_ctrl = 0x1,
#endif
};

static struct platform_device mipi_dsi_zip_cl_panel_device = {
	.name = "mipi_zip_cl",
	.id = 0,
	.dev = {
		.platform_data = &zip_cl_pdata,
	}
};

static struct platform_driver this_driver = {
	.probe  = zip_cl_lcd_probe,
	.driver = {
		.name   = "mipi_zip_cl",
	},
};

static struct msm_fb_panel_data zip_cl_panel_data = {
	.on		= zip_cl_lcd_on,
	.off		= zip_cl_lcd_off,
	.set_backlight  = zip_cl_set_backlight,
	.display_on	= zip_cl_display_on,
	.display_off	= zip_cl_display_off,
#ifdef CONFIG_MSM_CABC_VIDEO_ENHANCE
	.set_cabc	= zip_cl_set_cabc,
#endif
};

static struct msm_panel_info pinfo;
static int ch_used[3];

int mipi_zip_cl_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_zip_cl", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	zip_cl_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &zip_cl_panel_data,
		sizeof(zip_cl_panel_data));
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

static struct mipi_dsi_phy_ctrl dsi_sharp_cmd_mode_phy_db = {
/* DSI_BIT_CLK at 507MHz, 4 lane, RGB888 */
	{0x09, 0x08, 0x05, 0x00, 0x20},
	/* timing */
	{0xb9, 0x2a, 0x20, 0x00, 0x24, 0x50, 0x1d, 0x2a, 0x24,
	0x03, 0x04, 0xa0},
	/* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x0, 0xdf, 0xb1, 0xda, 0x00, 0x50, 0x48, 0x63,
	0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

static int __init mipi_cmd_sharp_init(void)
{
	int ret;

	pinfo.xres = 720;
	pinfo.yres = 1280;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 53;
	pinfo.height = 94;

	if (panel_type == PANEL_ID_ANDROMEDA_SHARP_HX) {
		pinfo.lcdc.h_back_porch = 29;
		pinfo.lcdc.h_front_porch = 55;
		pinfo.lcdc.h_pulse_width = 16;
		pinfo.lcdc.v_back_porch = 1;
		pinfo.lcdc.v_front_porch = 2;
		pinfo.lcdc.v_pulse_width = 1;
	} else if (panel_type == PANEL_ID_ANDROMEDA_AUO_NT) {
		pinfo.lcdc.h_back_porch = 5;
		pinfo.lcdc.h_front_porch = 3;
		pinfo.lcdc.h_pulse_width = 70;
		pinfo.lcdc.v_back_porch = 1;
		pinfo.lcdc.v_front_porch = 1;
		pinfo.lcdc.v_pulse_width = 1;
	}

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;

	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.camera_backlight = 185;
	pinfo.clk_rate = 540000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; /* adjust refx100 to prevent tearing */

	pinfo.lcd.v_back_porch = 1;
	pinfo.lcd.v_front_porch = 2;
	pinfo.lcd.v_pulse_width = 1;

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.esc_byte_ratio = 4;

	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1e;
	pinfo.mipi.stream = 0;	/* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; /* TE from vsycn gpio */
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.tx_eot_append = 1; /* to prevent flicker and color shift */
	pinfo.mipi.dsi_phy_db = &dsi_sharp_cmd_mode_phy_db;

	ret = mipi_zip_cl_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_720P_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	if (panel_type == PANEL_ID_ANDROMEDA_SHARP_HX) {
		init_on_cmds = sharp_hx_cmd_on_cmds;
		init_on_cmds_count = ARRAY_SIZE(sharp_hx_cmd_on_cmds);
	} else if (panel_type == PANEL_ID_ANDROMEDA_AUO_NT) {
		init_on_cmds = auo_nt_cmd_on_cmds;
		init_on_cmds_count = ARRAY_SIZE(auo_nt_cmd_on_cmds);
	}

	display_on_cmds = sharp_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(sharp_display_on_cmds);
	display_off_cmds = sharp_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sharp_display_off_cmds);
	backlight_cmds = sharp_hx_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(sharp_hx_cmd_backlight_cmds);

	return ret;
}

void __init zip_cl_init_fb(void)
{
	platform_device_register(&msm_fb_device);

	if(panel_type != PANEL_ID_NONE && board_mfg_mode() != 5) {
		PR_DISP_DEBUG("system_rev = %d\n", system_rev);
		/* XA (system_rev = 0): PMIC,
		   XB (system_rev = 1): External IC */
		if (system_rev >= 1)
			zip_cl_pdata.enable_wled_bl_ctrl = 0x0;

		platform_device_register(&mipi_dsi_zip_cl_panel_device);
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	}
}

static int __init zip_cl_init_panel(void)
{
	if(panel_type == PANEL_ID_NONE || board_mfg_mode() == 5) {
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	led_trigger_register_simple("bkl_trigger", &bkl_led_trigger);
	pr_info("%s: SUCCESS (WLED TRIGGER)\n", __func__);
	wled_trigger_initialized = 1;
	atomic_set(&lcd_power_state, 1);

	mipi_dsi_buf_alloc(&zip_cl_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&zip_cl_panel_rx_buf, DSI_BUF_SIZE);

	mipi_cmd_sharp_init();

	return platform_driver_register(&this_driver);
}

device_initcall_sync(zip_cl_init_panel);
