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
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <mach/msm_xo.h>

#include "board-t6tl.h"

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
static char ptype[60] = "PANEL type = ";
const size_t ptype_len = ( 60 - sizeof("PANEL type = "));

#define HDMI_PANEL_NAME "hdmi_msm"
#define TVOUT_PANEL_NAME "tvout_msm"

static int t6china_detect_panel(const char *name)
{
	if (!strncmp(name, HDMI_PANEL_NAME,
		strnlen(HDMI_PANEL_NAME,
			PANEL_NAME_MAX_LEN)))
		return 0;

	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = t6china_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name              = "msm_fb",
	.id                = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init t6china_allocate_fb_region(void)
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

struct mdp_reg mdp_gamma_renesas[] = {
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
        {0x9489C, 0x272727, 0x0},
        {0x948A0, 0x282828, 0x0},
        {0x948A4, 0x292929, 0x0},
        {0x948A8, 0x2A2A2A, 0x0},
        {0x948AC, 0x2B2B2B, 0x0},
        {0x948B0, 0x2C2C2C, 0x0},
        {0x948B4, 0x2D2D2D, 0x0},
        {0x948B8, 0x2E2E2E, 0x0},
        {0x948BC, 0x2F2F2F, 0x0},
        {0x948C0, 0x303030, 0x0},
        {0x948C4, 0x313131, 0x0},
        {0x948C8, 0x323232, 0x0},
        {0x948CC, 0x333333, 0x0},
        {0x948D0, 0x343434, 0x0},
        {0x948D4, 0x353535, 0x0},
        {0x948D8, 0x363636, 0x0},
        {0x948DC, 0x373737, 0x0},
        {0x948E0, 0x383838, 0x0},
        {0x948E4, 0x393939, 0x0},
        {0x948E8, 0x3A3A3A, 0x0},
        {0x948EC, 0x3B3B3B, 0x0},
        {0x948F0, 0x3C3C3C, 0x0},
        {0x948F4, 0x3D3D3D, 0x0},
        {0x948F8, 0x3E3E3E, 0x0},
        {0x948FC, 0x3F3F3F, 0x0},
        {0x94900, 0x404040, 0x0},
        {0x94904, 0x414141, 0x0},
        {0x94908, 0x424242, 0x0},
        {0x9490C, 0x434343, 0x0},
        {0x94910, 0x444444, 0x0},
        {0x94914, 0x454545, 0x0},
        {0x94918, 0x464646, 0x0},
        {0x9491C, 0x474747, 0x0},
        {0x94920, 0x484848, 0x0},
        {0x94924, 0x494949, 0x0},
        {0x94928, 0x4A4A4A, 0x0},
        {0x9492C, 0x4B4B4B, 0x0},
        {0x94930, 0x4C4C4C, 0x0},
        {0x94934, 0x4D4D4D, 0x0},
        {0x94938, 0x4E4E4E, 0x0},
        {0x9493C, 0x4F4F4F, 0x0},
        {0x94940, 0x505050, 0x0},
        {0x94944, 0x515151, 0x0},
        {0x94948, 0x525252, 0x0},
        {0x9494C, 0x535353, 0x0},
        {0x94950, 0x545454, 0x0},
        {0x94954, 0x555555, 0x0},
        {0x94958, 0x565656, 0x0},
        {0x9495C, 0x575757, 0x0},
        {0x94960, 0x585858, 0x0},
        {0x94964, 0x595959, 0x0},
        {0x94968, 0x5A5A5A, 0x0},
        {0x9496C, 0x5B5B5B, 0x0},
        {0x94970, 0x5C5C5C, 0x0},
        {0x94974, 0x5D5D5D, 0x0},
        {0x94978, 0x5E5E5E, 0x0},
        {0x9497C, 0x5F5F5F, 0x0},
        {0x94980, 0x606060, 0x0},
        {0x94984, 0x616161, 0x0},
        {0x94988, 0x626262, 0x0},
        {0x9498C, 0x636363, 0x0},
        {0x94990, 0x646464, 0x0},
        {0x94994, 0x656565, 0x0},
        {0x94998, 0x666666, 0x0},
        {0x9499C, 0x676767, 0x0},
        {0x949A0, 0x686868, 0x0},
        {0x949A4, 0x696969, 0x0},
        {0x949A8, 0x6A6A6A, 0x0},
        {0x949AC, 0x6B6B6B, 0x0},
        {0x949B0, 0x6C6C6C, 0x0},
        {0x949B4, 0x6D6D6D, 0x0},
        {0x949B8, 0x6E6E6E, 0x0},
        {0x949BC, 0x6F6F6F, 0x0},
        {0x949C0, 0x707070, 0x0},
        {0x949C4, 0x717171, 0x0},
        {0x949C8, 0x727272, 0x0},
        {0x949CC, 0x737373, 0x0},
        {0x949D0, 0x747474, 0x0},
        {0x949D4, 0x757575, 0x0},
        {0x949D8, 0x767676, 0x0},
        {0x949DC, 0x777777, 0x0},
        {0x949E0, 0x787878, 0x0},
        {0x949E4, 0x797979, 0x0},
        {0x949E8, 0x7A7A7A, 0x0},
        {0x949EC, 0x7B7B7B, 0x0},
        {0x949F0, 0x7C7C7C, 0x0},
        {0x949F4, 0x7D7D7D, 0x0},
        {0x949F8, 0x7E7E7E, 0x0},
        {0x949FC, 0x7F7F7F, 0x0},
        {0x94A00, 0x808080, 0x0},
        {0x94A04, 0x818181, 0x0},
        {0x94A08, 0x828282, 0x0},
        {0x94A0C, 0x838383, 0x0},
        {0x94A10, 0x848484, 0x0},
        {0x94A14, 0x858585, 0x0},
        {0x94A18, 0x868686, 0x0},
        {0x94A1C, 0x878787, 0x0},
        {0x94A20, 0x888788, 0x0},
        {0x94A24, 0x898889, 0x0},
        {0x94A28, 0x8A898A, 0x0},
        {0x94A2C, 0x8B8A8B, 0x0},
        {0x94A30, 0x8C8B8C, 0x0},
        {0x94A34, 0x8D8C8D, 0x0},
        {0x94A38, 0x8E8D8E, 0x0},
        {0x94A3C, 0x8F8E8F, 0x0},
        {0x94A40, 0x908F90, 0x0},
        {0x94A44, 0x919091, 0x0},
        {0x94A48, 0x929192, 0x0},
        {0x94A4C, 0x939293, 0x0},
        {0x94A50, 0x949394, 0x0},
        {0x94A54, 0x959495, 0x0},
        {0x94A58, 0x969596, 0x0},
        {0x94A5C, 0x979697, 0x0},
        {0x94A60, 0x989698, 0x0},
        {0x94A64, 0x999799, 0x0},
        {0x94A68, 0x9A989A, 0x0},
        {0x94A6C, 0x9B999B, 0x0},
        {0x94A70, 0x9C9A9C, 0x0},
        {0x94A74, 0x9D9B9D, 0x0},
        {0x94A78, 0x9E9C9E, 0x0},
        {0x94A7C, 0x9F9D9F, 0x0},
        {0x94A80, 0xA09EA0, 0x0},
        {0x94A84, 0xA19FA1, 0x0},
        {0x94A88, 0xA2A0A2, 0x0},
        {0x94A8C, 0xA3A1A3, 0x0},
        {0x94A90, 0xA4A2A4, 0x0},
        {0x94A94, 0xA5A3A5, 0x0},
        {0x94A98, 0xA6A4A6, 0x0},
        {0x94A9C, 0xA7A5A7, 0x0},
        {0x94AA0, 0xA8A5A8, 0x0},
        {0x94AA4, 0xA9A6A9, 0x0},
        {0x94AA8, 0xAAA7AA, 0x0},
        {0x94AAC, 0xABA8AB, 0x0},
        {0x94AB0, 0xACA9AC, 0x0},
        {0x94AB4, 0xADAAAD, 0x0},
        {0x94AB8, 0xAEABAE, 0x0},
        {0x94ABC, 0xAFACAF, 0x0},
        {0x94AC0, 0xB0ADB0, 0x0},
        {0x94AC4, 0xB1AEB1, 0x0},
        {0x94AC8, 0xB2AFB2, 0x0},
        {0x94ACC, 0xB3B0B3, 0x0},
        {0x94AD0, 0xB4B1B4, 0x0},
        {0x94AD4, 0xB5B2B5, 0x0},
        {0x94AD8, 0xB6B3B6, 0x0},
        {0x94ADC, 0xB7B4B7, 0x0},
        {0x94AE0, 0xB8B4B8, 0x0},
        {0x94AE4, 0xB9B5B9, 0x0},
        {0x94AE8, 0xBAB6BA, 0x0},
        {0x94AEC, 0xBBB7BB, 0x0},
        {0x94AF0, 0xBCB8BC, 0x0},
        {0x94AF4, 0xBDB9BD, 0x0},
        {0x94AF8, 0xBEBABE, 0x0},
        {0x94AFC, 0xBFBBBF, 0x0},
        {0x94B00, 0xC0BCC0, 0x0},
        {0x94B04, 0xC1BDC1, 0x0},
        {0x94B08, 0xC2BEC2, 0x0},
        {0x94B0C, 0xC3BFC3, 0x0},
        {0x94B10, 0xC4C0C4, 0x0},
        {0x94B14, 0xC5C1C5, 0x0},
        {0x94B18, 0xC6C2C6, 0x0},
        {0x94B1C, 0xC7C3C7, 0x0},
        {0x94B20, 0xC8C3C8, 0x0},
        {0x94B24, 0xC9C4C9, 0x0},
        {0x94B28, 0xCAC5CA, 0x0},
        {0x94B2C, 0xCBC6CB, 0x0},
        {0x94B30, 0xCCC7CC, 0x0},
        {0x94B34, 0xCDC8CD, 0x0},
        {0x94B38, 0xCEC9CE, 0x0},
        {0x94B3C, 0xCFCACF, 0x0},
        {0x94B40, 0xD0CBD0, 0x0},
        {0x94B44, 0xD1CCD1, 0x0},
        {0x94B48, 0xD2CDD2, 0x0},
        {0x94B4C, 0xD3CED3, 0x0},
        {0x94B50, 0xD4CFD4, 0x0},
        {0x94B54, 0xD5D0D5, 0x0},
        {0x94B58, 0xD6D1D6, 0x0},
        {0x94B5C, 0xD7D2D7, 0x0},
        {0x94B60, 0xD8D2D8, 0x0},
        {0x94B64, 0xD9D3D9, 0x0},
        {0x94B68, 0xDAD4DA, 0x0},
        {0x94B6C, 0xDBD5DB, 0x0},
        {0x94B70, 0xDCD6DC, 0x0},
        {0x94B74, 0xDDD7DD, 0x0},
        {0x94B78, 0xDED8DE, 0x0},
        {0x94B7C, 0xDFD9DF, 0x0},
        {0x94B80, 0xE0DAE0, 0x0},
        {0x94B84, 0xE1DBE1, 0x0},
        {0x94B88, 0xE2DCE2, 0x0},
        {0x94B8C, 0xE3DDE3, 0x0},
        {0x94B90, 0xE4DEE4, 0x0},
        {0x94B94, 0xE5DFE5, 0x0},
        {0x94B98, 0xE6E0E6, 0x0},
        {0x94B9C, 0xE7E1E7, 0x0},
        {0x94BA0, 0xE8E1E8, 0x0},
        {0x94BA4, 0xE9E2E9, 0x0},
        {0x94BA8, 0xEAE3EA, 0x0},
        {0x94BAC, 0xEBE4EB, 0x0},
        {0x94BB0, 0xECE5EC, 0x0},
        {0x94BB4, 0xEDE6ED, 0x0},
        {0x94BB8, 0xEEE7EE, 0x0},
        {0x94BBC, 0xEFE8EF, 0x0},
        {0x94BC0, 0xF0E9F0, 0x0},
        {0x94BC4, 0xF1EAF1, 0x0},
        {0x94BC8, 0xF2EBF2, 0x0},
        {0x94BCC, 0xF3ECF3, 0x0},
        {0x94BD0, 0xF4EDF4, 0x0},
        {0x94BD4, 0xF5EEF5, 0x0},
        {0x94BD8, 0xF6EFF6, 0x0},
        {0x94BDC, 0xF7F0F7, 0x0},
        {0x94BE0, 0xF8F0F8, 0x0},
        {0x94BE4, 0xF9F1F9, 0x0},
        {0x94BE8, 0xFAF2FA, 0x0},
        {0x94BEC, 0xFBF3FB, 0x0},
        {0x94BF0, 0xFCF4FC, 0x0},
        {0x94BF4, 0xFDF5FD, 0x0},
        {0x94BF8, 0xFEF6FE, 0x0},
        {0x94BFC, 0xFFF7FF, 0x0},
        {0x90070, 0x0F, 0x0},

};

struct mdp_reg mdp_gamma_renesas_c3[] = {
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
	{0x9489C, 0x272727, 0x0},
	{0x948A0, 0x282828, 0x0},
	{0x948A4, 0x292929, 0x0},
	{0x948A8, 0x2A2A2A, 0x0},
	{0x948AC, 0x2B2B2B, 0x0},
	{0x948B0, 0x2C2C2C, 0x0},
	{0x948B4, 0x2D2D2D, 0x0},
	{0x948B8, 0x2E2E2E, 0x0},
	{0x948BC, 0x2F2F2F, 0x0},
	{0x948C0, 0x303030, 0x0},
	{0x948C4, 0x313131, 0x0},
	{0x948C8, 0x323232, 0x0},
	{0x948CC, 0x333333, 0x0},
	{0x948D0, 0x343434, 0x0},
	{0x948D4, 0x353535, 0x0},
	{0x948D8, 0x363636, 0x0},
	{0x948DC, 0x373737, 0x0},
	{0x948E0, 0x383838, 0x0},
	{0x948E4, 0x393939, 0x0},
	{0x948E8, 0x3A3A3A, 0x0},
	{0x948EC, 0x3B3B3B, 0x0},
	{0x948F0, 0x3C3C3C, 0x0},
	{0x948F4, 0x3D3D3D, 0x0},
	{0x948F8, 0x3E3E3E, 0x0},
	{0x948FC, 0x3F3F3F, 0x0},
	{0x94900, 0x404040, 0x0},
	{0x94904, 0x414141, 0x0},
	{0x94908, 0x424242, 0x0},
	{0x9490C, 0x434343, 0x0},
	{0x94910, 0x444444, 0x0},
	{0x94914, 0x454545, 0x0},
	{0x94918, 0x464646, 0x0},
	{0x9491C, 0x474747, 0x0},
	{0x94920, 0x484848, 0x0},
	{0x94924, 0x494949, 0x0},
	{0x94928, 0x4A4A4A, 0x0},
	{0x9492C, 0x4B4B4B, 0x0},
	{0x94930, 0x4C4C4C, 0x0},
	{0x94934, 0x4D4D4D, 0x0},
	{0x94938, 0x4E4E4E, 0x0},
	{0x9493C, 0x4F4F4F, 0x0},
	{0x94940, 0x505050, 0x0},
	{0x94944, 0x515151, 0x0},
	{0x94948, 0x525252, 0x0},
	{0x9494C, 0x535353, 0x0},
	{0x94950, 0x545454, 0x0},
	{0x94954, 0x555555, 0x0},
	{0x94958, 0x565656, 0x0},
	{0x9495C, 0x575757, 0x0},
	{0x94960, 0x585858, 0x0},
	{0x94964, 0x595959, 0x0},
	{0x94968, 0x5A5A5A, 0x0},
	{0x9496C, 0x5B5B5B, 0x0},
	{0x94970, 0x5C5C5C, 0x0},
	{0x94974, 0x5D5D5D, 0x0},
	{0x94978, 0x5E5E5E, 0x0},
	{0x9497C, 0x5F5F5F, 0x0},
	{0x94980, 0x60605F, 0x0},
	{0x94984, 0x616160, 0x0},
	{0x94988, 0x626261, 0x0},
	{0x9498C, 0x636362, 0x0},
	{0x94990, 0x646463, 0x0},
	{0x94994, 0x656564, 0x0},
	{0x94998, 0x666665, 0x0},
	{0x9499C, 0x676766, 0x0},
	{0x949A0, 0x686867, 0x0},
	{0x949A4, 0x696968, 0x0},
	{0x949A8, 0x6A6A69, 0x0},
	{0x949AC, 0x6B6B69, 0x0},
	{0x949B0, 0x6C6C6A, 0x0},
	{0x949B4, 0x6D6D6B, 0x0},
	{0x949B8, 0x6E6E6C, 0x0},
	{0x949BC, 0x6F6F6D, 0x0},
	{0x949C0, 0x70706E, 0x0},
	{0x949C4, 0x71716F, 0x0},
	{0x949C8, 0x727270, 0x0},
	{0x949CC, 0x737371, 0x0},
	{0x949D0, 0x747472, 0x0},
	{0x949D4, 0x757573, 0x0},
	{0x949D8, 0x767674, 0x0},
	{0x949DC, 0x777775, 0x0},
	{0x949E0, 0x787876, 0x0},
	{0x949E4, 0x797977, 0x0},
	{0x949E8, 0x7A7A78, 0x0},
	{0x949EC, 0x7B7B79, 0x0},
	{0x949F0, 0x7C7C7A, 0x0},
	{0x949F4, 0x7D7D7B, 0x0},
	{0x949F8, 0x7E7E7C, 0x0},
	{0x949FC, 0x7F7F7D, 0x0},
	{0x94A00, 0x80807E, 0x0},
	{0x94A04, 0x81817F, 0x0},
	{0x94A08, 0x828280, 0x0},
	{0x94A0C, 0x838381, 0x0},
	{0x94A10, 0x848482, 0x0},
	{0x94A14, 0x858583, 0x0},
	{0x94A18, 0x868684, 0x0},
	{0x94A1C, 0x878785, 0x0},
	{0x94A20, 0x888886, 0x0},
	{0x94A24, 0x898987, 0x0},
	{0x94A28, 0x8A8A88, 0x0},
	{0x94A2C, 0x8B8B89, 0x0},
	{0x94A30, 0x8C8C8A, 0x0},
	{0x94A34, 0x8D8D8B, 0x0},
	{0x94A38, 0x8E8E8C, 0x0},
	{0x94A3C, 0x8F8F8D, 0x0},
	{0x94A40, 0x90908E, 0x0},
	{0x94A44, 0x91918F, 0x0},
	{0x94A48, 0x929290, 0x0},
	{0x94A4C, 0x939391, 0x0},
	{0x94A50, 0x949492, 0x0},
	{0x94A54, 0x959592, 0x0},
	{0x94A58, 0x969693, 0x0},
	{0x94A5C, 0x979794, 0x0},
	{0x94A60, 0x989895, 0x0},
	{0x94A64, 0x999996, 0x0},
	{0x94A68, 0x9A9A97, 0x0},
	{0x94A6C, 0x9B9B98, 0x0},
	{0x94A70, 0x9C9C99, 0x0},
	{0x94A74, 0x9D9D9A, 0x0},
	{0x94A78, 0x9E9E9B, 0x0},
	{0x94A7C, 0x9F9F9C, 0x0},
	{0x94A80, 0xA0A09D, 0x0},
	{0x94A84, 0xA1A19E, 0x0},
	{0x94A88, 0xA2A29F, 0x0},
	{0x94A8C, 0xA3A3A0, 0x0},
	{0x94A90, 0xA4A4A1, 0x0},
	{0x94A94, 0xA5A5A2, 0x0},
	{0x94A98, 0xA6A6A3, 0x0},
	{0x94A9C, 0xA7A7A4, 0x0},
	{0x94AA0, 0xA8A8A5, 0x0},
	{0x94AA4, 0xA9A9A6, 0x0},
	{0x94AA8, 0xAAAAA7, 0x0},
	{0x94AAC, 0xABABA8, 0x0},
	{0x94AB0, 0xACACA9, 0x0},
	{0x94AB4, 0xADADAA, 0x0},
	{0x94AB8, 0xAEAEAB, 0x0},
	{0x94ABC, 0xAFAFAC, 0x0},
	{0x94AC0, 0xB0B0AD, 0x0},
	{0x94AC4, 0xB1B1AE, 0x0},
	{0x94AC8, 0xB2B2AF, 0x0},
	{0x94ACC, 0xB3B3B0, 0x0},
	{0x94AD0, 0xB4B4B1, 0x0},
	{0x94AD4, 0xB5B5B2, 0x0},
	{0x94AD8, 0xB6B6B3, 0x0},
	{0x94ADC, 0xB7B7B4, 0x0},
	{0x94AE0, 0xB8B8B5, 0x0},
	{0x94AE4, 0xB9B9B6, 0x0},
	{0x94AE8, 0xBABAB7, 0x0},
	{0x94AEC, 0xBBBBB8, 0x0},
	{0x94AF0, 0xBCBCB9, 0x0},
	{0x94AF4, 0xBDBDBA, 0x0},
	{0x94AF8, 0xBEBEBB, 0x0},
	{0x94AFC, 0xBFBFBC, 0x0},
	{0x94B00, 0xC0C0BC, 0x0},
	{0x94B04, 0xC1C1BD, 0x0},
	{0x94B08, 0xC2C2BE, 0x0},
	{0x94B0C, 0xC3C3BF, 0x0},
	{0x94B10, 0xC4C4C0, 0x0},
	{0x94B14, 0xC5C5C1, 0x0},
	{0x94B18, 0xC6C6C2, 0x0},
	{0x94B1C, 0xC7C7C3, 0x0},
	{0x94B20, 0xC8C8C4, 0x0},
	{0x94B24, 0xC9C9C5, 0x0},
	{0x94B28, 0xCACAC6, 0x0},
	{0x94B2C, 0xCBCBC7, 0x0},
	{0x94B30, 0xCCCCC8, 0x0},
	{0x94B34, 0xCDCDC9, 0x0},
	{0x94B38, 0xCECECA, 0x0},
	{0x94B3C, 0xCFCFCB, 0x0},
	{0x94B40, 0xD0D0CC, 0x0},
	{0x94B44, 0xD1D1CD, 0x0},
	{0x94B48, 0xD2D2CE, 0x0},
	{0x94B4C, 0xD3D3CF, 0x0},
	{0x94B50, 0xD4D4D0, 0x0},
	{0x94B54, 0xD5D5D1, 0x0},
	{0x94B58, 0xD6D6D2, 0x0},
	{0x94B5C, 0xD7D7D3, 0x0},
	{0x94B60, 0xD8D8D4, 0x0},
	{0x94B64, 0xD9D9D5, 0x0},
	{0x94B68, 0xDADAD6, 0x0},
	{0x94B6C, 0xDBDBD7, 0x0},
	{0x94B70, 0xDCDCD8, 0x0},
	{0x94B74, 0xDDDDD9, 0x0},
	{0x94B78, 0xDEDEDA, 0x0},
	{0x94B7C, 0xDFDFDB, 0x0},
	{0x94B80, 0xE0E0DC, 0x0},
	{0x94B84, 0xE1E1DD, 0x0},
	{0x94B88, 0xE2E2DE, 0x0},
	{0x94B8C, 0xE3E3DF, 0x0},
	{0x94B90, 0xE4E4E0, 0x0},
	{0x94B94, 0xE5E5E1, 0x0},
	{0x94B98, 0xE6E6E2, 0x0},
	{0x94B9C, 0xE7E7E3, 0x0},
	{0x94BA0, 0xE8E8E4, 0x0},
	{0x94BA4, 0xE9E9E5, 0x0},
	{0x94BA8, 0xEAEAE5, 0x0},
	{0x94BAC, 0xEBEBE6, 0x0},
	{0x94BB0, 0xECECE7, 0x0},
	{0x94BB4, 0xEDEDE8, 0x0},
	{0x94BB8, 0xEEEEE9, 0x0},
	{0x94BBC, 0xEFEFEA, 0x0},
	{0x94BC0, 0xF0F0EB, 0x0},
	{0x94BC4, 0xF1F1EC, 0x0},
	{0x94BC8, 0xF2F2ED, 0x0},
	{0x94BCC, 0xF3F3EE, 0x0},
	{0x94BD0, 0xF4F4EF, 0x0},
	{0x94BD4, 0xF5F5F0, 0x0},
	{0x94BD8, 0xF6F6F1, 0x0},
	{0x94BDC, 0xF7F7F2, 0x0},
	{0x94BE0, 0xF8F8F3, 0x0},
	{0x94BE4, 0xF9F9F4, 0x0},
	{0x94BE8, 0xFAFAF5, 0x0},
	{0x94BEC, 0xFBFBF6, 0x0},
	{0x94BF0, 0xFCFCF7, 0x0},
	{0x94BF4, 0xFDFDF8, 0x0},
	{0x94BF8, 0xFEFEF9, 0x0},
	{0x94BFC, 0xFFFFF9, 0x0},
	{0x90070, 0x0F, 0x0},
};



int t6china_mdp_gamma(void)
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
	.mdp_gamma = t6china_mdp_gamma,
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
void __init t6china_mdp_writeback(struct memtype_reserve* reserve_table)
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
static struct dcs_cmd_req cmdreq;

static  struct pm8xxx_mpp_config_data MPP_ENABLE = {
	.type           = PM8XXX_MPP_TYPE_D_OUTPUT,
	.level          = PM8921_MPP_DIG_LEVEL_S4,
	.control		= PM8XXX_MPP_DOUT_CTRL_HIGH,
};

static  struct pm8xxx_mpp_config_data MPP_DISABLE = {
	.type           = PM8XXX_MPP_TYPE_D_OUTPUT,
	.level          = PM8921_MPP_DIG_LEVEL_S4,
	.control		= PM8XXX_MPP_DOUT_CTRL_LOW,
};

#ifdef CONFIG_HTC_PNPMGR
extern void set_screen_status(bool onoff);
#endif

static int mipi_dsi_panel_power(int on)
{
	static bool dsi_power_on = false;
	static struct regulator *reg_lvs5, *reg_l2;
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

		gpio_tlmm_config(GPIO_CFG(LCD_RST, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		dsi_power_on = true;
	}

	if (on) {
		if (!first_init) {
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
			rc = regulator_enable(reg_lvs5);
			if (rc) {
				pr_err("enable lvs5 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			msleep(1);
			rc = pm8xxx_mpp_config(LCM_P5V_EN_MPP_10, &MPP_ENABLE);
			if (rc < 0) {
				pr_err("enable lcd_5v+ failed, rc=%d\n", rc);
				return -ENODEV;
			}
			msleep(2);	
			rc = pm8xxx_mpp_config(LCM_N5V_EN_MPP_9, &MPP_ENABLE);
			if (rc < 0) {
				pr_err("enable lcd_5v- failed, rc=%d\n", rc);
				return -ENODEV;
			}
			msleep(7);	
			gpio_set_value(LCD_RST, 1);

			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_ON);
			msleep(10);
			
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
		rc = pm8xxx_mpp_config(BL_HW_EN_MPP_8, &MPP_DISABLE);
		if (rc < 0) {
			pr_err("disable bl_hw failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio_set_value(LCD_RST, 0);
		hr_msleep(3);	
		rc = pm8xxx_mpp_config(LCM_N5V_EN_MPP_9, &MPP_DISABLE);
		if (rc < 0) {
			pr_err("disable lcd_5v- failed, rc=%d\n", rc);
			return -ENODEV;
		}

		hr_msleep(2);
		rc = pm8xxx_mpp_config(LCM_P5V_EN_MPP_10, &MPP_DISABLE);
		if (rc < 0) {
			pr_err("disable lcd_5v- failed, rc=%d\n", rc);
			return -ENODEV;
		}

		hr_msleep(8);	
		rc = regulator_disable(reg_lvs5);
		if (rc) {
			pr_err("disable reg_lvs5 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
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

static atomic_t lcd_backlight_off;
#define CABC_DIMMING_SWITCH

static struct mipi_dsi_panel_platform_data *mipi_t6china_pdata;

static struct dsi_buf t6china_panel_tx_buf;
static struct dsi_buf t6china_panel_rx_buf;
static struct dsi_cmd_desc *video_on_cmds = NULL;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *backlight_cmds = NULL;
static struct dsi_cmd_desc *cmd_on_cmds = NULL;
#ifdef CABC_DIMMING_SWITCH
static struct dsi_cmd_desc *dim_on_cmds = NULL;
static struct dsi_cmd_desc *dim_off_cmds = NULL;
#endif
static struct dsi_cmd_desc *color_en_on_cmds = NULL;
static struct dsi_cmd_desc *color_en_off_cmds = NULL;
static struct dsi_cmd_desc **sre_ctrl_cmds = NULL;
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static struct dsi_cmd_desc *set_cabc_UI_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Video_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Camera_cmds = NULL;
#endif
static int backlight_cmds_count = 0;
static int video_on_cmds_count = 0;
static int display_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static int cmd_on_cmds_count = 0;
#ifdef CABC_DIMMING_SWITCH
static int dim_on_cmds_count = 0;
static int dim_off_cmds_count = 0;
#endif
static int color_en_on_cmds_count = 0;
static int color_en_off_cmds_count = 0;
static int sre_ctrl_cmds_count = 0;
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static int set_cabc_UI_cmds_count = 0;
static int set_cabc_Video_cmds_count = 0;
static int set_cabc_Camera_cmds_count = 0;
#endif
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static int cabc_mode = 0;
static int cur_cabc_mode = 0;
static struct mutex set_cabc_mutex;
void t6china_set_cabc (struct msm_fb_data_type *mfd, int mode);
#endif
static unsigned int pwm_min = 6;
static unsigned int pwm_default = 79;
static unsigned int pwm_max = 255;

static char enter_sleep[2] = {0x10, 0x00}; 
static char exit_sleep[2] = {0x11, 0x00}; 
static char display_off[2] = {0x28, 0x00}; 
static char display_on[2] = {0x29, 0x00}; 
static char nop[2] = {0x00, 0x00};

#ifdef CABC_DIMMING_SWITCH
static char dsi_dim_on[] = {0x53, 0x2C};
static char dsi_dim_off[] = {0x53, 0x24};

static struct dsi_cmd_desc jdi_renesas_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc jdi_renesas_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};
#endif

static char samsung_ctrl_brightness[2] = {0x51, 0xFF};

static struct dsi_cmd_desc samsung_cmd_backlight_cmds[] = {
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(samsung_ctrl_brightness), samsung_ctrl_brightness},
};

static char write_display_brightness[3]= {0x51, 0x0F, 0xFF};
static char write_control_display[2] = {0x53, 0x24}; 

static struct dsi_cmd_desc renesas_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static char t6_Color_enhancement[33]= {
        0xCA, 0x01, 0x02, 0x9A,
        0xA4, 0xB8, 0xB4, 0xB0,
        0xA4, 0x08, 0x28, 0x05,
        0x87, 0xB0, 0x50, 0x01,
        0xFF, 0x05, 0xF8, 0x0C,
        0x0C, 0x50, 0x40, 0x13,
        0x13, 0xF0, 0x08, 0x10,
        0x10, 0x3F, 0x3F, 0x3F,
        0x3F};

static char gamma_setting_red[31]= {
	0xc7, 0x00, 0x12, 0x18,
	0x20, 0x2C, 0x39, 0x42,
	0x51, 0x35, 0x3D, 0x48,
	0x56, 0x63, 0x6E, 0x7F,
	0x00, 0x12, 0x18, 0x20,
	0x2C, 0x39, 0x42, 0x51,
	0x35, 0x3D, 0x48, 0x56,
	0x63, 0x6E, 0x7F};

static char gamma_setting_green[20]= {
	0xC8, 0x01, 0x00, 0x00,
	0xFD, 0x03, 0xFC, 0xF1,
	0x00, 0xFA, 0xFC, 0xF6,
	0xD6, 0xF1, 0x00, 0x00,
	0x02, 0x06, 0xF1, 0xE0};

static char Manufacture_Command_setting[4] = {0xD6, 0x01};
static char deep_standby_off[2] = {0xB1, 0x01};

static char unlock[] = {0xB0, 0x00};
static char display_brightness[] = {0x51, 0xFF};
static char enable_te[] = {0x35, 0x00};
static char lock[] = {0xB0, 0x03};
static char Write_Content_Adaptive_Brightness_Control[2] = {0x55, 0x42};
static char common_setting[] = {
       0xCE, 0x6C, 0x40, 0x43,
       0x49, 0x55, 0x62, 0x71,
       0x82, 0x94, 0xA8, 0xB9,
       0xCB, 0xDB, 0xE9, 0xF5,
       0xFC, 0xFF, 0x01, 0x5A, 
       0x00, 0x00, 0x54, 0x20};

static char cabc_still[] = {0xB9, 0x03, 0x82, 0x3C, 0x10, 0x3C, 0x87};
static char cabc_movie[] = {0xBA, 0x03, 0x78, 0x64, 0x10, 0x64, 0xB4};
static char cabc_bl_limit[] = {0x5E, 0x1E};
static char SRE_Manual_0[] = {0xBB, 0x01, 0x00, 0x00};

static char unlock_command[2] = {0xB0, 0x04}; 
static char lock_command[2] = {0xB0, 0x03}; 

static struct dsi_cmd_desc jdi_renesas_cmd_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(t6_Color_enhancement), t6_Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gamma_setting_red), gamma_setting_red},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gamma_setting_green), gamma_setting_green},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock_command), lock_command},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cabc_bl_limit), cabc_bl_limit},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
};

