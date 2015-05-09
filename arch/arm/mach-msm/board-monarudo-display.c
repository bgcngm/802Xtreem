/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/msm_ion.h>
#include <asm/mach-types.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/ion.h>
#include <mach/msm_bus_board.h>
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include "devices.h"
#include "board-monarudo.h"
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <linux/i2c.h>
#include <mach/msm_xo.h>
#include <mach/htc_battery_common.h>

#define hr_msleep(x) msleep(x)

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (1920 * ALIGN(1080, 32) * 4 * 3)
#else
#define MSM_FB_PRIM_BUF_SIZE (1920 * ALIGN(1080, 32) * 4 * 2)
#endif

#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1920 * 1080 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};
struct msm_xo_voter *wa_xo;

#define MIPI_NOVATEK_PANEL_NAME "mipi_cmd_novatek_qhd"
#define MIPI_RENESAS_PANEL_NAME "mipi_video_renesas_fiwvga"
#define MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME "mipi_video_toshiba_wsvga"
#define MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME "mipi_video_chimei_wxga"
#define HDMI_PANEL_NAME "hdmi_msm"
#define TVOUT_PANEL_NAME "tvout_msm"

static int monarudo_detect_panel(const char *name)
{
#if 0
	if (panel_type == PANEL_ID_DLX_SONY_RENESAS) {
		if (!strncmp(name, MIPI_RENESAS_PANEL_NAME,
			strnlen(MIPI_RENESAS_PANEL_NAME,
				PANEL_NAME_MAX_LEN))){
			PR_DISP_INFO("monarudo_%s\n", name);
			return 0;
		}
	} else if (panel_type == PANEL_ID_DLX_SHARP_RENESAS) {
		if (!strncmp(name, MIPI_RENESAS_PANEL_NAME,
			strnlen(MIPI_RENESAS_PANEL_NAME,
				PANEL_NAME_MAX_LEN))){
			PR_DISP_INFO("monarudo_%s\n", name);
			return 0;
		}
	}
#endif
	if (!strncmp(name, HDMI_PANEL_NAME,
		strnlen(HDMI_PANEL_NAME,
			PANEL_NAME_MAX_LEN)))
		return 0;

	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = monarudo_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name              = "msm_fb",
	.id                = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init monarudo_allocate_fb_region(void)
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

#define MDP_VSYNC_GPIO 0

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 577474560 * 2,
		.ib = 866211840 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 605122560 * 2,
		.ib = 756403200 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 660418560 * 2,
		.ib = 825523200 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 764098560 * 2,
		.ib = 955123200 * 2,
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

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};

static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
};
#endif

struct mdp_reg *mdp_gamma = NULL;
int mdp_gamma_count = 0;

