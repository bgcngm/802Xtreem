#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/mach-types.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/android_alarm.h>
#include <linux/usb/android_composite.h>
#include <mach/htc_battery_cell.h>
#include <mach/board_htc.h>
#include <mach/board.h>
#ifdef CONFIG_BATTERY_DS2746
#include <linux/ds2746_battery.h>
#endif
#include <linux/i2c/smb349.h>

#include <mach/htc_gauge.h>
#include <mach/htc_charger.h>


struct delayed_work smb_state_check_task;
#define SMB_STATE_UPDATE_PERIOD_SEC			60

struct delayed_work smb_delay_phase2_check_task;
#define SMB_DELAY_PHASE2_PERIOD_SEC			5

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
static int screen_state = 1;
#endif

#define EVM	0x99
#define EVM1	99
#define XA	0
#define XB	1
#define XC	2
#define XD	3
#define PVT	0x80

const int SMB349_DC_INPUT_LIMIT[] = {500, 900,1000,1100,1200,1300,1500,1600,1700,1800,2000,2200,2400,2500,3000,3500, -1};

#define SMB349_THERMAL_THRES_VOL		4300
static int smb_state_pre =  STATE_HI_V_SCRN_OFF;
static int smb_state_curr  =  STATE_HI_V_SCRN_OFF;

/* file node /sys/module/smb349/parameters/disabled  */
static int pwrsrc_disabled; /* both usbin and battfet side */
static int batt_chg_disabled; /* both usbin and battfet side */
static int hsml_target_ma;

struct delayed_work		aicl_check_work;
#define AICL_CHECK_PERIOD_STAGE1_1S		1
#define AICL_CHECK_PERIOD_STAGE2_2S		2
static int aicl_worker_ongoing;
static int aicl_latest_result = 0;

static int aicl_sm;
#define AICL_SM_0_RESET				0
#define AICL_SM_1_USB_IN			1
#define AICL_SM_1ST_AC_IN				2
#define AICL_SM_1ST_AICL_PROCESSING		3
#define AICL_SM_1ST_AICL_DONE			4
#define AICL_SM_2ST_AICL_PREPARE		5
#define AICL_SM_2ST_AICL_PROCESSING		6
#define AICL_SM_2ST_AICL_DONE			7


#define SMB_ADAPTER_UNKNOWN			0
#define SMB_ADAPTER_USB			1
#define SMB_ADAPTER_UNDER_1A			2
#define SMB_ADAPTER_1A				3
#define SMB_ADAPTER_KDDI			4

/* pwrsrc disabled batt fet control mask (reason) */
#define SMB_PWRSRC_DISABLED_BIT_EOC		(1)
#define SMB_PWRSRC_DISABLED_BIT_KDRV		(1<<1)
#define SMB_PWRSRC_DISABLED_BIT_FILENODE	(1<<2)
#define SMB_PWRSRC_DISABLED_BIT_AICL		(1<<3)
#define SMB_PWRSRC_DISABLED_BIT_OTG_ENABLE	(1<<4)


/* sw disabled batt fet control mask (reason) */
#define SMB_BATT_CHG_DISABLED_BIT_EOC		(1)
#define SMB_BATT_CHG_DISABLED_BIT_KDRV		(1<<1)
#define SMB_BATT_CHG_DISABLED_BIT_FILENODE	(1<<2)
#define SMB_BATT_CHG_DISABLED_BIT_TEMP		(1<<3)
#define SMB_BATT_CHG_DISABLED_BIT_BAT		(1<<4)

static int smb_batt_charging_disabled; /* BAT FET side */
static int smb_pwrsrc_disabled;


/* static function declar */
static int _smb349_set_dc_input_curr_limit(int dc_current_limit);

static int _smb349_enable_otg_output(int enable);

struct mutex charger_lock;
struct mutex pwrsrc_lock;
struct mutex aicl_sm_lock;
struct mutex phase_lock;

static int smb_chip_rev;
static int is_otg_enable;
static int smb_adapter_type;
static int aicl_on;
static int aicl_result_threshold;
static int dc_input_max;
#define MAX_FASTCHG_INPUT_CURR		1600
#define DEFAULT_FASTCHG_INPUT_CURR		1000
#define LIMIT_PRECHG_CURR		200

#define SMB349_NAME			"smb349"
#define OutputFormatString 		pr_smb_info
#define	SMB349_WAIT4_AICL_COMPLETE_MS	2000
extern enum htc_power_source_type pwr_src;
extern int pm8921_bms_charging_began(void);

