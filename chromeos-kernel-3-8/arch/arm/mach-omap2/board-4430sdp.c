/*
 * Board support file for OMAP4430 SDP.
 *
 * Copyright (C) 2009 Texas Instruments
 *
 * Author: Santosh Shilimkar <santosh.shilimkar@ti.com>
 *
 * Based on mach-omap2/board-3430sdp.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/usb/otg.h>
#include <linux/spi/spi.h>
#include <linux/i2c/twl.h>
#include <linux/mfd/twl6040.h>
#include <linux/gpio_keys.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/leds.h>
#include <linux/leds_pwm.h>
#include <linux/platform_data/omap4-keypad.h>
#include <linux/usb/musb.h>

#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include "common.h"
#include "omap4-keypad.h"
#include <linux/wl12xx.h>
#include <linux/platform_data/omap-abe-twl6040.h>

#include "soc.h"
#include "mux.h"
#include "mmc.h"
#include "hsmmc.h"
#include "control.h"
#include "common-board-devices.h"
#include "dss-common.h"

#define ETH_KS8851_IRQ			34
#define ETH_KS8851_POWER_ON		48
#define ETH_KS8851_QUART		138
#define OMAP4_SFH7741_SENSOR_OUTPUT_GPIO	184
#define OMAP4_SFH7741_ENABLE_GPIO		188

#define GPIO_WIFI_PMENA		54
#define GPIO_WIFI_IRQ		53

static const int sdp4430_keymap[] = {
	KEY(0, 0, KEY_E),
	KEY(0, 1, KEY_R),
	KEY(0, 2, KEY_T),
	KEY(0, 3, KEY_HOME),
	KEY(0, 4, KEY_F5),
	KEY(0, 5, KEY_UNKNOWN),
	KEY(0, 6, KEY_I),
	KEY(0, 7, KEY_LEFTSHIFT),

	KEY(1, 0, KEY_D),
	KEY(1, 1, KEY_F),
	KEY(1, 2, KEY_G),
	KEY(1, 3, KEY_SEND),
	KEY(1, 4, KEY_F6),
	KEY(1, 5, KEY_UNKNOWN),
	KEY(1, 6, KEY_K),
	KEY(1, 7, KEY_ENTER),

	KEY(2, 0, KEY_X),
	KEY(2, 1, KEY_C),
	KEY(2, 2, KEY_V),
	KEY(2, 3, KEY_END),
	KEY(2, 4, KEY_F7),
	KEY(2, 5, KEY_UNKNOWN),
	KEY(2, 6, KEY_DOT),
	KEY(2, 7, KEY_CAPSLOCK),

	KEY(3, 0, KEY_Z),
	KEY(3, 1, KEY_KPPLUS),
	KEY(3, 2, KEY_B),
	KEY(3, 3, KEY_F1),
	KEY(3, 4, KEY_F8),
	KEY(3, 5, KEY_UNKNOWN),
	KEY(3, 6, KEY_O),
	KEY(3, 7, KEY_SPACE),

	KEY(4, 0, KEY_W),
	KEY(4, 1, KEY_Y),
	KEY(4, 2, KEY_U),
	KEY(4, 3, KEY_F2),
	KEY(4, 4, KEY_VOLUMEUP),
	KEY(4, 5, KEY_UNKNOWN),
	KEY(4, 6, KEY_L),
	KEY(4, 7, KEY_LEFT),

	KEY(5, 0, KEY_S),
	KEY(5, 1, KEY_H),
	KEY(5, 2, KEY_J),
	KEY(5, 3, KEY_F3),
	KEY(5, 4, KEY_F9),
	KEY(5, 5, KEY_VOLUMEDOWN),
	KEY(5, 6, KEY_M),
	KEY(5, 7, KEY_RIGHT),

	KEY(6, 0, KEY_Q),
	KEY(6, 1, KEY_A),
	KEY(6, 2, KEY_N),
	KEY(6, 3, KEY_BACK),
	KEY(6, 4, KEY_BACKSPACE),
	KEY(6, 5, KEY_UNKNOWN),
	KEY(6, 6, KEY_P),
	KEY(6, 7, KEY_UP),

	KEY(7, 0, KEY_PROG1),
	KEY(7, 1, KEY_PROG2),
	KEY(7, 2, KEY_PROG3),
	KEY(7, 3, KEY_PROG4),
	KEY(7, 4, KEY_F4),
	KEY(7, 5, KEY_UNKNOWN),
	KEY(7, 6, KEY_OK),
	KEY(7, 7, KEY_DOWN),
};
static struct omap_device_pad keypad_pads[] = {
	{	.name   = "kpd_col1.kpd_col1",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "kpd_col1.kpd_col1",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "kpd_col2.kpd_col2",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "kpd_col3.kpd_col3",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "kpd_col4.kpd_col4",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "kpd_col5.kpd_col5",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "gpmc_a23.kpd_col7",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "gpmc_a22.kpd_col6",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE1,
	},
	{	.name   = "kpd_row0.kpd_row0",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
	{	.name   = "kpd_row1.kpd_row1",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
	{	.name   = "kpd_row2.kpd_row2",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
	{	.name   = "kpd_row3.kpd_row3",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
	{	.name   = "kpd_row4.kpd_row4",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
	{	.name   = "kpd_row5.kpd_row5",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
	{	.name   = "gpmc_a18.kpd_row6",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
	{	.name   = "gpmc_a19.kpd_row7",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP | OMAP_WAKEUP_EN |
			OMAP_MUX_MODE1 | OMAP_INPUT_EN,
	},
};

static struct matrix_keymap_data sdp4430_keymap_data = {
	.keymap			= sdp4430_keymap,
	.keymap_size		= ARRAY_SIZE(sdp4430_keymap),
};

static struct omap4_keypad_platform_data sdp4430_keypad_data = {
	.keymap_data		= &sdp4430_keymap_data,
	.rows			= 8,
	.cols			= 8,
};

static struct omap_board_data keypad_data = {
	.id	    		= 1,
	.pads	 		= keypad_pads,
	.pads_cnt       	= ARRAY_SIZE(keypad_pads),
};

static struct gpio_led sdp4430_gpio_leds[] = {
	{
		.name	= "omap4:green:debug0",
		.gpio	= 61,
	},
	{
		.name	= "omap4:green:debug1",
		.gpio	= 30,
	},
	{
		.name	= "omap4:green:debug2",
		.gpio	= 7,
	},
	{
		.name	= "omap4:green:debug3",
		.gpio	= 8,
	},
	{
		.name	= "omap4:green:debug4",
		.gpio	= 50,
	},
	{
		.name	= "omap4:blue:user",
		.gpio	= 169,
	},
	{
		.name	= "omap4:red:user",
		.gpio	= 170,
	},
	{
		.name	= "omap4:green:user",
		.gpio	= 139,
	},

};

static struct gpio_keys_button sdp4430_gpio_keys[] = {
	{
		.desc			= "Proximity Sensor",
		.type			= EV_SW,
		.code			= SW_FRONT_PROXIMITY,
		.gpio			= OMAP4_SFH7741_SENSOR_OUTPUT_GPIO,
		.active_low		= 0,
	}
};

static struct gpio_led_platform_data sdp4430_led_data = {
	.leds	= sdp4430_gpio_leds,
	.num_leds	= ARRAY_SIZE(sdp4430_gpio_leds),
};

static struct led_pwm sdp4430_pwm_leds[] = {
	{
		.name		= "omap4:green:chrg",
		.pwm_id		= 1,
		.max_brightness	= 255,
		.pwm_period_ns	= 7812500,
	},
};

static struct led_pwm_platform_data sdp4430_pwm_data = {
	.num_leds	= ARRAY_SIZE(sdp4430_pwm_leds),
	.leds		= sdp4430_pwm_leds,
};

static struct platform_device sdp4430_leds_pwm = {
	.name	= "leds_pwm",
	.id	= -1,
	.dev	= {
		.platform_data = &sdp4430_pwm_data,
	},
};

static int omap_prox_activate(struct device *dev)
{
	gpio_set_value(OMAP4_SFH7741_ENABLE_GPIO , 1);
	return 0;
}

static void omap_prox_deactivate(struct device *dev)
{
	gpio_set_value(OMAP4_SFH7741_ENABLE_GPIO , 0);
}

static struct gpio_keys_platform_data sdp4430_gpio_keys_data = {
	.buttons	= sdp4430_gpio_keys,
	.nbuttons	= ARRAY_SIZE(sdp4430_gpio_keys),
	.enable		= omap_prox_activate,
	.disable	= omap_prox_deactivate,
};

static struct platform_device sdp4430_gpio_keys_device = {
	.name	= "gpio-keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &sdp4430_gpio_keys_data,
	},
};

static struct platform_device sdp4430_leds_gpio = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data = &sdp4430_led_data,
	},
};
static struct spi_board_info sdp4430_spi_board_info[] __initdata = {
	{
		.modalias               = "ks8851",
		.bus_num                = 1,
		.chip_select            = 0,
		.max_speed_hz           = 24000000,
		/*
		 * .irq is set to gpio_to_irq(ETH_KS8851_IRQ)
		 * in omap_4430sdp_init
		 */
	},
};

