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
#include "board-impression_j.h"
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <mach/msm_xo.h>

#define hr_msleep(x) msleep(x)

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (1280 * ALIGN(720, 32) * 4 * 3)
#else
#define MSM_FB_PRIM_BUF_SIZE (1280 * ALIGN(720, 32) * 4 * 2)
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

static int impression_j_detect_panel(const char *name)
{
#if 0
	if (panel_type == PANEL_ID_DLX_SONY_RENESAS) {
		if (!strncmp(name, MIPI_RENESAS_PANEL_NAME,
			strnlen(MIPI_RENESAS_PANEL_NAME,
				PANEL_NAME_MAX_LEN))){
			PR_DISP_INFO("impression_j_%s\n", name);
			return 0;
		}
	} else if (panel_type == PANEL_ID_DLX_SHARP_RENESAS) {
		if (!strncmp(name, MIPI_RENESAS_PANEL_NAME,
			strnlen(MIPI_RENESAS_PANEL_NAME,
				PANEL_NAME_MAX_LEN))){
			PR_DISP_INFO("impression_j_%s\n", name);
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
	.detect_client = impression_j_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name              = "msm_fb",
	.id                = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init impression_j_allocate_fb_region(void)
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
		.ib = 721843200 * 2,
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
struct mdp_reg mdp_gamma_novatek[] = {
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
        {0x9482C, 0x0B0B0B, 0x0},
        {0x94830, 0x0C0C0C, 0x0},
        {0x94834, 0x0D0D0D, 0x0},
        {0x94838, 0x0E0E0E, 0x0},
        {0x9483C, 0x0F0F0F, 0x0},
        {0x94840, 0x101010, 0x0},
        {0x94844, 0x111111, 0x0},
        {0x94848, 0x121212, 0x0},
        {0x9484C, 0x131313, 0x0},
        {0x94850, 0x141414, 0x0},
        {0x94854, 0x151515, 0x0},
        {0x94858, 0x161616, 0x0},
        {0x9485C, 0x171717, 0x0},
        {0x94860, 0x181818, 0x0},
        {0x94864, 0x191919, 0x0},
        {0x94868, 0x1A1A1A, 0x0},
        {0x9486C, 0x1B1B1B, 0x0},
        {0x94870, 0x1C1C1C, 0x0},
        {0x94874, 0x1D1D1D, 0x0},
        {0x94878, 0x1E1E1E, 0x0},
        {0x9487C, 0x1F1F1F, 0x0},
        {0x94880, 0x202020, 0x0},
        {0x94884, 0x212121, 0x0},
        {0x94888, 0x222222, 0x0},
        {0x9488C, 0x232323, 0x0},
        {0x94890, 0x242424, 0x0},
        {0x94894, 0x252525, 0x0},
        {0x94898, 0x262626, 0x0},
        {0x9489C, 0x262727, 0x0},
        {0x948A0, 0x272828, 0x0},
        {0x948A4, 0x282929, 0x0},
        {0x948A8, 0x292A2A, 0x0},
        {0x948AC, 0x2A2B2B, 0x0},
        {0x948B0, 0x2B2C2C, 0x0},
        {0x948B4, 0x2C2D2D, 0x0},
        {0x948B8, 0x2D2E2E, 0x0},
        {0x948BC, 0x2E2F2F, 0x0},
        {0x948C0, 0x2F3030, 0x0},
        {0x948C4, 0x303131, 0x0},
        {0x948C8, 0x313232, 0x0},
        {0x948CC, 0x323333, 0x0},
        {0x948D0, 0x333434, 0x0},
        {0x948D4, 0x343535, 0x0},
        {0x948D8, 0x353636, 0x0},
        {0x948DC, 0x363737, 0x0},
        {0x948E0, 0x373838, 0x0},
        {0x948E4, 0x383939, 0x0},
        {0x948E8, 0x393A3A, 0x0},
        {0x948EC, 0x3A3B3B, 0x0},
        {0x948F0, 0x3B3C3C, 0x0},
        {0x948F4, 0x3C3D3D, 0x0},
        {0x948F8, 0x3D3E3E, 0x0},
        {0x948FC, 0x3E3F3F, 0x0},
        {0x94900, 0x3F4040, 0x0},
        {0x94904, 0x404141, 0x0},
        {0x94908, 0x414242, 0x0},
        {0x9490C, 0x424343, 0x0},
        {0x94910, 0x434444, 0x0},
        {0x94914, 0x444545, 0x0},
        {0x94918, 0x454646, 0x0},
        {0x9491C, 0x464747, 0x0},
        {0x94920, 0x474848, 0x0},
        {0x94924, 0x484949, 0x0},
        {0x94928, 0x494A4A, 0x0},
        {0x9492C, 0x4A4B4B, 0x0},
        {0x94930, 0x4B4C4C, 0x0},
        {0x94934, 0x4C4D4D, 0x0},
        {0x94938, 0x4D4E4E, 0x0},
        {0x9493C, 0x4E4F4F, 0x0},
        {0x94940, 0x4F5050, 0x0},
        {0x94944, 0x505151, 0x0},
        {0x94948, 0x515252, 0x0},
        {0x9494C, 0x525353, 0x0},
        {0x94950, 0x535454, 0x0},
        {0x94954, 0x545555, 0x0},
        {0x94958, 0x555656, 0x0},
        {0x9495C, 0x565757, 0x0},
        {0x94960, 0x575858, 0x0},
        {0x94964, 0x585959, 0x0},
        {0x94968, 0x595A5A, 0x0},
        {0x9496C, 0x5A5B5B, 0x0},
        {0x94970, 0x5B5C5C, 0x0},
        {0x94974, 0x5C5D5D, 0x0},
        {0x94978, 0x5D5E5E, 0x0},
        {0x9497C, 0x5E5F5F, 0x0},
        {0x94980, 0x5F6060, 0x0},
        {0x94984, 0x606161, 0x0},
        {0x94988, 0x616262, 0x0},
        {0x9498C, 0x626363, 0x0},
        {0x94990, 0x636464, 0x0},
        {0x94994, 0x646565, 0x0},
        {0x94998, 0x656666, 0x0},
        {0x9499C, 0x666767, 0x0},
        {0x949A0, 0x676868, 0x0},
        {0x949A4, 0x686969, 0x0},
        {0x949A8, 0x686A6A, 0x0},
        {0x949AC, 0x696B6B, 0x0},
        {0x949B0, 0x6A6C6C, 0x0},
        {0x949B4, 0x6B6D6D, 0x0},
        {0x949B8, 0x6C6E6E, 0x0},
        {0x949BC, 0x6D6F6F, 0x0},
        {0x949C0, 0x6E7070, 0x0},
        {0x949C4, 0x6F7171, 0x0},
        {0x949C8, 0x707272, 0x0},
        {0x949CC, 0x717373, 0x0},
        {0x949D0, 0x727474, 0x0},
        {0x949D4, 0x737575, 0x0},
        {0x949D8, 0x747676, 0x0},
        {0x949DC, 0x757777, 0x0},
        {0x949E0, 0x767878, 0x0},
        {0x949E4, 0x777979, 0x0},
        {0x949E8, 0x787A7A, 0x0},
        {0x949EC, 0x797B7B, 0x0},
        {0x949F0, 0x7A7C7C, 0x0},
        {0x949F4, 0x7B7D7D, 0x0},
        {0x949F8, 0x7C7E7E, 0x0},
        {0x949FC, 0x7D7F7F, 0x0},
        {0x94A00, 0x7E8080, 0x0},
        {0x94A04, 0x7F8181, 0x0},
        {0x94A08, 0x808282, 0x0},
        {0x94A0C, 0x818383, 0x0},
        {0x94A10, 0x828484, 0x0},
        {0x94A14, 0x838585, 0x0},
        {0x94A18, 0x848686, 0x0},
        {0x94A1C, 0x858787, 0x0},
        {0x94A20, 0x868888, 0x0},
        {0x94A24, 0x878989, 0x0},
        {0x94A28, 0x888A8A, 0x0},
        {0x94A2C, 0x898B8B, 0x0},
        {0x94A30, 0x8A8C8C, 0x0},
        {0x94A34, 0x8B8D8D, 0x0},
        {0x94A38, 0x8C8E8E, 0x0},
        {0x94A3C, 0x8D8F8F, 0x0},
        {0x94A40, 0x8E9090, 0x0},
        {0x94A44, 0x8F9191, 0x0},
        {0x94A48, 0x909292, 0x0},
        {0x94A4C, 0x919393, 0x0},
        {0x94A50, 0x929494, 0x0},
        {0x94A54, 0x939595, 0x0},
        {0x94A58, 0x949696, 0x0},
        {0x94A5C, 0x959797, 0x0},
        {0x94A60, 0x969898, 0x0},
        {0x94A64, 0x979999, 0x0},
        {0x94A68, 0x989A9A, 0x0},
        {0x94A6C, 0x999B9B, 0x0},
        {0x94A70, 0x9A9C9C, 0x0},
        {0x94A74, 0x9B9D9D, 0x0},
        {0x94A78, 0x9C9E9E, 0x0},
        {0x94A7C, 0x9D9F9F, 0x0},
        {0x94A80, 0x9EA0A0, 0x0},
        {0x94A84, 0x9FA1A1, 0x0},
        {0x94A88, 0xA0A2A2, 0x0},
        {0x94A8C, 0xA1A3A3, 0x0},
        {0x94A90, 0xA2A4A4, 0x0},
        {0x94A94, 0xA3A5A5, 0x0},
        {0x94A98, 0xA4A6A6, 0x0},
        {0x94A9C, 0xA5A7A7, 0x0},
        {0x94AA0, 0xA6A8A8, 0x0},
        {0x94AA4, 0xA7A9A9, 0x0},
        {0x94AA8, 0xA8AAAA, 0x0},
        {0x94AAC, 0xA9ABAB, 0x0},
        {0x94AB0, 0xAAACAC, 0x0},
        {0x94AB4, 0xABADAD, 0x0},
        {0x94AB8, 0xACAEAE, 0x0},
        {0x94ABC, 0xADAFAF, 0x0},
        {0x94AC0, 0xAEB0B0, 0x0},
        {0x94AC4, 0xAFB1B1, 0x0},
        {0x94AC8, 0xB0B2B2, 0x0},
        {0x94ACC, 0xB1B3B3, 0x0},
        {0x94AD0, 0xB2B4B4, 0x0},
        {0x94AD4, 0xB3B5B5, 0x0},
        {0x94AD8, 0xB4B6B6, 0x0},
        {0x94ADC, 0xB5B7B7, 0x0},
        {0x94AE0, 0xB6B8B8, 0x0},
        {0x94AE4, 0xB7B9B9, 0x0},
        {0x94AE8, 0xB8BABA, 0x0},
        {0x94AEC, 0xB9BBBB, 0x0},
        {0x94AF0, 0xBABCBC, 0x0},
        {0x94AF4, 0xBBBDBD, 0x0},
        {0x94AF8, 0xBCBEBE, 0x0},
        {0x94AFC, 0xBDBFBF, 0x0},
        {0x94B00, 0xBEC0C0, 0x0},
        {0x94B04, 0xBFC1C1, 0x0},
        {0x94B08, 0xC0C2C2, 0x0},
        {0x94B0C, 0xC1C3C3, 0x0},
        {0x94B10, 0xC2C4C4, 0x0},
        {0x94B14, 0xC3C5C5, 0x0},
        {0x94B18, 0xC4C6C6, 0x0},
        {0x94B1C, 0xC5C7C7, 0x0},
        {0x94B20, 0xC6C8C8, 0x0},
        {0x94B24, 0xC7C9C9, 0x0},
        {0x94B28, 0xC8CACA, 0x0},
        {0x94B2C, 0xC9CBCB, 0x0},
        {0x94B30, 0xCACCCC, 0x0},
        {0x94B34, 0xCBCDCD, 0x0},
        {0x94B38, 0xCCCECE, 0x0},
        {0x94B3C, 0xCDCFCF, 0x0},
        {0x94B40, 0xCED0D0, 0x0},
        {0x94B44, 0xCFD1D1, 0x0},
        {0x94B48, 0xD0D2D2, 0x0},
        {0x94B4C, 0xD1D3D3, 0x0},
        {0x94B50, 0xD2D4D4, 0x0},
        {0x94B54, 0xD3D5D5, 0x0},
        {0x94B58, 0xD4D6D6, 0x0},
        {0x94B5C, 0xD4D7D7, 0x0},
        {0x94B60, 0xD5D8D8, 0x0},
        {0x94B64, 0xD6D9D9, 0x0},
        {0x94B68, 0xD7DADA, 0x0},
        {0x94B6C, 0xD8DBDB, 0x0},
        {0x94B70, 0xD9DCDC, 0x0},
        {0x94B74, 0xDADDDD, 0x0},
        {0x94B78, 0xDBDEDE, 0x0},
        {0x94B7C, 0xDCDFDF, 0x0},
        {0x94B80, 0xDDE0E0, 0x0},
        {0x94B84, 0xDEE1E1, 0x0},
        {0x94B88, 0xDFE2E2, 0x0},
        {0x94B8C, 0xE0E3E3, 0x0},
        {0x94B90, 0xE1E4E4, 0x0},
        {0x94B94, 0xE2E5E5, 0x0},
        {0x94B98, 0xE3E6E6, 0x0},
        {0x94B9C, 0xE4E7E7, 0x0},
        {0x94BA0, 0xE5E8E8, 0x0},
        {0x94BA4, 0xE6E9E9, 0x0},
        {0x94BA8, 0xE7EAEA, 0x0},
        {0x94BAC, 0xE8EBEB, 0x0},
        {0x94BB0, 0xE9ECEC, 0x0},
        {0x94BB4, 0xEAEDED, 0x0},
        {0x94BB8, 0xEBEEEE, 0x0},
        {0x94BBC, 0xECEFEF, 0x0},
        {0x94BC0, 0xEDF0F0, 0x0},
        {0x94BC4, 0xEEF1F1, 0x0},
        {0x94BC8, 0xEFF2F2, 0x0},
        {0x94BCC, 0xF0F3F3, 0x0},
        {0x94BD0, 0xF1F4F4, 0x0},
        {0x94BD4, 0xF2F5F5, 0x0},
        {0x94BD8, 0xF3F6F6, 0x0},
        {0x94BDC, 0xF4F7F7, 0x0},
        {0x94BE0, 0xF5F8F8, 0x0},
        {0x94BE4, 0xF6F9F9, 0x0},
        {0x94BE8, 0xF7FAFA, 0x0},
        {0x94BEC, 0xF8FBFB, 0x0},
        {0x94BF0, 0xF9FCFC, 0x0},
        {0x94BF4, 0xFAFDFD, 0x0},
        {0x94BF8, 0xFBFEFE, 0x0},
        {0x94BFC, 0xFCFFFF, 0x0},
        {0x90070, 0x0F, 0x0},
};

int impression_mdp_gamma(void)
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
	.cont_splash_enabled = 0x00,
	.mdp_gamma = impression_mdp_gamma,
	.mdp_iommu_split_domain = 1,
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
void __init impression_j_mdp_writeback(struct memtype_reserve* reserve_table)
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
static int first_init = 1;
static bool dsi_power_on;
struct dcs_cmd_req cmdreq;
uint32_t cfg_panel_te_active[] = {GPIO_CFG(LCD_TE, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)};
uint32_t cfg_panel_te_sleep[] = {GPIO_CFG(LCD_TE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)};
static struct dsi_cmd_desc nvt_LowTemp_wrkr_enter[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, 2, (char[]){0x26, 0x08}},
};

static struct dsi_cmd_desc nvt_LowTemp_wrkr_exit[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, 2, (char[]){0xFF, 0x00}},
};

#ifdef CONFIG_HTC_PNPMGR
extern void set_screen_status(bool onoff);
#endif

static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_lvs5, *reg_l10, *reg_l2;
	static int gpio11;
	int rc;

	PR_DISP_INFO("%s: on=%d\n", __func__, on);

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

		reg_l10 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_avdd");
		if (IS_ERR_OR_NULL(reg_l10)) {
				pr_err("could not get 8921_l10, rc = %ld\n",
						PTR_ERR(reg_l10));
				return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l10, 3000000, 3000000);
		if (rc) {
				pr_err("set_voltage l10 failed, rc=%d\n", rc);
				return -EINVAL;
		}

		gpio11 = PM8921_GPIO_PM_TO_SYS(LCD_RSTz);
		rc = gpio_request(gpio11, "LCD_RSTz");
		if (rc) {
			pr_err("request gpio 11 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		dsi_power_on = true;
	}

	if (on) {
		if (!first_init) {
			rc = regulator_enable(reg_lvs5);
			if (rc) {
				pr_err("enable lvs5 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			msleep(5);
			rc = regulator_set_optimum_mode(reg_l10, 110000);
			if (rc < 0) {
				pr_err("set_optimum_mode l10 failed, rc=%d\n", rc);
				return -EINVAL;
			}
			rc = regulator_enable(reg_l10);
			if (rc) {
				pr_err("enable l10 failed, rc=%d\n", rc);
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

			msleep(20);
			gpio_set_value_cansleep(gpio11, 1);
			msleep(1);
			if (panel_type == PANEL_ID_IMN_SHARP_NT) {
				cmdreq.cmds = nvt_LowTemp_wrkr_enter;
				cmdreq.cmds_cnt = ARRAY_SIZE(nvt_LowTemp_wrkr_enter);
				cmdreq.flags = CMD_REQ_COMMIT;
				cmdreq.rlen = 0;
				cmdreq.cb = NULL;
				mipi_dsi_cmdlist_put(&cmdreq);

				cmdreq.cmds = nvt_LowTemp_wrkr_exit;
				cmdreq.cmds_cnt = ARRAY_SIZE(nvt_LowTemp_wrkr_exit);
				cmdreq.flags = CMD_REQ_COMMIT;
				cmdreq.rlen = 0;
				cmdreq.cb = NULL;
				mipi_dsi_cmdlist_put(&cmdreq);

				msleep(10);
			}
			gpio_set_value_cansleep(gpio11, 0);
			udelay(10);
			gpio_set_value_cansleep(gpio11, 1);
			msleep(20);
			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_OFF);
		} else {
			
			rc = regulator_enable(reg_lvs5);
			if (rc) {
				pr_err("enable lvs5 failed, rc=%d\n", rc);
				return -ENODEV;
			}

			rc = regulator_set_optimum_mode(reg_l10, 110000);
			if (rc < 0) {
				pr_err("set_optimum_mode l10 failed, rc=%d\n", rc);
				return -EINVAL;
			}
			rc = regulator_enable(reg_l10);
			if (rc) {
				pr_err("enable l10 failed, rc=%d\n", rc);
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
		rc = gpio_tlmm_config(cfg_panel_te_active[0], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__,
					cfg_panel_te_active[0], rc);
		}

	} else {
		rc = gpio_tlmm_config(cfg_panel_te_sleep[0], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__,
					cfg_panel_te_sleep[0], rc);
		}

		gpio_set_value_cansleep(gpio11, 0);

		msleep(10);
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_l10);
		if (rc) {
			pr_err("disable reg_l10 failed, rc=%d\n", rc);
			return -ENODEV;
		}

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
	.vsync_gpio = LCD_TE,
	.dsi_power_save = mipi_dsi_panel_power,
};

static struct mipi_dsi_panel_platform_data *mipi_impression_j_pdata;

static struct dsi_buf impression_j_panel_tx_buf;
static struct dsi_buf impression_j_panel_rx_buf;
static struct dsi_cmd_desc *cmd_on_cmds = NULL;
static int cmd_on_cmds_count = 0;

static char enter_sleep[2] = {0x10, 0x00}; 
static char exit_sleep[2] = {0x11, 0x00}; 
static char display_off[2] = {0x28, 0x00}; 
static char display_on[2] = {0x29, 0x00}; 

static char led_pwm1[2] = {0x51, 0xff};	
static char led_pwm2[2] = {0x53, 0x24}; 
static char led_pwm3[2] = {0x55, 0x03}; 

static struct dsi_cmd_desc backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1},
};
static struct dsi_cmd_desc display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static char himax_password[4] = {0xB9, 0xFF, 0x83, 0x92}; 
static char himax_ba[8] = {0xBA, 0x12, 0x83, 0x00, 0xD6, 0xC6, 0x00, 0x0A}; 
static char himax_c0[3] = {0xC0, 0x01, 0x94}; 
static char himax_c6[8] = {0xC6, 0x35, 0x00, 0x00, 0x04}; 

static char himax_d4[2] = {0xD4, 0x00}; 
static char himax_d5[4] = {0xD5, 0x00, 0x00, 0x02}; 
static char himax_bf[4] = {0xBF, 0x05, 0x60, 0x02}; 
#if 1
static char himax_e0[35] = {
				0xE0,0x00,0x04,0x09,
				0x30,0x30,0x3F,0x16,
				0x31,0x06,0x0D,0x0D,
				0x12,0x15,0x12,0x13,
				0x0A,0x1A,0x00,0x04,
				0x09,0x30,0x30,0x3F,
				0x16,0x31,0x06,0x0D,
				0x0D,0x12,0x15,0x12,
				0x13,0x0A,0x1A};
static char himax_e1[35] = {
				0xE1,0x00,0x07,0x0A,
				0x30,0x30,0x3F,0x17,
				0x34,0x04,0x0A,0x0C,
				0x11,0x14,0x11,0x11,
				0x0A,0x17,0x00,0x07,
				0x0A,0x30,0x30,0x3F,
				0x17,0x34,0x04,0x0A,
				0x0C,0x11,0x14,0x11,
				0x11,0x0A,0x17};
static char himax_e2[35] = {
				0xE2,0x00,0x08,0x0D,
				0x30,0x30,0x3F,0x18,
				0x35,0x04,0x0B,0x0B,
				0x11,0x14,0x11,0x10,
				0x0A,0x1F,0x00,0x08,
				0x0D,0x30,0x30,0x3F,
				0x18,0x35,0x04,0x0B,
				0x0B,0x11,0x14,0x11,
				0x10,0x0A,0x1F};
#endif
static char himax_e3[2] = {0xE3, 0x11}; 
static char himax_e5[] = {	0xE5, 0x00, 0x15, 0x0B,
				0x09, 0x05, 0x00, 0x80,
				0x20, 0x80, 0x10, 0x00,
				0x07, 0x07, 0x07, 0x07,
				0x07, 0x80, 0x0A};

static char himax_35[2] = {0x35, 0x00};
static char pwm_freq[] = {0xC9,0x1F,0x01,0x0E,0x3F,0x00,0x80};
static char himax_ca[] = {	0xCA, 0x28, 0x26, 0x24,
				0x23, 0x22, 0x21, 0x20,
				0x20,0x20};

static struct dsi_cmd_desc sharp_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,	sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,	sizeof(himax_password), himax_password},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_d4), himax_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_ba), himax_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c0), himax_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c6), himax_c6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_d5), himax_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,	sizeof(himax_bf), himax_bf},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,	sizeof(himax_e0), himax_e0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,	sizeof(himax_e1), himax_e1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,	sizeof(himax_e2), himax_e2},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,	sizeof(himax_e3), himax_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,	sizeof(himax_e5), himax_e5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,	sizeof(himax_35), himax_35},
	

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,	sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,	sizeof(led_pwm3), led_pwm3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(pwm_freq), pwm_freq},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_ca), himax_ca},
	
	
};
static char set_threelane[2] = {0xBA, 0x02}; 
static char display_mode_cmd[2] = {0xC2, 0x08}; 
static char enable_te[2] = {0x35, 0x00};
struct dsi_cmd_desc sharp_nt_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#if 1
        
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x03}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x08}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x07}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x0B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x11}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x18}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x20}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x27}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x2A}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x2E}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x2F}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x2C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x24}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x1B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x13}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x0C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x07}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif
#if 0
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
#endif
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x0E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