struct mdp_reg mdp_gamma_sharp[] = {
	{0x94800, 0x000000, 0x0},
	{0x94804, 0x010101, 0x0},
        {0x94808, 0x020202, 0x0},
        {0x9480C, 0x030303, 0x0},
        {0x94810, 0x040404, 0x0},
        {0x94814, 0x050505, 0x0},
        {0x94818, 0x060606, 0x0},
        {0x9481C, 0x070707, 0x0},
        {0x94820, 0x080808, 0x0},
        {0x94824, 0x090909, 0x0},
        {0x94828, 0x0A0A0A, 0x0},
        {0x9482C, 0x0C0C0C, 0x0},
        {0x94830, 0x0D0D0D, 0x0},
        {0x94834, 0x0E0E0E, 0x0},
        {0x94838, 0x0F0F0F, 0x0},
        {0x9483C, 0x101010, 0x0},
        {0x94840, 0x121212, 0x0},
        {0x94844, 0x131313, 0x0},
        {0x94848, 0x141414, 0x0},
        {0x9484C, 0x151515, 0x0},
        {0x94850, 0x161616, 0x0},
        {0x94854, 0x171717, 0x0},
        {0x94858, 0x181818, 0x0},
        {0x9485C, 0x191919, 0x0},
        {0x94860, 0x1A1A1A, 0x0},
        {0x94864, 0x1B1B1B, 0x0},
        {0x94868, 0x1C1C1C, 0x0},
        {0x9486C, 0x1D1D1D, 0x0},
        {0x94870, 0x1E1E1E, 0x0},
        {0x94874, 0x1F1F1F, 0x0},
        {0x94878, 0x202020, 0x0},
        {0x9487C, 0x222222, 0x0},
        {0x94880, 0x232323, 0x0},
        {0x94884, 0x242424, 0x0},
        {0x94888, 0x252525, 0x0},
        {0x9488C, 0x272727, 0x0},
        {0x94890, 0x282828, 0x0},
        {0x94894, 0x292929, 0x0},
        {0x94898, 0x2A2A2A, 0x0},
        {0x9489C, 0x2B2B2B, 0x0},
        {0x948A0, 0x2C2C2C, 0x0},
        {0x948A4, 0x2E2E2E, 0x0},
        {0x948A8, 0x2F2F2F, 0x0},
        {0x948AC, 0x303030, 0x0},
        {0x948B0, 0x323232, 0x0},
        {0x948B4, 0x333333, 0x0},
        {0x948B8, 0x343434, 0x0},
        {0x948BC, 0x353535, 0x0},
        {0x948C0, 0x363636, 0x0},
        {0x948C4, 0x373737, 0x0},
        {0x948C8, 0x383838, 0x0},
        {0x948CC, 0x3A3A3A, 0x0},
        {0x948D0, 0x3B3B3B, 0x0},
        {0x948D4, 0x3D3D3D, 0x0},
        {0x948D8, 0x3E3E3E, 0x0},
        {0x948DC, 0x3F3F3F, 0x0},
        {0x948E0, 0x404040, 0x0},
        {0x948E4, 0x424242, 0x0},
        {0x948E8, 0x434343, 0x0},
        {0x948EC, 0x454545, 0x0},
        {0x948F0, 0x464646, 0x0},
        {0x948F4, 0x474747, 0x0},
        {0x948F8, 0x484848, 0x0},
        {0x948FC, 0x4A4A4A, 0x0},
        {0x94900, 0x4B4B4B, 0x0},
        {0x94904, 0x4D4D4D, 0x0},
        {0x94908, 0x4E4E4E, 0x0},
        {0x9490C, 0x4F4F4F, 0x0},
        {0x94910, 0x505050, 0x0},
        {0x94914, 0x515151, 0x0},
        {0x94918, 0x525252, 0x0},
        {0x9491C, 0x535353, 0x0},
        {0x94920, 0x545454, 0x0},
        {0x94924, 0x555555, 0x0},
        {0x94928, 0x565656, 0x0},
        {0x9492C, 0x575757, 0x0},
        {0x94930, 0x595959, 0x0},
        {0x94934, 0x5A5A5A, 0x0},
        {0x94938, 0x5B5B5B, 0x0},
        {0x9493C, 0x5C5C5C, 0x0},
        {0x94940, 0x5D5D5D, 0x0},
        {0x94944, 0x5E5E5E, 0x0},
        {0x94948, 0x5F5F5F, 0x0},
        {0x9494C, 0x606060, 0x0},
        {0x94950, 0x616161, 0x0},
        {0x94954, 0x626262, 0x0},
        {0x94958, 0x636363, 0x0},
        {0x9495C, 0x646464, 0x0},
        {0x94960, 0x656565, 0x0},
        {0x94964, 0x666666, 0x0},
        {0x94968, 0x676767, 0x0},
        {0x9496C, 0x686868, 0x0},
        {0x94970, 0x696969, 0x0},
        {0x94974, 0x6A6A6A, 0x0},
        {0x94978, 0x6A6A6A, 0x0},
        {0x9497C, 0x6B6B6B, 0x0},
        {0x94980, 0x6C6C6C, 0x0},
        {0x94984, 0x6D6D6D, 0x0},
        {0x94988, 0x6E6E6E, 0x0},
        {0x9498C, 0x6F6F6F, 0x0},
        {0x94990, 0x707070, 0x0},
        {0x94994, 0x717171, 0x0},
        {0x94998, 0x727272, 0x0},
        {0x9499C, 0x737373, 0x0},
        {0x949A0, 0x747474, 0x0},
        {0x949A4, 0x757575, 0x0},
        {0x949A8, 0x767676, 0x0},
        {0x949AC, 0x777777, 0x0},
        {0x949B0, 0x787878, 0x0},
        {0x949B4, 0x797979, 0x0},
        {0x949B8, 0x7A7A7A, 0x0},
        {0x949BC, 0x7B7B7B, 0x0},
        {0x949C0, 0x7C7C7C, 0x0},
        {0x949C4, 0x7D7D7D, 0x0},
        {0x949C8, 0x7E7E7E, 0x0},
        {0x949CC, 0x7F7F7F, 0x0},
        {0x949D0, 0x808080, 0x0},
        {0x949D4, 0x818181, 0x0},
        {0x949D8, 0x828282, 0x0},
        {0x949DC, 0x838383, 0x0},
        {0x949E0, 0x848484, 0x0},
        {0x949E4, 0x858585, 0x0},
        {0x949E8, 0x868686, 0x0},
        {0x949EC, 0x878787, 0x0},
        {0x949F0, 0x888888, 0x0},
        {0x949F4, 0x898989, 0x0},
        {0x949F8, 0x8A8A8A, 0x0},
        {0x949FC, 0x8B8B8B, 0x0},
        {0x94A00, 0x8C8C8C, 0x0},
        {0x94A04, 0x8D8D8D, 0x0},
        {0x94A08, 0x8E8E8E, 0x0},
        {0x94A0C, 0x8F8F8F, 0x0},
        {0x94A10, 0x909090, 0x0},
        {0x94A14, 0x919191, 0x0},
        {0x94A18, 0x919191, 0x0},
        {0x94A1C, 0x929292, 0x0},
        {0x94A20, 0x939393, 0x0},
        {0x94A24, 0x949494, 0x0},
        {0x94A28, 0x959595, 0x0},
        {0x94A2C, 0x969696, 0x0},
        {0x94A30, 0x979797, 0x0},
        {0x94A34, 0x989898, 0x0},
        {0x94A38, 0x999999, 0x0},
        {0x94A3C, 0x9A9A9A, 0x0},
        {0x94A40, 0x9B9B9B, 0x0},
        {0x94A44, 0x9C9C9C, 0x0},
        {0x94A48, 0x9C9C9C, 0x0},
        {0x94A4C, 0x9D9D9D, 0x0},
        {0x94A50, 0x9E9E9E, 0x0},
        {0x94A54, 0x9F9F9F, 0x0},
        {0x94A58, 0xA0A0A0, 0x0},
        {0x94A5C, 0xA1A1A1, 0x0},
        {0x94A60, 0xA2A2A2, 0x0},
        {0x94A64, 0xA3A3A3, 0x0},
        {0x94A68, 0xA4A4A4, 0x0},
        {0x94A6C, 0xA5A5A5, 0x0},
        {0x94A70, 0xA6A6A6, 0x0},
        {0x94A74, 0xA7A7A7, 0x0},
        {0x94A78, 0xA8A8A8, 0x0},
        {0x94A7C, 0xA9A9A9, 0x0},
        {0x94A80, 0xAAAAAA, 0x0},
        {0x94A84, 0xABABAB, 0x0},
        {0x94A88, 0xACACAC, 0x0},
        {0x94A8C, 0xADADAD, 0x0},
        {0x94A90, 0xAEAEAE, 0x0},
        {0x94A94, 0xAEAEAE, 0x0},
        {0x94A98, 0xAFAFAF, 0x0},
        {0x94A9C, 0xB0B0B0, 0x0},
        {0x94AA0, 0xB1B1B1, 0x0},
        {0x94AA4, 0xB2B2B2, 0x0},
        {0x94AA8, 0xB3B3B3, 0x0},
        {0x94AAC, 0xB4B4B4, 0x0},
        {0x94AB0, 0xB5B5B5, 0x0},
        {0x94AB4, 0xB6B6B6, 0x0},
        {0x94AB8, 0xB6B6B6, 0x0},
        {0x94ABC, 0xB7B7B7, 0x0},
        {0x94AC0, 0xB8B8B8, 0x0},
        {0x94AC4, 0xB9B9B9, 0x0},
        {0x94AC8, 0xBABABA, 0x0},
        {0x94ACC, 0xBBBBBB, 0x0},
        {0x94AD0, 0xBCBCBC, 0x0},
        {0x94AD4, 0xBDBDBD, 0x0},
        {0x94AD8, 0xBEBEBE, 0x0},
        {0x94ADC, 0xBFBFBF, 0x0},
        {0x94AE0, 0xC0C0C0, 0x0},
        {0x94AE4, 0xC1C1C1, 0x0},
        {0x94AE8, 0xC2C2C2, 0x0},
        {0x94AEC, 0xC3C3C3, 0x0},
        {0x94AF0, 0xC4C4C4, 0x0},
        {0x94AF4, 0xC4C4C4, 0x0},
        {0x94AF8, 0xC5C5C5, 0x0},
        {0x94AFC, 0xC6C6C6, 0x0},
        {0x94B00, 0xC7C7C7, 0x0},
        {0x94B04, 0xC8C8C8, 0x0},
        {0x94B08, 0xC9C9C9, 0x0},
        {0x94B0C, 0xCACACA, 0x0},
        {0x94B10, 0xCBCBCB, 0x0},
        {0x94B14, 0xCBCBCB, 0x0},
        {0x94B18, 0xCCCCCC, 0x0},
        {0x94B1C, 0xCDCDCD, 0x0},
        {0x94B20, 0xCECECE, 0x0},
        {0x94B24, 0xCFCFCF, 0x0},
        {0x94B28, 0xD0D0D0, 0x0},
        {0x94B2C, 0xD1D1D1, 0x0},
        {0x94B30, 0xD2D2D2, 0x0},
        {0x94B34, 0xD3D3D3, 0x0},
        {0x94B38, 0xD4D4D4, 0x0},
        {0x94B3C, 0xD5D5D5, 0x0},
        {0x94B40, 0xD6D6D6, 0x0},
        {0x94B44, 0xD7D7D7, 0x0},
        {0x94B48, 0xD7D7D7, 0x0},
        {0x94B4C, 0xD8D8D8, 0x0},
        {0x94B50, 0xD9D9D9, 0x0},
        {0x94B54, 0xDADADA, 0x0},
        {0x94B58, 0xDBDBDB, 0x0},
        {0x94B5C, 0xDCDCDC, 0x0},
        {0x94B60, 0xDDDDDD, 0x0},
        {0x94B64, 0xDDDDDD, 0x0},
        {0x94B68, 0xDEDEDE, 0x0},
        {0x94B6C, 0xDFDFDF, 0x0},
        {0x94B70, 0xE0E0E0, 0x0},
        {0x94B74, 0xE1E1E1, 0x0},
        {0x94B78, 0xE2E2E2, 0x0},
        {0x94B7C, 0xE3E3E3, 0x0},
        {0x94B80, 0xE4E4E4, 0x0},
        {0x94B84, 0xE5E5E5, 0x0},
        {0x94B88, 0xE6E6E6, 0x0},
        {0x94B8C, 0xE7E7E7, 0x0},
        {0x94B90, 0xE8E8E8, 0x0},
        {0x94B94, 0xE8E8E8, 0x0},
        {0x94B98, 0xE9E9E9, 0x0},
        {0x94B9C, 0xEAEAEA, 0x0},
        {0x94BA0, 0xEBEBEB, 0x0},
        {0x94BA4, 0xECECEC, 0x0},
        {0x94BA8, 0xEDEDED, 0x0},
        {0x94BAC, 0xEDEDED, 0x0},
        {0x94BB0, 0xEEEEEE, 0x0},
        {0x94BB4, 0xEFEFEF, 0x0},
        {0x94BB8, 0xF0F0F0, 0x0},
        {0x94BBC, 0xF1F1F1, 0x0},
        {0x94BC0, 0xF2F2F2, 0x0},
        {0x94BC4, 0xF3F3F3, 0x0},
        {0x94BC8, 0xF4F4F4, 0x0},
        {0x94BCC, 0xF5F5F5, 0x0},
        {0x94BD0, 0xF6F6F6, 0x0},
        {0x94BD4, 0xF7F7F7, 0x0},
        {0x94BD8, 0xF8F8F8, 0x0},
        {0x94BDC, 0xF8F8F8, 0x0},
        {0x94BE0, 0xF9F9F9, 0x0},
        {0x94BE4, 0xFAFAFA, 0x0},
        {0x94BE8, 0xFBFBFB, 0x0},
        {0x94BEC, 0xFCFCFC, 0x0},
        {0x94BF0, 0xFDFDFD, 0x0},
        {0x94BF4, 0xFDFDFD, 0x0},
        {0x94BF8, 0xFEFEFE, 0x0},
        {0x94BFC, 0xFFFFFF, 0x0},
        {0x90070, 0x0F, 0x0}
};