#define pr_smb_fmt(fmt) "[BATT][smb349] " fmt
#define pr_smb_err_fmt(fmt) "[BATT][smb349] err:" fmt
#define pr_smb_info(fmt, ...) \
	printk(KERN_INFO pr_smb_fmt(fmt), ##__VA_ARGS__)
#define pr_smb_err(fmt, ...) \
	printk(KERN_ERR pr_smb_err_fmt(fmt), ##__VA_ARGS__)

#define OutputFormatString 		pr_smb_info

/* there is a 32 seconds hw safety timer for boost mode */
#define SMB349_CHECK_INTERVAL (15)
/*#define SET_VDPM_AS_476	1*/

/*#define SMB349_MASKED_DBG			1*/
#define SMB349_AICL_DBG								1
#define SMB349_SET_DC_CURR_DBG								1
#define SMB349_SET_SWITCH_FREQ_DBG								1
/*#define SMB349_SET_FC_CURR_DBG								1*/

#ifdef BIT
#undef BIT
#define BIT(x)  (1<<(x))
#endif

#define SMB349_MASK(BITS, POS)  ((unsigned char)(((1 << BITS) - 1) << POS))

static unsigned int smb349_charging_src_old = 0;
static unsigned int smb349_charging_src_new = 0;
static int target_max_voltage_mv = 0;

static struct workqueue_struct *smb349_wq;
static struct work_struct smb349_state_work;
static int smb349_state_int;
///static struct delayed_work set_vdpm_work;
///static struct delayed_work kick_dog;

struct smb349_chg_int_data {
	int gpio_chg_int;
	int smb349_reg;
	struct delayed_work int_work;
};

///static struct smb349_chg_int_data *chg_int_data;
static LIST_HEAD(smb349_chg_int_list);
static DEFINE_MUTEX(notify_lock);
static int _smb34x_set_fastchg_curr(int fc_current);
///static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static int smb349_initial = -1;
///static int smb349_low_chg;

#ifdef CONFIG_SUPPORT_DQ_BATTERY
static int htc_is_dq_pass;
#endif

/**
 * Insmod parameters
 */
/* I2C_CLIENT_INSMOD_1(tps65200); */

static int smb349_probe(struct i2c_client *client,
			const struct i2c_device_id *id);
static int smb349_remove(struct i2c_client *client);

/* Supersonic for Switch charger */
struct smb349_i2c_client {
	struct i2c_client *client;
	u8 address;
	/* max numb of i2c_msg required is for read =2 */
	struct i2c_msg xfer_msg[2];
	/* To lock access to xfer_msg */
	struct mutex xfer_lock;
};
static struct smb349_i2c_client smb349_i2c_module;

/**
Function:smb349_i2c_write
Target:	Write a byte to Switch charger
Timing:	TBD
INPUT: 	value-> write value
		reg  -> reg offset
		num-> number of byte to write
return :TRUE-->OK
		FALSE-->Fail
 */
static int smb349_i2c_write(u8 *value, u8 reg, u8 num_bytes)
{
	int ret;
	struct smb349_i2c_client *smb;
	struct i2c_msg *msg;

	smb = &smb349_i2c_module;

	mutex_lock(&smb->xfer_lock);
	/*
	 * [MSG1]: fill the register address data
	 * fill the data Tx buffer
	 */
	msg = &smb->xfer_msg[0];
	msg->addr = smb->address;
	msg->len = num_bytes + 1;
	msg->flags = 0;
	msg->buf = value;
	/* over write the first byte of buffer with the register address */
	*value = reg;
	ret = i2c_transfer(smb->client->adapter, smb->xfer_msg, 1);
	mutex_unlock(&smb->xfer_lock);

	/* i2cTransfer returns num messages.translate it pls.. */
	if (ret >= 0)
		ret = 0;
	return ret;
}


/**
Function:smb349_i2c_read
Target:	Read a byte from Switch charger
Timing:	TBD
INPUT: 	value-> store buffer
		reg  -> reg offset to read
		num-> number of byte to read
return :TRUE-->OK
		FALSE-->Fail
 */
static int smb349_i2c_read(u8 *value, u8 reg, u8 num_bytes)
{
	int ret;
	u8 val;
	struct smb349_i2c_client *smb;
	struct i2c_msg *msg;

	smb = &smb349_i2c_module;

	mutex_lock(&smb->xfer_lock);
	/* [MSG1] fill the register address data */
	msg = &smb->xfer_msg[0];
	msg->addr = smb->address;
	msg->len = 1;
	msg->flags = 0; /* Read the register value */
	val = reg;
	msg->buf = &val;
	/* [MSG2] fill the data rx buffer */
	msg = &smb->xfer_msg[1];
	msg->addr = smb->address;
	msg->flags = I2C_M_RD;  /* Read the register value */
	msg->len = num_bytes;   /* only n bytes */
	msg->buf = value;
	ret = i2c_transfer(smb->client->adapter, smb->xfer_msg, 2);
	mutex_unlock(&smb->xfer_lock);

	/* i2cTransfer returns num messages.translate it pls.. */
	if (ret >= 0)
		ret = 0;
	return ret;
}


/**
Function:smb349_i2c_write_byte
Target:	Write a byte from Switch charger
Timing:	TBD
INPUT: 	value-> store buffer
		reg  -> reg offset to read
return :TRUE-->OK
		FALSE-->Fail
 */
static int smb349_i2c_write_byte(u8 value, u8 reg)
{
	/* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
	int result;
	int i;
	u8 temp_buffer[2] = { 0 };
	/* offset 1 contains the data */
	temp_buffer[1] = value;
	/*  There is workaround here to retry when returns fail since of
	    I2C resume order changed in kernel 3.0

	    Max retry times : 10
	    Will break when writing byte successfully (return 0)
	*/
	for (i = 0; i < 10; i++) {
		result = smb349_i2c_write(temp_buffer, reg, 1);
		if (result == 0)
			break;
		pr_smb_err("smb349 I2C write retry count = %d, result = %d\n", i+1, result);
		msleep(10);
	}
	if (result != 0)
		pr_smb_err("smb349 I2C write fail = %d\n", result);

	return result;
}

/**
Function:smb349_i2c_read_byte
Target:	Read a byte from Switch charger
Timing:	TBD
INPUT: 	value-> store buffer
		reg  -> reg offset to read
return :TRUE-->OK
		FALSE-->Fail
 */
static int smb349_i2c_read_byte(u8 *value, u8 reg)
{
	int result = 0;
	int i;
	/*  Similar to the case of smb349_i2c_write_byte

	    Max retry times : 10
	    Will break when reading byte successfully (return 0)
	*/
	for (i = 0; i < 10; i++) {
		result = smb349_i2c_read(value, reg, 1);
		if (result == 0)
			break;
		pr_smb_err("smb349 I2C read retry count = %d, result = %d\n", i+1, result);
		msleep(10);
	}
	if (result != 0)
		pr_smb_err("smb349 I2C read fail = %d\n", result);

	return result;
}

#if (defined(CONFIG_TPS65200) && defined(CONFIG_MACH_PRIMODS))
int tps65200_mask_interrupt_register(int status)
{
	if (status == CHARGER_USB) {	// Vbus in
		smb349_i2c_write_byte(0x7F, 0x0C);
	} else if (status == CHARGER_BATTERY) {
		smb349_i2c_write_byte(0xFF, 0x0C);
		/* Clear reverse protection counter when cable out*/
		reverse_protection_handler(REVERSE_PROTECTION_CONTER_CLEAR);
	}
	return 0;
}
EXPORT_SYMBOL(tps65200_mask_interrupt_register);
#endif


static int get_prechg_curr_def(int targ_ma_curr)
{
	int ret = 0;
	if(smb_chip_rev == SMB_349)
	{
		switch (targ_ma_curr) {
			case 100:
				ret = PRECHG_CURR_100MA;
				break;
			case 150:
				ret = PRECHG_CURR_150MA;
				break;
			case 200:
				ret = PRECHG_CURR_200MA;
				break;
			case 250:
				ret = PRECHG_CURR_250MA;
				break;
			case 300:
				ret = PRECHG_CURR_300MA;
				break;
			case 350:
				ret = PRECHG_CURR_350MA;
				break;
			case 50:
				ret = PRECHG_CURR_50MA;
				break;
			default:
				pr_smb_err("%s ask for %d define\n", __func__, targ_ma_curr);
				BUG();
				break;
		}
	} else {
		switch (targ_ma_curr) {
			case 100:
				ret = SMB340_PRECHG_CURR_100MA;
				break;
			case 200:
				ret = SMB340_PRECHG_CURR_200MA;
				break;
			case 300:
				ret = SMB340_PRECHG_CURR_300MA;
				break;
			case 400:
				ret = SMB340_PRECHG_CURR_400MA;
				break;
			case 500:
				ret = SMB340_PRECHG_CURR_500MA;
				break;
			case 600:
				ret = SMB340_PRECHG_CURR_600MA;
				break;
			case 700:
				ret = SMB340_PRECHG_CURR_700MA;
				break;
			default:
				pr_smb_err("%s ask for %d define\n", __func__, targ_ma_curr);
				BUG();
				break;
		}
	}


	return ret;
}

#if 0
static int get_dc_input_curr_def(int targ_ma_curr)
{
	int ret = 0;

		switch (targ_ma_curr) {
			case 500:
				ret = DC_INPUT_500MA;
				break;
			case 900:
				ret = DC_INPUT_900MA;
				break;
			case 1000:
				ret = DC_INPUT_1000MA;
				break;
			case 1100:
				ret = DC_INPUT_1100MA;
				break;
			case 1200:
				ret = DC_INPUT_1200MA;
				break;
			case 1300:
				ret = DC_INPUT_1300MA;
				break;
			case 1500:
				ret = DC_INPUT_1500MA;
				break;
			case 1600:
				ret = DC_INPUT_1600MA;
				break;
			case 1700:
				ret = DC_INPUT_1700MA;
				break;
			case 1800:
				ret = DC_INPUT_1800MA;
				break;
			case 2000:
				ret = DC_INPUT_2000MA;
				break;
			case 2200:
				ret = DC_INPUT_2200MA;
				break;
			case 2400:
				ret = DC_INPUT_2400MA;
				break;
			case 2500:
				ret = DC_INPUT_2500MA;
				break;
			case 3000:
				ret = DC_INPUT_3000MA;
				break;
			case 3500:
				ret = DC_INPUT_3500MA;
				break;
			default:
				pr_smb_err("%s ask for %d define\n", __func__, targ_ma_curr);
				BUG();
				break;
		}

	return ret;
}

#endif

static int get_fastchg_curr_def(int targ_ma_curr)
{
	int ret = 0;
	if(smb_chip_rev == SMB_349)
	{
		switch (targ_ma_curr) {

			case 500:
				ret = FAST_CHARGE_500MA;
				break;
			case 600:
				ret = FAST_CHARGE_600MA;
				break;
			case 700:
				ret = FAST_CHARGE_700MA;
				break;
			case 800:
				ret = FAST_CHARGE_800MA;
				break;
			case 900:
				ret = FAST_CHARGE_900MA;
				break;
			case 1000:
				ret = FAST_CHARGE_1000MA;
				break;
			case 1100:
				ret = FAST_CHARGE_1100MA;
				break;
			case 1200:
				ret = FAST_CHARGE_1200MA;
				break;
			case 1300:
				ret = FAST_CHARGE_1300MA;
				break;
			case 1400:
				ret = FAST_CHARGE_1400MA;
				break;
			case 1500:
				ret = FAST_CHARGE_1500MA;
				break;
			case 1600:
				ret = FAST_CHARGE_1600MA;
				break;
			case 1700:
				ret = FAST_CHARGE_1700MA;
				break;
			case 1800:
				ret = FAST_CHARGE_1800MA;
				break;
			case 1900:
				ret = FAST_CHARGE_1900MA;
				break;
			case 2000:
				ret = FAST_CHARGE_2000MA;
				break;
			default:
				pr_smb_err("%s ask for %d define\n", __func__, targ_ma_curr);
				BUG();
				break;
		}
	} else {
		switch (targ_ma_curr) {
			case 1000:
				ret = SMB340_FASTCHG_1000MA;
				break;
			case 1200:
				ret = SMB340_FASTCHG_1200MA;
				break;
			case 1400:
				ret = SMB340_FASTCHG_1400MA;
				break;
			case 1600:
				ret = SMB340_FASTCHG_1600MA;
				break;
			case 1800:
				ret = SMB340_FASTCHG_1800MA;
				break;
			case 2000:
				ret = SMB340_FASTCHG_2000MA;
				break;
			case 2200:
				ret = SMB340_FASTCHG_2200MA;
				break;
			case 2400:
				ret = SMB340_FASTCHG_2400MA;
				break;
			case 2600:
				ret = SMB340_FASTCHG_2600MA;
				break;
			case 2800:
				ret = SMB340_FASTCHG_2800MA;
				break;
			case 3000:
				ret = SMB340_FASTCHG_3000MA;
				break;
			case 3200:
				ret = SMB340_FASTCHG_3200MA;
				break;
			case 3400:
				ret = SMB340_FASTCHG_3400MA;
				break;
			case 3600:
				ret = SMB340_FASTCHG_3600MA;
				break;
			case 3800:
				ret = SMB340_FASTCHG_3800MA;
				break;
			case 4000:
				ret = SMB340_FASTCHG_4000MA;
				break;
			default:
				pr_smb_err("%s ask for %d define\n", __func__, targ_ma_curr);
				BUG();
				break;
		}
	}


	return ret;
}

/*
			Screen ON			Screen OFF
--------------------------------------------------------------------------------
		|
vol > 4.2	|	STATE_HI_V_SCRN_ON		STATE_HI_V_SCRN_OFF
		|
		|					 ^	|
---------------------------------------------------------|------X---------------
		|					 |OK	|NO
		|					 |	v
		|				<----
vol < 4.2	|	STATE_LO_V_SCRN_ON	---->	STATE_LO_V_SCRN_OFF
		|
--------------------------------------------------------------------------------

*/
static void smb349_adjust_kddi_dc_input_curr(void)
{

	if(smb_adapter_type == SMB_ADAPTER_KDDI)
	{
		if(screen_state)
		{
			pr_smb_info("%s set DC_INPUT_1000MA\n",__func__);
			_smb349_set_dc_input_curr_limit(DC_INPUT_1000MA);
		}
		else
		{
			pr_smb_info("%s set DC_INPUT_1700MA\n",__func__);
			_smb349_set_dc_input_curr_limit(dc_input_max);
		}
	}
}

static void smb349_adjust_fast_charge_curr(void)
{

	switch (smb349_charging_src_new) {

	case HTC_PWR_SOURCE_TYPE_AC:
	case HTC_PWR_SOURCE_TYPE_9VAC:
#ifdef CONFIG_HAS_EARLYSUSPEND
		if((smb_state_pre == STATE_LO_V_SCRN_ON) && (smb_state_curr == STATE_LO_V_SCRN_OFF))
		{
			pr_smb_info("%s set %dMA\n",__func__, MAX_FASTCHG_INPUT_CURR);
			_smb34x_set_fastchg_curr(get_fastchg_curr_def(MAX_FASTCHG_INPUT_CURR));
		}
		if((smb_state_pre == STATE_LO_V_SCRN_OFF) && (smb_state_curr != STATE_LO_V_SCRN_OFF))
		{
			pr_smb_info("%s set %dMA\n",__func__, DEFAULT_FASTCHG_INPUT_CURR);
			_smb34x_set_fastchg_curr(get_fastchg_curr_def(DEFAULT_FASTCHG_INPUT_CURR));
		}
#else
		_smb34x_set_fastchg_curr(get_fastchg_curr_def(DEFAULT_FASTCHG_INPUT_CURR));
#endif  //CONFIG_HAS_EARLYSUSPEND
			break;

		default:
			break;

		}

	// debug
	///smb349_dump_fast_charge_input_curr(ffc_reg);
}



int smb349_dump_reg_verbose(u8 reg, int verbose)
{
	unsigned char temp = 0;
	smb349_i2c_read_byte(&temp,  reg);

	if(verbose)
		pr_smb_info("%s, reg(0x%x):0x%x \n", __func__, reg, temp);

	return temp;
}


int smb349_dump_reg(u8 reg)
{
	return smb349_dump_reg_verbose(reg, 1);
}

int smb349_get_irq_status(u8 reg, unsigned int flag)
{
	unsigned char temp = 0;
	u8 mask = 0;
	if (smb349_initial < 0)
		return smb349_initial;


	smb349_i2c_read_byte(&temp,  reg);

	mask = SMB349_MASK(1, flag);
	temp &= mask;

	pr_smb_info("%s, reg(0x%x) flag 0x%x is %d\n", __func__, reg, flag, temp);

	if(temp)
		return 1;
	else
		return 0;
}

int smb349_is_usbcs_register_mode(void)
{
	int ret = 0;
	unsigned char temp = 0;

	smb349_i2c_read_byte(&temp,  PIN_ENABLE_CTRL_REG);
///	pr_smb_info("%s, read reg(0x%x):0x%x \n", __func__, PIN_ENABLE_CTRL_REG, temp);

	if (temp & USBCS_PIN_MODE)
	{
/*                pr_smb_info("%s, pin control \n", __func__);*/
		return 1;
	}
/*        else*/
/*                pr_smb_info("%s, register control \n", __func__);*/

	return ret;
}



int smb349_is_power_ok(void)
{
	unsigned char temp = 0;

	smb349_dump_reg(IRQ_F_REG);
	smb349_i2c_read_byte(&temp,  IRQ_F_REG);
	if (temp & POWER_OK_STATUS)
	{
/*                pr_smb_info("%s, yes \n", __func__);*/
		return 1;
	}
/*        else*/
/*                pr_smb_info("%s, no\n", __func__);*/

	return 0;
}


int smb349_is_AICL_complete(void)
{
	unsigned char temp = 0;

	smb349_dump_reg(IRQ_D_REG);
	smb349_i2c_read_byte(&temp,  IRQ_D_REG);
	if (temp & AICL_COMPLETE_STATUS_BIT)
	{
/*                pr_smb_info("%s, yes \n", __func__);*/
		return 1;
	}
/*        else*/
/*                pr_smb_info("%s, no\n", __func__);*/

	return 0;
}


int smb349_is_AICL_enabled(void)
{
	unsigned char temp = 0;

	smb349_i2c_read_byte(&temp,  VAR_FUNC_REG);
	if (temp & AUTOMATIC_INPUT_CURR_LIMIT_BIT)
	{
		return 1;
	}

	return 0;

}

int smb349_is_hc_mode(void)
{
	int ret = 0;
	unsigned char temp = 0;

	ret = smb349_i2c_read_byte(&temp,  STATUS_E_REG);
	if(ret) goto exit_err;

	temp = temp & SMB349_MASK(2, 5);

	if (temp)
	{
		ret = 0;
	}
	else
	{
		ret = 1;
	}

/*        smb349_dump_reg(STATUS_E_REG);*/

	return ret;

	exit_err:
		pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}


int smb349_is_suspend_mode(void)
{
	int ret = 0;
	unsigned char temp = 0;

	ret = smb349_i2c_read_byte(&temp,  STATUS_E_REG);
	if(ret) goto exit_err;

	temp = temp & IS_SUSPEND_MODE_MASK;

	if (temp)
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}

/*        smb349_dump_reg(STATUS_E_REG);*/

	return ret;

	exit_err:
		pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}

static void smb349_state_work_func(struct work_struct *work)
{

	int smb_ovp_result;

	smb349_is_charger_overvoltage(&smb_ovp_result);

	htc_gauge_set_chg_ovp(smb_ovp_result);

	pr_smb_info("%s, smb_ovp_result: %d\n", __func__, smb_ovp_result);
	return;
}

static irqreturn_t smb349_state_handler(int irq, void *data)
{
	pr_smb_info("%s\n", __func__);

	queue_work(smb349_wq, &smb349_state_work);

	return IRQ_HANDLED;
}

int smb349_get_charging_enabled(int *result)
{
	int ans = 0;
	int ret= 0;

	ret = smb349_is_charging_enabled(&ans);
	if(ret) goto exit_err;

	if (ans)
		*result = smb349_charging_src_new;
	else
		*result = HTC_PWR_SOURCE_TYPE_BATT;

	pr_smb_info("%s, result: %d\n", __func__, *result);

	return 0;

	exit_err:
		pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}

EXPORT_SYMBOL(smb349_get_charging_enabled);


int smb349_get_AICL_result(void)
{
	int ret = 0;
	unsigned char temp = 0;

/*#define STATUS_C_REG	0x3D*/
	ret = smb349_i2c_read_byte(&temp,  STATUS_E_REG);
	if(ret) goto exit_err;

	temp = temp & SMB349_MASK(4, 0);

/*        pr_smb_info("%s, AICL result=0x%x\n", __func__, temp);*/

	switch (temp)
	{
		case AICL_RESULT_500MA:
			OutputFormatString("%s, AICL_RESULT_500MA\n", __func__);
			break;
		case AICL_RESULT_900MA:
			OutputFormatString("%s, AICL_RESULT_900MA\n", __func__);
			break;
		case AICL_RESULT_1000MA:
			OutputFormatString("%s, AICL_RESULT_1000MA\n", __func__);
			break;
		case AICL_RESULT_1100MA:
			OutputFormatString("%s, AICL_RESULT_1100MA\n", __func__);
			break;
		case AICL_RESULT_1200MA:
			OutputFormatString("%s, AICL_RESULT_1200MA\n", __func__);
			break;
		case AICL_RESULT_1300MA:
			OutputFormatString("%s, AICL_RESULT_1300MA\n", __func__);
			break;
		case AICL_RESULT_1500MA:
			OutputFormatString("%s, AICL_RESULT_1500MA\n", __func__);
			break;
		case AICL_RESULT_1600MA:
			OutputFormatString("%s, AICL_RESULT_1600MA\n", __func__);
			break;
		case AICL_RESULT_1700MA:
			OutputFormatString("%s, AICL_RESULT_1700MA\n", __func__);
			break;
		case AICL_RESULT_1800MA:
			OutputFormatString("%s, AICL_RESULT_1800MA\n", __func__);
			break;
		case AICL_RESULT_2000MA:
			OutputFormatString("%s, AICL_RESULT_2000MA\n", __func__);
			break;
		case AICL_RESULT_2200MA:
			OutputFormatString("%s, AICL_RESULT_2200MA\n", __func__);
			break;
		case AICL_RESULT_2400MA:
			OutputFormatString("%s, AICL_RESULT_2400MA\n", __func__);
			break;
		case AICL_RESULT_2500MA:
			OutputFormatString("%s, AICL_RESULT_2500MA\n", __func__);
			break;
		case AICL_RESULT_3000MA:
			OutputFormatString("%s, AICL_RESULT_3000MA\n", __func__);
			break;
		case AICL_RESULT_3500MA:
			OutputFormatString("%s, AICL_RESULT_3500MA\n", __func__);
			break;

		default:
			OutputFormatString("%s, error, no valid value is read\n", __func__);
			break;

	}

	return temp;

	exit_err:
		pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}



int smb340_get_fastchg_curr(void)
{
	int ret = 0;
	unsigned char temp = 0;
	int icurrent = 0;

/*#define STATUS_C_REG	0x3D*/
	ret = smb349_i2c_read_byte(&temp,  SMB_CHG_CURR_REG);
	if(ret) goto exit_err;

	temp = temp & FAST_CHG_CURRENT_MASK;
	icurrent = temp >> SMB349_FAST_CHG_SHIFT;
/*        smb349_dump_reg(SMB_CHG_CURR_REG);*/
	switch (icurrent)
	{
		case SMB340_FASTCHG_1000MA:
			pr_smb_info("%s, SMB340_FASTCHG_1000MA:\n", __func__);
			break;

		case SMB340_FASTCHG_1200MA:
			pr_smb_info("%s, SMB340_FASTCHG_1200MA:\n", __func__);
			break;

		case SMB340_FASTCHG_1400MA:
			pr_smb_info("%s, SMB340_FASTCHG_1400MA:\n", __func__);
			break;

		case SMB340_FASTCHG_1600MA:
			pr_smb_info("%s, SMB340_FASTCHG_1600MA:\n", __func__);
			break;

		case SMB340_FASTCHG_1800MA:
			pr_smb_info("%s, SMB340_FASTCHG_1800MA:\n", __func__);
			break;

		case SMB340_FASTCHG_2000MA:
			pr_smb_info("%s, SMB340_FASTCHG_2000MA:\n", __func__);
			break;

		case SMB340_FASTCHG_2200MA:
			pr_smb_info("%s, SMB340_FASTCHG_2200MA:\n", __func__);
			break;

		case SMB340_FASTCHG_2400MA:
			pr_smb_info("%s, SMB340_FASTCHG_2400MA:\n", __func__);
			break;

		case SMB340_FASTCHG_2600MA:
			pr_smb_info("%s, SMB340_FASTCHG_2600MA:\n", __func__);
			break;

		case SMB340_FASTCHG_2800MA:
			pr_smb_info("%s, SMB340_FASTCHG_2800MA:\n", __func__);
			break;

		case SMB340_FASTCHG_3000MA:
			pr_smb_info("%s, SMB340_FASTCHG_3000MA:\n", __func__);
			break;

		case SMB340_FASTCHG_3200MA:
			pr_smb_info("%s, SMB340_FASTCHG_3200MA:\n", __func__);
			break;

		case SMB340_FASTCHG_3400MA:
			pr_smb_info("%s, SMB340_FASTCHG_3400MA:\n", __func__);
			break;

		case SMB340_FASTCHG_3600MA:
			pr_smb_info("%s, SMB340_FASTCHG_3600MA:\n", __func__);
			break;

		case SMB340_FASTCHG_3800MA:
			pr_smb_info("%s, SMB340_FASTCHG_3800MA:\n", __func__);
			break;

		case SMB340_FASTCHG_4000MA:
			pr_smb_info("%s, SMB340_FASTCHG_4000MA:\n", __func__);
			break;

		default:
			break;
	}

	return icurrent;

	exit_err:
	pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}

int smb349_get_fastchg_curr(void)
{
	int ret = 0;
	unsigned char temp = 0;
	int icurrent = 0;

/*#define STATUS_C_REG	0x3D*/
	ret = smb349_i2c_read_byte(&temp,  SMB_CHG_CURR_REG);
	if(ret) goto exit_err;

	temp = temp & FAST_CHG_CURRENT_MASK;
	icurrent = temp >> SMB349_FAST_CHG_SHIFT;
/*        smb349_dump_reg(SMB_CHG_CURR_REG);*/
	switch (icurrent)
	{
		case FAST_CHARGE_500MA:
			pr_smb_info("%s, FAST_CHARGE_500MAFAST_CHARGE_500MA:\n", __func__);
			break;

		case FAST_CHARGE_600MA:
			pr_smb_info("%s, FAST_CHARGE_600MA:\n", __func__);
			break;

		case FAST_CHARGE_700MA:
			pr_smb_info("%s, FAST_CHARGE_700MA:\n", __func__);
			break;

		case FAST_CHARGE_800MA:
			pr_smb_info("%s, FAST_CHARGE_800MA:\n", __func__);
			break;

		case FAST_CHARGE_900MA:
			pr_smb_info("%s, FAST_CHARGE_900MA:\n", __func__);
			break;

		case FAST_CHARGE_1000MA:
			pr_smb_info("%s, FAST_CHARGE_1000MA:\n", __func__);
			break;

		case FAST_CHARGE_1100MA:
			pr_smb_info("%s, FAST_CHARGE_1100MA:\n", __func__);
			break;

		case FAST_CHARGE_1200MA:
			pr_smb_info("%s, FAST_CHARGE_1200MA:\n", __func__);
			break;

		case FAST_CHARGE_1300MA:
			pr_smb_info("%s, FAST_CHARGE_1300MA:\n", __func__);
			break;

		case FAST_CHARGE_1400MA:
			pr_smb_info("%s, FAST_CHARGE_1400MA:\n", __func__);
			break;

		case FAST_CHARGE_1500MA:
			pr_smb_info("%s, FAST_CHARGE_1500MA:\n", __func__);
			break;

		case FAST_CHARGE_1600MA:
			pr_smb_info("%s, FAST_CHARGE_1600MA:\n", __func__);
			break;

		case FAST_CHARGE_1700MA:
			pr_smb_info("%s, FAST_CHARGE_1700MA:\n", __func__);
			break;

		case FAST_CHARGE_1800MA:
			pr_smb_info("%s, FAST_CHARGE_1800MA:\n", __func__);
			break;

		case FAST_CHARGE_1900MA:
			pr_smb_info("%s, FAST_CHARGE_1900MA:\n", __func__);
			break;

		case FAST_CHARGE_2000MA:
			pr_smb_info("%s, FAST_CHARGE_2000MA:\n", __func__);
			break;

		default:
			break;
	}

	return icurrent;

	exit_err:
	pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}


int smb_get_fastchg_curr(void)
{

if(smb_chip_rev == SMB_349)
	return smb349_get_fastchg_curr();
else
	return smb340_get_fastchg_curr();
}


int smb349_get_dc_input_curr_limit(void)
{
	int ret = 0;
	unsigned char temp = 0;

/*#define STATUS_C_REG	0x3D*/
	ret = smb349_i2c_read_byte(&temp,  SMB_CHG_CURR_REG);
	if(ret) goto exit_err;

	temp = temp & DC_INPUT_CURRENT_LIMIT_MASK;

/*        smb349_dump_reg(SMB_CHG_CURR_REG);*/
	switch (temp)
	{
			case DC_INPUT_500MA:
				pr_smb_info("%s, DC_INPUT_500MA:\n", __func__);
				break;

			case DC_INPUT_900MA:
				pr_smb_info("%s, DC_INPUT_900MA\n", __func__);
				break;

			case DC_INPUT_1000MA:
				pr_smb_info("%s, DC_INPUT_1000MA\n", __func__);
				break;

			case DC_INPUT_1100MA:
				pr_smb_info("%s, DC_INPUT_1100MA\n", __func__);
				break;

			case DC_INPUT_1200MA:
				pr_smb_info("%s, DC_INPUT_1200MA\n", __func__);
				break;

			case DC_INPUT_1300MA:
				pr_smb_info("%s, DC_INPUT_1300MA\n", __func__);
				break;

			case DC_INPUT_1500MA:
				pr_smb_info("%s, DC_INPUT_1500MA\n", __func__);
				break;

			case DC_INPUT_1600MA:
				pr_smb_info("%s, DC_INPUT_1600MA\n", __func__);
				break;

			case DC_INPUT_1700MA:
				pr_smb_info("%s, DC_INPUT_1700MA\n", __func__);
				break;

			case DC_INPUT_1800MA:
				pr_smb_info("%s, DC_INPUT_1800MA\n", __func__);
				break;

			case DC_INPUT_2000MA:
				pr_smb_info("%s, DC_INPUT_2000MA\n", __func__);
				break;

			case DC_INPUT_2200MA:
				pr_smb_info("%s, DC_INPUT_2200MA\n", __func__);
				break;

			case DC_INPUT_2400MA:
				pr_smb_info("%s, DC_INPUT_2400MA\n", __func__);
				break;

			case DC_INPUT_2500MA:
				pr_smb_info("%s, DC_INPUT_2500MA\n", __func__);
				break;

			case DC_INPUT_3000MA:
				pr_smb_info("%s, DC_INPUT_3000MA\n", __func__);
				break;

			case DC_INPUT_3500MA:
				pr_smb_info("%s, DC_INPUT_3500MA\n", __func__);
				break;

			default:
				break;
		}


	return temp;

	exit_err:
	pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}


int smb349_get_charging_stage(void)
{
	int ret = 0;
	unsigned char temp = 0;

/*#define STATUS_C_REG	0x3D*/
	ret = smb349_i2c_read_byte(&temp,  STATUS_C_REG);
	if(ret) goto exit_err;

	temp = temp & SMB349_MASK(2, 1);
	temp = temp >> 1;

/*
	switch (temp)
	{
		case SMB349_NO_CHARGING:
			pr_smb_info("%s, NO_CHARGING\n", __func__);
			break;

		case SMB349_PRE_CHARGING:
			pr_smb_info("%s, PRE_CHARGING\n", __func__);
			break;

		case SMB349_FAST_CHARGING:
			pr_smb_info("%s, FAST_CHARGING\n", __func__);
			break;

		case SMB349_TAPER_CHARGING:
			pr_smb_info("%s, TAPER_CHARGING\n", __func__);
			break;

		default:
			break;
	}
*/

	return temp;

	exit_err:
	pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}


int smb349_is_charger_overvoltage(int* result)
{
	int ret = 0;
	pr_smb_info("%s\n", __func__);


	ret = smb349_get_irq_status(IRQ_E_REG, SMB349_DCIN_OVER_VOLTAGE_STATUS);

	*result = ret;

	return 0;
}



int smb349_is_charger_error(void)
{
	int ret = 0;
	unsigned char temp = 0;

///efine STATUS_C_REG	0x3D
	ret = smb349_i2c_read_byte(&temp,  STATUS_C_REG);
	if(ret) goto exit_err;

	if (temp & BIT(6))
	{
		ret = 1;
/*                pr_smb_info("%s, yes \n", __func__);*/
	}
	else
	{
		ret = 0;
/*                pr_smb_info("%s, no \n", __func__);*/
	}

	return ret;

	exit_err:
	pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}

int smb349_is_charging_enabled(int *result)
{
	int ret = 0;
	unsigned char temp = 0;

///efine STATUS_C_REG	0x3D
	ret = smb349_i2c_read_byte(&temp,  STATUS_C_REG);
	if(ret) goto exit_err;

	if (temp & BIT(0))
		*result = 1;
	else
		*result = 0;

	/* if aicl worker is ongoing, also return 1 since the true charging will be enabled 2 sec later */
	if(aicl_worker_ongoing)
		*result = 1;

	return ret;

	exit_err:
		pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}



int smb349_is_charger_bit_low_active(void)
{
	unsigned char temp = 0;
	int ret = 0;

	///	#define PIN_ENABLE_CTRL_REG		0x06
	smb349_i2c_read_byte(&temp,  PIN_ENABLE_CTRL_REG);
///	pr_smb_info("%s, read reg(0x%x):0x%x \n", __func__, PIN_ENABLE_CTRL_REG, temp);

	if (temp & I2C_CONTROL_CHARGER_BIT)
		ret = 1;

	if(ret)
		pr_smb_info("%s, low active \n", __func__);
	else
		pr_smb_info("%s, high active \n", __func__);

	return ret;

}



int smb349_is_batt_temp_fault_disable_chg(int *result)
{

	if(smb_batt_charging_disabled & SMB_BATT_CHG_DISABLED_BIT_TEMP)
		*result = 1;
	else
		*result = 0;

	pr_smb_info("%s, result: %d\n", __func__, *result);

	return 0;
}


int smb349_masked_write(int reg, u8 mask, u8 val)
{
	unsigned char temp = 0;
	int ret = 0;

	if (smb349_initial < 0)
		return smb349_initial;

	smb349_i2c_read_byte(&temp,  reg);

#ifdef SMB349_MASKED_DBG
	pr_smb_info("%s reg 0x%x, mask: 0x%x, val: 0x%x\n", __func__, reg, mask, val);
	pr_smb_info("%s reg 0x%x:before modify : 0x%x\n", __func__, reg, temp);
#endif

	temp &= ~mask;

#ifdef SMB349_MASKED_DBG
	pr_smb_info("%s reg 0x%x middle:0x%x\n", __func__, reg, temp);
#endif

	temp |= val & mask;
	ret = smb349_i2c_write_byte(temp, reg);

#ifdef SMB349_MASKED_DBG
	pr_smb_info("%s reg 0x%x:after modify : 0x%x\n", __func__, reg, temp);
#endif

	return ret;
}


void smb349_dbg(void)
{

	pr_smb_info("%s ++\n", __func__);

/*
#define SMB_CHG_CURR_REG					0x00
#define CHG_OTHER_CURRENT_REG		0x01
#define VAR_FUNC_REG							0x02
#define FLOAT_VOLTAGE_REG				0x03
*/
	smb349_dump_reg(SMB_CHG_CURR_REG);
	smb349_dump_reg(CHG_OTHER_CURRENT_REG);
	smb349_dump_reg(VAR_FUNC_REG);
	smb349_dump_reg(FLOAT_VOLTAGE_REG);


/*
#define CHG_CTRL_REG					0x04
#define STAT_TIMER_REG				0x05
#define PIN_ENABLE_CTRL_REG	0x06
#define THERM_CTRL_A_REG			0x07
#define SYSOK_USB3_SEL_REG		0x08
#define CTRL_FUNC_REG		0x09
*/

	smb349_dump_reg(CHG_CTRL_REG);
	smb349_dump_reg(STAT_TIMER_REG);
	smb349_dump_reg(PIN_ENABLE_CTRL_REG);
	smb349_dump_reg(THERM_CTRL_A_REG);
	smb349_dump_reg(SYSOK_USB3_SEL_REG);
	smb349_dump_reg(CTRL_FUNC_REG);

/*
#define OTG_TLIM_THERM_CNTRL_REG	0x0A
#define LIMIT_CELL_TEMP_MONI_REG 0x0B
#define FAULT_IRQ_REG				0x0C
#define STATUS_IRQ_REG				0x0D
#define SYSOK_REG				0x0E
*/
	smb349_dump_reg(OTG_TLIM_THERM_CNTRL_REG);
	smb349_dump_reg(LIMIT_CELL_TEMP_MONI_REG);
	smb349_dump_reg(FAULT_IRQ_REG);
	smb349_dump_reg(STATUS_IRQ_REG);
	smb349_dump_reg(SYSOK_REG);

/*
#define AUTO_INPUT_VOL_DET_REG		0x10
#define I2C_BUS_REG									0x12
*/
	smb349_dump_reg(AUTO_INPUT_VOL_DET_REG);
	smb349_dump_reg(I2C_BUS_REG);

/*
#define CMD_A_REG		0x30
#define CMD_B_REG		0x31
#define CMD_C_REG		0x33
*/
	smb349_dump_reg(CMD_A_REG);
	smb349_dump_reg(CMD_B_REG);
	smb349_dump_reg(CMD_C_REG);

/*
#define IRQ_A_REG		0x35
#define IRQ_B_REG		0x36
#define IRQ_C_REG		0x37
#define IRQ_D_REG		0x38
#define IRQ_E_REG		0x39
#define IRQ_F_REG		0x3A
*/

	smb349_dump_reg(IRQ_A_REG);
	smb349_dump_reg(IRQ_B_REG);
	smb349_dump_reg(IRQ_C_REG);
	smb349_dump_reg(IRQ_D_REG);
	smb349_dump_reg(IRQ_E_REG);
	smb349_dump_reg(IRQ_F_REG);

/*
#define STATUS_A_REG		0x3B
#define STATUS_B_REG		0x3C
#define STATUS_C_REG		0x3D
#define STATUS_D_REG		0x3E
#define STATUS_E_REG		0x3F
*/
	smb349_dump_reg(STATUS_A_REG);
	smb349_dump_reg(STATUS_B_REG);
	smb349_dump_reg(STATUS_C_REG);
	smb349_dump_reg(STATUS_D_REG);
	smb349_dump_reg(STATUS_E_REG);

	pr_smb_info("%s --\n", __func__);

}

static int smb349_update_state(void)
{
	int temp_state = 0;
	int voltage = 0;
	int rc = 0;

	if(htc_gauge_get_battery_voltage(&voltage))
		return rc;

	if(screen_state)
	{
		if(voltage > SMB349_THERMAL_THRES_VOL)
			temp_state = STATE_HI_V_SCRN_ON;
		else
			temp_state = STATE_LO_V_SCRN_ON;
	}
	else
	{
		if(voltage > SMB349_THERMAL_THRES_VOL)
			temp_state = STATE_HI_V_SCRN_OFF;
		else
			temp_state = STATE_LO_V_SCRN_OFF;
	}

	pr_smb_info("%s temp_state: %d,  old(%d), new(%d)\n",__func__, temp_state, smb_state_pre, smb_state_curr);

	if(temp_state != smb_state_curr)
	{
		/* bacup old state */
		smb_state_pre = smb_state_curr;
		smb_state_curr =  temp_state;
		return 1;
	}
	return 0;

}

int smb349_dump_all(void)
{
	pr_smb_info("%s, %s %s\n", __func__, __DATE__, __TIME__);

	if (smb349_initial < 0)
		return smb349_initial;

/*        fixme	*/
/*        smb349_mpp_init(7);*/

///	smb349_dbg();
	smb349_partial_reg_dump();

	return 0;
}

EXPORT_SYMBOL(smb349_dump_all);

int smb349_get_charging_src(int *result)
{
	pr_smb_info("%s\n", __func__);
	return smb349_get_charging_enabled(result);
}
EXPORT_SYMBOL(smb349_get_charging_src);

int smb349_allow_fast_charging_setting(void)
{
	pr_smb_info("%s \n", __func__);
	// allow fast-charging current setting
	/// 0x30 -> BIT(6)
	smb349_masked_write(CMD_A_REG, CURRENT_TERMINATION_MASK, CURR_TERM_END_CHG_BIT);
		return 0;
}

int _smb349_set_otg_i2c_ctrl(void)
{
	int ret = 0;
	int target_val = 0;

	target_val = (int) SMB349_OTG_I2C_CONTROL << SMB349_OTG_I2C_PIN_SHIFT;

	pr_smb_info("%s\n", __func__);

/*        smb349_dump_reg(CTRL_FUNC_REG);*/

	smb349_masked_write(CTRL_FUNC_REG, OTG_I2C_PIN_MASK, target_val);
	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CTRL_FUNC_REG, (unsigned int)target_val);

/*        smb349_dump_reg(CTRL_FUNC_REG);*/

	return ret;
}

