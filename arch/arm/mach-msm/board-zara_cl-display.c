/* linux/arch/arm/mach-msm/display/zara_cl-panel.c
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
#include "board-zara_cl.h"
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include <asm/system_info.h>
#include <linux/leds.h>
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <mach/perflock.h>
#include <linux/mfd/pm8xxx/gpio.h>
#include "board-8930.h"
#include <mach/ncp6924.h>

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

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	},
};

static int zara_cl_detect_panel(const char *name)
{
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = zara_cl_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init zara_cl_allocate_fb_region(void)
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
	/* Video mode panel can enable this feature for contiguouus splash */
	.cont_splash_enabled = 0x01,
};

void __init zara_cl_mdp_writeback(struct memtype_reserve* reserve_table)
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

		/* V_LCM_3V3_EN */
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

		if (panel_type == PANEL_ID_CANIS_LG_NOVATEK) {
			/* LCM 1.8V power */
			rc = ncp6924_enable_ldo(NCP6924_ID_LDO4, true);
			if (rc < 0) {
				pr_err("Enable LCM 1.8V via NCP6924 failed!\n");
				return -EINVAL;
			}
			gpio_set_value(MSM_V_LCMIO_1V8_EN, 1);
			hr_msleep(1);

			/* LCM 3.3V power */
			rc = regulator_enable(reg_l10);
			if (rc) {
				pr_err("enable l10 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			hr_msleep(30);

			/* We will pull reset pin after LP11 for LG panel */
			/* Pull in zara_cl_lcd_on() */

			/* MIPI DSI power */
			rc = regulator_enable(reg_l2);
			if (rc) {
				pr_err("enable l2 failed, rc=%d\n", rc);
				return -ENODEV;
			}
		} else if (panel_type == PANEL_ID_CANIS_JDI_NOVATEK) {
			/* LCM 1.8V power */
			rc = ncp6924_enable_ldo(NCP6924_ID_LDO4, true);
			if (rc < 0) {
				pr_err("Enable LCM 1.8V via NCP6924 failed!\n");
				return -EINVAL;
			}
			gpio_set_value(MSM_V_LCMIO_1V8_EN, 1);
			hr_msleep(2);

			/* LCM 3.3V power */
			rc = regulator_enable(reg_l10);
			if (rc) {
				pr_err("enable l10 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			hr_msleep(2);

			gpio_set_value(MSM_LCD_RSTz, 1);
			hr_msleep(1);
			gpio_set_value(MSM_LCD_RSTz, 0);
			hr_msleep(1);
			gpio_set_value(MSM_LCD_RSTz, 1);
			hr_msleep(20);

			/* MIPI DSI power */
			rc = regulator_enable(reg_l2);
			if (rc) {
				pr_err("enable l2 failed, rc=%d\n", rc);
				return -ENODEV;
			}
		}
	} else {
		if (panel_type == PANEL_ID_CANIS_LG_NOVATEK) {
			gpio_set_value(MSM_LCD_RSTz, 0);
			hr_msleep(130);

			gpio_set_value(MSM_V_LCMIO_1V8_EN, 0);
			rc = ncp6924_enable_ldo(NCP6924_ID_LDO4, false);
			if (rc < 0) {
				pr_err("Disable LCM 1.8V via NCP6924 failed!\n");
				return -EINVAL;
			}
			usleep(100);

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
		} else if (panel_type == PANEL_ID_CANIS_JDI_NOVATEK) {
			gpio_set_value(MSM_LCD_RSTz, 0);
			hr_msleep(5);

			gpio_set_value(MSM_V_LCMIO_1V8_EN, 0);
			rc = ncp6924_enable_ldo(NCP6924_ID_LDO4, false);
			if (rc < 0) {
				pr_err("Disable LCM 1.8V via NCP6924 failed!\n");
				return -EINVAL;
			}
			usleep(2);

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
	}

	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_VSYNC_GPIO,
	.dsi_power_save = mipi_dsi_panel_power,
};

static atomic_t lcd_power_state;
static struct mipi_dsi_panel_platform_data *mipi_zara_cl_pdata;

struct dcs_cmd_req cmdreq;
static struct dsi_buf zara_cl_panel_tx_buf;
static struct dsi_buf zara_cl_panel_rx_buf;
static struct dsi_cmd_desc *init_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static int init_on_cmds_count = 0;
static int display_off_cmds_count = 0;

static char sleep_out[] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char sleep_in[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char led_pwm[2] = {0x51, 0xF0}; /* DTYPE_DCS_WRITE1 */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char dsi_novatek_dim_on[] = {0x53, 0x2C};/* DTYPE_DCS_LWRITE */ /* bkl_ctrl on and dimming on */
static char dsi_novatek_dim_off[] = {0x53, 0x24};/* DTYPE_DCS_LWRITE */ /* bkl_ctrl on and dimming off */

static struct dsi_cmd_desc novatek_dim_on_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_novatek_dim_on), dsi_novatek_dim_on},
};
static struct dsi_cmd_desc novatek_dim_off_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_novatek_dim_off), dsi_novatek_dim_off},
};