static struct gpio sdp4430_eth_gpios[] __initdata = {
	{ ETH_KS8851_POWER_ON,	GPIOF_OUT_INIT_HIGH,	"eth_power"	},
	{ ETH_KS8851_QUART,	GPIOF_OUT_INIT_HIGH,	"quart"		},
	{ ETH_KS8851_IRQ,	GPIOF_IN,		"eth_irq"	},
};

static int __init omap_ethernet_init(void)
{
	int status;

	/* Request of GPIO lines */
	status = gpio_request_array(sdp4430_eth_gpios,
				    ARRAY_SIZE(sdp4430_eth_gpios));
	if (status)
		pr_err("Cannot request ETH GPIOs\n");

	return status;
}

static struct regulator_consumer_supply sdp4430_vbat_supply[] = {
	REGULATOR_SUPPLY("vddvibl", "twl6040-vibra"),
	REGULATOR_SUPPLY("vddvibr", "twl6040-vibra"),
};

static struct regulator_init_data sdp4430_vbat_data = {
	.constraints = {
		.always_on	= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(sdp4430_vbat_supply),
	.consumer_supplies	= sdp4430_vbat_supply,
};

static struct fixed_voltage_config sdp4430_vbat_pdata = {
	.supply_name	= "VBAT",
	.microvolts	= 3750000,
	.init_data	= &sdp4430_vbat_data,
	.gpio		= -EINVAL,
};

static struct platform_device sdp4430_vbat = {
	.name		= "reg-fixed-voltage",
	.id		= -1,
	.dev = {
		.platform_data = &sdp4430_vbat_pdata,
	},
};

static struct platform_device sdp4430_dmic_codec = {
	.name	= "dmic-codec",
	.id	= -1,
};

static struct platform_device sdp4430_hdmi_audio_codec = {
	.name	= "hdmi-audio-codec",
	.id	= -1,
};

static struct omap_abe_twl6040_data sdp4430_abe_audio_data = {
	.card_name = "SDP4430",
	.has_hs		= ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,
	.has_hf		= ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,
	.has_ep		= 1,
	.has_aux	= ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,
	.has_vibra	= ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,