int smb349_allow_volatile_wrtting(void)
{
	pr_smb_info("%s \n", __func__);
	// allow fast-charging current setting
	/// 0x30 -> BIT(7)
	smb349_masked_write(CMD_A_REG, VOLIATILE_WRITE_PERMISSIOIN_MASK, VOLATILE_W_PERM_BIT);
		return 0;
}

int smb349_not_allow_charging_cycle_end(void)
{
	pr_smb_info("%s\n", __func__);
	smb349_masked_write(CHG_CTRL_REG, CURRENT_TERMINATION_MASK, CURR_TERM_END_CHG_BIT);

	return 0;
}

static int _smb349_enable_charging(bool enable)
{
	int ret = 0;
	int enable_flag = 0, disable_flag = 0;

	pr_smb_info("%s, enable: %d\n", __func__, enable);

	if (smb349_initial < 0)
		return smb349_initial;

	if(smb349_is_charger_bit_low_active())
	{
		enable_flag = 0;
		disable_flag = CHG_ENABLE_BIT;
	}
	else
	{
		disable_flag = 0;
		enable_flag = CHG_ENABLE_BIT;
	}

#if 0
	smb349_dump_reg(CMD_A_REG);
#endif

	if(enable)
	{
		ret = smb349_masked_write(CMD_A_REG, CHARGING_ENABLE_MASK, enable_flag);
		pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CMD_A_REG, enable_flag);
	}
	else
	{
		ret = smb349_masked_write(CMD_A_REG, CHARGING_ENABLE_MASK, disable_flag);
		pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CMD_A_REG, disable_flag);
	}

#if 0
	smb349_dump_reg(CMD_A_REG);
#endif

	return ret;
}

static int smb349_enable_charging_with_reason(bool chg_enable, int reason)
{
	int ret = 0;

	pr_smb_info("%s  enable:%d, reason:0x%x\n",__func__, chg_enable, reason);

	mutex_lock(&charger_lock);
	if (chg_enable)
		smb_batt_charging_disabled &= ~reason;	/* clr bit */
	else
		smb_batt_charging_disabled |= reason;	/* set bit */

	if(smb_batt_charging_disabled && chg_enable)
		pr_smb_info("%s  enable fail due to smb_batt_charging_disabled: 0x%x\n",__func__, smb_batt_charging_disabled);

	ret = _smb349_enable_charging(!smb_batt_charging_disabled);	/* see if enable charger */

	mutex_unlock(&charger_lock);


	pr_smb_info("%s  0x%x\n",__func__, smb_batt_charging_disabled);

	return ret;
}

int smb349_enable_charging(bool enable)
{
	pr_smb_info("%s  enable: %d\n",__func__, enable);
	return smb349_enable_charging_with_reason(enable, SMB_BATT_CHG_DISABLED_BIT_KDRV);
}

EXPORT_SYMBOL(smb349_enable_charging);