/* LG panel with Novatek NT35517 (copy from CP3) */

static char cmd_set_page1[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x01
};
static char pwr_ctrl_AVDD[] = {0xB6, 0x34};
static char pwr_ctrl_AVEE[] = {0xB7, 0x34};
static char pwr_ctrl_VCL[] = {0xB8, 0x13};
static char pwr_ctrl_VGH[] = {0xB9, 0x24};
static char pwr_ctrl_VGLX[] = {0xBA, 0x23};
static char set_VGMP_VGSP_vol[] = {0xBC, 0x00, 0x88, 0x00};
static char set_VGMN_VGSN_vol[] = {0xBD, 0x00, 0x84, 0x00};
static char GPO_ctrl[] = {0xC0, 0x04, 0x00};
static char gamma_curve_ctrl[] = {0xCF, 0x04};
static char gamma_corr_red1[] = {
	0xD1, 0x00, 0x00, 0x00,
	0x48, 0x00, 0x71, 0x00,
	0x95, 0x00, 0xA4, 0x00,
	0xC1, 0x00, 0xD4, 0x00,
	0xFA
};
static char gamma_corr_red2[] = {
	0xD2, 0x01, 0x22, 0x01,
	0x5F, 0x01, 0x89, 0x01,
	0xCC, 0x02, 0x03, 0x02,
	0x05, 0x02, 0x38, 0x02,
	0x71
};
static char gamma_corr_red3[] = {
	0xD3, 0x02, 0x90, 0x02,
	0xC9, 0x02, 0xF4, 0x03,
	0x1A, 0x03, 0x35, 0x03,
	0x52, 0x03, 0x62, 0x03,
	0x76
};
static char gamma_corr_red4[] = {
	0xD4, 0x03, 0x8F, 0x03,
	0xC0
};
static char normal_display_mode_on[] = {0x13, 0x00};
static char disp_on[] = {0x29, 0x00};
static char cmd_set_page0[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x00
};
static char disp_opt_ctrl[] = {
	0xB1, 0x68, 0x00, 0x01
};
static char disp_scan_line_ctrl[] = {0xB4, 0x78};
static char eq_ctrl[] = {
	0xB8, 0x01, 0x02, 0x02,
	0x02
};
static char inv_drv_ctrl[] = {0xBC, 0x00, 0x00, 0x00};
static char display_timing_control[] = {
	0xC9, 0x63, 0x06, 0x0D,
	0x1A, 0x17, 0x00
};
static char write_ctrl_display[] = {0x53, 0x24};
static char te_on[] = {0x35, 0x00};
static char pwm_freq_ctrl[] = {0xE0, 0x01, 0x03};
static char pwr_blk_enable[] = {
	0xFF, 0xAA, 0x55, 0x25,
	0x01};
static char pwr_blk_disable[] = {
	0xFF, 0xAA, 0x55, 0x25,
	0x00};
static char set_para_idx[] = {0x6F, 0x0A};
static char pwr_blk_sel[] = {0xFA, 0x03};
static char bkl_off[] = {0x53, 0x00};
static char set_cabc_level[] = {0x55, 0x92};
static char vivid_color_setting[] = {
	0xD6, 0x00, 0x05, 0x10,
	0x17, 0x22, 0x26, 0x29,
	0x29, 0x26, 0x23, 0x17,
	0x12, 0x06, 0x02, 0x01,
	0x00};
static char cabc_still[] = {
	0xE3, 0xFF, 0xFB, 0xF3,
	0xEC, 0xE2, 0xCA, 0xC3,
	0xBC, 0xB5, 0xB3
};
static char idx_13[] = {0x6F, 0x13};
static char idx_14[] = {0x6F, 0x14};
static char idx_15[] = {0x6F, 0x15};
static char val_80[] = {0xF5, 0x80};
static char val_FF[] = {0xF5, 0xFF};
static char ctrl_ie_sre[] = {
	0xD4, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00};