static struct dsi_cmd_desc jdi_renesas_cmd_on_cmds_c3[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(t6_Color_enhancement), t6_Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock_command), lock_command},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cabc_bl_limit), cabc_bl_limit},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
};

static struct dsi_cmd_desc jdi_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 48, sizeof(enter_sleep), enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(deep_standby_off), deep_standby_off}
};

static int resume_blk = 0;

static int t6china_lcd_on(struct platform_device *pdev)
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
		if (mipi->mode == DSI_VIDEO_MODE) {
			cmdreq.cmds = video_on_cmds;
			cmdreq.cmds_cnt = video_on_cmds_count;
		} else {
			cmdreq.cmds = cmd_on_cmds;
			cmdreq.cmds_cnt = cmd_on_cmds_count;
		}
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
	}
	first_init = 0;
	PR_DISP_INFO("%s, %s\n", __func__, ptype);

	return 0;
}

static int t6china_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	resume_blk = 1;

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}
static int __devinit t6china_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_t6china_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}
static void t6china_display_on(struct msm_fb_data_type *mfd)
{
	

	cmdreq.cmds = display_on_cmds;
	cmdreq.cmds_cnt = display_on_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

static void t6china_display_off(struct msm_fb_data_type *mfd)
{
	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

#ifdef CABC_DIMMING_SWITCH
static void t6china_dim_on(struct msm_fb_data_type *mfd)
{
	if (atomic_read(&lcd_backlight_off)) {
		PR_DISP_DEBUG("%s: backlight is off. Skip dimming setting\n", __func__);
		return;
	}

	if (dim_on_cmds == NULL)
		return;

	PR_DISP_DEBUG("%s\n", __func__);

	cmdreq.cmds = dim_on_cmds;
	cmdreq.cmds_cnt = dim_on_cmds_count;

	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
}
#endif


#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char pwm_value;

static unsigned char t6china_shrink_pwm(int val)
{
	unsigned char shrink_br = BRI_SETTING_MAX;

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

	pwm_value = shrink_br; 

	PR_DISP_INFO("brightness orig=%d, transformed=%d\n", val, shrink_br);

	return shrink_br;
}


static void t6china_set_backlight(struct msm_fb_data_type *mfd)
{
	int rc;

	samsung_ctrl_brightness[1] = t6china_shrink_pwm((unsigned char)(mfd->bl_level));

	if (resume_blk) {
		resume_blk = 0;

		rc = pm8xxx_mpp_config(BL_HW_EN_MPP_8, &MPP_ENABLE);
		if (rc < 0) {
			pr_err("enable bl_hw failed, rc=%d\n", rc);
			return;
		}
	}

#ifdef CABC_DIMMING_SWITCH
        
        if (samsung_ctrl_brightness[1] == 0 || display_brightness[1] == 0 || write_display_brightness[2] == 0) {
                atomic_set(&lcd_backlight_off, 1);
		cmdreq.cmds = dim_off_cmds;
		cmdreq.cmds_cnt = dim_off_cmds_count;
				cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
				cmdreq.rlen = 0;
				cmdreq.cb = NULL;

				mipi_dsi_cmdlist_put(&cmdreq);
        } else
                atomic_set(&lcd_backlight_off, 0);
#endif
	cmdreq.cmds = backlight_cmds;
	cmdreq.cmds_cnt = backlight_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	
	if (cabc_mode == 3) {
		t6china_set_cabc(mfd, cabc_mode);
	}
#endif

	if ((mfd->bl_level) == 0) {
		rc = pm8xxx_mpp_config(BL_HW_EN_MPP_8, &MPP_DISABLE);
		if (rc < 0) {
			pr_err("disable bl_hw failed, rc=%d\n", rc);
			return;
		}
		resume_blk = 1;
	}

	return;
}

static char renesas_color_en_on[2]= {0xCA, 0x01};
static char renesas_color_en_off[2]= {0xCA, 0x00};

static struct dsi_cmd_desc sharp_renesas_c1_color_enhance_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_on), renesas_color_en_on},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_c1_color_enhance_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_off), renesas_color_en_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};