struct mdp_reg mdp_gamma_sony[] = {
	{0x94800, 0x000000, 0x0},
	{0x94804, 0x010101, 0x0},
        {0x94808, 0x020202, 0x0},
        {0x9480C, 0x030303, 0x0},
        {0x94810, 0x040404, 0x0},
        {0x94814, 0x050505, 0x0},
        {0x94818, 0x060606, 0x0},
        {0x9481C, 0x070707, 0x0},
        {0x94820, 0x080808, 0x0},
        {0x94824, 0x090909, 0x0},
        {0x94828, 0x0A0A0A, 0x0},
        {0x9482C, 0x0C0C0C, 0x0},
        {0x94830, 0x0D0D0D, 0x0},
        {0x94834, 0x0E0E0E, 0x0},
        {0x94838, 0x0F0F0F, 0x0},
        {0x9483C, 0x101010, 0x0},
        {0x94840, 0x121212, 0x0},
        {0x94844, 0x131313, 0x0},
        {0x94848, 0x141414, 0x0},
        {0x9484C, 0x151515, 0x0},
        {0x94850, 0x161616, 0x0},
        {0x94854, 0x171717, 0x0},
        {0x94858, 0x181818, 0x0},
        {0x9485C, 0x191919, 0x0},
        {0x94860, 0x1A1A1A, 0x0},
        {0x94864, 0x1B1B1B, 0x0},
        {0x94868, 0x1C1C1C, 0x0},
        {0x9486C, 0x1D1D1D, 0x0},
        {0x94870, 0x1E1E1E, 0x0},
        {0x94874, 0x1F1F1F, 0x0},
        {0x94878, 0x202020, 0x0},
        {0x9487C, 0x222222, 0x0},
        {0x94880, 0x232323, 0x0},
        {0x94884, 0x242424, 0x0},
        {0x94888, 0x252525, 0x0},
        {0x9488C, 0x272727, 0x0},
        {0x94890, 0x282828, 0x0},
        {0x94894, 0x292929, 0x0},
        {0x94898, 0x2A2A2A, 0x0},
        {0x9489C, 0x2B2B2B, 0x0},
        {0x948A0, 0x2C2C2C, 0x0},
        {0x948A4, 0x2E2E2E, 0x0},
        {0x948A8, 0x2F2F2F, 0x0},
        {0x948AC, 0x303030, 0x0},
        {0x948B0, 0x323232, 0x0},
        {0x948B4, 0x333333, 0x0},
        {0x948B8, 0x343434, 0x0},
        {0x948BC, 0x353535, 0x0},
        {0x948C0, 0x363636, 0x0},
        {0x948C4, 0x373737, 0x0},
        {0x948C8, 0x383838, 0x0},
        {0x948CC, 0x3A3A3A, 0x0},
        {0x948D0, 0x3B3B3B, 0x0},
        {0x948D4, 0x3D3D3D, 0x0},
        {0x948D8, 0x3E3E3E, 0x0},
        {0x948DC, 0x3F3F3F, 0x0},
        {0x948E0, 0x404040, 0x0},
        {0x948E4, 0x424242, 0x0},
        {0x948E8, 0x434343, 0x0},
        {0x948EC, 0x454545, 0x0},
        {0x948F0, 0x464646, 0x0},
        {0x948F4, 0x474747, 0x0},
        {0x948F8, 0x484848, 0x0},
        {0x948FC, 0x4A4A4A, 0x0},
        {0x94900, 0x4B4B4B, 0x0},
        {0x94904, 0x4D4D4D, 0x0},
        {0x94908, 0x4E4E4E, 0x0},
        {0x9490C, 0x4F4F4F, 0x0},
        {0x94910, 0x505050, 0x0},
        {0x94914, 0x515151, 0x0},
        {0x94918, 0x525252, 0x0},
        {0x9491C, 0x535353, 0x0},
        {0x94920, 0x545454, 0x0},
        {0x94924, 0x555555, 0x0},
        {0x94928, 0x565656, 0x0},
        {0x9492C, 0x575757, 0x0},
        {0x94930, 0x595959, 0x0},
        {0x94934, 0x5A5A5A, 0x0},
        {0x94938, 0x5B5B5B, 0x0},
        {0x9493C, 0x5C5C5C, 0x0},
        {0x94940, 0x5D5D5D, 0x0},
        {0x94944, 0x5E5E5E, 0x0},
        {0x94948, 0x5F5F5F, 0x0},
        {0x9494C, 0x606060, 0x0},
        {0x94950, 0x616161, 0x0},
        {0x94954, 0x626262, 0x0},
        {0x94958, 0x636363, 0x0},
        {0x9495C, 0x646464, 0x0},
        {0x94960, 0x656565, 0x0},
        {0x94964, 0x666666, 0x0},
        {0x94968, 0x676767, 0x0},
        {0x9496C, 0x686868, 0x0},
        {0x94970, 0x696969, 0x0},
        {0x94974, 0x6A6A6A, 0x0},
        {0x94978, 0x6A6A6A, 0x0},
        {0x9497C, 0x6B6B6B, 0x0},
        {0x94980, 0x6C6C6C, 0x0},
        {0x94984, 0x6D6D6D, 0x0},
        {0x94988, 0x6E6E6E, 0x0},
        {0x9498C, 0x6F6F6F, 0x0},
        {0x94990, 0x707070, 0x0},
        {0x94994, 0x717171, 0x0},
        {0x94998, 0x727272, 0x0},
        {0x9499C, 0x737373, 0x0},
        {0x949A0, 0x747474, 0x0},
        {0x949A4, 0x757575, 0x0},
        {0x949A8, 0x767676, 0x0},
        {0x949AC, 0x777777, 0x0},
        {0x949B0, 0x787878, 0x0},
        {0x949B4, 0x797979, 0x0},
        {0x949B8, 0x7A7A7A, 0x0},
        {0x949BC, 0x7B7B7B, 0x0},
        {0x949C0, 0x7C7C7C, 0x0},
        {0x949C4, 0x7D7D7D, 0x0},
        {0x949C8, 0x7E7E7E, 0x0},
        {0x949CC, 0x7F7F7F, 0x0},
        {0x949D0, 0x808080, 0x0},
        {0x949D4, 0x818181, 0x0},
        {0x949D8, 0x828282, 0x0},
        {0x949DC, 0x838383, 0x0},
        {0x949E0, 0x848484, 0x0},
        {0x949E4, 0x858585, 0x0},
        {0x949E8, 0x868686, 0x0},
        {0x949EC, 0x878787, 0x0},
        {0x949F0, 0x888888, 0x0},
        {0x949F4, 0x898989, 0x0},
        {0x949F8, 0x8A8A8A, 0x0},
        {0x949FC, 0x8B8B8B, 0x0},
        {0x94A00, 0x8C8C8C, 0x0},
        {0x94A04, 0x8D8D8D, 0x0},
        {0x94A08, 0x8E8E8E, 0x0},
        {0x94A0C, 0x8F8F8F, 0x0},
        {0x94A10, 0x909090, 0x0},
        {0x94A14, 0x919191, 0x0},
        {0x94A18, 0x919191, 0x0},
        {0x94A1C, 0x929292, 0x0},
        {0x94A20, 0x939393, 0x0},
        {0x94A24, 0x949494, 0x0},
        {0x94A28, 0x959595, 0x0},
        {0x94A2C, 0x969696, 0x0},
        {0x94A30, 0x979797, 0x0},
        {0x94A34, 0x989898, 0x0},
        {0x94A38, 0x999999, 0x0},
        {0x94A3C, 0x9A9A9A, 0x0},
        {0x94A40, 0x9B9B9B, 0x0},
        {0x94A44, 0x9C9C9C, 0x0},
        {0x94A48, 0x9C9C9C, 0x0},
        {0x94A4C, 0x9D9D9D, 0x0},
        {0x94A50, 0x9E9E9E, 0x0},
        {0x94A54, 0x9F9F9F, 0x0},
        {0x94A58, 0xA0A0A0, 0x0},
        {0x94A5C, 0xA1A1A1, 0x0},
        {0x94A60, 0xA2A2A2, 0x0},
        {0x94A64, 0xA3A3A3, 0x0},
        {0x94A68, 0xA4A4A4, 0x0},
        {0x94A6C, 0xA5A5A5, 0x0},
        {0x94A70, 0xA6A6A6, 0x0},
        {0x94A74, 0xA7A7A7, 0x0},
        {0x94A78, 0xA8A8A8, 0x0},
        {0x94A7C, 0xA9A9A9, 0x0},
        {0x94A80, 0xAAAAAA, 0x0},
        {0x94A84, 0xABABAB, 0x0},
        {0x94A88, 0xACACAC, 0x0},
        {0x94A8C, 0xADADAD, 0x0},
        {0x94A90, 0xAEAEAE, 0x0},
        {0x94A94, 0xAEAEAE, 0x0},
        {0x94A98, 0xAFAFAF, 0x0},
        {0x94A9C, 0xB0B0B0, 0x0},
        {0x94AA0, 0xB1B1B1, 0x0},
        {0x94AA4, 0xB2B2B2, 0x0},
        {0x94AA8, 0xB3B3B3, 0x0},
        {0x94AAC, 0xB4B4B4, 0x0},
        {0x94AB0, 0xB5B5B5, 0x0},
        {0x94AB4, 0xB6B6B6, 0x0},
        {0x94AB8, 0xB6B6B6, 0x0},
        {0x94ABC, 0xB7B7B7, 0x0},
        {0x94AC0, 0xB8B8B8, 0x0},
        {0x94AC4, 0xB9B9B9, 0x0},
        {0x94AC8, 0xBABABA, 0x0},
        {0x94ACC, 0xBBBBBB, 0x0},
        {0x94AD0, 0xBCBCBC, 0x0},
        {0x94AD4, 0xBDBDBD, 0x0},
        {0x94AD8, 0xBEBEBE, 0x0},
        {0x94ADC, 0xBFBFBF, 0x0},
        {0x94AE0, 0xC0C0C0, 0x0},
        {0x94AE4, 0xC1C1C1, 0x0},
        {0x94AE8, 0xC2C2C2, 0x0},
        {0x94AEC, 0xC3C3C3, 0x0},
        {0x94AF0, 0xC4C4C4, 0x0},
        {0x94AF4, 0xC4C4C4, 0x0},
        {0x94AF8, 0xC5C5C5, 0x0},
        {0x94AFC, 0xC6C6C6, 0x0},
        {0x94B00, 0xC7C7C7, 0x0},
        {0x94B04, 0xC8C8C8, 0x0},
        {0x94B08, 0xC9C9C9, 0x0},
        {0x94B0C, 0xCACACA, 0x0},
        {0x94B10, 0xCBCBCB, 0x0},
        {0x94B14, 0xCBCBCB, 0x0},
        {0x94B18, 0xCCCCCC, 0x0},
        {0x94B1C, 0xCDCDCD, 0x0},
        {0x94B20, 0xCECECE, 0x0},
        {0x94B24, 0xCFCFCF, 0x0},
        {0x94B28, 0xD0D0D0, 0x0},
        {0x94B2C, 0xD1D1D1, 0x0},
        {0x94B30, 0xD2D2D2, 0x0},
        {0x94B34, 0xD3D3D3, 0x0},
        {0x94B38, 0xD4D4D4, 0x0},
        {0x94B3C, 0xD5D5D5, 0x0},
        {0x94B40, 0xD6D6D6, 0x0},
        {0x94B44, 0xD7D7D7, 0x0},
        {0x94B48, 0xD7D7D7, 0x0},
        {0x94B4C, 0xD8D8D8, 0x0},
        {0x94B50, 0xD9D9D9, 0x0},
        {0x94B54, 0xDADADA, 0x0},
        {0x94B58, 0xDBDBDB, 0x0},
        {0x94B5C, 0xDCDCDC, 0x0},
        {0x94B60, 0xDDDDDD, 0x0},
        {0x94B64, 0xDDDDDD, 0x0},
        {0x94B68, 0xDEDEDE, 0x0},
        {0x94B6C, 0xDFDFDF, 0x0},
        {0x94B70, 0xE0E0E0, 0x0},
        {0x94B74, 0xE1E1E1, 0x0},
        {0x94B78, 0xE2E2E2, 0x0},
        {0x94B7C, 0xE3E3E3, 0x0},
        {0x94B80, 0xE4E4E4, 0x0},
        {0x94B84, 0xE5E5E5, 0x0},
        {0x94B88, 0xE6E6E6, 0x0},
        {0x94B8C, 0xE7E7E7, 0x0},
        {0x94B90, 0xE8E8E8, 0x0},
        {0x94B94, 0xE8E8E8, 0x0},
        {0x94B98, 0xE9E9E9, 0x0},
        {0x94B9C, 0xEAEAEA, 0x0},
        {0x94BA0, 0xEBEBEB, 0x0},
        {0x94BA4, 0xECECEC, 0x0},
        {0x94BA8, 0xEDEDED, 0x0},
        {0x94BAC, 0xEDEDED, 0x0},
        {0x94BB0, 0xEEEEEE, 0x0},
        {0x94BB4, 0xEFEFEF, 0x0},
        {0x94BB8, 0xF0F0F0, 0x0},
        {0x94BBC, 0xF1F1F1, 0x0},
        {0x94BC0, 0xF2F2F2, 0x0},
        {0x94BC4, 0xF3F3F3, 0x0},
        {0x94BC8, 0xF4F4F4, 0x0},
        {0x94BCC, 0xF5F5F5, 0x0},
        {0x94BD0, 0xF6F6F6, 0x0},
        {0x94BD4, 0xF7F7F7, 0x0},
        {0x94BD8, 0xF8F8F8, 0x0},
        {0x94BDC, 0xF8F8F8, 0x0},
        {0x94BE0, 0xF9F9F9, 0x0},
        {0x94BE4, 0xFAFAFA, 0x0},
        {0x94BE8, 0xFBFBFB, 0x0},
        {0x94BEC, 0xFCFCFC, 0x0},
        {0x94BF0, 0xFDFDFD, 0x0},
        {0x94BF4, 0xFDFDFD, 0x0},
        {0x94BF8, 0xFEFEFE, 0x0},
        {0x94BFC, 0xFFFFFF, 0x0},
        {0x90070, 0x0F, 0x0}
};