static char skin_tone_setting1[] = {
	0xD7, 0x30, 0x30, 0x30,
	0x28, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00};
static char skin_tone_setting2[] = {
	0xD8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x28,
	0x30, 0x00};
#if 0
static char test[] = {0xCF, 0x00};

static char rp1[] = {
	0xD1, 0x00, 0x47, 0x00,
	0x60, 0x00, 0x81, 0x00,
	0x9B, 0x00, 0xAC, 0x00,
	0xCE, 0x00, 0xEB, 0x01,
	0x13
};
static char rp2[] = {
	0xD2,0x01,0x34,0x01,
	0x6B,0x01,0x97,0x01,
	0xD9,0x02,0x0F,0x02,
	0x10,0x02,0x46,0x02,
	0x82
};
static char rp3[] = {
	0xD3, 0x02, 0xA8, 0x02,
	0xDF, 0x03, 0x03, 0x03,
	0x2D, 0x03, 0x48, 0x03,
	0x61, 0x03, 0x62, 0x03,
	0x63
};
static char rp4[] = {
	0xD4, 0x03, 0x78, 0x03,
	0x7B
};
static char gp1[] = {
	0xD5, 0x00, 0x4C, 0x00,
	0x60, 0x00, 0x7F, 0x00,
	0x97, 0x00, 0xAB, 0x00,
	0xCC, 0x00, 0xE7, 0x01,
	0x13
};
static char gp2[] = {
	0xD6, 0x01, 0x34, 0x01,
	0x69, 0x01, 0x94, 0x01,
	0xD7, 0x02, 0x0D, 0x02,
	0x0F, 0x02, 0x45, 0x02,
	0x81
};
static char gp3[] = {
	0xD7, 0x02, 0xA8, 0x02,
	0xDC, 0x02, 0xFF, 0x03,
	0x30, 0x03, 0x4F, 0x03,
	0x78, 0x03, 0x9D, 0x03,
	0xE6
};
static char gp4[] = {
	0xD8, 0x03, 0xFE, 0x03,
	0xFE
};
static char bp1[] = {
	0xD9, 0x00, 0x52, 0x00,
	0x60, 0x00, 0x78, 0x00,
	0x8D, 0x00, 0x9F, 0x00,
	0xBE, 0x00, 0xDA, 0x01,
	0x07
};
static char bp2[] = {
	0xDD, 0x01, 0x29, 0x01,
	0x5F, 0x01, 0x8C, 0x01,
	0xD1, 0x02, 0x08, 0x02,
	0x0A, 0x02, 0x42, 0x02,
	0x7D
};
static char bp3[] = {
	0xDE, 0x02, 0xA4, 0x02,
	0xDA, 0x02, 0xFF, 0x03,
	0x38, 0x03, 0x66, 0x03,
	0xFB, 0x03, 0xFC, 0x03,
	0xFD
};
static char bp4[] = {
	0xDF, 0x03, 0xFE, 0x03,
	0xFE
};
static char rn1[] = {
	0xE0, 0x00, 0x47, 0x00,
	0x60, 0x00, 0x81, 0x00,
	0x9B, 0x00, 0xAC, 0x00,
	0xCE, 0x00, 0xEB, 0x01,
	0x13
};
static char rn2[] = {
	0xE1, 0x01, 0x34, 0x01,
	0x6B, 0x01, 0x97, 0x01,
	0xD9, 0x02, 0x0F, 0x02,
	0x10, 0x02, 0x46, 0x02,
	0x82
};
static char rn3[] = {
	0xE2, 0x02, 0xA8, 0x02,
	0xDF, 0x03, 0x03, 0x03,
	0x2D, 0x03, 0x48, 0x03,
	0x61, 0x03, 0x62, 0x03,
	0x63
};
static char rn4[] = {
	0xE3, 0x03, 0x78, 0x03,
	0x7B
};
static char gn1[] = {
	0xE4, 0x00, 0x4C, 0x00,
	0x60, 0x00, 0x7F, 0x00,
	0x97, 0x00, 0xAB, 0x00,
	0xCC, 0x00, 0xE7, 0x01,
	0x13
};
static char gn2[] = {
	0xE5, 0x01, 0x34, 0x01,
	0x69, 0x01, 0x94, 0x01,
	0xD7, 0x02, 0x0D, 0x02,
	0x0F, 0x02, 0x45, 0x02,
	0x81
};
static char gn3[] = {
	0xE6, 0x02, 0xA8, 0x02,
	0xDC, 0x02, 0xFF, 0x03,
	0x30, 0x03, 0x4F, 0x03,
	0x78, 0x03, 0x9D, 0x03,
	0xE6
};
static char gn4[] = {
	0xE7, 0x03, 0xFE, 0x03,
	0xFE
};
static char bn1[] = {
	0xE8, 0x00, 0x52, 0x00,
	0x60, 0x00, 0x78, 0x00,
	0x8D, 0x00, 0x9F, 0x00,
	0xBE, 0x00, 0xDA, 0x01,
	0x07
};
static char bn2[] = {
	0xE9, 0x01, 0x29, 0x01,
	0x5F, 0x01, 0x8C, 0x01,
	0xD1, 0x02, 0x08, 0x02,
	0x0A, 0x02, 0x42, 0x02,
	0x7D
};
static char bn3[] = {
	0xEA, 0x02, 0xA4, 0x02,
	0xDA, 0x02, 0xFF, 0x03,
	0x38, 0x03, 0x66, 0x03,
	0xFB, 0x03, 0xFC, 0x03,
	0xFD
};
static char bn4[] = {
	0xEB, 0x03, 0xFE, 0x03,
	0xFE
};
#endif
static struct dsi_cmd_desc lg_novatek_video_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_AVDD), pwr_ctrl_AVDD},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_AVEE), pwr_ctrl_AVEE},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VCL), pwr_ctrl_VCL},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VGH), pwr_ctrl_VGH},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VGLX), pwr_ctrl_VGLX},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_VGMP_VGSP_vol), set_VGMP_VGSP_vol},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_VGMN_VGSN_vol), set_VGMN_VGSN_vol},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(GPO_ctrl), GPO_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_curve_ctrl), gamma_curve_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red1), gamma_corr_red1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red2), gamma_corr_red2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red3), gamma_corr_red3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red4), gamma_corr_red4},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
#if 0
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(test), test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp1), rp1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp2), rp2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp3), rp3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp4), rp4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp1), gp1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp2), gp2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp3), gp3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp4), gp4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp1), bp1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp2), bp2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp3), bp3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp4), bp4},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn1), rn1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn2), rn2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn3), rn3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn4), rn4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn1), gn1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn2), gn2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn3), gn3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn4), gn4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn1), bn1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn2), bn2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn3), bn3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn4), bn4},
#endif
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(disp_opt_ctrl), disp_opt_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(disp_scan_line_ctrl), disp_scan_line_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(eq_ctrl), eq_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(inv_drv_ctrl), inv_drv_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(display_timing_control), display_timing_control},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(te_on), te_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwm_freq_ctrl), pwm_freq_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_cabc_level), set_cabc_level},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(ctrl_ie_sre), ctrl_ie_sre},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(vivid_color_setting), vivid_color_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting1), skin_tone_setting1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting2), skin_tone_setting2},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_enable), pwr_blk_enable},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_para_idx), set_para_idx},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_blk_sel), pwr_blk_sel},

	/* disable CABC for gray pattern */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_13), idx_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_80), val_80},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_14), idx_14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_15), idx_15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},

	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(normal_display_mode_on), normal_display_mode_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(disp_on), disp_on},
};