#if 1 
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x08} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0x21} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0x22} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x01} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x1C} },
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

#endif

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x2D} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0xFF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0xF7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0xEF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0xE7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0xDF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0xD7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0xCF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0xC7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0xBF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0xB7} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x06}},

	
	

	
	

	
	

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},
};

static struct dsi_cmd_desc display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
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

	tp = &impression_j_panel_tx_buf;
	rp = &impression_j_panel_rx_buf;
	cmd = &renesas_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	pr_info("%s: manufacture_id=%x", __func__, *lp);
	return *lp;
}
#endif

static int impression_j_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;
	if (!first_init) {
		cmdreq.cmds = cmd_on_cmds;
		cmdreq.cmds_cnt = cmd_on_cmds_count;
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
	}
	first_init = 0;
	

	PR_DISP_INFO("%s done\n", __func__);
	return 0;
}

static int impression_j_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(display_off_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s done\n", __func__);
	return 0;
}
static int __devinit impression_j_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_impression_j_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	PR_DISP_INFO("%s done\n", __func__);
	return 0;
}
static void impression_j_display_on(struct msm_fb_data_type *mfd)
{
#if 0
	mutex_lock(&mfd->dma->ov_mutex);


	mipi_dsi_cmds_tx(&impression_j_panel_tx_buf, display_on_cmds,
			ARRAY_SIZE(display_on_cmds));

	mutex_unlock(&mfd->dma->ov_mutex);
#endif
	cmdreq.cmds = display_on_cmds;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

#define PWM_MIN                   6
#define PWM_DEFAULT               80
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static unsigned char impression_j_shrink_pwm(int val)
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

static void impression_j_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;

	mipi  = &mfd->panel_info.mipi;
#if 0
	mutex_lock(&mfd->dma->ov_mutex);
	if (mdp4_overlay_dsi_state_get() <= ST_DSI_SUSPEND) {
		mutex_unlock(&mfd->dma->ov_mutex);
		return;
	}

	led_pwm1[1] = impression_j_shrink_pwm((unsigned char)(mfd->bl_level));

	mipi_dsi_cmds_tx(&impression_j_panel_tx_buf, backlight_cmds,
			ARRAY_SIZE(backlight_cmds));
	mutex_unlock(&mfd->dma->ov_mutex);
#endif
	led_pwm1[1] = impression_j_shrink_pwm((unsigned char)(mfd->bl_level));

	cmdreq.cmds = backlight_cmds;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	return;
}