static void t6china_color_enhance(struct msm_fb_data_type *mfd, int on)
{
	if (color_en_on_cmds == NULL || color_en_off_cmds == NULL)
		return;

	if (on) {
		cmdreq.cmds = color_en_on_cmds;
		cmdreq.cmds_cnt = color_en_on_cmds_count;
		cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);

		PR_DISP_INFO("color enhance on\n");
	} else {
		cmdreq.cmds = color_en_off_cmds;
		cmdreq.cmds_cnt = color_en_off_cmds_count;
		cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);

		PR_DISP_INFO("color enhance off\n");
	}
}

static char SRE_Manual1[] = {0xBB, 0x01, 0x00, 0x00};
static char SRE_Manual2[] = {0xBB, 0x01, 0x03, 0x02};
static char SRE_Manual3[] = {0xBB, 0x01, 0x08, 0x05};
static char SRE_Manual4[] = {0xBB, 0x01, 0x13, 0x08};
static char SRE_Manual5[] = {0xBB, 0x01, 0x1C, 0x0E};
static char SRE_Manual6[] = {0xBB, 0x01, 0x25, 0x10};
static char SRE_Manual7[] = {0xBB, 0x01, 0x38, 0x18};
static char SRE_Manual8[] = {0xBB, 0x01, 0x5D, 0x28};
static char SRE_Manual9[] = {0xBB, 0x01, 0x83, 0x38};
static char SRE_Manual10[] = {0xBB, 0x01, 0xA8, 0x48};