	.has_dmic	= 1,
	.has_hsmic	= 1,
	.has_mainmic	= 1,
	.has_submic	= 1,
	.has_afm	= ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,

	.jack_detection = 1,
	/* MCLK input is 38.4MHz */
	.mclk_freq	= 38400000,
};

static struct platform_device sdp4430_abe_audio = {
	.name		= "omap-abe-twl6040",
	.id		= -1,
	.dev = {
		.platform_data = &sdp4430_abe_audio_data,
	},
};

static struct platform_device *sdp4430_devices[] __initdata = {
	&sdp4430_gpio_keys_device,
	&sdp4430_leds_gpio,
	&sdp4430_leds_pwm,
	&sdp4430_vbat,
	&sdp4430_dmic_codec,
	&sdp4430_abe_audio,
	&sdp4430_hdmi_audio_codec,
};

static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_UTMI,
	.mode			= MUSB_OTG,
	.power			= 100,
};

static struct omap2_hsmmc_info mmc[] = {
	{
		.mmc		= 2,
		.caps		=  MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.nonremovable   = true,
		.ocr_mask	= MMC_VDD_29_30,
		.no_off_init	= true,
	},
	{
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{
		.mmc		= 5,
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_POWER_OFF_CARD,
		.pm_caps	= MMC_PM_KEEP_POWER,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.ocr_mask	= MMC_VDD_165_195,
		.nonremovable	= true,
	},
	{}	/* Terminator */
};

static struct regulator_consumer_supply sdp4430_vaux_supply[] = {
	REGULATOR_SUPPLY("vmmc", "omap_hsmmc.1"),
};

static struct regulator_consumer_supply omap4_sdp4430_vmmc5_supply = {
	.supply = "vmmc",
	.dev_name = "omap_hsmmc.4",
};

static struct regulator_init_data sdp4430_vmmc5 = {
	.constraints = {
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &omap4_sdp4430_vmmc5_supply,
};

static struct fixed_voltage_config sdp4430_vwlan = {
	.supply_name		= "vwl1271",
	.microvolts		= 1800000, /* 1.8V */
	.gpio			= GPIO_WIFI_PMENA,
	.startup_delay		= 70000, /* 70msec */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &sdp4430_vmmc5,
};

static struct platform_device omap_vwlan_device = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev = {
		.platform_data = &sdp4430_vwlan,
	},
};

static struct regulator_init_data sdp4430_vaux1 = {
	.constraints = {
		.min_uV			= 1000000,
		.max_uV			= 3000000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = ARRAY_SIZE(sdp4430_vaux_supply),
	.consumer_supplies      = sdp4430_vaux_supply,
};

static struct regulator_init_data sdp4430_vusim = {
	.constraints = {
		.min_uV			= 1200000,
		.max_uV			= 2900000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct twl6040_codec_data twl6040_codec = {
	/* single-step ramp for headset and handsfree */
	.hs_left_step	= 0x0f,
	.hs_right_step	= 0x0f,
	.hf_left_step	= 0x1d,
	.hf_right_step	= 0x1d,
};

static struct twl6040_vibra_data twl6040_vibra = {
	.vibldrv_res = 8,
	.vibrdrv_res = 3,
	.viblmotor_res = 10,
	.vibrmotor_res = 10,
	.vddvibl_uV = 0,	/* fixed volt supply - VBAT */
	.vddvibr_uV = 0,	/* fixed volt supply - VBAT */
};

static struct twl6040_platform_data twl6040_data = {
	.codec		= &twl6040_codec,
	.vibra		= &twl6040_vibra,
	.audpwron_gpio	= 127,
};

static struct i2c_board_info __initdata sdp4430_i2c_1_boardinfo[] = {
	{
		I2C_BOARD_INFO("twl6040", 0x4b),
		.irq = 119 + OMAP44XX_IRQ_GIC_START,
		.platform_data = &twl6040_data,
	},
};

static struct twl4030_platform_data sdp4430_twldata = {
	/* Regulators */
	.vusim		= &sdp4430_vusim,
	.vaux1		= &sdp4430_vaux1,
};

static struct i2c_board_info __initdata sdp4430_i2c_3_boardinfo[] = {
	{
		I2C_BOARD_INFO("tmp105", 0x48),
	},
	{
		I2C_BOARD_INFO("bh1780", 0x29),
	},
};
static struct i2c_board_info __initdata sdp4430_i2c_4_boardinfo[] = {
	{
		I2C_BOARD_INFO("hmc5843", 0x1e),
	},
};
static int __init omap4_i2c_init(void)
{
	omap4_pmic_get_config(&sdp4430_twldata, TWL_COMMON_PDATA_USB,
			TWL_COMMON_REGULATOR_VDAC |
			TWL_COMMON_REGULATOR_VAUX2 |
			TWL_COMMON_REGULATOR_VAUX3 |
			TWL_COMMON_REGULATOR_VMMC |
			TWL_COMMON_REGULATOR_VPP |
			TWL_COMMON_REGULATOR_VANA |
			TWL_COMMON_REGULATOR_VCXIO |
			TWL_COMMON_REGULATOR_VUSB |
			TWL_COMMON_REGULATOR_CLK32KG |
			TWL_COMMON_REGULATOR_V1V8 |
			TWL_COMMON_REGULATOR_V2V1);
	omap4_pmic_init("twl6030", &sdp4430_twldata, sdp4430_i2c_1_boardinfo,
			ARRAY_SIZE(sdp4430_i2c_1_boardinfo));
	omap_register_i2c_bus(2, 400, NULL, 0);
	omap_register_i2c_bus(3, 400, sdp4430_i2c_3_boardinfo,
				ARRAY_SIZE(sdp4430_i2c_3_boardinfo));
	omap_register_i2c_bus(4, 400, sdp4430_i2c_4_boardinfo,
				ARRAY_SIZE(sdp4430_i2c_4_boardinfo));
	return 0;
}

static void __init omap_sfh7741prox_init(void)
{
	int error;

	error = gpio_request_one(OMAP4_SFH7741_ENABLE_GPIO,
				 GPIOF_OUT_INIT_LOW, "sfh7741");
	if (error < 0)
		pr_err("%s:failed to request GPIO %d, error %d\n",
			__func__, OMAP4_SFH7741_ENABLE_GPIO, error);
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	OMAP4_MUX(USBB2_ULPITLL_CLK, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* NIRQ2 for twl6040 */
	OMAP4_MUX(SYS_NIRQ2, OMAP_MUX_MODE0 |
		  OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_WAKEUPENABLE),
	/* GPIO_127 for twl6040 */
	OMAP4_MUX(HDQ_SIO, OMAP_MUX_MODE3 | OMAP_PIN_OUTPUT),
	/* McPDM */
	OMAP4_MUX(ABE_PDM_UL_DATA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP4_MUX(ABE_PDM_DL_DATA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP4_MUX(ABE_PDM_FRAME, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP4_MUX(ABE_PDM_LB_CLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP4_MUX(ABE_CLKS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	/* DMIC */
	OMAP4_MUX(ABE_DMIC_CLK1, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP4_MUX(ABE_DMIC_DIN1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP4_MUX(ABE_DMIC_DIN2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP4_MUX(ABE_DMIC_DIN3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	/* McBSP1 */
	OMAP4_MUX(ABE_MCBSP1_CLKX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP4_MUX(ABE_MCBSP1_DR, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP4_MUX(ABE_MCBSP1_DX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT |
		  OMAP_PULL_ENA),
	OMAP4_MUX(ABE_MCBSP1_FSX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	/* McBSP2 */
	OMAP4_MUX(ABE_MCBSP2_CLKX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP4_MUX(ABE_MCBSP2_DR, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP4_MUX(ABE_MCBSP2_DX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT |
		  OMAP_PULL_ENA),
	OMAP4_MUX(ABE_MCBSP2_FSX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),

	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

#else
#define board_mux	NULL
 #endif

static void __init omap4_sdp4430_wifi_mux_init(void)
{
	omap_mux_init_gpio(GPIO_WIFI_IRQ, OMAP_PIN_INPUT |
				OMAP_PIN_OFF_WAKEUPENABLE);
	omap_mux_init_gpio(GPIO_WIFI_PMENA, OMAP_PIN_OUTPUT);

	omap_mux_init_signal("sdmmc5_cmd.sdmmc5_cmd",
				OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("sdmmc5_clk.sdmmc5_clk",
				OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("sdmmc5_dat0.sdmmc5_dat0",
				OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("sdmmc5_dat1.sdmmc5_dat1",
				OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("sdmmc5_dat2.sdmmc5_dat2",
				OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("sdmmc5_dat3.sdmmc5_dat3",
				OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);

}

static struct wl12xx_platform_data omap4_sdp4430_wlan_data __initdata = {
	.board_ref_clock = WL12XX_REFCLOCK_26,
	.board_tcxo_clock = WL12XX_TCXOCLOCK_26,
};

static void __init omap4_sdp4430_wifi_init(void)
{
	int ret;

	omap4_sdp4430_wifi_mux_init();
	omap4_sdp4430_wlan_data.irq = gpio_to_irq(GPIO_WIFI_IRQ);
	ret = wl12xx_set_platform_data(&omap4_sdp4430_wlan_data);
	if (ret)
		pr_err("Error setting wl12xx data: %d\n", ret);
	ret = platform_device_register(&omap_vwlan_device);
	if (ret)
		pr_err("Error registering wl12xx device: %d\n", ret);
}

static void __init omap_4430sdp_init(void)
{
	int status;
	int package = OMAP_PACKAGE_CBS;

	if (omap_rev() == OMAP4430_REV_ES1_0)
		package = OMAP_PACKAGE_CBL;
	omap4_mux_init(board_mux, NULL, package);

	omap4_i2c_init();
	omap_sfh7741prox_init();
	platform_add_devices(sdp4430_devices, ARRAY_SIZE(sdp4430_devices));
	omap_serial_init();
	omap_sdrc_init(NULL, NULL);
	omap4_sdp4430_wifi_init();
	omap4_twl6030_hsmmc_init(mmc);

	usb_musb_init(&musb_board_data);

	status = omap_ethernet_init();
	if (status) {
		pr_err("Ethernet initialization failed: %d\n", status);
	} else {
		sdp4430_spi_board_info[0].irq = gpio_to_irq(ETH_KS8851_IRQ);
		spi_register_board_info(sdp4430_spi_board_info,
				ARRAY_SIZE(sdp4430_spi_board_info));
	}

	status = omap4_keyboard_init(&sdp4430_keypad_data, &keypad_data);
	if (status)
		pr_err("Keypad initialization failed: %d\n", status);

	omap_4430sdp_display_init();
}

MACHINE_START(OMAP_4430SDP, "OMAP4430 4430SDP board")
	/* Maintainer: Santosh Shilimkar - Texas Instruments Inc */
	.atag_offset	= 0x100,
	.smp		= smp_ops(omap4_smp_ops),
	.reserve	= omap_reserve,
	.map_io		= omap4_map_io,
	.init_early	= omap4430_init_early,
	.init_irq	= gic_init_irq,
	.handle_irq	= gic_handle_irq,
	.init_machine	= omap_4430sdp_init,
	.init_late	= omap4430_init_late,
	.init_time	= omap4_local_timer_init,
	.restart	= omap44xx_restart,
MACHINE_END