int smb349_eoc_notify(enum htc_extchg_event_type main_event)
{
	int enable = 0;

	if(main_event == HTC_EXTCHG_EVENT_TYPE_EOC_START_CHARGE)
		enable = 1;
	else if(main_event == HTC_EXTCHG_EVENT_TYPE_EOC_STOP_CHARGE)
		enable = 0;
	else
	{
		pr_smb_info("%s, not supported event: %d\n", __func__, main_event);
		return -1;
	}

	pr_smb_info("%s  enable: %d\n",__func__, enable);
	return smb349_enable_charging_with_reason(enable, SMB_BATT_CHG_DISABLED_BIT_EOC);
}

EXPORT_SYMBOL(smb349_eoc_notify);

int smb349_temp_notify(enum htc_extchg_event_type main_event)
{
	int rc = 0;

	switch (main_event)
	{
		case HTC_EXTCHG_EVENT_TYPE_TEMP_NORMAL:
				pr_smb_info("%s NORMAL\n ", __func__);
			break;

		case HTC_EXTCHG_EVENT_TYPE_TEMP_HOT:
				pr_smb_info("%s HOT\n ", __func__);
			break;

		case HTC_EXTCHG_EVENT_TYPE_TEMP_WARM:
				pr_smb_info("%s WARM\n ", __func__);
			break;

		case HTC_EXTCHG_EVENT_TYPE_TEMP_COOL:
				pr_smb_info("%s COOL\n ", __func__);
			break;

		case HTC_EXTCHG_EVENT_TYPE_TEMP_COLD:
			pr_smb_info("%s COLD\n ", __func__);
			break;

		default:
			pr_smb_info("%s, not supported event: %d\n", __func__, main_event);
			return 0;
			break;
	}



	if((main_event == HTC_EXTCHG_EVENT_TYPE_TEMP_HOT) || (main_event == HTC_EXTCHG_EVENT_TYPE_TEMP_COLD))
		rc = smb349_enable_charging_with_reason(0, SMB_BATT_CHG_DISABLED_BIT_TEMP);
	else
		rc = smb349_enable_charging_with_reason(1, SMB_BATT_CHG_DISABLED_BIT_TEMP);

	return rc;
}

EXPORT_SYMBOL(smb349_temp_notify);

int smb349_event_notify(enum htc_extchg_event_type main_event)
{
	int rc = 0;

	pr_smb_info("%s no such main_event:%d\n ", __func__, main_event);

	smb349_adjust_max_chg_vol(main_event);
	smb349_temp_notify(main_event);
	smb349_eoc_notify(main_event);

	return rc;
}

EXPORT_SYMBOL(smb349_event_notify);


#if 0
static int _smb349_set_otg_current(int fcc)
{
	int ret = 0;
	int my_freq = 0;

	pr_smb_info("%s\n", __func__);

/*	#define OTG_TLIM_THERM_CNTRL_REG	0x0A	*/
#ifdef SMB349_SET_SWITCH_FREQ_DBG
	smb349_dump_reg(OTG_TLIM_THERM_CNTRL_REG);
#endif

	my_freq = (int) fcc << SMB34X_OTG_CURR_LIMIT_SHIFT;
	ret = smb349_masked_write(OTG_TLIM_THERM_CNTRL_REG, OTG_CURRENT_MASK, my_freq);
	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, OTG_TLIM_THERM_CNTRL_REG, my_freq);

#ifdef SMB349_SET_SWITCH_FREQ_DBG
	smb349_dump_reg(OTG_TLIM_THERM_CNTRL_REG);
#endif

	return ret;
}
#endif
static int _smb349_enable_pwrsrc(bool enable)
{
	int ret = 0;
	pr_smb_info("%s, enable: %d\n", __func__, enable);

	if (smb349_initial < 0)
		return smb349_initial;

#if 0
	smb349_dump_reg(CMD_A_REG);
#endif

	if(enable)
	{
		/* if enable charging, otg must be turned off, or charging will fail */
		if(is_otg_enable)
			_smb349_enable_otg_output(0);

		/* enable means disable suspend_mode */
		ret = smb349_masked_write(CMD_A_REG, SUSPEND_MODE_MASK, 0);
		pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CMD_A_REG, 0);
	}
	else
	{
		/* disable means enable suspend_mode */
		ret = smb349_masked_write(CMD_A_REG, SUSPEND_MODE_MASK, SUSPEND_MODE_BIT);
		pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CMD_A_REG, SUSPEND_MODE_BIT);
	}

#if 0
	smb349_dump_reg(CMD_A_REG);
#endif

	return ret;
}

static int smb349_enable_pwrsrc_with_reason(bool pwr_enable, int reason)
{

	int ret = 0;

	mutex_lock(&pwrsrc_lock);

	if (pwr_enable)
		smb_pwrsrc_disabled &= ~reason;	/* clr bit */
	else
		smb_pwrsrc_disabled |= reason;	/* set bit */

	if(smb_pwrsrc_disabled && pwr_enable)
		pr_smb_info("%s, enable fail due to  smb_pwrsrc_disabled: 0x%x\n", __func__, smb_pwrsrc_disabled);

	ret = _smb349_enable_pwrsrc(!smb_pwrsrc_disabled);	/* see if enable charger */

	mutex_unlock(&pwrsrc_lock);

	pr_smb_info("%s  enable: %d, 0x%x\n",__func__, pwr_enable, smb_pwrsrc_disabled);

	return ret;
}
int smb349_enable_pwrsrc(bool enable)
{
	return smb349_enable_pwrsrc_with_reason(enable, SMB_PWRSRC_DISABLED_BIT_KDRV);
}

EXPORT_SYMBOL(smb349_enable_pwrsrc);


static int _smb349_enable_otg_output(int enable)
{
	int ret = 0;
	pr_smb_info("%s, enable:%d\n", __func__, enable);

/*        smb349_dump_reg(CMD_A_REG);*/

	if (enable)
	{
		/* step 1, disable pwrsrc charger with reason=otg */
		smb349_enable_pwrsrc_with_reason(0, SMB_PWRSRC_DISABLED_BIT_OTG_ENABLE);

/*                _smb349_set_otg_current(SMB_OTG_CURR_LIMIT_750MA);*/
		/* step 2, enable otg output */
		ret = smb349_masked_write(CMD_A_REG, OTG_ENABLE_MASK, BIT(4));
/*                pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CTRL_FUNC_REG, BIT(4));*/
	}
	else
	{
		/* step 1, disable otg output*/
		ret = smb349_masked_write(CMD_A_REG, OTG_ENABLE_MASK, 0);
/*                pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CTRL_FUNC_REG, 0);*/

		/* step 2, clear the pwrsrc disable reason=OTG */
		if(smb_pwrsrc_disabled & SMB_PWRSRC_DISABLED_BIT_OTG_ENABLE)
			smb349_enable_pwrsrc_with_reason(1, SMB_PWRSRC_DISABLED_BIT_OTG_ENABLE);
	}

	is_otg_enable = enable;
/*        smb349_dump_reg(CMD_A_REG);*/

	return ret;
}

int smb349_enable_5v_output(bool mhl_in)
{
	pr_smb_info("%s mhl_in: %d\n",__func__, mhl_in);

	_smb349_set_otg_i2c_ctrl();
	_smb349_enable_otg_output(mhl_in);

	return 0;
}
EXPORT_SYMBOL(smb349_enable_5v_output);

/* pmic 8921 ext_usb charger hook func + */
bool smb349_is_trickle_charging(void *ctx)
{
	pr_smb_info("%s\n", __func__);
	return false;
}
EXPORT_SYMBOL(smb349_is_trickle_charging);

int smb349_start_charging(void *ctx)
{
	pr_smb_info("%s\n", __func__);
	/* TODO */
	return 0;
}
EXPORT_SYMBOL(smb349_start_charging);

int smb349_stop_charging(void *ctx)
{
	pr_smb_info("%s\n", __func__);
	/* TODO */
	return 0;
}
EXPORT_SYMBOL(smb349_stop_charging);
/* pmic 8921 ext_usb charger hook func - */


/*
#define SMB349_USBCS_REGISTER_CTRL		0
#define SMB349_USBCS_PIN_CTRL					1
*/
int smb349_switch_usbcs_mode(int mode)
{
	int ret = 0;
	pr_smb_info("%s, mode:%d\n", __func__, mode);

	switch (mode)
	{
	case SMB349_USBCS_REGISTER_CTRL:
		ret = smb349_masked_write(PIN_ENABLE_CTRL_REG, USBCS_MASK, USBCS_REGISTER_MODE);
/*                pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, PIN_ENABLE_CTRL_REG, USBCS_REGISTER_MODE);*/

		break;

	case SMB349_USBCS_PIN_CTRL:
	default:
		ret = smb349_masked_write(PIN_ENABLE_CTRL_REG, USBCS_MASK, BIT(4));
/*                pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, PIN_ENABLE_CTRL_REG, USBCS_PIN_MODE);*/
		break;
	}

	return ret;
}

static int _smb34x_set_fastchg_curr(int fcc)
{

	int target_curr;
	int ret;

	pr_smb_info("%s, fcc: 0x%x\n", __func__, fcc);
	target_curr = (int)(fcc << SMB349_FAST_CHG_SHIFT);

	pr_smb_info("%s, target_curr: 0x%x \n", __func__, target_curr);

#ifdef SMB349_SET_FC_CURR_DBG
	smb349_dump_reg(SMB_CHG_CURR_REG);
#endif

	ret = smb349_masked_write(SMB_CHG_CURR_REG, FAST_CHG_CURRENT_MASK, target_curr);
#ifdef SMB349_SET_FC_CURR_DBG
	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, SMB_CHG_CURR_REG, target_curr);
#endif

#ifdef SMB349_SET_FC_CURR_DBG
	smb349_dump_reg(SMB_CHG_CURR_REG);
#endif

	return ret;
}

static int _smb349_set_dc_input_curr_limit(int dc_current_limit)
{
	int ret = 0;
	pr_smb_info("%s, 0x%x \n", __func__, dc_current_limit);

#ifdef SMB349_SET_DC_CURR_DBG
	smb349_dump_reg(SMB_CHG_CURR_REG);
#endif

	ret = smb349_masked_write(SMB_CHG_CURR_REG, DC_INPUT_CURRENT_LIMIT_MASK, dc_current_limit);
	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, SMB_CHG_CURR_REG, dc_current_limit);

#ifdef SMB349_SET_DC_CURR_DBG
	smb349_dump_reg(SMB_CHG_CURR_REG);
#endif

	return ret;
}

#if 0

int smb349_get_switch_freq(void)
{
	int ret = 0;
	unsigned char temp = 0;

/*	#define OTG_TLIM_THERM_CNTRL_REG	0x0A	*/
	ret = smb349_i2c_read_byte(&temp,  OTG_TLIM_THERM_CNTRL_REG);
	if(ret) goto exit_err;

	temp = temp & SWITCH_FREQ_MASK;
	temp = temp >> 6;
	switch (temp)
	{
		case SWITCH_FREQ_750KHZ:
			pr_smb_info("%s, SWITCH_FREQ_750KHZ\n", __func__);
			break;

		case SWITCH_FREQ_1MHZ:
			pr_smb_info("%s, SWITCH_FREQ_1MHZ\n", __func__);
			break;

		case SWITCH_FREQ_1D5MHZ:
			pr_smb_info("%s, SWITCH_FREQ_1D5MHZ\n", __func__);
			break;

		case SWITCH_FREQ_3MHZ:
			pr_smb_info("%s, SWITCH_FREQ_3MHZ\n", __func__);
			break;

		default:
			break;

	}

/*        smb349_dump_reg(OTG_TLIM_THERM_CNTRL_REG);*/
	return temp;

	exit_err:
	pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;
}


static int smb349_set_switch_freq(int target_freq)
{
	int ret = 0;
	int my_freq = 0;
	pr_smb_info("%s, 0x%x \n", __func__, target_freq);

/*	#define OTG_TLIM_THERM_CNTRL_REG	0x0A	*/
#ifdef SMB349_SET_SWITCH_FREQ_DBG
	smb349_dump_reg(OTG_TLIM_THERM_CNTRL_REG);
#endif

	my_freq = (int) target_freq << SMB349_SWITCH_FREQ_SHIFT;
	ret = smb349_masked_write(OTG_TLIM_THERM_CNTRL_REG, SWITCH_FREQ_MASK, my_freq);
	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, OTG_TLIM_THERM_CNTRL_REG, my_freq);

#ifdef SMB349_SET_SWITCH_FREQ_DBG
	smb349_dump_reg(OTG_TLIM_THERM_CNTRL_REG);
#endif

	return ret;
}

#endif


int smb349_set_i2c_charger_ctrl_active_low(void)
{
	int ret = 0;
	int my_freq = 0;

	my_freq = (int) SMB349_I2C_CONTROL_ACTIVE_LOW << SMB349_PIN_CONTROL_SHIFT;

	pr_smb_info("%s\n", __func__);

	smb349_dump_reg(PIN_ENABLE_CTRL_REG);

	smb349_masked_write(PIN_ENABLE_CTRL_REG, PIN_CONTROL_ACTIVE_MASK, my_freq);
	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, PIN_ENABLE_CTRL_REG, (unsigned int)my_freq);

	smb349_dump_reg(PIN_ENABLE_CTRL_REG);

	return ret;
}


int smb349_set_i2c_charger_ctrl_active_high(void)
{
	int ret = 0;
	int my_freq = 0;

	my_freq = (int) SMB349_I2C_CONTROL_ACTIVE_HIGH << SMB349_PIN_CONTROL_SHIFT;

	pr_smb_info("%s\n", __func__);

	smb349_dump_reg(PIN_ENABLE_CTRL_REG);

	smb349_masked_write(PIN_ENABLE_CTRL_REG, PIN_CONTROL_ACTIVE_MASK, my_freq);
/*        pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, PIN_ENABLE_CTRL_REG, (unsigned int)my_freq);*/

	smb349_dump_reg(PIN_ENABLE_CTRL_REG);

	return ret;
}




int smb349_get_float_voltage(void)
{
	int ret = 0;
	unsigned char temp = 0;

	pr_smb_info("%s\n", __func__);

	ret = smb349_i2c_read_byte(&temp,  FLOAT_VOLTAGE_REG);
	if(ret) goto exit_err;

	temp = temp & FLOAT_VOLTAGE_MASK;

	switch (temp)
	{
		case SMB349_FLOAT_VOL_4200_MV:
			ret = 4200;
			break;

		case SMB349_FLOAT_VOL_4220_MV:
			ret = 4220;
			break;

		case SMB349_FLOAT_VOL_4240_MV:
			ret = 4240;
			break;

		case SMB349_FLOAT_VOL_4260_MV:
			ret = 4260;
			break;

		case SMB349_FLOAT_VOL_4280_MV:
			ret = 4280;
			break;

		case SMB349_FLOAT_VOL_4300_MV:
			ret = 4300;
			break;

		case SMB349_FLOAT_VOL_4320_MV:
			ret = 4320;
			break;

		case SMB349_FLOAT_VOL_4340_MV:
			ret = 4340;
			break;

		case SMB349_FLOAT_VOL_4350_MV:
			ret = 4350;
			break;

		default:
			break;
			}

	return ret;

	exit_err:
	pr_smb_err("%s, i2c reading error\n", __func__);
		return -1;

}


int _smb349_set_float_voltage(unsigned int fv)
{
	int ret = 0;
	pr_smb_info("%s	fv:0x%x\n", __func__, fv);


	if(smb349_is_usbcs_register_mode())
	{
		pr_smb_info("%s, error due to USBCS = 1\n",	__func__);
		return EIO;
	}

	ret = smb349_masked_write(FLOAT_VOLTAGE_REG, FLOAT_VOLTAGE_MASK, fv);
	if(ret)
		pr_smb_info("%s, write reg(0x%x):0x%x fail, ret=%d\n",	__func__, FLOAT_VOLTAGE_REG, (unsigned int)fv, ret);
	else
		pr_smb_info("%s, write reg(0x%x):0x%x success\n", __func__, FLOAT_VOLTAGE_REG, (unsigned int)fv);
	return ret;
}



int smb349_set_to_usb5(void)
{
	int ret = 0;
	pr_smb_info("%s\n", __func__);
#if 0
	smb349_dump_reg(CMD_B_REG);
#endif

	ret = smb349_masked_write(CMD_B_REG, USB_1_5_MODE_MASK, USB_1_5_MODE);
	if(ret)
		pr_smb_info("%s, error \n", __func__);

	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CMD_B_REG, USB_1_5_MODE);