static struct dsi_cmd_desc sharp_renesas_sre1_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual1), SRE_Manual1},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre2_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual2), SRE_Manual2},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre3_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual3), SRE_Manual3},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre4_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual4), SRE_Manual4},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre5_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual5), SRE_Manual5},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre6_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual6), SRE_Manual6},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre7_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual7), SRE_Manual7},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre8_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual8), SRE_Manual8},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre9_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual9), SRE_Manual9},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre10_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual10), SRE_Manual10},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};

static struct dsi_cmd_desc *sharp_renesas_sre_ctrl_cmds[10] = {
		sharp_renesas_sre1_ctrl_cmds,
		sharp_renesas_sre2_ctrl_cmds,
		sharp_renesas_sre3_ctrl_cmds,
		sharp_renesas_sre4_ctrl_cmds,
		sharp_renesas_sre5_ctrl_cmds,
		sharp_renesas_sre6_ctrl_cmds,
		sharp_renesas_sre7_ctrl_cmds,
		sharp_renesas_sre8_ctrl_cmds,
		sharp_renesas_sre9_ctrl_cmds,
		sharp_renesas_sre10_ctrl_cmds,
};
static void t6china_sre_ctrl(struct msm_fb_data_type *mfd, unsigned long level)
{
	static long prev_level = 0, current_stage = 0, prev_stage = 0, tmp_stage = 0;

	if (prev_level != level) {

		prev_level = level;

		if (level >= 0 && level < 8000) {
			current_stage = 1;
		} else if (level >= 8000 && level < 16000) {
			current_stage = 2;
		} else if (level >= 16000 && level < 24000) {
			current_stage = 3;
		} else if (level >= 24000 && level < 32000) {
			current_stage = 4;
		} else if (level >= 32000 && level < 40000) {
			current_stage = 5;
		} else if (level >= 40000 && level < 48000) {
			current_stage = 6;
		} else if (level >= 48000 && level < 56000) {
			current_stage = 7;
		} else if (level >= 56000 && level < 65000) {
			current_stage = 8;
		} else if (level >= 65000 && level < 65500) {
			current_stage = 9;
		} else if (level >= 65500 && level < 65536) {
			current_stage = 10;
		} else {
			current_stage = 11;
			PR_DISP_INFO("out of range of ADC, set it to 11 as default\n");
		}

		if ( prev_stage == current_stage)
			return;
		tmp_stage = prev_stage;
		prev_stage = current_stage;

		if (sre_ctrl_cmds == NULL)
			return;

		if (current_stage == 1) {
			cmdreq.cmds = sre_ctrl_cmds[0];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 2) {
			cmdreq.cmds = sre_ctrl_cmds[1];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 3) {
			cmdreq.cmds = sre_ctrl_cmds[2];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 4) {
			cmdreq.cmds = sre_ctrl_cmds[3];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 5) {
			cmdreq.cmds = sre_ctrl_cmds[4];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 6) {
			cmdreq.cmds = sre_ctrl_cmds[5];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 7) {
			cmdreq.cmds = sre_ctrl_cmds[6];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 8) {
			cmdreq.cmds = sre_ctrl_cmds[7];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 9) {
			cmdreq.cmds = sre_ctrl_cmds[8];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 10) {
			cmdreq.cmds = sre_ctrl_cmds[9];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else {
			cmdreq.cmds = sre_ctrl_cmds[0];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		}

		cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);

		PR_DISP_INFO("SRE level %lu prev_stage %lu current_stage %lu\n", level, tmp_stage, current_stage);
	}
}

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static char sharp_renesas_cabc_UI[2] = {0x55, 0x42};
static char sharp_renesas_cabc_Video[2] = {0x55, 0x43};
static struct dsi_cmd_desc sharp_renesas_set_cabc_UI_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(sharp_renesas_cabc_UI), sharp_renesas_cabc_UI},
};
static struct dsi_cmd_desc sharp_renesas_set_cabc_Video_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(sharp_renesas_cabc_Video), sharp_renesas_cabc_Video},
};