int monarudo_mdp_gamma(void)
{
	if (mdp_gamma == NULL)
		return 0;

	mdp_color_enhancement(mdp_gamma, mdp_gamma_count);
	return 0;
}


static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_44,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.cont_splash_enabled = 0x01,
	.mdp_iommu_split_domain = 1,
	.mdp_gamma = monarudo_mdp_gamma,
	.mdp_max_clk = 266667000,
};

static char wfd_check_mdp_iommu_split_domain(void)
{
    return mdp_pdata.mdp_iommu_split_domain;
}

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct msm_wfd_platform_data wfd_pdata = {
    .wfd_check_mdp_iommu_split = wfd_check_mdp_iommu_split_domain,
};

static struct platform_device wfd_panel_device = {
    .name = "wfd_panel",
    .id = 0,
    .dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
    .name          = "msm_wfd",
    .id            = -1,
    .dev.platform_data = &wfd_pdata,
};
#endif

void __init monarudo_mdp_writeback(struct memtype_reserve* reserve_table)
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

#ifdef CONFIG_HTC_PNPMGR
extern void set_screen_status(bool onoff);
#endif

static int first_init = 1;
static bool dsi_power_on;
static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_lvs5, *reg_l2;
	static int gpio36, gpio37;
	int rc;

	pr_debug("%s: on=%d\n", __func__, on);

	if (!dsi_power_on) {
		reg_lvs5 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_vddio");
		if (IS_ERR_OR_NULL(reg_lvs5)) {
			pr_err("could not get 8921_lvs5, rc = %ld\n",
				PTR_ERR(reg_lvs5));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_pll_vdda");
		if (IS_ERR_OR_NULL(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		gpio36 = PM8921_GPIO_PM_TO_SYS(V_LCM_N5V_EN); 
		rc = gpio_request(gpio36, "lcd_5v-");
		if (rc) {
			pr_err("request lcd_5v- failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio37 = PM8921_GPIO_PM_TO_SYS(V_LCM_P5V_EN); 
		rc = gpio_request(gpio37, "lcd_5v+");
		if (rc) {
			pr_err("request lcd_5v+ failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_tlmm_config(GPIO_CFG(LCD_RST, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		dsi_power_on = true;
	}

	if (on) {
		if (!first_init) {
			rc = regulator_enable(reg_lvs5);
			if (rc) {
				pr_err("enable lvs5 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			msleep(200);

			gpio_set_value_cansleep(gpio37, 1);
			msleep(10);
			gpio_set_value_cansleep(gpio36, 1);

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
			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_ON);

			gpio_set_value(LCD_RST, 0);
			msleep(10);
			gpio_set_value(LCD_RST, 1);
			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_OFF);
		} else {
			
			rc = regulator_enable(reg_lvs5);
			if (rc) {
				pr_err("enable lvs5 failed, rc=%d\n", rc);
				return -ENODEV;
			}
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
			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_ON);
			msleep(10);
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_OFF);
		}
	} else {
		if (system_rev == XB) {
			gpio_tlmm_config(GPIO_CFG(MBAT_IN_XA_XB, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(MBAT_IN_XA_XB, 0);
		} else if (system_rev >= XC) {
			gpio_tlmm_config(GPIO_CFG(BL_HW_EN_XC_XD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(BL_HW_EN_XC_XD, 0);
		}

		gpio_set_value(LCD_RST, 0);
		msleep(10);
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_set_value_cansleep(gpio36, 0);
		msleep(10);
		gpio_set_value_cansleep(gpio37, 0);
		msleep(100);
		rc = regulator_disable(reg_lvs5);
		if (rc) {
			pr_err("disable reg_lvs5 failed, rc=%d\n", rc);
			return -ENODEV;
		}
	}

#ifdef CONFIG_HTC_PNPMGR
	if (on)
		set_screen_status(true);
	else
		set_screen_status(false);
#endif

	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.dsi_power_save = mipi_dsi_panel_power,
};

static struct mipi_dsi_panel_platform_data *mipi_monarudo_pdata;

static struct dsi_buf monarudo_panel_tx_buf;
static struct dsi_buf monarudo_panel_rx_buf;
static struct dsi_cmd_desc *video_on_cmds = NULL;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static int video_on_cmds_count = 0;
static int display_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static char enter_sleep[2] = {0x10, 0x00}; 
static char exit_sleep[2] = {0x11, 0x00}; 
static char display_off[2] = {0x28, 0x00}; 
static char display_on[2] = {0x29, 0x00}; 

static char write_display_brightness[3]= {0x51, 0x0F, 0xFF};
static char write_control_display[2] = {0x53, 0x24}; 
static struct dsi_cmd_desc renesas_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_display_brightness), write_display_brightness},
};
static struct dsi_cmd_desc renesas_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};
static char interface_setting_0[2] = {0xB0, 0x04};
#if 0
static char Backlght_Control_2[8]= {
	0xB9, 0x0F, 0x18, 0x04,
	0x40, 0x9F, 0x1F, 0x80};
static char BackLight_Control_4[8]= {
	0xBA, 0x0F, 0x18, 0x04,
	0x40, 0x9F, 0x1F, 0xD7};
static char ContrastOptimize[7]= {
	0xD8, 0x01, 0x80, 0x80,
	0x40, 0x42, 0x21};
static char Test_Image_Generator[7]= {
	0xDE, 0x00, 0xFF, 0x07,
	0x10, 0x00, 0x77};
static char gamma_setting_red[25]= {
	0xC7, 0x01, 0x0A, 0x11,
	0x1A, 0x29, 0x45, 0x3B,
	0x4E, 0x5B, 0x64, 0x6C,
	0x75, 0x01, 0x0A, 0x11,
	0x1A, 0x28, 0x41, 0x38,
	0x4C, 0x59, 0x63, 0x6B,
	0x74};
static char gamma_setting_green[25]= {
	0xC8, 0x01, 0x0A, 0x11,
	0x1A, 0x29, 0x45, 0x3B,
	0x4E, 0x5B, 0x64, 0x6C,
	0x75, 0x01, 0x0A, 0x11,
	0x1A, 0x28, 0x41, 0x38,
	0x4C, 0x59, 0x63, 0x6B,
	0x74};
static char gamma_setting_blue[25]= {
	0xC9, 0x01, 0x0A, 0x11,
	0x1A, 0x29, 0x45, 0x3B,
	0x4E, 0x5B, 0x64, 0x6C,
	0x75, 0x01, 0x0A, 0x11,
	0x1A, 0x28, 0x41, 0x38,
	0x4C, 0x59, 0x63, 0x6B,
	0x74};
#endif

static char Color_enhancement[33]= {
	0xCA, 0x01, 0x02, 0xA4,
	0xA4, 0xB8, 0xB4, 0xB0,
	0xA4, 0x08, 0x28, 0x05,
	0xB9, 0x90, 0x70, 0x01,
	0xFF, 0x05, 0xF8, 0x0C,
	0x0C, 0x0C, 0x0C, 0x13,
	0x13, 0xF0, 0x20, 0x10,
	0x10, 0x10, 0x10, 0x10,
	0x10};


static char BackLight_Control_6[8]= {
	0xCE, 0x00, 0x07, 0x00,
	0xC1, 0x24, 0xB2, 0x02};
static char Manufacture_Command_setting[4] = {0xD6, 0x01};
static char nop[4] = {0x00, 0x00};
static char CABC[2] = {0x55, 0x01};
static char hsync_output[4] = {0xC3, 0x01, 0x00, 0x10};
static char protect_on[4] = {0xB0, 0x03};
static char TE_OUT[4] = {0x35, 0x00};
static char deep_standby_off[2] = {0xB1, 0x01};

static struct dsi_cmd_desc sharp_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Color_enhancement), Color_enhancement},
	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(TE_OUT), TE_OUT},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sony_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hsync_output), hsync_output},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Color_enhancement), Color_enhancement},
	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(protect_on), protect_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(TE_OUT), TE_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 20,
		sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 48, sizeof(enter_sleep), enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(deep_standby_off), deep_standby_off},
};