static struct dsi_cmd_desc lg_novatek_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(bkl_off), bkl_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 45, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 130, sizeof(sleep_in), sleep_in}
};

static struct dsi_cmd_desc lg_novatek_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm), led_pwm},
};

/* LG panel with Novatek NT35517 (copy from CP3) end */

/* JDI panel with Novatek NT35517 (copy from CP3) */

static char RGBCTR[] = {
	0xB0, 0x00, 0x16, 0x14,
	0x34, 0x34};
static char DPRSLCTR[] = {0xB2, 0x54, 0x01, 0x80};
static char SDHDTCTR[] = {0xB6, 0x0A};
static char GSEQCTR[] = {0xB7, 0x00, 0x22};
static char SDEQCTR[] = {
	0xB8, 0x00, 0x00, 0x07,
	0x00};
static char SDVPCTR[] = {0xBA, 0x02};
static char SGOPCTR[] = {0xBB, 0x44, 0x40};
static char DPFRCTR1[] = {
	0xBD, 0x01, 0xD1, 0x16,
	0x14};
static char DPTMCTR10_2[] = {0xC1, 0x03};
static char DPTMCTR10[] = {
	0xCA, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00};
static char PWMFRCTR[] = {0xE0, 0x01, 0x03};
static char SETAVDD[] = {0xB0, 0x07};
static char SETAVEE[] = {0xB1, 0x07};
static char SETVCL[] = {0xB2, 0x00};
static char SETVGH[] = {0xB3, 0x10};
static char SETVGLX[] = {0xB4, 0x0A};
static char BT1CTR[] = {0xB6, 0x34};
static char BT2CTR[] = {0xB7, 0x35};
static char BT3CTR[] = {0xB8, 0x16};
static char BT4CTR[] = {0xB9, 0x33};
static char BT5CTR[] = {0xBA, 0x15};
static char SETVGL_REG[] = {0xC4, 0x05};
static char GSVCTR[] = {0xCA, 0x21};
static char MAUCCTR[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x00, 0x00};
static char SETPARIDX[] = {0x6F, 0x01};
static char skew_delay_en[] = {0xF8, 0x24};
static char skew_delay[] = {0xFC, 0x41};