void t6china_set_cabc (struct msm_fb_data_type *mfd, int mode)
{
	int req_cabc_zoe_mode = 2;

	if (set_cabc_UI_cmds == NULL || set_cabc_Video_cmds == NULL || set_cabc_Camera_cmds == NULL)
		return;

	mutex_lock(&set_cabc_mutex);
	cabc_mode = mode;

	if (mode == 1) {
               cmdreq.cmds = set_cabc_UI_cmds;
               cmdreq.cmds_cnt = set_cabc_UI_cmds_count;
	} else if (mode == 2) {
               cmdreq.cmds = set_cabc_Video_cmds;
               cmdreq.cmds_cnt = set_cabc_Video_cmds_count;
	} else if (mode == 3) {
		if (pwm_value < 168 && cur_cabc_mode == 3) {
			req_cabc_zoe_mode = 2;
			cmdreq.cmds = set_cabc_Video_cmds;
			cmdreq.cmds_cnt = set_cabc_Video_cmds_count;
		} else if (pwm_value >= 168 && cur_cabc_mode == 3) {
			req_cabc_zoe_mode = 3;
			cmdreq.cmds = set_cabc_Camera_cmds;
			cmdreq.cmds_cnt = set_cabc_Camera_cmds_count;
		} else if (pwm_value == 255) {
			req_cabc_zoe_mode = 3;
			cmdreq.cmds = set_cabc_Camera_cmds;
			cmdreq.cmds_cnt = set_cabc_Camera_cmds_count;
		} else {
			req_cabc_zoe_mode = 2;
			cmdreq.cmds = set_cabc_Video_cmds;
			cmdreq.cmds_cnt = set_cabc_Video_cmds_count;
		}

		if (cur_cabc_mode != req_cabc_zoe_mode) {
			cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
			cmdreq.rlen = 0;
			cmdreq.cb = NULL;
			mipi_dsi_cmdlist_put(&cmdreq);

			cur_cabc_mode = req_cabc_zoe_mode;
			PR_DISP_INFO("set_cabc_zoe mode = %d\n", cur_cabc_mode);
		}
		mutex_unlock(&set_cabc_mutex);
		return;
	} else {
		mutex_unlock(&set_cabc_mutex);
		return;
	}

	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	cur_cabc_mode = mode;
	mutex_unlock(&set_cabc_mutex);
	PR_DISP_INFO("set_cabc mode = %d\n", mode);
}
#endif