#if 0
static char manufacture_id[2] = {0x04, 0x00}; 

static struct dsi_cmd_desc renesas_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_renesas_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &monarudo_panel_tx_buf;
	rp = &monarudo_panel_rx_buf;
	cmd = &renesas_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	pr_info("%s: manufacture_id=%x", __func__, *lp);
	return *lp;
}
#endif

static int resume_blk = 0;
static struct i2c_client *blk_pwm_client;
static struct dcs_cmd_req cmdreq;

static int monarudo_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;
	if(!first_init) {
		if (mipi->mode == DSI_VIDEO_MODE) {
			mipi_dsi_cmds_tx(&monarudo_panel_tx_buf, video_on_cmds,
				video_on_cmds_count);
		}
	}
	first_init = 0;
	

	return 0;
}

static int monarudo_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	resume_blk = 1;
	return 0;
}
static int __devinit monarudo_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_monarudo_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}
static void monarudo_display_on(struct msm_fb_data_type *mfd)
{
	
	msleep(120);

	cmdreq.cmds = display_on_cmds;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

static void monarudo_display_off(struct msm_fb_data_type *mfd)
{
	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

#define PWM_MIN                   13
#define PWM_DEFAULT               82
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char monarudo_shrink_pwm(int val)
{
	unsigned char shrink_br = BRI_SETTING_MAX;

	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
		shrink_br = PWM_MIN;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
		shrink_br = (val - BRI_SETTING_MIN) * (PWM_DEFAULT - PWM_MIN) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + PWM_MIN;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
		shrink_br = (val - BRI_SETTING_DEF) * (PWM_MAX - PWM_DEFAULT) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + PWM_DEFAULT;
	} else if (val > BRI_SETTING_MAX)
		shrink_br = PWM_MAX;

	PR_DISP_INFO("brightness orig=%d, transformed=%d\n", val, shrink_br);

	return shrink_br;
}

static void monarudo_set_backlight(struct msm_fb_data_type *mfd)
{
	int rc;

	write_display_brightness[2] = monarudo_shrink_pwm((unsigned char)(mfd->bl_level));

	if (resume_blk) {
		resume_blk = 0;

		if (system_rev == XB) {
			gpio_tlmm_config(GPIO_CFG(MBAT_IN_XA_XB, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(MBAT_IN_XA_XB, 1);
		} else if (system_rev >= XC) {
			gpio_tlmm_config(GPIO_CFG(BL_HW_EN_XC_XD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(BL_HW_EN_XC_XD, 1);
		}

		rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x10, 0xC5);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x19, 0x13);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x14, 0xC2);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x79, 0xFF);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x1D, 0xFA);
		if (rc)
			pr_err("i2c write fail\n");
	}

	cmdreq.cmds = (struct dsi_cmd_desc*)&renesas_cmd_backlight_cmds;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	if((mfd->bl_level) == 0) {
		if (system_rev == XB) {
			gpio_tlmm_config(GPIO_CFG(MBAT_IN_XA_XB, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(MBAT_IN_XA_XB, 0);
		} else if (system_rev >= XC) {
			gpio_tlmm_config(GPIO_CFG(BL_HW_EN_XC_XD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(BL_HW_EN_XC_XD, 0);
		}

		resume_blk = 1;
	}

	return;
}

static struct platform_driver this_driver = {
	.probe  = monarudo_lcd_probe,
	.driver = {
		.name   = "mipi_monarudo",
	},
};

static struct msm_fb_panel_data monarudo_panel_data = {
	.on	= monarudo_lcd_on,
	.off	= monarudo_lcd_off,
	.set_backlight = monarudo_set_backlight,
	.display_on = monarudo_display_on,
	.display_off = monarudo_display_off,
};

static struct msm_panel_info pinfo;
static int ch_used[3] = {0};

static int mipi_monarudo_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_monarudo", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	monarudo_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &monarudo_panel_data,
		sizeof(monarudo_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}
	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	
	
	{0x03, 0x08, 0x05, 0x00, 0x20},
	
	{0xDB, 0x35, 0x24, 0x00, 0x65, 0x68, 0x29,
	0x38, 0x3D, 0x3, 0x4, 0xA0},
	
	{0x5F, 0x00, 0x00, 0x10},
	
	{0xFF, 0x00, 0x06, 0x00},
	
	{0x00, 0x38, 0x32, 0xDA, 0x00, 0x10, 0x0F, 0x61,
	0x41, 0x0F, 0x01,
	0x00, 0x1A, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 },
};
static int __init mipi_video_sharp_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 61;
	pinfo.height = 110;
	pinfo.camera_backlight = 176;

	pinfo.lcdc.h_back_porch = 50;
	pinfo.lcdc.h_front_porch = 100;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = 4;
	pinfo.lcd.v_front_porch = 4;
	pinfo.lcd.v_pulse_width = 2;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 848000000;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x02;
	pinfo.mipi.t_clk_pre = 0x2C;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 6;

	ret = mipi_monarudo_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FWVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	video_on_cmds = sharp_video_on_cmds;
	video_on_cmds_count = ARRAY_SIZE(sharp_video_on_cmds);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = sharp_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sharp_display_off_cmds);

	mdp_gamma = mdp_gamma_sharp;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_sharp);


	return ret;
}