static struct dsi_cmd_desc jdi_novatek_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(RGBCTR), RGBCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPRSLCTR), DPRSLCTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDHDTCTR), SDHDTCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(GSEQCTR), GSEQCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SDEQCTR), SDEQCTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDVPCTR), SDVPCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SGOPCTR), SGOPCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPFRCTR1), DPFRCTR1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(DPTMCTR10_2), DPTMCTR10_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPTMCTR10), DPTMCTR10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(PWMFRCTR), PWMFRCTR},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_cabc_level), set_cabc_level},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(ctrl_ie_sre), ctrl_ie_sre},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(vivid_color_setting), vivid_color_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting1), skin_tone_setting1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting2), skin_tone_setting2},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_enable), pwr_blk_enable},
	/* disable CABC for gray pattern */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_13), idx_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_80), val_80},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_14), idx_14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_15), idx_15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETPARIDX), SETPARIDX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(skew_delay_en), skew_delay_en},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETPARIDX), SETPARIDX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(skew_delay), skew_delay},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_disable), pwr_blk_disable},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVDD), SETAVDD},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVEE), SETAVEE},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVCL), SETVCL},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGH), SETVGH},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGLX), SETVGLX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT1CTR), BT1CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT2CTR), BT2CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT3CTR), BT3CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT4CTR), BT4CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT5CTR), BT5CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGL_REG), SETVGL_REG},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(GSVCTR), GSVCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(MAUCCTR), MAUCCTR},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(disp_on), disp_on},
};

static struct dsi_cmd_desc jdi_novatek_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 40, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in}
};

/* JDI panel with Novatek NT35517 (copy from CP3) end */

static int zara_cl_lcd_on(struct platform_device *pdev)
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
		PR_DISP_INFO("Display On - 1st time\n");
		mfd->init_mipi_lcd = 1;
	} else
		PR_DISP_INFO("Display On \n");

	/* Pull reset pin for LG panel */
	if (panel_type == PANEL_ID_CANIS_LG_NOVATEK) {
		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(5);
		gpio_set_value(MSM_LCD_RSTz, 0);
		hr_msleep(5);
		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(30);
	}

	mipi_dsi_cmds_tx(&zara_cl_panel_tx_buf, init_on_cmds, init_on_cmds_count);

	atomic_set(&lcd_power_state, 1);

	PR_DISP_DEBUG("Init done\n");

	return 0;
}

static int zara_cl_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	atomic_set(&lcd_power_state, 0);

	return 0;
}

static int __devinit zara_cl_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (pdev->id == 0) {
		mipi_zara_cl_pdata = pdev->dev.platform_data;

		if (mipi_zara_cl_pdata
			&& mipi_zara_cl_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_zara_cl_pdata->phy_ctrl_settings);
		}

		if (mipi_zara_cl_pdata
			&& mipi_zara_cl_pdata->dlane_swap) {
			dlane_swap = (mipi_zara_cl_pdata->dlane_swap);
		}

		return 0;
	}

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) {
		mfd = platform_get_drvdata(current_pdev);
		if (!mfd)
			return -ENODEV;
		if (mfd->key != MFD_KEY)
			return -EINVAL;

		mipi = &mfd->panel_info.mipi;

		if (phy_settings != NULL)
			mipi->dsi_phy_db = phy_settings;

		if (dlane_swap)
			mipi->dlane_swap = dlane_swap;
	}
	return 0;
}

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static unsigned char zara_cl_shrink_pwm(int val)
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