static struct platform_driver this_driver = {
	.probe  = t6china_lcd_probe,
	.driver = {
		.name   = "mipi_t6china",
	},
};

static struct msm_fb_panel_data t6china_panel_data = {
	.on	= t6china_lcd_on,
	.off	= t6china_lcd_off,
	.set_backlight = t6china_set_backlight,
	.display_on = t6china_display_on,
	.display_off = t6china_display_off,
	.color_enhance = t6china_color_enhance,
#ifdef CABC_DIMMING_SWITCH
	.dimming_on = t6china_dim_on,
#endif
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	.set_cabc = t6china_set_cabc,
#endif
	.sre_ctrl = t6china_sre_ctrl,
};

static struct msm_panel_info pinfo;
static int ch_used[3] = {0};

static int mipi_t6china_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;
	pr_err("%s\n", __func__);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_t6china", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	t6china_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &t6china_panel_data,
		sizeof(t6china_panel_data));
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

static struct mipi_dsi_phy_ctrl dsi_jdi_cmd_mode_phy_db = {
	
        {0x03, 0x08, 0x05, 0x00, 0x20},
        
	{0xD7, 0x34, 0x23, 0x00, 0x63, 0x6A, 0x28, 0x37, 0x3C, 0x03, 0x04},
        
        {0x5F, 0x00, 0x00, 0x10},
        