#if 0
	smb349_dump_reg(CMD_B_REG);
#endif

	return ret;
}


int smb349_set_hc_mode(unsigned int enable)
{
	int ret = 0;
	pr_smb_info("%s enable: %d\n", __func__, enable);

	if(enable)
	{
		ret = smb349_masked_write(CMD_B_REG, USB_HC_MODE_MASK, USB_HC_MODE);
	}
	else
	{
		ret = smb349_masked_write(CMD_B_REG, USB_HC_MODE_MASK, 0);
	}

	return ret;
}

int smb349_set_AICL_mode(unsigned int enable)
{
	int ret = 0;

	pr_smb_info("%s,%x \n", __func__, enable);

	if(enable)
	{
		smb349_masked_write(VAR_FUNC_REG, AICL_MASK, AUTOMATIC_INPUT_CURR_LIMIT_BIT);
#ifdef SMB349_AICL_DBG
		pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, VAR_FUNC_REG, (unsigned int)AUTOMATIC_INPUT_CURR_LIMIT_BIT);
#endif
	}
	else
	{
#ifdef SMB349_AICL_DBG
		pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, VAR_FUNC_REG, 0);
#endif
		smb349_masked_write(VAR_FUNC_REG, AICL_MASK, 0);
	}

	return ret;
}


int smb349_get_charging_src_reg(void)
{
    pr_smb_info("%s  smb349_charging_src_new: %d\n", __func__, smb349_charging_src_new);
	return smb349_charging_src_new;
}

int smb349_get_i2c_slave_id(void)
{
	return smb349_dump_reg(I2C_BUS_REG);
}

void smb349_partial_reg_dump(void)
{
	int chg_enable = 0;
	int is_susp = 0;
	int is_hc_mode = 0;
	int is_usbcs = 0;
	int chg_stage = 0;
	int chg_src_reg = 0;
	int chg_err = 0;
	int dc_input_curr = 0;
	int float_vol = 0;
	int fastchg_curr = 0;
	int aicl_result = 0;
	int power_ok = 0;
	int aicl_enable = 0;
	int aicl_complete = 0;

#if 0
	pr_smb_info("smb349_is_pwrsrc_suspend: %d \n", smb349_is_suspend_mode());
	smb349_is_charging_enabled(&chg_enable);
	pr_smb_info("smb349_is_charging_enabled: %d \n", chg_enable);
	pr_smb_info("smb349_is_hc_mode: %d \n", smb349_is_hc_mode());
	pr_smb_info("smb349_is_usbcs_register_mode: %d \n",	smb349_is_usbcs_register_mode());
	smb349_get_charging_stage();
	pr_smb_info("smb349_get_charging_src_reg: %d \n", smb349_get_charging_src_reg());
	pr_smb_info("smb349_is_charger_error: %d \n", 	smb349_is_charger_error());
	smb349_get_dc_input_curr_limit();
	smb349_get_float_voltage();
	pr_smb_info("smb_get_fastchg_curr: %d \n", 	smb_get_fastchg_curr());
	pr_smb_info("smb349_get_AICL_result: %d \n", 	smb349_get_AICL_result());
	smb349_get_switch_freq();
	pr_smb_info("smb349_is_power_ok: %d \n", smb349_is_power_ok());
	pr_smb_info("smb349_is_AICL_enabled: %d \n", 	smb349_is_AICL_enabled());
	pr_smb_info("smb349_is_AICL_complete: %d \n", smb349_is_AICL_complete());
#endif

	is_susp = smb349_is_suspend_mode();
	smb349_is_charging_enabled(&chg_enable);
	is_hc_mode = smb349_is_hc_mode();
	is_usbcs = smb349_is_usbcs_register_mode();
	chg_stage = smb349_get_charging_stage();
	chg_src_reg = smb349_get_charging_src_reg();
	chg_err = smb349_is_charger_error();
	dc_input_curr = smb349_get_dc_input_curr_limit();
	float_vol = smb349_get_float_voltage();
	fastchg_curr = smb_get_fastchg_curr();
	aicl_result = smb349_get_AICL_result();
	power_ok = smb349_is_power_ok();
	aicl_enable = smb349_is_AICL_enabled();
	aicl_complete = smb349_is_AICL_complete();

	 pr_smb_info("is_susp=%d,chg_enable=%d,is_hc_mode=%d,is_usbcs=%d,chg_stage=%d,chg_src_reg=%d,"
		 " chg_err=%d,dc_input_curr=%d,float_vol=%d\n",
			is_susp,
			chg_enable,
			is_hc_mode,
			is_usbcs,
			chg_stage,
			chg_src_reg,
			chg_err,
			dc_input_curr,
			float_vol);

	 pr_smb_info("fastchg_curr=%d,aicl_result=%d,power_ok=%d,"
		 "  aicl_enable=%d,aicl_complete=%d,pwrsrc_disabled=0x%x,batt_charging_disabled=0x%x\n",
			fastchg_curr,
			aicl_result,
			power_ok,
			aicl_enable,
			aicl_complete,
			smb_pwrsrc_disabled,
			smb_batt_charging_disabled);


}


int smb349_adjust_max_chg_vol(enum htc_extchg_event_type main_event)
{
	int current_vol = 0;
	int target_vol = 0;
	struct smb349_charger_batt_param *chg_batt_param = NULL;

	chg_batt_param = htc_battery_cell_get_cur_cell_charger_cdata();

	if(!chg_batt_param)
	{
		pr_smb_info("%s(%d): chg_batt_param is NULL\n", __func__, __LINE__);
		return -1;
	}

	switch (main_event)
	{
		case HTC_EXTCHG_EVENT_TYPE_TEMP_COOL:
			target_vol = chg_batt_param->cool_bat_voltage;
			pr_smb_info("%s, COOL, target_vol: %d\n", __func__, target_vol);
			break;

		case HTC_EXTCHG_EVENT_TYPE_TEMP_WARM:
			target_vol = chg_batt_param->warm_bat_voltage;
			pr_smb_info("%s, WARM, target_vol: %d\n", __func__, target_vol);
			break;

		case HTC_EXTCHG_EVENT_TYPE_TEMP_NORMAL:
			target_vol = chg_batt_param->max_voltage;
			pr_smb_info("%s, NORMAL, target_vol: %d\n", __func__, target_vol);
			break;

		default:
			pr_smb_info("%s, not supported event: %d\n", __func__, main_event);
			return 0;
			break;

	}

	current_vol = smb349_get_float_voltage();

	pr_smb_info("%s, target_vol: %d, current_vol: %d\n", __func__, target_vol, current_vol);

	if(current_vol == target_vol)
		return 0;

	target_max_voltage_mv = target_vol;

	switch (target_max_voltage_mv)
	{
		case 0:
			pr_smb_info("%s, not getting correct target_max_voltage_mv yet\n", __func__);
			break;
		case 4000:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4000_MV);
			break;
		case 4200:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4200_MV);
			break;
		case 4220:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4220_MV);
			break;
		case 4240:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4240_MV);
			break;
		case 4260:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4260_MV);
			break;
		case 4280:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4280_MV);
			break;
		case 4300:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4300_MV);
			break;
		case 4320:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4320_MV);
			break;
		case 4340:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4340_MV);
			break;
		case 4350:
			_smb349_set_float_voltage(SMB349_FLOAT_VOL_4350_MV);
			break;
		default:
			pr_smb_err("%s, error, no valid value is read\n", __func__);
			break;
	}

	return 0;
}

void smb349_set_max_charging_vol(void)
{

	struct smb349_charger_batt_param *chg_batt_param = NULL;
	int current_vol = 0;

	if(target_max_voltage_mv == 0)
	{
		chg_batt_param = htc_battery_cell_get_cur_cell_charger_cdata();
		if (chg_batt_param) {
			target_max_voltage_mv = chg_batt_param->max_voltage;
		}
		else
			pr_smb_info("%s(%d): chg_batt_param is NULL\n", __func__, __LINE__);
	}

	current_vol = smb349_get_float_voltage();

	pr_smb_info("%s current_vol:%d, target_vol: %d\n", __func__, current_vol, target_max_voltage_mv);

/*		config battery charge voltage 	*/
	if(current_vol != target_max_voltage_mv)
	{
		switch (target_max_voltage_mv)
		{
			case 0:
				pr_smb_info("%s, not getting correct target_max_voltage_mv yet\n", __func__);
				break;
			case 4200:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4200_MV);
				break;
			case 4220:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4220_MV);
				break;
			case 4240:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4240_MV);
				break;
			case 4260:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4260_MV);
				break;
			case 4280:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4280_MV);
				break;
			case 4300:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4300_MV);
				break;
			case 4320:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4320_MV);
				break;
			case 4340:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4340_MV);
				break;
			case 4350:
				_smb349_set_float_voltage(SMB349_FLOAT_VOL_4350_MV);
				break;
			default:
				pr_smb_err("%s, error, no valid value is read\n", __func__);
				break;
		}
	}
}