static int __init mipi_video_sony_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 61;
	pinfo.height = 110;
	pinfo.camera_backlight = 176;

	pinfo.lcdc.h_back_porch = 50;
	pinfo.lcdc.h_front_porch = 100;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 3;
	pinfo.lcdc.v_front_porch = 3;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = 3;
	pinfo.lcd.v_front_porch = 3;
	pinfo.lcd.v_pulse_width = 2;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 848000000;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x02;
	pinfo.mipi.t_clk_pre = 0x2C;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;

	ret = mipi_monarudo_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FWVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	video_on_cmds = sony_video_on_cmds;
	video_on_cmds_count = ARRAY_SIZE(sony_video_on_cmds);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = sony_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);

	mdp_gamma = mdp_gamma_sony;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_sony);

	return ret;
}

static const struct i2c_device_id pwm_i2c_id[] = {
	{ "pwm_i2c", 0 },
	{ }
};

static int pwm_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
		return -ENODEV;

	blk_pwm_client = client;

	return rc;
}

static struct i2c_driver pwm_i2c_driver = {
	.driver = {
		.name = "pwm_i2c",
		.owner = THIS_MODULE,
	},
	.probe = pwm_i2c_probe,
	.remove =  __exit_p( pwm_i2c_remove),
	.id_table =  pwm_i2c_id,
};
static void __exit pwm_i2c_remove(void)
{
	i2c_del_driver(&pwm_i2c_driver);
}