        {0xFF, 0x00, 0x06, 0x00},
        
	{0x00, 0xA8, 0x30, 0xCA, 0x00, 0x20, 0x0F, 0x62, 0x70, 0x88, 0x99, 0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};

static int __init mipi_cmd_jdi_renesas_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 73;
	pinfo.height = 130;
	pinfo.camera_backlight = 185;

	pinfo.lcdc.h_back_porch = 27;
	pinfo.lcdc.h_front_porch = 38;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = pinfo.lcdc.v_back_porch;
	pinfo.lcd.v_front_porch = pinfo.lcdc.v_front_porch;
	pinfo.lcd.v_pulse_width = pinfo.lcdc.v_pulse_width;

	pinfo.lcdc.border_clr = 0;      
	pinfo.lcdc.underflow_clr = 0xff;        
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 830000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; 

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;

	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x3;
	pinfo.mipi.t_clk_pre = 0x2B;
	pinfo.mipi.stream = 0;  
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; 
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;

	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_jdi_cmd_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 5;

	ret = mipi_t6china_device_register(&pinfo, MIPI_DSI_PRIM,
		MIPI_DSI_PANEL_FWVGA_PT);

	strncat(ptype, "PANEL_ID_SCORPIUS_JDI_RENESAS", ptype_len);
	cmd_on_cmds = jdi_renesas_cmd_on_cmds;
	cmd_on_cmds_count = ARRAY_SIZE(jdi_renesas_cmd_on_cmds);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = jdi_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(jdi_display_off_cmds);
	backlight_cmds = samsung_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef CABC_DIMMING_SWITCH
	dim_on_cmds = jdi_renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(jdi_renesas_dim_on_cmds);
	dim_off_cmds = jdi_renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(jdi_renesas_dim_off_cmds);
#endif
	color_en_on_cmds = sharp_renesas_c1_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_c1_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_off_cmds);
	sre_ctrl_cmds = sharp_renesas_sre_ctrl_cmds;
	sre_ctrl_cmds_count = ARRAY_SIZE(sharp_renesas_sre1_ctrl_cmds);
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	set_cabc_UI_cmds = sharp_renesas_set_cabc_UI_cmds;
	set_cabc_UI_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_UI_cmds);
	set_cabc_Video_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Video_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
	set_cabc_Camera_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Camera_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