static void zara_cl_set_backlight(struct msm_fb_data_type *mfd)
{
	led_pwm[1] = zara_cl_shrink_pwm((unsigned char)(mfd->bl_level));

	/* Check LCD power state */
	if (atomic_read(&lcd_power_state) == 0) {
		PR_DISP_DEBUG("%s: LCD is off. Skip backlight setting\n", __func__);
		return;
	}

	/* Turn off dimming before suspend */
	if (led_pwm[1] == 0) {
		cmdreq.cmds = novatek_dim_off_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(novatek_dim_off_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
	}

	cmdreq.cmds = lg_novatek_cmd_backlight_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(lg_novatek_cmd_backlight_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	return;
}

static void zara_cl_dim_on(struct msm_fb_data_type *mfd)
{
	PR_DISP_DEBUG("%s\n",  __FUNCTION__);

	cmdreq.cmds = novatek_dim_on_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(novatek_dim_on_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
}

static void zara_cl_display_on(struct msm_fb_data_type *mfd)
{
}

static void zara_cl_display_off(struct msm_fb_data_type *mfd)
{
	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}
static struct mipi_dsi_panel_platform_data zara_cl_pdata = {
	/* Swap data lane because Zara_cl has the same hardware as TC2 */
	.dlane_swap = 1,
	.enable_wled_bl_ctrl = 0x0,
};

static struct platform_device mipi_dsi_zara_cl_panel_device = {
	.name = "mipi_zara_cl",
	.id = 0,
	.dev = {
		.platform_data = &zara_cl_pdata,
	}
};

static struct platform_driver this_driver = {
	.probe  = zara_cl_lcd_probe,
	.driver = {
		.name   = "mipi_zara_cl",
	},
};

static struct msm_fb_panel_data zara_cl_panel_data = {
	.on		= zara_cl_lcd_on,
	.off		= zara_cl_lcd_off,
	.set_backlight  = zara_cl_set_backlight,
	.display_on	= zara_cl_display_on,
	.display_off	= zara_cl_display_off,
	.dimming_on	= zara_cl_dim_on,
};

static struct msm_panel_info pinfo;
static int ch_used[3];

int mipi_zara_cl_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_zara_cl", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	zara_cl_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &zara_cl_panel_data,
		sizeof(zara_cl_panel_data));
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

/* Modify phy for 8930 */
static struct mipi_dsi_phy_ctrl mipi_dsi_lg_novatek_phy_ctrl = {
	/* DSI Bit Clock at 448 MHz, 2 lane, RGB888 */
	/* regulator */
	{0x09, 0x08, 0x05, 0x00, 0x20},
	/* timing */
	/* Modify DSIPHY_TIMING_CTRL_5 (0x454) from 0x47 to 0x49 to avoid flicker issue */
	{0x7F, 0x1C, 0x13, 0x00, 0x41, 0x49, 0x17,
	0x1F, 0x20, 0x03, 0x04, 0xa0},
	/* phy ctrl */
	{0x5F, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x00, 0x52, 0x30, 0xc4, 0x00, 0x10, 0x07, 0x62,
	0x71, 0x88, 0x99,
	/* Set DSI PLL optimal values at PLL_CTRL registers 16 and 17 to
         * reduce the PLL phase lock time.
         * Change DSI1_DSIPHY_PLL_CTRL_16 from 0x00 to 0x0e
         * Change DSI1_DSIPHY_PLL_CTRL_17 from 0x20 to 0x01
         */
	0x0, 0x14, 0x03, 0x0, 0x2, 0x0e, 0x01, 0x0, 0x01, 0x0},
};

/* DSI register: copy from K2 */
static struct mipi_dsi_reg_set dsi_video_mode_reg_db[] = {
	/* DSI1_DSIPHY_LN1 */
	{0x0340, 0xC0},	/* DSI1_DSIPHY_LN1_CFG0 */
	{0x0344, 0xEF},	/* DSI1_DSIPHY_LN1_CFG1 */
	{0x0358, 0x00},	/* DSI1_DSIPHY_LN1_TEST_STR1 */
	/* DSI1_DSIPHY_LN2 */
	{0x0380, 0xC0},	/* DSI1_DSIPHY_LN2_CFG0 */
	{0x0384, 0xEF},	/* DSI1_DSIPHY_LN2_CFG1 */
	{0x0398, 0x00},	/* DSI1_DSIPHY_LN2_TEST_STR1 */
	{0x0400, 0x80},	/* DSI1_DSIPHY_LNCK_CFG0 */
	{0x0404, 0x23},	/* DSI1_DSIPHY_LNCK_CFG1 */
	{0x0408, 0x00},	/* DSI1_DSIPHY_LNCK_CFG2 */
	{0x040c, 0x00},	/* DSI1_DSIPHY_LNCK_TEST_DATAPATH */
	{0x0414, 0x01},	/* DSI1_DSIPHY_LNCK_TEST_STR0 */
	{0x0418, 0x00}	/* DSI1_DSIPHY_LNCK_TEST_STR1 */
};

static int __init mipi_cmd_lg_novatek_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 56;
	pinfo.height = 99;
	pinfo.camera_backlight = 182;

	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 20;
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.clk_rate = 463000000;

	pinfo.lcdc.border_clr = 0;  /* blk */
	pinfo.lcdc.underflow_clr = 0xff;  /* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1B;
	pinfo.mipi.stream = 0;  /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;

	pinfo.mipi.dsi_phy_db = &mipi_dsi_lg_novatek_phy_ctrl;
	pinfo.mipi.dsi_reg_db = dsi_video_mode_reg_db;
	pinfo.mipi.dsi_reg_db_size = ARRAY_SIZE(dsi_video_mode_reg_db);

	ret = mipi_zara_cl_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	PR_DISP_INFO("%s: panel_type=PANEL_ID_CANIS_LG_NOVATEK\n", __func__);
	init_on_cmds = lg_novatek_video_on_cmds;
	init_on_cmds_count = ARRAY_SIZE(lg_novatek_video_on_cmds);
	display_off_cmds = lg_novatek_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(lg_novatek_display_off_cmds);

	return ret;
}

static int __init mipi_cmd_jdi_novatek_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 56;
	pinfo.height = 99;
	pinfo.camera_backlight = 182;

	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 20;
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.clk_rate = 463000000;

	pinfo.lcdc.border_clr = 0;  /* blk */
	pinfo.lcdc.underflow_clr = 0xff;  /* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1B;
	pinfo.mipi.stream = 0;  /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;

	pinfo.mipi.dsi_phy_db = &mipi_dsi_lg_novatek_phy_ctrl;
	pinfo.mipi.dsi_reg_db = dsi_video_mode_reg_db;
	pinfo.mipi.dsi_reg_db_size = ARRAY_SIZE(dsi_video_mode_reg_db);

	ret = mipi_zara_cl_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	PR_DISP_INFO("%s: panel_type=PANEL_ID_CANIS_JDI_NOVATEK\n", __func__);
	init_on_cmds = jdi_novatek_video_on_cmds;
	init_on_cmds_count = ARRAY_SIZE(jdi_novatek_video_on_cmds);
	display_off_cmds = jdi_novatek_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(jdi_novatek_display_off_cmds);

	return ret;
}

void __init zara_cl_init_fb(void)
{
	platform_device_register(&msm_fb_device);

	if(panel_type != PANEL_ID_NONE) {
		if (board_mfg_mode() == 4) mdp_pdata.cont_splash_enabled = 0x0;
		platform_device_register(&mipi_dsi_zara_cl_panel_device);
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	}
}

static int __init zara_cl_init_panel(void)
{
	if(panel_type == PANEL_ID_NONE) {
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	atomic_set(&lcd_power_state, 1);

	mipi_dsi_buf_alloc(&zara_cl_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&zara_cl_panel_rx_buf, DSI_BUF_SIZE);

	if (panel_type == PANEL_ID_CANIS_LG_NOVATEK)
		mipi_cmd_lg_novatek_init();
	else if (panel_type == PANEL_ID_CANIS_JDI_NOVATEK)
		mipi_cmd_jdi_novatek_init();

	return platform_driver_register(&this_driver);
}

device_initcall_sync(zara_cl_init_panel);