int smb349_charger_get_attr_text(char *buf, int size)
{
///	int rc;
///	struct pm8xxx_adc_chan_result result;
	int len = 0;
	int val = 0;
	const int verbose = 0;

/*        pr_smb_info("%s ++, size: %d\n", __func__, size);*/

	if (smb349_initial < 0)
		return smb349_initial;


	len = strlen(buf);
	if(smb_chip_rev == SMB_349)
		len += scnprintf(buf + len, size - len, " SMB349 ---------------------\n");
	else
		len += scnprintf(buf + len, size - len, " SMB340 ---------------------\n");

/*        pr_smb_info("%s , len: %d size: %d\n", __func__, len, size);*/

	len += scnprintf(buf + len, size - len,
			"CHG_CUR_REG(0x%x):  0x%x;\n"
			"CHG_OTH_CUR_REG(0x%x):  0x%x;\n"
			"VAR_FUNC_REG(0x%x):  0x%x;\n"
			"FLOAT_VOLTAGE_REG(0x%x):  0x%x;\n"
			"CHG_CTRL_REG(0x%x):  0x%x;\n",
			SMB_CHG_CURR_REG, smb349_dump_reg_verbose(SMB_CHG_CURR_REG, verbose),
			CHG_OTHER_CURRENT_REG, smb349_dump_reg_verbose(CHG_OTHER_CURRENT_REG, verbose),
			VAR_FUNC_REG, smb349_dump_reg_verbose(VAR_FUNC_REG, verbose),
			FLOAT_VOLTAGE_REG, smb349_dump_reg_verbose(FLOAT_VOLTAGE_REG, verbose),
			CHG_CTRL_REG, smb349_dump_reg_verbose(CHG_CTRL_REG, verbose)
			);


	val = smb349_dump_reg_verbose(STAT_TIMER_REG, verbose);
	len += scnprintf(buf + len, size - len, "STAT_TIMER_REG(0x%x):  0x%x;\n", STAT_TIMER_REG, val);

	val = smb349_dump_reg_verbose(PIN_ENABLE_CTRL_REG, verbose);
	len += scnprintf(buf + len, size - len, "PIN_ENABLE_CTRL_REG(0x%x):  0x%x;\n", PIN_ENABLE_CTRL_REG, val);

	val = smb349_dump_reg_verbose(THERM_CTRL_A_REG, verbose);
	len += scnprintf(buf + len, size - len, "THERM_CTRL_A_REG(0x%x):  0x%x;\n", THERM_CTRL_A_REG, val);

	val = smb349_dump_reg_verbose(SYSOK_USB3_SEL_REG, verbose);
	len += scnprintf(buf + len, size - len, "SYSOK_USB3_SEL_REG(0x%x):  0x%x;\n", SYSOK_USB3_SEL_REG, val);

	val = smb349_dump_reg_verbose(CTRL_FUNC_REG, verbose);
	len += scnprintf(buf + len, size - len, "CTRL_FUNC_REG(0x%x):  0x%x;\n", CTRL_FUNC_REG, val);

	val = smb349_dump_reg_verbose(OTG_TLIM_THERM_CNTRL_REG, verbose);
	len += scnprintf(buf + len, size - len, "OTGTLIM_THERCTRL_REG(0x%x):  0x%x;\n", OTG_TLIM_THERM_CNTRL_REG, val);

	val = smb349_dump_reg_verbose(LIMIT_CELL_TEMP_MONI_REG, verbose);
	len += scnprintf(buf + len, size - len, "LIMITCELL_TEMPMONI_REG(0x%x):  0x%x;\n", LIMIT_CELL_TEMP_MONI_REG, val);

	val = smb349_dump_reg_verbose(FAULT_IRQ_REG, verbose);
	len += scnprintf(buf + len, size - len, "FAULT_IRQ_REG(0x%x):  0x%x;\n", FAULT_IRQ_REG, val);

	val = smb349_dump_reg_verbose(STATUS_IRQ_REG, verbose);
	len += scnprintf(buf + len, size - len, "STATUS_IRQ_REG(0x%x):  0x%x;\n", STATUS_IRQ_REG, val);

	val = smb349_dump_reg_verbose(SYSOK_REG, verbose);
	len += scnprintf(buf + len, size - len, "SYSOK_REG(0x%x):  0x%x;\n", SYSOK_REG, val);

/*        pr_smb_info("%s , len: %d size: %d\n", __func__, len, size);*/

	val = smb349_dump_reg_verbose(AUTO_INPUT_VOL_DET_REG, verbose);
	len += scnprintf(buf + len, size - len, "AUTOINP_VOLDET_REG(0x%x):  0x%x;\n", AUTO_INPUT_VOL_DET_REG, val);

	val = smb349_dump_reg_verbose(I2C_BUS_REG, verbose);
	len += scnprintf(buf + len, size - len, "I2C_BUS_REG(0x%x):  0x%x;\n", I2C_BUS_REG, val);

	len += scnprintf(buf + len, size - len, "pwrsrc_disabled_reason: 0x%x;\n",  smb_pwrsrc_disabled);
	len += scnprintf(buf + len, size - len, "charging_disabled_reason: 0x%x;\n",  smb_batt_charging_disabled);

	smb349_is_charging_enabled(&val);
	len += scnprintf(buf + len, size - len, "is_charging_enabled: %d;\n",  val);

	smb349_is_charger_overvoltage(&val);
	len += scnprintf(buf + len, size - len, "is_charger_overvoltage: %d;\n",  val);

	len += scnprintf(buf + len, size - len, "is_power_ok: %d;\n", smb349_is_power_ok());
	len += scnprintf(buf + len, size - len, "is_hc_mode: %d;\n",  smb349_is_hc_mode());
	len += scnprintf(buf + len, size - len, "is_usbcs_register_mode: %d;\n",  smb349_is_usbcs_register_mode());
	len += scnprintf(buf + len, size - len, "charging_stage: %d;\n",  smb349_get_charging_stage());
	len += scnprintf(buf + len, size - len, "charging_src_reg: %d;\n",  smb349_get_charging_src_reg());
	len += scnprintf(buf + len, size - len, "is_charger_error: %d;\n",  smb349_is_charger_error());
/*        len += scnprintf(buf + len, size - len, "get_i2c_slave_id: 0x%x;\n",  smb349_get_i2c_slave_id());*/
/*        len += scnprintf(buf + len, size - len, "smb349_charging_src_new: %d;\n",  smb349_charging_src_new);  /// sf debug, remove me later*/


	len += scnprintf(buf + len, size - len, "smb349_get_float_voltage:");

	switch (smb349_get_float_voltage())
	{
		case 0:
			len += scnprintf(buf + len, size - len, "  not getting correct target_max_voltage_mv yet\n");
			break;
		case 4000:
			len += scnprintf(buf + len, size - len, " 4000_MV;\n");
			break;
		case 4200:
			len += scnprintf(buf + len, size - len, " 4200_MV;\n");
			break;
		case 4220:
			len += scnprintf(buf + len, size - len, " 4220_MV;\n");
			break;
		case 4240:
			len += scnprintf(buf + len, size - len, " 4240_MV;\n");
			break;
		case 4260:
			len += scnprintf(buf + len, size - len, " 4260_MV;\n");
			break;
		case 4280:
			len += scnprintf(buf + len, size - len, " 4280_MV;\n");
			break;
		case 4300:
			len += scnprintf(buf + len, size - len, " 4300_MV;\n");
			break;
		case 4320:
			len += scnprintf(buf + len, size - len, " 4320_MV;\n");
			break;
		case 4340:
			len += scnprintf(buf + len, size - len, " 4340_MV;\n");
			break;
		case 4350:
			len += scnprintf(buf + len, size - len, " 4350_MV;\n");
			break;
		default:
			len += scnprintf(buf + len, size - len, " error, no valid value is read;\n");
			break;
	}


	len += scnprintf(buf + len, size - len, "fast_charge_curr:");

	if(smb_chip_rev == SMB_349)
	{
		switch (smb_get_fastchg_curr())
		{
			case FAST_CHARGE_1000MA:
				len += scnprintf(buf + len, size - len, " 1000MA;\n");
				break;

			case FAST_CHARGE_1100MA:
				len += scnprintf(buf + len, size - len, " 1100MA;\n");
				break;

			case FAST_CHARGE_1200MA:
				len += scnprintf(buf + len, size - len, " 1200MA;\n");
				break;

			case FAST_CHARGE_1600MA:
				len += scnprintf(buf + len, size - len, " 1600MA;\n");
				break;

			default:
				len += scnprintf(buf + len, size - len, " unknown;\n");
				break;
		}
	}
	else if(smb_chip_rev == SMB_340)
	{
		switch (smb_get_fastchg_curr())
		{
			case SMB340_FASTCHG_1000MA:
				len += scnprintf(buf + len, size - len, " 1000MA;\n");
				break;

			case SMB340_FASTCHG_1200MA:
				len += scnprintf(buf + len, size - len, " 1200MA;\n");
				break;

			case SMB340_FASTCHG_1400MA:
				len += scnprintf(buf + len, size - len, " 1400MA;\n");
				break;

			case SMB340_FASTCHG_1600MA:
				len += scnprintf(buf + len, size - len, " 1600MA;\n");
				break;

			default:
				len += scnprintf(buf + len, size - len, " unknown;\n");
				break;
		}
	}
	else
	{
		len += scnprintf(buf + len, size - len, " unknown chip;\n");
		/* unknown smb chip revision */
	}

	len += scnprintf(buf + len, size - len, "dc_input_curr_limit:");
	switch (smb349_get_dc_input_curr_limit())
	{
		case DC_INPUT_500MA:
			len += scnprintf(buf + len, size - len, " 500MA;\n");
			break;

		case DC_INPUT_900MA:
			len += scnprintf(buf + len, size - len, " 900MA;\n");
			break;

		case DC_INPUT_1000MA:
			len += scnprintf(buf + len, size - len, " 1000MA;\n");
			break;

		case DC_INPUT_1100MA:
			len += scnprintf(buf + len, size - len, " 1100MA;\n");
			break;

		case DC_INPUT_1200MA:
			len += scnprintf(buf + len, size - len, " 1200MA;\n");
			break;

		case DC_INPUT_1300MA:
			len += scnprintf(buf + len, size - len, " 1300MA;\n");
			break;

		case DC_INPUT_1500MA:
			len += scnprintf(buf + len, size - len, " 1500MA;\n");
			break;

		case DC_INPUT_1600MA:
			len += scnprintf(buf + len, size - len, " 1600MA;\n");
			break;

		case DC_INPUT_1700MA:
			len += scnprintf(buf + len, size - len, " 1700MA;\n");
			break;

		case DC_INPUT_1800MA:
			len += scnprintf(buf + len, size - len, " 1800MA;\n");
			break;

		case DC_INPUT_2000MA:
			len += scnprintf(buf + len, size - len, " 2000MA;\n");
			break;

		case DC_INPUT_2200MA:
			len += scnprintf(buf + len, size - len, " 2200MA;\n");
			break;

		case DC_INPUT_2400MA:
			len += scnprintf(buf + len, size - len, " 2400MA;\n");
			break;

		case DC_INPUT_2500MA:
			len += scnprintf(buf + len, size - len, " 2500MA;\n");
			break;

		case DC_INPUT_3000MA:
			len += scnprintf(buf + len, size - len, " 3000MA;\n");
			break;

		case DC_INPUT_3500MA:
			len += scnprintf(buf + len, size - len, " 3500MA;\n");
			break;

		default:
			len += scnprintf(buf + len, size - len, " unknown;\n");
			break;
	}


	len += scnprintf(buf + len, size - len, "last_AICL_result:");
	switch (aicl_latest_result)
	{
		case AICL_RESULT_500MA:
			len += scnprintf(buf + len, size - len, " 500MA;\n");
			break;
		case AICL_RESULT_900MA:
			len += scnprintf(buf + len, size - len, " 900MA;\n");
			break;
		case AICL_RESULT_1000MA:
			len += scnprintf(buf + len, size - len, " 1000MA;\n");
			break;
		case AICL_RESULT_1100MA:
			len += scnprintf(buf + len, size - len, " 1100MA;\n");
			break;
		case AICL_RESULT_1200MA:
			len += scnprintf(buf + len, size - len, " 1200MA;\n");
			break;
		case AICL_RESULT_1300MA:
			len += scnprintf(buf + len, size - len, " 1300MA;\n");
			break;
		case AICL_RESULT_1500MA:
			len += scnprintf(buf + len, size - len, " 1500MA;\n");
			break;
		case AICL_RESULT_1600MA:
			len += scnprintf(buf + len, size - len, " 1600MA;\n");
			break;
		case AICL_RESULT_1700MA:
			len += scnprintf(buf + len, size - len, " 1700MA;\n");
			break;
		case AICL_RESULT_1800MA:
			len += scnprintf(buf + len, size - len, " 1800MA;\n");
			break;
		case AICL_RESULT_2000MA:
			len += scnprintf(buf + len, size - len, " 2000MA;\n");
			break;
		case AICL_RESULT_2200MA:
			len += scnprintf(buf + len, size - len, " 2200MA;\n");
			break;
		case AICL_RESULT_2400MA:
			len += scnprintf(buf + len, size - len, " 2400MA;\n");
			break;
		case AICL_RESULT_2500MA:
			len += scnprintf(buf + len, size - len, " 2500MA;\n");
			break;
		case AICL_RESULT_3000MA:
			len += scnprintf(buf + len, size - len, " 3000MA;\n");
			break;
		case AICL_RESULT_3500MA:
			len += scnprintf(buf + len, size - len, " 3500MA;\n");
			break;
		default:
			len += scnprintf(buf + len, size - len, " unknown;\n");
			break;

	}
	len += scnprintf(buf + len, size - len, "current_AICL_result:");
	switch (smb349_get_AICL_result())
	{
		case AICL_RESULT_500MA:
			len += scnprintf(buf + len, size - len, " 500MA;\n");
			break;
		case AICL_RESULT_900MA:
			len += scnprintf(buf + len, size - len, " 900MA;\n");
			break;
		case AICL_RESULT_1000MA:
			len += scnprintf(buf + len, size - len, " 1000MA;\n");
			break;
		case AICL_RESULT_1100MA:
			len += scnprintf(buf + len, size - len, " 1100MA;\n");
			break;
		case AICL_RESULT_1200MA:
			len += scnprintf(buf + len, size - len, " 1200MA;\n");
			break;
		case AICL_RESULT_1300MA:
			len += scnprintf(buf + len, size - len, " 1300MA;\n");
			break;
		case AICL_RESULT_1500MA:
			len += scnprintf(buf + len, size - len, " 1500MA;\n");
			break;
		case AICL_RESULT_1600MA:
			len += scnprintf(buf + len, size - len, " 1600MA;\n");
			break;
		case AICL_RESULT_1700MA:
			len += scnprintf(buf + len, size - len, " 1700MA;\n");
			break;
		case AICL_RESULT_1800MA:
			len += scnprintf(buf + len, size - len, " 1800MA;\n");
			break;
		case AICL_RESULT_2000MA:
			len += scnprintf(buf + len, size - len, " 2000MA;\n");
			break;
		case AICL_RESULT_2200MA:
			len += scnprintf(buf + len, size - len, " 2200MA;\n");
			break;
		case AICL_RESULT_2400MA:
			len += scnprintf(buf + len, size - len, " 2400MA;\n");
			break;
		case AICL_RESULT_2500MA:
			len += scnprintf(buf + len, size - len, " 2500MA;\n");
			break;
		case AICL_RESULT_3000MA:
			len += scnprintf(buf + len, size - len, " 3000MA;\n");
			break;
		case AICL_RESULT_3500MA:
			len += scnprintf(buf + len, size - len, " 3500MA;\n");
			break;
		default:
			len += scnprintf(buf + len, size - len, " unknown;\n");
			break;

	}

	len += scnprintf(buf + len, size - len, "is_AICL_enabled: %d;\n", smb349_is_AICL_enabled());
	len += scnprintf(buf + len, size - len, "is_AICL_complete: %d;\n",  smb349_is_AICL_complete());
	len += scnprintf(buf + len, size - len, "smb_adapter_type:");
	switch (smb_adapter_type)
	{
		case SMB_ADAPTER_UNKNOWN:
			len += scnprintf(buf + len, size - len, " UNKNOWN;\n");
			break;
		case SMB_ADAPTER_UNDER_1A:
			len += scnprintf(buf + len, size - len, " UNDER_1A;\n");
			break;
		case SMB_ADAPTER_1A:
			len += scnprintf(buf + len, size - len, " 1A;\n");
			break;
		case SMB_ADAPTER_KDDI:
			len += scnprintf(buf + len, size - len, " KDDI;\n");
			break;
		default:
			break;
	}

	len += scnprintf(buf + len, size - len, "aicl_sm:");

	switch (aicl_sm)
	{
		case AICL_SM_0_RESET:
			len += scnprintf(buf + len, size - len, " 0_RESET;\n");
			break;
		case AICL_SM_1ST_AICL_DONE	:
			len += scnprintf(buf + len, size - len, " 1ST_AICL_DONE;\n");
			break;
		case AICL_SM_1ST_AICL_PROCESSING:
			len += scnprintf(buf + len, size - len, " 1ST_AICL_PROCESSING;\n");
			break;
		case AICL_SM_1ST_AC_IN:
			len += scnprintf(buf + len, size - len, " 1_AC_IN;\n");
			break;
		case AICL_SM_1_USB_IN:
			len += scnprintf(buf + len, size - len, " USB_IN;\n");
			break;
		case AICL_SM_2ST_AICL_PREPARE:
			len += scnprintf(buf + len, size - len, " 2ST_AICL_PREPARE;\n");
			break;
		case AICL_SM_2ST_AICL_PROCESSING:
			len += scnprintf(buf + len, size - len, " 2ST_AICL_PROCESSING;\n");
			break;
		case AICL_SM_2ST_AICL_DONE:
			len += scnprintf(buf + len, size - len, " 2ST_AICL_DONE;\n");
			break;
		default:
			break;
	}

#if 0
	/* it's by default 1MZ now. Thus no need to dump anymore */
	len += scnprintf(buf + len, size - len, "switch_freq:");
	switch (smb349_get_switch_freq())
	{
		case SWITCH_FREQ_750KHZ:
			len += scnprintf(buf + len, size - len, " 750KHZ;\n");
			break;
		case SWITCH_FREQ_1MHZ:
			len += scnprintf(buf + len, size - len, " 1MHZ;\n");
			break;
		default:
			break;
	}
#endif

	val = smb349_dump_reg_verbose(CMD_A_REG, verbose);
	len += scnprintf(buf + len, size - len, "CMD_A_REG(0x%x):  0x%x;\n", CMD_A_REG, val);

	val = smb349_dump_reg_verbose(CMD_B_REG, verbose);
	len += scnprintf(buf + len, size - len, "CMD_B_REG(0x%x):  0x%x;\n", CMD_B_REG, val);

	val = smb349_dump_reg_verbose(CMD_C_REG, verbose);
	len += scnprintf(buf + len, size - len, "CMD_C_REG(0x%x):  0x%x;\n", CMD_C_REG, val);

/*        pr_smb_info("%s , len: %d size: %d\n", __func__, len, size);*/

	val = smb349_dump_reg_verbose(IRQ_A_REG, verbose);
	len += scnprintf(buf + len, size - len, "IRQ_A_REG(0x%x):  0x%x;\n", IRQ_A_REG, val);

	val = smb349_dump_reg_verbose(IRQ_B_REG, verbose);
	len += scnprintf(buf + len, size - len, "IRQ_B_REG(0x%x):  0x%x;\n", IRQ_B_REG, val);

	val = smb349_dump_reg_verbose(IRQ_C_REG, verbose);
	len += scnprintf(buf + len, size - len, "IRQ_C_REG(0x%x):  0x%x;\n", IRQ_C_REG, val);

	val = smb349_dump_reg_verbose(IRQ_D_REG, verbose);
	len += scnprintf(buf + len, size - len, "IRQ_D_REG(0x%x):  0x%x;\n", IRQ_D_REG, val);

	val = smb349_dump_reg_verbose(IRQ_E_REG, verbose);
	len += scnprintf(buf + len, size - len, "IRQ_E_REG(0x%x):  0x%x;\n", IRQ_E_REG, val);

/*        pr_smb_info("%s , len: %d size: %d\n", __func__, len, size);*/

	val = smb349_dump_reg_verbose(IRQ_F_REG, verbose);
	len += scnprintf(buf + len, size - len, "IRQ_F_REG(0x%x):  0x%x;\n", IRQ_F_REG, val);

	val = smb349_dump_reg_verbose(STATUS_A_REG, verbose);
	len += scnprintf(buf + len, size - len, "STATUS_A_REG(0x%x):  0x%x;\n", STATUS_A_REG, val);

	val = smb349_dump_reg_verbose(STATUS_B_REG, verbose);
	len += scnprintf(buf + len, size - len, "STATUS_B_REG(0x%x):  0x%x;\n", STATUS_B_REG, val);

	val = smb349_dump_reg_verbose(STATUS_C_REG, verbose);
	len += scnprintf(buf + len, size - len, "STATUS_C_REG(0x%x):  0x%x;\n", STATUS_C_REG, val);

	val = smb349_dump_reg_verbose(STATUS_D_REG, verbose);
	len += scnprintf(buf + len, size - len, "STATUS_D_REG(0x%x):  0x%x;\n", STATUS_D_REG, val);

	val = smb349_dump_reg_verbose(STATUS_E_REG, verbose);
	len += scnprintf(buf + len, size - len, "STATUS_E_REG(0x%x):  0x%x;\n", STATUS_E_REG, val);



/*        len += scnprintf(buf + len, size - len, " SMB349 ---------------------%d\n", len);*/

/*        pr_smb_info("%s , len: %d size: %d\n", __func__, len, size);*/

	pr_smb_info("%s --\n", __func__);

	return len;
}


static int _smb349_set_aicl_threshold(int dc_current_limit)
{
	return _smb349_set_dc_input_curr_limit(dc_current_limit);
}

static void smb349_create_aicl_worker(int sec)
{

		/* clear queue worker if any */
		cancel_delayed_work_sync(&aicl_check_work);

		aicl_worker_ongoing = 1;
		pr_smb_info("%s, aicl_worker_ongoing = %d, sec: %d \n", __func__, aicl_worker_ongoing, sec);

		queue_delayed_work(smb349_wq, &aicl_check_work, (HZ * sec));
}

static void smb349_aicl_phase1(void)
{

	pr_smb_info("%s	++\n", __func__);

	if(aicl_on)
	{
		/* issue a delay worker */
		/* ready to do AICL phase1  */

		/* issue a delay worker */
		mutex_lock(&aicl_sm_lock);
		aicl_sm = AICL_SM_1ST_AC_IN;
		smb_adapter_type = SMB_ADAPTER_UNKNOWN;  /* clear kddi variable */
		mutex_unlock(&aicl_sm_lock);
		pr_smb_info("%s	aicl_sm: %d\n", __func__, aicl_sm);
		pr_smb_info("%s smb_adapter_type: %d\n", __func__, smb_adapter_type);


		/* step 1, enable aicl */
		smb349_set_AICL_mode(1);

		/* step 2, setup roof of aicl result */
		_smb349_set_aicl_threshold(DC_INPUT_1000MA);

		/* step 3, change to usb mode first */
		smb349_set_hc_mode(SMB349_USB_MODE);

		/* step 4, change to hc mode again, trigger aicl */
		smb349_set_hc_mode(SMB349_HC_MODE);

		/* step 5, setup worker to get result later */
		smb349_create_aicl_worker(AICL_CHECK_PERIOD_STAGE1_1S);

	}
	else
		pr_smb_info("%s	fail!! aicl_on: %d, aicl_sm:%d, smb_adapter_type: %d \n",
		__func__, aicl_on, aicl_sm, smb_adapter_type);

	pr_smb_info("%s	--\n", __func__);
}