#endif

	mdp_gamma = mdp_gamma_renesas;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas);

	pwm_min = 6;
	pwm_default = 81;
	pwm_max = 255;

	PR_DISP_INFO("%s\n", __func__);

	return ret;
}

static int __init mipi_cmd_jdi_renesas_init_c3(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 73;
	pinfo.height = 130;
	pinfo.camera_backlight = 185;

	pinfo.lcdc.h_back_porch = 27;
	pinfo.lcdc.h_front_porch = 38;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = pinfo.lcdc.v_back_porch;
	pinfo.lcd.v_front_porch = pinfo.lcdc.v_front_porch;
	pinfo.lcd.v_pulse_width = pinfo.lcdc.v_pulse_width;

	pinfo.lcdc.border_clr = 0;      
	pinfo.lcdc.underflow_clr = 0xff;        
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 830000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; 

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;

	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x3;
	pinfo.mipi.t_clk_pre = 0x2B;
	pinfo.mipi.stream = 0;  
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; 
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;

	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_jdi_cmd_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 5;

	ret = mipi_t6china_device_register(&pinfo, MIPI_DSI_PRIM,
		MIPI_DSI_PANEL_FWVGA_PT);

	strncat(ptype, "PANEL_ID_SCORPIUS_JDI_RENESAS_C3", ptype_len);
	cmd_on_cmds = jdi_renesas_cmd_on_cmds_c3;
	cmd_on_cmds_count = ARRAY_SIZE(jdi_renesas_cmd_on_cmds_c3);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = jdi_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(jdi_display_off_cmds);
	backlight_cmds = samsung_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef CABC_DIMMING_SWITCH
	dim_on_cmds = jdi_renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(jdi_renesas_dim_on_cmds);
	dim_off_cmds = jdi_renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(jdi_renesas_dim_off_cmds);
#endif
	color_en_on_cmds = sharp_renesas_c1_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_c1_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_off_cmds);
	sre_ctrl_cmds = sharp_renesas_sre_ctrl_cmds;
	sre_ctrl_cmds_count = ARRAY_SIZE(sharp_renesas_sre1_ctrl_cmds);
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	set_cabc_UI_cmds = sharp_renesas_set_cabc_UI_cmds;
	set_cabc_UI_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_UI_cmds);
	set_cabc_Video_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Video_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
	set_cabc_Camera_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Camera_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
#endif

	mdp_gamma = mdp_gamma_renesas_c3;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas_c3);

	pwm_min = 6;
	pwm_default = 81;
	pwm_max = 255;

	PR_DISP_INFO("%s\n", __func__);

	return ret;
}

void __init t6china_init_fb(void)
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

static int __init t6china_panel_init(void)
{
	if(panel_type == PANEL_ID_NONE)	{
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	mipi_dsi_buf_alloc(&t6china_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&t6china_panel_rx_buf, DSI_BUF_SIZE);

	if (panel_type == PANEL_ID_SCORPIUS_JDI_RENESAS) {
		mipi_cmd_jdi_renesas_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_SCORPIUS_JDI_RENESAS\n", __func__);
	} else if (panel_type == PANEL_ID_SCORPIUS_JDI_RENESAS_C3) {
		mipi_cmd_jdi_renesas_init_c3();
		PR_DISP_INFO("%s panel ID = PANEL_ID_SCORPIUS_JDI_RENESAS_C3\n", __func__);
	} else {
		PR_DISP_ERR("%s: panel not supported!!\n", __func__);
		return -ENODEV;
	}


#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	mutex_init(&set_cabc_mutex);
#endif

	PR_DISP_INFO("%s\n", __func__);

	return platform_driver_register(&this_driver);
}
late_initcall(t6china_panel_init);