void __init monarudo_init_fb(void)
{

	platform_device_register(&msm_fb_device);

	if(panel_type != PANEL_ID_NONE) {
		if ((board_mfg_mode() == 4) ||
			(board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode() & 0x1)))
			mdp_pdata.cont_splash_enabled = 0x0;
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
		wa_xo = msm_xo_get(MSM_XO_TCXO_D0, "mipi");
	}
	msm_fb_register_device("dtv", &dtv_pdata);
#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
    platform_device_register(&wfd_panel_device);
    platform_device_register(&wfd_device);
#endif
}

static int __init monarudo_panel_init(void)
{
	int ret;

	if(panel_type == PANEL_ID_NONE)	{
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	ret = i2c_add_driver(&pwm_i2c_driver);

	if (ret)
		pr_err(KERN_ERR "%s: failed to add i2c driver\n", __func__);

	mipi_dsi_buf_alloc(&monarudo_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&monarudo_panel_rx_buf, DSI_BUF_SIZE);

	if (panel_type == PANEL_ID_DLX_SHARP_RENESAS) {
		mipi_video_sharp_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_DLX_SHARP_RENESAS\n", __func__);
	} else if (panel_type == PANEL_ID_DLX_SONY_RENESAS) {
		mipi_video_sony_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_DLX_SONY_RENESAS\n", __func__);
	} else {
		PR_DISP_ERR("%s: panel not supported!!\n", __func__);
		return -ENODEV;
	}

	PR_DISP_INFO("%s\n", __func__);

	return platform_driver_register(&this_driver);
}
device_initcall_sync(monarudo_panel_init);