static void smb349_aicl_phase2(void)
{
	pr_smb_info("%s	++\n", __func__);

	if((aicl_on) && (aicl_sm == AICL_SM_1ST_AICL_DONE) && (smb_adapter_type == SMB_ADAPTER_1A))
	{
		/* issue a delay worker */
		/* ready to do AICL phase 2 */
		mutex_lock(&aicl_sm_lock);
		aicl_sm = AICL_SM_2ST_AICL_PREPARE;
		mutex_unlock(&aicl_sm_lock);
		pr_smb_info("%s	aicl_sm: %d\n", __func__, aicl_sm);


		/* step 1, enable aicl */
		smb349_set_AICL_mode(1);

		/* step 2.1, setup roof of aicl result */
		_smb349_set_aicl_threshold(DC_INPUT_1700MA);

		/* step 2.2, setup battery current to 1600mA, for phase2 1.7A AICL  tests*/
		_smb34x_set_fastchg_curr(get_fastchg_curr_def(MAX_FASTCHG_INPUT_CURR));

		/* step 3, change to usb mode first */
		smb349_set_hc_mode(SMB349_USB_MODE);

		/* step 4, change to hc mode again, trigger aicl */
		smb349_set_hc_mode(SMB349_HC_MODE);

		/* step 5, setup worker to get result later */
		smb349_create_aicl_worker(AICL_CHECK_PERIOD_STAGE2_2S);
	}
	else
		pr_smb_info("%s	fail !!aicl_on: %d, aicl_sm:%d, smb_adapter_type: %d \n",
		__func__, aicl_on, aicl_sm, smb_adapter_type);

	pr_smb_info("%s	--\n", __func__);
}

static	void delay_phase2_check_worker(struct work_struct *work)
{
	pr_smb_info("%s \n", __func__);
	mutex_lock(&phase_lock);
	smb349_aicl_phase2();
	mutex_unlock(&phase_lock);
}

int smb349_config(void)
{
	pr_smb_info("%s ++\n", __func__);

	/* debug */
	smb349_partial_reg_dump();

	smb349_allow_volatile_wrtting();
/*        smb349_dump_reg(CMD_A_REG);*/

/*        smb349_not_allow_charging_cycle_end();*/
/*        smb349_dump_reg(CHG_CTRL_REG);*/

/*        smb349_allow_fast_charging_setting();*/
/*        smb349_dump_reg(CMD_A_REG);*/

/* 	before enable register mode, if it's usb in the default usb mode is 1, means only 100mA charging current is provided.
	   We Must switch from usb1 to usb5 mode to avoid little current charging, which may cause device power lost */
	smb349_set_to_usb5();

	// switch to I2C command base mode
	smb349_switch_usbcs_mode(SMB349_USBCS_REGISTER_CTRL);
/*        smb349_dump_reg(PIN_ENABLE_CTRL_REG);*/

/* 	need to set switch_freq to 1MHz or unplug-reload will fail	*/
/*  2013/1/31, no need to set anymore because it's correct 1MZ by default now */
/*		smb349_set_switch_freq(SWITCH_FREQ_1MHZ);		*/

	if(aicl_on)
		smb349_set_AICL_mode(1);
	else
		smb349_set_AICL_mode(0);

		_smb349_set_aicl_threshold(DC_INPUT_1000MA);
/*********************************************************************/

///	smb349_set_max_charging_vol();

/*********************************************************************/

/*        smb349_dump_reg(VAR_FUNC_REG);*/

///	smb349_set_pwrsrc(SLOW_CHARGE);
///	smb349_set_pwrsrc(FAST_CHARGE);
///	smb349_set_pwrsrc(KDDI_CHARGE);

///	smb349_dbg();

	pr_smb_info("%s --\n", __func__);

	return 0;

}

static int _smb34x_set_prechg_curr(int pre_charge_curr)
{
	int ret = 0;
	int target_curr = 0;
	pr_smb_info("%s, 0x%x \n", __func__, pre_charge_curr);


	target_curr = (int)(pre_charge_curr << SMB349_PRE_CHG_SHIFT);

#ifdef SMB349_SET_DC_CURR_DBG
	smb349_dump_reg(CHG_OTHER_CURRENT_REG);
#endif

	ret = smb349_masked_write(CHG_OTHER_CURRENT_REG, PRE_CHG_CURRENT_MASK, target_curr);
	pr_smb_info("%s, write reg(0x%x):0x%x \n", __func__, CHG_OTHER_CURRENT_REG, target_curr);

#ifdef SMB349_SET_DC_CURR_DBG
	smb349_dump_reg(CHG_OTHER_CURRENT_REG);
#endif

	return ret;
}


static int _smb349_limit_charge_enable(bool enable)
{
///#define SYSOK_REG		0x0E
	int ret = 0;

	pr_smb_info("%s enable:%d\n", __func__, enable);

	if(enable)
	{
		ret = smb349_masked_write(SYSOK_REG, PRE_CHG_TO_FAST_CHG_ENABLE_MASK, BIT(1));
	}
	else
	{
		ret = smb349_masked_write(SYSOK_REG, PRE_CHG_TO_FAST_CHG_ENABLE_MASK, 0);
	}

	return ret;
}

int smb349_is_batt_charge_enable(void)
{
	if(smb_batt_charging_disabled)
		return 0;
	else
		return 1;
}
EXPORT_SYMBOL(smb349_is_batt_charge_enable);


#ifdef CONFIG_DUTY_CYCLE_LIMIT
int smb349_limit_charge_enable(int chg_limit_reason, int chg_limit_timer_sub_mask, int limit_charge_timer_ma)
{
	int ret;
	int chg_limit_current = LIMIT_PRECHG_CURR;

	pr_smb_info("%s:chg_limit_reason=%d, chg_limit_timer_sub_mask=%d, limit_charge_timer_ma=%d\n",
		__func__, chg_limit_reason, chg_limit_timer_sub_mask, limit_charge_timer_ma);

	/* NOTE: limit_charge_timer_ma works if it is not 0 and timer_enable is not 0 */
	if (limit_charge_timer_ma != 0 && !!(chg_limit_reason & chg_limit_timer_sub_mask))
		chg_limit_current = limit_charge_timer_ma;

	pr_smb_info("%s:chg_limit_current = %d\n", __func__, chg_limit_current);

	ret = _smb34x_set_prechg_curr(get_prechg_curr_def(chg_limit_current));

	if (chg_limit_reason)
		ret = _smb349_limit_charge_enable(1);
	else
		ret = _smb349_limit_charge_enable(0);

	return 0;
}
#else
int smb349_limit_charge_enable(bool enable)
{
///#define SYSOK_REG		0x0E
	int ret = 0;

	pr_smb_info("%s enable:%d\n", __func__, enable);

	ret = _smb34x_set_prechg_curr(get_prechg_curr_def(LIMIT_PRECHG_CURR));

	if(enable)
	{
		ret = _smb349_limit_charge_enable(1);
	}
	else
	{
		ret = _smb349_limit_charge_enable(0);
	}

	return ret;
}
EXPORT_SYMBOL(smb349_limit_charge_enable);
#endif


/* call ext charger temp handler */
static void smb_state_check_worker(struct work_struct *w)
{

	pr_smb_info("%s ++\n", __func__);

	/* ever found the smb340 was not reset, but charging vol was changed.
		thus reconfiguire it here, no matter ic is reset or not
	*/
	if(smb349_is_power_ok())
			smb349_set_max_charging_vol();

	/* use usbcs register to monitor if smb340 is reset or not.
		Suppose it should be register control.
		If it changes back to pin control, means smb340 being reset due to unknown reason
	*/
	if (smb349_is_usbcs_register_mode() == SMB349_USBCS_PIN_CTRL)
	{
		/* something wrong! there's vbus quick drop cause AICL being enabled again and
		register being clear. Need to reconfig charging register */
		pr_smb_info("%s need to reconfig register!, aicl_worker_ongoing:%d, aicl_on:%d\n", __func__, aicl_worker_ongoing, aicl_on);

		/*	step 1,	reconfig */
		smb349_config();

		/* step 1.1, setup correct charging voltage */
		if(smb349_is_power_ok())
			smb349_set_max_charging_vol();

		/*	step 1.2,	up-side-down the charging enable bit define, to make sure it's not-charging by default */
		smb349_set_i2c_charger_ctrl_active_high();

		pr_smb_info("%s	smb349_charging_src_new: %d\n", __func__, smb349_charging_src_new);
		switch (smb349_charging_src_new)
		{
			case HTC_PWR_SOURCE_TYPE_USB:
			case HTC_PWR_SOURCE_TYPE_UNKNOWN_USB:
				mutex_lock(&aicl_sm_lock);
				aicl_sm = AICL_SM_1_USB_IN;
				smb_adapter_type = SMB_ADAPTER_USB;
				mutex_unlock(&aicl_sm_lock);
				pr_smb_info("%s	aicl_sm: %d\n", __func__, aicl_sm);
				pr_smb_info("%s smb_adapter_type: %d\n", __func__, smb_adapter_type);
				break;

			case HTC_PWR_SOURCE_TYPE_AC:
			case HTC_PWR_SOURCE_TYPE_9VAC:

				if(aicl_on)
				{
					/* aicl phase 1 */
					mutex_lock(&phase_lock);
					smb349_aicl_phase1();
					mutex_unlock(&phase_lock);
				}
				else
				{

					smb_adapter_type = SMB_ADAPTER_1A;
					/* set hc mode */
					smb349_set_hc_mode(SMB349_HC_MODE);
					/* set fast charging current to 1000mA */
					_smb34x_set_fastchg_curr(get_fastchg_curr_def(DEFAULT_FASTCHG_INPUT_CURR));
					/* set dc input current to 1000mA */
					_smb349_set_dc_input_curr_limit(DC_INPUT_1000MA);

				}
				break;

				default:

					break;

		}

		/* recover pwrsrc */
		mutex_lock(&pwrsrc_lock);
		_smb349_enable_pwrsrc(!smb_pwrsrc_disabled);	/* see if enable charger */
		mutex_unlock(&pwrsrc_lock);

		/* recover battery charging */
		mutex_lock(&charger_lock);
		_smb349_enable_charging(!smb_batt_charging_disabled);	/* see if enable charger */
		mutex_unlock(&charger_lock);

	}

	if(smb349_update_state())
		smb349_adjust_fast_charge_curr();

	/* issue next checking worker */
	queue_delayed_work(smb349_wq, &smb_state_check_task, (HZ * SMB_STATE_UPDATE_PERIOD_SEC));

	pr_smb_info("%s --\n", __func__);
}


static	void aicl_check_worker(struct work_struct *work)
{
	int is_hc_mode = 0;
	int is_aicl_complete = 0;

	mutex_lock(&phase_lock);

	pr_smb_info("%s	++\n", __func__);
	is_aicl_complete = smb349_is_AICL_complete();
	pr_smb_info("smb349_is_AICL_complete: %d \n", is_aicl_complete);
	is_hc_mode = smb349_is_hc_mode();
	pr_smb_info("smb349_is_hc_mode:	%d \n",	is_hc_mode);

	mutex_lock(&aicl_sm_lock);
	if(aicl_sm == AICL_SM_1ST_AC_IN)
	{
		aicl_sm =  AICL_SM_1ST_AICL_PROCESSING;
	}
	else if(aicl_sm == AICL_SM_2ST_AICL_PREPARE)
	{
		aicl_sm = AICL_SM_2ST_AICL_PROCESSING;
	}
	else
	{
		pr_smb_info("%s	aicl_sm_interleaving !! exit! aicl_sm:%d\n", __func__, aicl_sm);
		mutex_unlock(&aicl_sm_lock);
		mutex_unlock(&phase_lock);
		return;
	}
	mutex_unlock(&aicl_sm_lock);

	pr_smb_info("%s	aicl_sm: %d\n", __func__, aicl_sm);

	if((!is_hc_mode) || (!is_aicl_complete))
	{
/*		Woops! charger is locked	due	to AICL	process	*/
/*		NOTICE !!  MUST set DC to 1000mA	first before switch	to HC mode again,*/
/*		   because if the mode switch to USB mode again, means the charger is*/
/*		   locked due to AICL process.*/
		pr_smb_info("%s config as poor AC\n", __func__);

/*		reconfig */
		smb349_config();
/*		step 1 set dc input	current	to 500mA */
		if(aicl_sm == AICL_SM_1ST_AICL_PROCESSING)
			_smb349_set_dc_input_curr_limit(DC_INPUT_500MA);
		if(aicl_sm == AICL_SM_2ST_AICL_PROCESSING)
			_smb349_set_dc_input_curr_limit(DC_INPUT_1000MA);

/* set fast charge input current to 1000mA */
		_smb34x_set_fastchg_curr(get_fastchg_curr_def(DEFAULT_FASTCHG_INPUT_CURR));

/*		step 2 switch from USB mode	to HC mode again*/
		smb349_set_hc_mode(SMB349_HC_MODE);

		mutex_lock(&aicl_sm_lock);
		smb_adapter_type = SMB_ADAPTER_UNDER_1A;
		mutex_unlock(&aicl_sm_lock);
		pr_smb_info("%s smb_adapter_type: %d\n", __func__, smb_adapter_type);

	}
	else {
/*		hc_mode	=1 , means still in	hc mode	and	aicl result	is good.*/
/*		then filter	the	result if not reach	1500mA,	means it is	regular	ac.*/
/*		Reconfig regular ac	DC input current limit to 1000mA forcely*/
		aicl_latest_result = smb349_get_AICL_result();
		if(aicl_latest_result < aicl_result_threshold)
		{
			pr_smb_info("%s config as regular AC,aicl_latest_result: %d\n", __func__, aicl_latest_result);

		if(aicl_latest_result < AICL_RESULT_1000MA)
		{
			mutex_lock(&aicl_sm_lock);
			smb_adapter_type = SMB_ADAPTER_UNDER_1A;
			mutex_unlock(&aicl_sm_lock);
			pr_smb_info("%s smb_adapter_type: %d\n", __func__, smb_adapter_type);

			_smb349_set_dc_input_curr_limit(aicl_latest_result);
		}
		else
		{
			mutex_lock(&aicl_sm_lock);
			smb_adapter_type = SMB_ADAPTER_1A;
			mutex_unlock(&aicl_sm_lock);
			pr_smb_info("%s smb_adapter_type: %d\n", __func__, smb_adapter_type);

			_smb349_set_dc_input_curr_limit(DC_INPUT_1000MA);
		}

			if(!is_hc_mode)	smb349_set_hc_mode(SMB349_HC_MODE);
		_smb34x_set_fastchg_curr(get_fastchg_curr_def(DEFAULT_FASTCHG_INPUT_CURR));

		}
		else
		{
			pr_smb_info("%s config as kddi AC\n", __func__);
			_smb349_set_dc_input_curr_limit(dc_input_max);

			mutex_lock(&aicl_sm_lock);
			smb_adapter_type = SMB_ADAPTER_KDDI;
			mutex_unlock(&aicl_sm_lock);
			pr_smb_info("%s smb_adapter_type: %d\n", __func__, smb_adapter_type);

			_smb34x_set_fastchg_curr(get_fastchg_curr_def(MAX_FASTCHG_INPUT_CURR));

		}

		smb349_adjust_fast_charge_curr();
		smb349_adjust_kddi_dc_input_curr();
	}

	/* re-config float voltage since the setting may be reset during AICL process */
	smb349_set_max_charging_vol();

	/*
	 * disable aicl checking, or later dc input current adjusting will not work from low current to high due to chip limitation.
	 *
	 * [Example] Only disable aicl checking for KDDI adaptor, since max input current will increase to 1.7A from 1A if screen
	 *           is off and temperature is normal.
	 */

	if (smb_adapter_type == SMB_ADAPTER_KDDI)
		smb349_set_AICL_mode(0);


	/* aicl worker is done */
	aicl_worker_ongoing = 0;

	switch (aicl_sm)
	{
		case AICL_SM_1ST_AICL_PROCESSING:
			mutex_lock(&aicl_sm_lock);
			aicl_sm =  AICL_SM_1ST_AICL_DONE;
			mutex_unlock(&aicl_sm_lock);
			pr_smb_info("%s	aicl_sm: %d\n", __func__, aicl_sm);

			/* in delay_phase2 charging , screen is off */
			if(screen_state == 0)
			{
				pr_smb_info("%s	create_delay_phase2_worker, trigger after 5 sec,  aicl_sm=%d\n", __func__, SMB_DELAY_PHASE2_PERIOD_SEC);
				/* issue a worker */
				queue_delayed_work(smb349_wq, &smb_delay_phase2_check_task, (HZ * SMB_DELAY_PHASE2_PERIOD_SEC));
			}
			break;

		case AICL_SM_2ST_AICL_PROCESSING:
			mutex_lock(&aicl_sm_lock);
			aicl_sm = AICL_SM_2ST_AICL_DONE;
			mutex_unlock(&aicl_sm_lock);
			pr_smb_info("%s	aicl_sm: %d\n", __func__, aicl_sm);
			break;

	}

	printk("%s --\n", __func__);
	mutex_unlock(&phase_lock);
	return;
}