static struct platform_driver this_driver = {
	.probe  = impression_j_lcd_probe,
	.driver = {
		.name   = "mipi_impression_j",
	},
};

static struct msm_fb_panel_data impression_j_panel_data = {
	.on	= impression_j_lcd_on,
	.off	= impression_j_lcd_off,
	.set_backlight = impression_j_set_backlight,
	.display_on = impression_j_display_on,
};

static struct msm_panel_info pinfo;
static int ch_used[3] = {0};

static int mipi_impression_j_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_impression_j", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	impression_j_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &impression_j_panel_data,
		sizeof(impression_j_panel_data));
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

static struct mipi_dsi_phy_ctrl phy_ctrl_720p_id311100 = {
	
	
	{0x03, 0x0A, 0x04, 0x00, 0x20},
	
	{0x96, 0x36, 0x17, 0x00, 0x4A, 0x54, 0x1B,
	0x39, 0x27, 0x03, 0x04, 0xA0},
	
	{0x5f, 0x00, 0x00, 0x10},
	
	{0xFF, 0x00, 0x06, 0x00},
	
	{0x0, 0x11, 0xB1, 0xDA, 0x00, 0x50, 0x48, 0x63,
	0x40, 0x07, 0x00,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
static struct mipi_dsi_phy_ctrl mipi_dsi_sharp_panel_idA1B100_phy_ctrl_720p = {
	
	
	{0x03, 0x08, 0x05, 0x00, 0x20},
	
	{0x9B, 0x38, 0x18, 0x00, 0x4B, 0x51, 0x1C,
	0x3B, 0x29, 0x03, 0x04, 0xA0},
	
	{0x5F, 0x00, 0x00, 0x10},
	
	{0xFF, 0x00, 0x06, 0x00},
	
	{0x0, 0x38, 0x32, 0xDA, 0x00, 0x10, 0x0F, 0x61,
	0x41, 0x0F, 0x01,
	0x00, 0x1A, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 },
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
        pinfo.width = 58;
        pinfo.height = 103;
	pinfo.camera_backlight = 183;

	pinfo.lcdc.h_back_porch = 116;
	pinfo.lcdc.h_front_porch = 184;
	pinfo.lcdc.h_pulse_width = 24;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 24;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = 4;
	pinfo.lcd.v_front_porch = 24;
	pinfo.lcd.v_pulse_width = 2;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 548000000;

	pinfo.is_3d_panel = FB_TYPE_3D_PANEL;
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; 

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;

	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 10;
	pinfo.mipi.t_clk_pre = 164;
	pinfo.mipi.stream = 0;	
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; 
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;

	pinfo.mipi.frame_rate = 58;
	pinfo.mipi.dsi_phy_db = &phy_ctrl_720p_id311100;

	ret = mipi_impression_j_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FWVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	cmd_on_cmds = sharp_cmd_on_cmds;
	cmd_on_cmds_count = ARRAY_SIZE(sharp_cmd_on_cmds);

	PR_DISP_INFO("%s\n", __func__);
	return ret;
}
static int __init mipi_video_sharp_nt_720p_pt_init(void)
{
	int ret;

	pinfo.type = MIPI_CMD_PANEL;
	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6096; 
	pinfo.mipi.te_sel = 1; 
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;

	pinfo.xres = 720;
	pinfo.yres = 1280;

	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 58;
	pinfo.height = 103;
	pinfo.camera_backlight = 183;

	pinfo.lcdc.h_back_porch = 125;
	pinfo.lcdc.h_front_porch = 122;
	pinfo.lcdc.h_pulse_width = 1;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 6;
	pinfo.lcdc.v_pulse_width = 1;

	pinfo.lcd.v_back_porch = 2;
	pinfo.lcd.v_front_porch = 6;
	pinfo.lcd.v_pulse_width = 1;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 569000000;

	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x10;
	pinfo.mipi.t_clk_pre = 0x21;
	pinfo.mipi.stream = 0; 

	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 57;
	pinfo.mipi.dsi_phy_db = &mipi_dsi_sharp_panel_idA1B100_phy_ctrl_720p;

	ret = mipi_impression_j_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_WVGA_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	cmd_on_cmds = sharp_nt_cmd_on_cmds;
	cmd_on_cmds_count = ARRAY_SIZE(sharp_nt_cmd_on_cmds);

	mdp_gamma = mdp_gamma_novatek;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_novatek);

	PR_DISP_INFO("%s\n", __func__);
	return ret;
}

void __init impression_j_init_fb(void)
{

	platform_device_register(&msm_fb_device);

	if(panel_type != PANEL_ID_NONE) {
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

static int __init impression_j_panel_init(void)
{

	if(panel_type == PANEL_ID_NONE)	{
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	mipi_dsi_buf_alloc(&impression_j_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&impression_j_panel_rx_buf, DSI_BUF_SIZE);

	if (panel_type == PANEL_ID_IMN_SHARP_HX) {
		mipi_cmd_sharp_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_IMN_SHARP_HX\n", __func__);
	} else if (panel_type == PANEL_ID_IMN_SHARP_NT) {
		mipi_video_sharp_nt_720p_pt_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_IMN_SHARP_NT\n", __func__);

	} else {
		PR_DISP_ERR("%s: panel not supported!!\n", __func__);
		return -ENODEV;
	}

	PR_DISP_INFO("%s\n", __func__);

	return platform_driver_register(&this_driver);
}
late_initcall(impression_j_panel_init);