static u32 htc_fake_charger_for_testing(enum htc_power_source_type src)
{
	/* set charger to 1A AC  by default */
	enum htc_power_source_type new_src = HTC_PWR_SOURCE_TYPE_PQM_FASTCHARGE;

	if((src > HTC_PWR_SOURCE_TYPE_9VAC) || (src == HTC_PWR_SOURCE_TYPE_BATT))
		return src;

	pr_info("%s(%d -> %d)\n", __func__, src , new_src);
	return new_src;
}


static int smb349_set_cable_type(enum htc_power_source_type input_src)
{

	pr_smb_info("%s ++\n", __func__);

	smb349_charging_src_old = smb349_charging_src_new;
	smb349_charging_src_new = input_src;


	if (get_kernel_flag() & KERNEL_FLAG_ENABLE_FAST_CHARGE)
		input_src = htc_fake_charger_for_testing(input_src);

	/* clear queue worker if any */
	cancel_delayed_work_sync(&aicl_check_work);

	aicl_worker_ongoing = 0;

	switch (input_src) {
		case HTC_PWR_SOURCE_TYPE_BATT:
		default:
			pr_smb_info("%s DISABLE_CHARGE\n", __func__);

			mutex_lock(&aicl_sm_lock);
			aicl_sm = AICL_SM_0_RESET;
			smb_adapter_type = SMB_ADAPTER_UNKNOWN;  /* clear kddi variable */
			mutex_unlock(&aicl_sm_lock);
			pr_smb_info("%s smb_adapter_type: %d\n", __func__, smb_adapter_type);
			pr_smb_info("%s	aicl_sm: %d\n", __func__, aicl_sm);

			/* reconfig */
			smb349_config();

			smb349_set_i2c_charger_ctrl_active_high();

///		smb349_set_hc_mode(SMB349_HC_MODE);
/*********************************************************************/
/*  debug 1 */
/*                pr_smb_info("%s ------------------------------------------------------------\n", __func__);*/
/*                smb349_dbg();*/
/*                pr_smb_info("%s ------------------------------------------------------------\n", __func__);*/
/*********************************************************************/


/*********************************************************************/
/* debug 2 */
/*                smb349_get_switch_freq();*/
/*                smb349_dump_reg(OTG_TLIM_THERM_CNTRL_REG);*/
/*                smb349_dump_reg(IRQ_F_REG);*/
/*********************************************************************/
		break;

	case HTC_PWR_SOURCE_TYPE_WIRELESS:
	case HTC_PWR_SOURCE_TYPE_DETECTING:
	case HTC_PWR_SOURCE_TYPE_UNKNOWN_USB:
	case HTC_PWR_SOURCE_TYPE_USB:
		mutex_lock(&aicl_sm_lock);
		aicl_sm = AICL_SM_1_USB_IN;
		smb_adapter_type = SMB_ADAPTER_USB;
		mutex_unlock(&aicl_sm_lock);
		pr_smb_info("%s	aicl_sm: %d, smb_adapter_type: %d\n", __func__, aicl_sm, smb_adapter_type);
		pr_smb_info("%s SLOW_CHARGE\n", __func__);
		break;

	case HTC_PWR_SOURCE_TYPE_AC:
	case HTC_PWR_SOURCE_TYPE_9VAC:
		pr_smb_info("%s FAST_CHARGE\n", __func__);

	if(aicl_on)
	{
		/* issue a delay worker */
		mutex_lock(&phase_lock);
		smb349_aicl_phase1();
		mutex_unlock(&phase_lock);
	}
	else
	{
/* disble AICL, hard coded the AC charging */

		smb_adapter_type = SMB_ADAPTER_1A;
		/* set hc mode */
		smb349_set_hc_mode(SMB349_HC_MODE);
		/* set fast charging current to 1000mA */
		_smb34x_set_fastchg_curr(get_fastchg_curr_def(DEFAULT_FASTCHG_INPUT_CURR));
		/* set dc input current to 1000mA */
		_smb349_set_dc_input_curr_limit(DC_INPUT_1000MA);

		/* enable charger */
///		smb349_enable_charging(1);
	}

		break;


	case HTC_PWR_SOURCE_TYPE_MHL_AC:
		pr_smb_info("%s AC_MHL_CHARGE\n", __func__);

		/* disable aicl checking */
		smb349_set_AICL_mode(0);

		/* MHL + AC case, config charger to 900mA */
		_smb349_set_dc_input_curr_limit(DC_INPUT_900MA);

		/*	 switch from USB mode to HC mode		*/
		smb349_set_hc_mode(SMB349_HC_MODE);

		/* enable charger */
///		smb349_enable_charging(1);

		break;

	case HTC_PWR_SOURCE_TYPE_PQM_FASTCHARGE:
		pr_smb_info("%s kernel flag is set, force to use KDDI adapter setting\n", __func__);

		/* disable aicl checking */
		smb349_set_AICL_mode(0);

		/* MHL + AC case, config charger to 900mA */
		_smb349_set_dc_input_curr_limit(dc_input_max);

		_smb34x_set_fastchg_curr(get_fastchg_curr_def(MAX_FASTCHG_INPUT_CURR));

		/*	 switch from USB mode to HC mode		*/
		smb349_set_hc_mode(SMB349_HC_MODE);

		break;

	}

	pr_smb_info("%s --\n", __func__);
	return 0;
}



int smb349_set_pwrsrc_and_charger_enable(enum htc_power_source_type input_src,
		bool chg_enable, bool pwrsrc_enable)
{
	int rc = 0;

	if (smb349_initial < 0)
		return smb349_initial;

	pr_smb_info("%s input_src=%d, chg_enable=%d, pwrsrc_enable=%d\n",__func__, input_src, chg_enable, pwrsrc_enable);

	/* handle pwrsrc_enable */
	smb349_enable_pwrsrc_with_reason(pwrsrc_enable, SMB_PWRSRC_DISABLED_BIT_KDRV);

	/* handle input_src */
	smb349_set_cable_type(input_src);

	/* handle chg_enable*/

	if(input_src == HTC_PWR_SOURCE_TYPE_BATT)
	{
		/* if cable remove, clear battery disable reason (EOC) */
		smb_batt_charging_disabled &= ~(SMB_BATT_CHG_DISABLED_BIT_EOC);

		/* if cable remove, clear previous hsml current setting */
		hsml_target_ma = 0;
	}

	smb349_enable_charging_with_reason(chg_enable, SMB_BATT_CHG_DISABLED_BIT_KDRV);

	if((!smb_batt_charging_disabled) && (input_src != HTC_PWR_SOURCE_TYPE_BATT))
		smb349_set_max_charging_vol();

	return rc;
}

EXPORT_SYMBOL(smb349_set_pwrsrc_and_charger_enable);


#ifdef CONFIG_HAS_EARLYSUSPEND
static void smb349_early_suspend(struct early_suspend *h)
{
	screen_state = 0;
	pr_smb_info("%s screen_state : %d\n", __func__, screen_state);

#ifdef CONFIG_HAS_EARLYSUSPEND
	mutex_lock(&phase_lock);
	smb349_aicl_phase2();
	mutex_unlock(&phase_lock);
#endif

	/* adjust kddi dc input current due to thermal concern*/
	smb349_adjust_kddi_dc_input_curr();


	/* update thermal state and check if need to modify fast-charge current */
	if(smb349_update_state())
		smb349_adjust_fast_charge_curr();
}

static void smb349_late_resume(struct early_suspend *h)
{
	screen_state = 1;
	pr_smb_info("%s screen_state: %d\n", __func__, screen_state);

	/* adjust kddi dc input current due to thermal concern*/
	smb349_adjust_kddi_dc_input_curr();

	/* update thermal state and check if need to modify fast-charge current */
	if(smb349_update_state())
		smb349_adjust_fast_charge_curr();
}
#endif		//CONFIG_HAS_EARLYSUSPEND


int smb349_set_hsml_target_ma(int target_ma)
{
	pr_info("%s target_ma: %d\n", __func__, target_ma);
	hsml_target_ma = target_ma;

	return 0;
}

EXPORT_SYMBOL(smb349_set_hsml_target_ma);

#if 0
static int get_proper_dc_input_curr_limit_via_hsml(int current_ma)
{

	int i = 0;
	int target_ma = 0;

	if(hsml_target_ma == 0) {
		return current_ma;
	}

	for(i=0; SMB349_DC_INPUT_LIMIT[i] > 0; i++) {
		if(hsml_target_ma < SMB349_DC_INPUT_LIMIT[i])
			break;
	}

	if(i == 0)
		target_ma = SMB349_DC_INPUT_LIMIT[i];
	else if(SMB349_DC_INPUT_LIMIT[i] < 0)
		target_ma = SMB349_DC_INPUT_LIMIT[i-1];
	else
		target_ma = SMB349_DC_INPUT_LIMIT[i-1];

	pr_info("%s, new target_ma=%dmA\n", __func__, target_ma);
	return get_dc_input_curr_def(target_ma);
}

#endif

/**
 * set_disable_status_param -
 *
 * Internal function to disable battery charging and also disable drawing
 * any current from the source. The device is forced to run on a battery
 * after this.
 */
static int set_disable_status_param(const char *val, struct kernel_param *kp)
{
	int ret;

	if (smb349_initial < 0)
		return smb349_initial;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}

	///pr_info("factory set disable param to %d\n", pwrsrc_disabled);

	smb349_enable_pwrsrc_with_reason(!pwrsrc_disabled, SMB_PWRSRC_DISABLED_BIT_FILENODE);

	return 0;
}
module_param_call(disabled, set_disable_status_param, param_get_uint,
					&pwrsrc_disabled, 0644);

/**
 * set_disable_status_param -
 *
 * Internal function to disable battery charging and also disable drawing
 * any current from the source. The device is forced to run on a battery
 * after this.
 */
static int set_batt_charge_off_param(const char *val, struct kernel_param *kp)
{
	int ret;

	if (smb349_initial < 0)
		return smb349_initial;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}

	///pr_info("factory set disable param to %d\n", batt_chg_disabled);

	smb349_enable_charging_with_reason(!batt_chg_disabled, SMB_BATT_CHG_DISABLED_BIT_FILENODE);

	return 0;
}
module_param_call(batt_chg_disabled, set_batt_charge_off_param, param_get_uint,
					&batt_chg_disabled, 0644);




int pm8xxx_dump_mpp(struct seq_file *m, int curr_len, char *gpio_buffer);

static int smb349_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct smb349_i2c_client   *data = &smb349_i2c_module;

	struct smb349_platform_data *pdata =
					client->dev.platform_data;

	pr_smb_info("%s\n", __func__);

	if (i2c_check_functionality(client->adapter, I2C_FUNC_I2C) == 0) {
		pr_smb_err("I2C fail\n");
		return -EIO;
	}

	smb349_wq = create_singlethread_workqueue("smb349");
	if (!smb349_wq) {
		pr_smb_err("Failed to create smb349 workqueue.");
		return -ENOMEM;
	}


#ifdef CONFIG_SUPPORT_DQ_BATTERY
	htc_is_dq_pass = pdata->dq_result;
	if (htc_is_dq_pass)
		pr_smb_info("HV battery is detected.\n");
	else
		pr_smb_info("not HV battery.\n");

#endif


	/*  For chg_stat interrupt initialization. */
	smb349_state_int = 0;
	if(pdata->chg_stat_gpio > 0)
	{
		int rc = 0;
		rc = request_any_context_irq(pdata->chg_stat_gpio,
					smb349_state_handler,
					IRQF_TRIGGER_RISING,
					"chg_stat", NULL);

		if (rc < 0)
			pr_smb_err("request chg_stat irq failed!\n");
		else {
			INIT_WORK(&smb349_state_work, smb349_state_work_func);
			smb349_state_int = pdata->chg_stat_gpio;
		}
	}

	smb_chip_rev = pdata->chip_rev;
	if(!smb_chip_rev)
	{
		pr_smb_err("does not get clear chip revision\n");
		BUG();
	}

	/* get project-specific aicl result threshold and max charging dc input current */
	aicl_result_threshold = pdata->aicl_result_threshold;
	dc_input_max = pdata->dc_input_max;
	aicl_on = pdata->aicl_on;

	/* aicl result checking threshold should not less than 1000ma */
	BUG_ON(aicl_result_threshold < AICL_RESULT_1000MA);

#if 0
	if (pdata->chg_susp_gpio > 0)
	{
		unsigned ret = 0;
		int curr_len = 0;

		pr_smb_info("%s: chg_susp_gpio: %d\n", __func__, pdata->chg_susp_gpio);

///		curr_len = pm8xxx_dump_mpp(NULL, curr_len,	NULL);

		smb349_susp.control = PM8XXX_MPP_DOUT_CTRL_HIGH;

		ret = pm8xxx_mpp_config(PM8921_MPP_PM_TO_SYS(pdata->chg_susp_gpio), &smb349_susp);
		if (ret < 0)
			pr_smb_err("%s: SUSP configuration failed\n", __func__);
	}
#endif

///	INIT_DELAYED_WORK(&set_vdpm_work, set_vdpm);

///	INIT_DELAYED_WORK(&kick_dog, kick_tps_watchdog);

	INIT_DELAYED_WORK(&aicl_check_work, aicl_check_worker);
	INIT_DELAYED_WORK(&smb_state_check_task, smb_state_check_worker);
	INIT_DELAYED_WORK(&smb_delay_phase2_check_task, delay_phase2_check_worker);

	/* issue a worker */
	queue_delayed_work(smb349_wq, &smb_state_check_task, (HZ * SMB_STATE_UPDATE_PERIOD_SEC));

	data->address = client->addr;
	data->client = client;
	mutex_init(&data->xfer_lock);
	smb349_initial = 1;

	/* in offmode charging , screen is off */
	if(board_mfg_mode() == 5)
		screen_state = 0;

	/* reconfig */
	smb349_config();

	/* default disable pwrsrc charging */
	if(board_mfg_mode() != 5)
		_smb349_set_dc_input_curr_limit(DC_INPUT_500MA);

#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.suspend = smb349_early_suspend;
	early_suspend.resume = smb349_late_resume;
	register_early_suspend(&early_suspend);
#endif


	pr_smb_info("[smb349]: Driver registration done\n");

	return 0;
}

static int smb349_remove(struct i2c_client *client)
{
	struct smb349_i2c_client   *data = i2c_get_clientdata(client);

	pr_smb_info("%s\n", __func__);

	if (data->client && data->client != client)
		i2c_unregister_device(data->client);
	smb349_i2c_module.client = NULL;
	if (smb349_wq)
		destroy_workqueue(smb349_wq);
	return 0;
}

static void smb349_shutdown(struct i2c_client *client)
{
	pr_smb_info("%s\n", __func__);

	 _smb349_enable_charging(0);
}

static const struct i2c_device_id smb349_id[] = {
	{SMB349_NAME, 0 },
	{  },
};
static struct i2c_driver smb349_driver = {
	.driver.name    = SMB349_NAME,
	.id_table   = smb349_id,
	.probe      = smb349_probe,
	.remove     = smb349_remove,
	.shutdown   = smb349_shutdown,
};

static int __init smb349_init(void)
{
	pr_smb_info("%s\n", __func__);

	mutex_init(&charger_lock);
	mutex_init(&pwrsrc_lock);
	mutex_init(&aicl_sm_lock);
	mutex_init(&phase_lock);

	return i2c_add_driver(&smb349_driver);
}


static void __exit smb349_exit(void)
{
	pr_smb_info("%s \n", __func__);
///	kfree(chg_int_data);
	return i2c_del_driver(&smb349_driver);
}

MODULE_AUTHOR("Shanfu Lin <shanfu_lin@htc.com>");
fs_initcall(smb349_init);
module_exit(smb349_exit);

MODULE_DESCRIPTION("Driver for SMB349 charger chip");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("i2c:" SMB349_NAME);

