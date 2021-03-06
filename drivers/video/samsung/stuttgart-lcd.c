/* linux/drivers/video/samsung/otm1280a_dummymipilcd.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modified by Samsung Electronics (UK) on May 2010
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>

#include <mach/dsim.h>
#include <mach/mipi_ddi.h>

#include "s5p-dsim.h"
#include "s3cfb.h"

//#define MIPI_TX_CHECK 1
#define MIPI_DBG 1

#define CONFIG_BACKLIGHT_STUTTGART
#define MAX_BRIGHTNESS		(255)

#define LCD_CE_INTERFACE

#ifdef LCD_CABC_MODE
#define CABC_ON			1
#define CABC_OFF		0
#endif

#ifdef ESD_FOR_LCD
#define DELAY_TIME     200
#define TE_IRQ IRQ_EINT(28)
#define TE_TIME        500
#endif

static struct mipi_ddi_platform_data *ddi_pd;
static int current_brightness = 100;
#ifdef LCD_CABC_MODE
static int cabc_mode = 0;
#endif
#ifdef LCD_CE_INTERFACE
static int flag = 0;
static int ce_mode = 1;
#endif
//struct pwm_device       *pwm;
#ifdef CONFIG_BACKLIGHT_STUTTGART
static struct backlight_properties props;
#endif
#ifdef ESD_FOR_LCD
struct delayed_work lcd_delayed_work;
struct delayed_work lcd_init_work;
struct timer_list te_timer;
static int esd_test = 0;
#endif

void otm1280a_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void otm1280a_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, dcs_cmd, param1);
}
#if 0
static void otm1280a_write_3(unsigned char dcs_cmd, unsigned char param1, unsigned char param2, unsigned char param3)
{
	unsigned char buf[4];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;
	buf[3] = param3;

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 4);
}

static void otm1280a_write(void)
{
	unsigned char buf[15] = {0xf8, 0x01, 0x27, 0x27, 0x07, 0x07, 0x54,
		0x9f, 0x63, 0x86, 0x1a,
		0x33, 0x0d, 0x00, 0x00};
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
}
#endif

static void otm1280a_display_off(struct device *dev)
{
	int ret;
	printk("##%s#####\n", __func__);

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, 0);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x53, 0x00);
	if (lcd_id == LCD_BOE) {
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x28, 0x00);
		printk("%s BOE\n", __func__);
	} else if (lcd_id == LCD_LG) 
	{
		printk("%s lG\n", __func__);

		otm1280a_write_0(0x10);
		otm1280a_write_0(0x28);
		mdelay(20);
		otm1280a_write_1(0xc2, 0x00);
		do{
			unsigned char buf[] = {0xc4, 0x00,0x00, 0x00, 0x00,0x00};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return;
			}
		}while(0);
		otm1280a_write_1(0xc1, 0x02);
		mdelay(20);
		otm1280a_write_1(0xc1, 0x03);
		//mdelay(20);
		mdelay(50);
	}else		//yassy or cmi who use nova chip
	{
		otm1280a_write_0(0x28);
		otm1280a_write_0(0x10);
		mdelay(150);
		printk("%s NOVA\n", __func__);
	}
}

void otm1280a_sleep_in(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x10, 0);
}

void otm1280a_sleep_out(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x11, 0);
}

static void otm1280a_display_on(struct device *dev)
{
	//printk("##%s#####\n", __func__);

	//if (lcd_id == LCD_BOE) {
	//	mdelay(1);
	//}else 
	if (lcd_id == LCD_LG) 
	{
		int ret;
		otm1280a_write_1(0xc2, 0x02);
		//mdelay(15);
		otm1280a_write_1(0xc2, 0x06);
		//mdelay(15);
		otm1280a_write_1(0xc2, 0x4E);
		//mdelay(90);
		otm1280a_write_0(0x11);
		//mdelay(20);
		do{
			unsigned char buf[] = {0xF9,0x80,0x00};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return;
			}
		}while(0);
		//mdelay(15);
		otm1280a_write_0(0x29);
		//mdelay(10);
	}//else		//yassy or cmi who use nova chip
	//{
		//otm1280a_write_0(0x11);
		//mdelay(200);
		//otm1280a_write_0(0x29);
		//mdelay(10);
		//mdelay(1);
	//}
}

#define LCD_RESET EXYNOS4_GPF0(0)
#define LCD_FRM EXYNOS4_GPD0(0)
void lcd_reset(void)
{
	if (lcd_id == LCD_BOE) {
		printk("lcd reset for BOE\n");
		s3c_gpio_setpull(LCD_RESET, S3C_GPIO_PULL_NONE);
		gpio_direction_output(LCD_RESET, 1);
		mdelay(1);   
		gpio_direction_output(LCD_RESET, 0);           
		mdelay(10);               
		gpio_direction_output(LCD_RESET, 1);
		mdelay(20);
	} else if (lcd_id == LCD_LG) {
		printk("lcd reset for LG\n");
		s3c_gpio_setpull(LCD_RESET, S3C_GPIO_PULL_NONE);
		gpio_direction_output(LCD_RESET, 1);
		mdelay(10);   
		gpio_direction_output(LCD_RESET, 0);           
		mdelay(10);               
		gpio_direction_output(LCD_RESET, 1);
		mdelay(10);
	}else if (lcd_id == LCD_NOVA_CMI) {
		printk("lcd reset for NOVA_CMI\n");
		s3c_gpio_setpull(LCD_RESET, S3C_GPIO_PULL_NONE);
		gpio_direction_output(LCD_RESET, 1);
		mdelay(10);   
		gpio_direction_output(LCD_RESET, 0);           
		mdelay(1);               
		gpio_direction_output(LCD_RESET, 1);
		mdelay(20);
	} else  if (lcd_id == LCD_NOVA_YASSY) {
		printk("lcd reset for NOVA_YASSY\n");
		s3c_gpio_setpull(LCD_RESET, S3C_GPIO_PULL_NONE);
		gpio_direction_output(LCD_RESET, 1);
		mdelay(10);   
		gpio_direction_output(LCD_RESET, 0);           
		mdelay(1);               
		gpio_direction_output(LCD_RESET, 1);
		mdelay(20);

	}
	
}

#ifdef CONFIG_BACKLIGHT_STUTTGART
static int s5p_bl_update_status(struct backlight_device *bd)
{
	int bl = bd->props.brightness;
	int ret;


	current_brightness = bl * MAX_BRIGHTNESS / 255;
	printk("%s: current_brightness=%d, bl=%d",__func__, current_brightness, bl);
	if (current_brightness < 10 && current_brightness > 0)current_brightness = 10;
	if (ddi_pd->resume_complete != 1) {
		return 0;
	}

	//mdelay(10);
	do{
		unsigned char buf[2] = {0x00,0x00};
		buf[0] = 0x51;
		buf[1] = bl;

		ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
		if (ret == 0){
			printk("cmmd_write error\n");
			return ret;
		}
	}while(0);
	/*
	   mdelay(1);
	   do{
	   unsigned char buf[2] = {0x53,0x24};

	   ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
	   if (ret == 0){
	   printk("cmmd_write error\n");
	   return ret;
	   }
	   }while(0);
	   */
	//printk("%s current_brightness = %d\n", __func__, current_brightness);

	return 0;
}
#if 0
static int s5p_bl_get_brightness(struct backlilght_device *bd)
{
	return 0;
}
#endif

static const struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
#if 0
	.get_brightness = s5p_bl_get_brightness,
#endif
};
#endif

static int lcd_init_cmd_write(unsigned int dsim_base, unsigned int data_id, unsigned char * buf, unsigned int size)
{
	int ret = DSIM_FALSE;
	u32 read_buf[100];

	memset(read_buf, 0, sizeof(read_buf));
	ret = ddi_pd->cmd_write(ddi_pd->dsim_base, data_id,  (unsigned int )buf, size);
	if (ret == 0){
		printk("cmmd_write error\n");
		return DSIM_FALSE;
	}
#ifdef  MIPI_TX_CHECK
	int read_size;
	ddi_pd->cmd_write(ddi_pd->dsim_base, SET_MAX_RTN_PKT_SIZE, 100, 0);
	s5p_dsim_clr_state(ddi_pd->dsim_base,RxDatDone);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_RD_NO_PARA, (u8)buf[0], 0);
	s5p_dsim_wait_state(ddi_pd->dsim_base, RxDatDone);
	s5p_dsim_read_rx_fifo(ddi_pd->dsim_base, read_buf, &read_size);
	int i = 0;
#ifdef MIPI_DBG
	u8 *p = (u8*)read_buf;
	for (i = 0; i < read_size; i++)
		printk("%#x ", p[i]);
	printk("\n");
#endif
	if (memcmp((u8 *)&buf[1], (u8*)read_buf, size -1) == 0)
		return DSIM_TRUE;
	else
		return DSIM_FALSE;
#endif	
	return DSIM_TRUE;

}
#ifdef LCD_CABC_MODE
int otm1280a_cabc_control(int mode)
{
	int ret;

	if (lcd_id == LCD_BOE) {
		printk(KERN_ERR "%s Invalid argument, the lcd's vender is BOE, not support cabc function\n", __func__);
		return -EINVAL;
	}
	//cabc control
	if (mode < 0 || mode > 3) {
		printk(KERN_ERR "%s Invalid argument, 0: off, 1: ui mode, 2:still mode 3: move mode\n", __func__);
		return -EINVAL;
	}
	if (mode) {
		printk("%s %s mode = %d\n", __func__, mode ? "on" : "off", mode);
		otm1280a_write_1(0x51, current_brightness);
		otm1280a_write_1(0x53, 0x2c);
		otm1280a_write_1(0x55, mode);
		otm1280a_write_1(0x5e, 0x00);
		do{
			unsigned char buf[] = {0xc8,0x82,0x86,0x01,0x11};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
	} else {
		printk("%s cabc off \n", __func__);
		otm1280a_write_1(0x51, current_brightness);
		otm1280a_write_1(0x55, 0x00);
	}
	return 0;
}

static int otm1280a_cabc_mode(const char *val, struct kernel_param *kp)
{
	int value;
	int ret = param_set_int(val, kp);

	if(ret < 0)
	{   
		printk(KERN_ERR"%s Invalid argument\n", __func__);
		return -EINVAL;
	}   
	value = *((int*)kp->arg);
	return otm1280a_cabc_control(value);
}
#endif

typedef struct 
{
	unsigned char add;
	unsigned char data;
}para_1_data;
para_1_data yt_code[]={
	//{0xFF,0x00},
	//{0xBA,0x03},//0x02//4 lane
	//{0xC2,0x03},//video mode
	{0xFF,0x01},
	{0x00,0x3A},
	{0x01,0x33},
	{0x02,0x53},
	{0x09,0x85},
	{0x0E,0x25},
	{0x0F,0x0A},
	{0x0B,0x97},
	{0x0C,0x97},
	{0x11,0x8A},
	{0x36,0x7B},
	{0x71,0x2C},
	{0xFF,0x05},
	
	//ltps timing
	{0x01,0x00},
	{0x02,0x8D},
	{0x03,0x8D},
	{0x04,0x8D},
	{0x05,0x30},
	{0x06,0x33},
	{0x07,0x77},
	{0x08,0x00},
	{0x09,0x00},
	{0x0A,0x00},
	{0x0B,0x80},
	{0x0C,0xC8},
	{0x0D,0x00},
	{0x0E,0x1B},
	{0x0F,0x07},
	{0x10,0x57},
	{0x11,0x00},
	{0x12,0x00},
	{0x13,0x1E},
	{0x14,0x00},
	{0x15,0x1A},
	{0x16,0x05},
	{0x17,0x00},
	{0x18,0x1E},
	{0x19,0xFF},
	{0x1A,0x00},
	{0x1B,0xFC},   
	//GRESET_inverse
	{0x1C,0x80},
	{0x1D,0x00},
	{0x1E,0x10},
	{0x1F,0x77},
	{0x20,0x00},
	
	//Gate inverse
	{0x21,0x06},	//invers display, default is 0x00
	{0x22,0x55},
	{0x23,0x0D},
	{0x31,0xA0},
	{0x32,0x00},
	{0x33,0xB8},
	{0x34,0xBB},
	{0x35,0x11},
	{0x36,0x01},
	{0x37,0x0B},
	{0x38,0x01},
	{0x39,0x0B},
	{0x44,0x08},
	{0x45,0x80},
	{0x46,0xCC},
	{0x47,0x04},
	{0x48,0x00},
	{0x49,0x00},
	{0x4A,0x01},
	{0x6C,0x03},
	{0x6D,0x03},
	{0x6E,0x2F},
	{0x43,0x00},
	{0x4B,0x23},
	{0x4C,0x01},
	{0x50,0x23},
	{0x51,0x01},
	{0x58,0x23},
	{0x59,0x01},
	{0x5D,0x23},
	{0x5E,0x01},
	{0x62,0x23},
	{0x63,0x01},
	{0x67,0x23},
	{0x68,0x01},
	{0x89,0x00},
	{0x8D,0x01},
	{0x8E,0x64},
	{0x8F,0x20},
	{0x97,0x8E},
	{0x82,0x8C},
	{0x83,0x02},
	{0xBB,0x0A},
	{0xBC,0x0A},
	{0x24,0x25},
	{0x25,0x55},
	{0x26,0x05},
	{0x27,0x23},
	{0x28,0x01},
	{0x29,0x31},
	{0x2A,0x5D},
	{0x2B,0x01},
	{0x2F,0x00},
	{0x30,0x10},
	{0xA7,0x12},
	{0x2D,0x03},
	//CMD2,Page0_Gamma2.0
	{0xFF,0x01},
	//R+
{0x75,0x00},   
{0x76,0x01},   
{0x77,0x00},   
{0x78,0x20},   
{0x79,0x00},   
{0x7A,0x4B},   
{0x7B,0x00},   
{0x7C,0x6A},   
{0x7D,0x00},   
{0x7E,0x82},   
{0x7F,0x00},   
{0x80,0x96},   
{0x81,0x00},   
{0x82,0xA8},   
{0x83,0x00},   
{0x84,0xB8},   
{0x85,0x00},   
{0x86,0xC7},   
{0x87,0x00},   
{0x88,0xF7},   
{0x89,0x01},   
{0x8A,0x1E},   
{0x8B,0x01},   
{0x8C,0x59},   
{0x8D,0x01},   
{0x8E,0x89},   
{0x8F,0x01},   
{0x90,0xD4},   
{0x91,0x02},   
{0x92,0x11},   
{0x93,0x02},   
{0x94,0x12},   
{0x95,0x02},   
{0x96,0x4A},   
{0x97,0x02},   
{0x98,0x88},   
{0x99,0x02},   
{0x9A,0xAF},   
{0x9B,0x02},   
{0x9C,0xE5},   
{0x9D,0x03},   
{0x9E,0x0A},   
{0x9F,0x03},   
{0xA0,0x3C},   
{0xA2,0x03},   
{0xA3,0x4D},   
{0xA4,0x03},   
{0xA5,0x83},   
{0xA6,0x03},   
{0xA7,0xAF},   
{0xA9,0x03},   
{0xAA,0xD1},   
{0xAB,0x03},   
{0xAC,0xE9},   
{0xAD,0x03},   
{0xAE,0xF7},   
{0xAF,0x03},   
{0xB0,0xFE},   
{0xB1,0x03},   
{0xB2,0xFF},   
{0xB3,0x00},   
{0xB4,0x01},   
{0xB5,0x00},   
{0xB6,0x20},   
{0xB7,0x00},   
{0xB8,0x4B},   
{0xB9,0x00},   
{0xBA,0x6A},   
{0xBB,0x00},   
{0xBC,0x82},   
{0xBD,0x00},   
{0xBE,0x96},   
{0xBF,0x00},   
{0xC0,0xA8},   
{0xC1,0x00},   
{0xC2,0xB8},   
{0xC3,0x00},   
{0xC4,0xC7},   
{0xC5,0x00},   
{0xC6,0xF7},   
{0xC7,0x01},   
{0xC8,0x1E},   
{0xC9,0x01},   
{0xCA,0x59},   
{0xCB,0x01},   
{0xCC,0x89},   
{0xCD,0x01},   
{0xCE,0xD4},   
{0xCF,0x02},   
{0xD0,0x11},   
{0xD1,0x02},   
{0xD2,0x12},   
{0xD3,0x02},   
{0xD4,0x4A},   
{0xD5,0x02},   
{0xD6,0x88},   
{0xD7,0x02},   
{0xD8,0xAF},   
{0xD9,0x02},   
{0xDA,0xE5},   
{0xDB,0x03},   
{0xDC,0x0A},   
{0xDD,0x03},   
{0xDE,0x3C},   
{0xDF,0x03},   
{0xE0,0x4D},   
{0xE1,0x03},   
{0xE2,0x83},   
{0xE3,0x03},   
{0xE4,0xAF},   
{0xE5,0x03},   
{0xE6,0xD1},   
{0xE7,0x03},   
{0xE8,0xE9},   
{0xE9,0x03},   
{0xEA,0xF7},   
{0xEB,0x03},   
{0xEC,0xFE},   
{0xED,0x03},   
{0xEE,0xFF},   
{0xEF,0x00},   
{0xF0,0xDB},   
{0xF1,0x00},   
{0xF2,0xE1},   
{0xF3,0x00},   
{0xF4,0xEE},   
{0xF5,0x00},   
{0xF6,0xFB},   
{0xF7,0x01},   
{0xF8,0x06},   
{0xF9,0x01},   
{0xFA,0x11},   
{0xFF,0x02},   
{0x00,0x01},   
{0x01,0x1B},   
{0x02,0x01},   
{0x03,0x24},   
{0x04,0x01},   
{0x05,0x2B},   
{0x06,0x01},   
{0x07,0x46},   
{0x08,0x01},   
{0x09,0x5E},   
{0x0A,0x01},   
{0x0B,0x89},   
{0x0C,0x01},   
{0x0D,0xAD},   
{0x0E,0x01},   
{0x0F,0xEA},   
{0x10,0x02},   
{0x11,0x1E},   
{0x12,0x02},   
{0x13,0x1F},   
{0x14,0x02},   
{0x15,0x52},   
{0x16,0x02},   
{0x17,0x8B},   
{0x18,0x02},   
{0x19,0xAF},   
{0x1A,0x02},   
{0x1B,0xDE},   
{0x1C,0x02},   
{0x1D,0xFD},   
{0x1E,0x03},   
{0x1F,0x23},   
{0x20,0x03},   
{0x21,0x2E},   
{0x22,0x03},   
{0x23,0x3A},   
{0x24,0x03},   
{0x25,0x3B},   
{0x26,0x03},   
{0x27,0x3C},   
{0x28,0x03},   
{0x29,0x3D},   
{0x2A,0x03},   
{0x2B,0x94},   
{0x2D,0x03},   
{0x2F,0x97},   
{0x30,0x03},   
{0x31,0xFF},   
{0x32,0x00},   
{0x33,0xDB},   
{0x34,0x00},   
{0x35,0xE1},   
{0x36,0x00},   
{0x37,0xEE},   
{0x38,0x00},   
{0x39,0xFB},   
{0x3A,0x01},   
{0x3B,0x06},   
{0x3D,0x01},   
{0x3F,0x11},   
{0x40,0x01},   
{0x41,0x1B},   
{0x42,0x01},   
{0x43,0x24},   
{0x44,0x01},   
{0x45,0x2B},   
{0x46,0x01},   
{0x47,0x46},   
{0x48,0x01},   
{0x49,0x5E},   
{0x4A,0x01},   
{0x4B,0x89},   
{0x4C,0x01},   
{0x4D,0xAD},   
{0x4E,0x01},   
{0x4F,0xEA},   
{0x50,0x02},   
{0x51,0x1E},   
{0x52,0x02},   
{0x53,0x1F},   
{0x54,0x02},   
{0x55,0x52},   
{0x56,0x02},   
{0x58,0x8B},   
{0x59,0x02},   
{0x5A,0xAF},   
{0x5B,0x02},   
{0x5C,0xDE},   
{0x5D,0x02},   
{0x5E,0xFD},   
{0x5F,0x03},   
{0x60,0x23},   
{0x61,0x03},   
{0x62,0x2E},   
{0x63,0x03},   
{0x64,0x3A},   
{0x65,0x03},   
{0x66,0x3B},   
{0x67,0x03},   
{0x68,0x3C},   
{0x69,0x03},   
{0x6A,0x3D},   
{0x6B,0x03},   
{0x6C,0x94},   
{0x6D,0x03},   
{0x6E,0x97},   
{0x6F,0x03},   
{0x70,0xFF},   
{0x71,0x01},   
{0x72,0x11},   
{0x73,0x01},   
{0x74,0x14},   
{0x75,0x01},   
{0x76,0x1B},   
{0x77,0x01},   
{0x78,0x23},   
{0x79,0x01},   
{0x7A,0x29},   
{0x7B,0x01},   
{0x7C,0x30},   
{0x7D,0x01},   
{0x7E,0x37},   
{0x7F,0x01},   
{0x80,0x3D},   
{0x81,0x01},   
{0x82,0x43},   
{0x83,0x01},   
{0x84,0x5A},   
{0x85,0x01},   
{0x86,0x6F},   
{0x87,0x01},   
{0x88,0x94},   
{0x89,0x01},   
{0x8A,0xB5},   
{0x8B,0x01},   
{0x8C,0xEE},   
{0x8D,0x02},   
{0x8E,0x20},   
{0x8F,0x02},   
{0x90,0x21},   
{0x91,0x02},   
{0x92,0x53},   
{0x93,0x02},   
{0x94,0x8D},   
{0x95,0x02},   
{0x96,0xB1},   
{0x97,0x02},   
{0x98,0xE1},   
{0x99,0x03},   
{0x9A,0x01},   
{0x9B,0x03},   
{0x9C,0x2A},   
{0x9D,0x03},   
{0x9E,0x36},   
{0x9F,0x03},   
{0xA0,0x42},   
{0xA2,0x03},   
{0xA3,0x54},   
{0xA4,0x03},   
{0xA5,0x62},   
{0xA6,0x03},   
{0xA7,0x76},   
{0xA9,0x03},   
{0xAA,0x94},   
{0xAB,0x03},   
{0xAC,0x97},   
{0xAD,0x03},   
{0xAE,0xFF},   
{0xAF,0x01},   
{0xB0,0x11},   
{0xB1,0x01},   
{0xB2,0x14},   
{0xB3,0x01},   
{0xB4,0x1B},   
{0xB5,0x01},   
{0xB6,0x23},   
{0xB7,0x01},   
{0xB8,0x29},   
{0xB9,0x01},   
{0xBA,0x30},   
{0xBB,0x01},   
{0xBC,0x37},   
{0xBD,0x01},   
{0xBE,0x3D},   
{0xBF,0x01},   
{0xC0,0x43},   
{0xC1,0x01},   
{0xC2,0x5A},   
{0xC3,0x01},   
{0xC4,0x6F},   
{0xC5,0x01},   
{0xC6,0x94},   
{0xC7,0x01},   
{0xC8,0xB5},   
{0xC9,0x01},   
{0xCA,0xEE},   
{0xCB,0x02},   
{0xCC,0x20},   
{0xCD,0x02},   
{0xCE,0x21},   
{0xCF,0x02},   
{0xD0,0x53},   
{0xD1,0x02},   
{0xD2,0x8D},   
{0xD3,0x02},   
{0xD4,0xB1},   
{0xD5,0x02},   
{0xD6,0xE1},   
{0xD7,0x03},   
{0xD8,0x01},   
{0xD9,0x03},   
{0xDA,0x2A},   
{0xDB,0x03},   
{0xDC,0x36},   
{0xDD,0x03},   
{0xDE,0x42},   
{0xDF,0x03},   
{0xE0,0x54},   
{0xE1,0x03},   
{0xE2,0x62},   
{0xE3,0x03},   
{0xE4,0x76},   
{0xE5,0x03},   
{0xE6,0x94},   
{0xE7,0x03},   
{0xE8,0x97},   
{0xE9,0x03},   
{0xEA,0xFF},   

//color enhence
{0xFF,0x03}, 
{0xFE,0x08}, 
{0x18,0x00}, 
{0x19,0x00}, 
{0x1A,0x00}, 
{0x25,0x26}, 
{0x00,0x00}, 
{0x01,0x03}, 
{0x02,0x07}, 
{0x03,0x0D}, 
{0x04,0x0F}, 
{0x05,0x13}, 
{0x06,0x17}, 
{0x07,0x1B}, 
{0x08,0x1F}, 
{0x09,0x23}, 
{0x0A,0x27}, 
{0x0B,0x2B}, 
{0x0C,0x2F}, 
{0x0D,0x33}, 
{0x0E,0x37}, 
{0x0F,0x3D}, 

	//CMD 1
	{0xFF,0x00},
	{0xFB,0x01},
	//CMD2,PAGE0
	{0xFF,0x01},
	{0xFB,0x01},
	//CMD2,PAGE1
	{0xFF,0x02},
	{0xFB,0x01},
	//CMD2,PAGE2
	{0xFF,0x03},
	{0xFB,0x01},
	//CMD2,PAGE3
	{0xFF,0x04},
	{0xFB,0x01},
	//CMD2,PAGE4
	{0xFF,0x05},
	{0xFB,0x01},
	//CMD1
	{0xFF,0x00},
	{0xFF,0xff},	//end
};

static int lcd_pre_init(void)
{
	int ret;

	if (lcd_id == LCD_BOE) {
		//printk("####%s  LCD_BOE####\n", __func__);
		lcd_reset();
		otm1280a_write_0(0x11);
		mdelay(200);
		do{
			unsigned char buf[] = {0xB9,0xFF,0x83,0x94};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		do{
			//unsigned char buf[] = {0xBA,0x13};
			unsigned char buf[] = {0xBA,0x13,0x83,0x00,0x16,0xa6,0x50,0x08,0xff,0x0f,0x24,0x03,0x21,0x24,0x25,0x20,0x02,0x10};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
		//besure 0xcc is wrote correctly
		otm1280a_write_1(0xcc, 0x05);
#if 1
		do{
			unsigned char buf[] = {
0xC1,
0x01,
0x00,
0x07,
0x10,
0x16,
0x1B,
0x25,
0x2C,
0x34,
0x38,
0x44,
0x4B,
0x52,
0x5B,
0x61,
0x6A,
0x71,
0x79,
0x82,
0x89,
0x91,
0x9B,
0xA4,
0xAC,
0xB5,
0xBD,
0xC6,
0xCD,
0xD6,
0xDD,
0xE4,
0xEC,
0xF4,
0xFB,
0x05,
0xEA,
0xC9,
0xB0,
0x02,
0x5F,
0xFD,
0x73,
0xC0,
0x00,
0x03,
0x0E,
0x14,
0x19,
0x20,
0x28,
0x2F,
0x33,
0x3D,
0x43,
0x4A,
0x52,
0x57,
0x60,
0x66,
0x6E,
0x75,
0x7C,
0x83,
0x8B,
0x93,
0x9B,
0xA3,
0xAA,
0xB3,
0xBA,
0xC2,
0xC9,
0xCF,
0xD7,
0xDE,
0xE5,
0x05,
0xEA,
0xC9,
0xB1,
0x02,
0x5F,
0xFD,
0x73,
0xC0,
0x00,
0x01,
0x0F,
0x16,
0x1C,
0x26,
0x2E,
0x36,
0x3B,
0x47,
0x4E,
0x56,
0x5F,
0x65,
0x6F,
0x76,
0x7F,
0x88,
0x90,
0x99,
0xA2,
0xAC,
0xB4,
0xBD,
0xC5,
0xCD,
0xD5,
0xDE,
0xE5,
0xEB,
0xF4,
0xFA,
0xFF,
0x05,
0xEA,
0xC9,
0xB1,
0x02,
0x5F,
0xFD,
0x73,
0xC0
};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
#endif
	}else if (lcd_id == LCD_LG)
	{
		//printk("####%s  LCD_LG####\n", __func__);
		mdelay(1);
	}else if (lcd_id == LCD_NOVA_CMI) 
	{
		lcd_reset();
		//printk("####%s  LCD_NOVA_CMI####\n", __func__);
		otm1280a_write_1(0xFF, 0x00);
		mdelay(1);
		otm1280a_write_1(0xBA, 0x03);	////4LANE
		mdelay(1);
		otm1280a_write_1(0xC2, 0x03);	//Video mode
		mdelay(1);
	}else if (lcd_id == LCD_NOVA_YASSY) 
	{
		lcd_reset();
		//printk("####%s  LCD_NOVA_YASSY####\n", __func__);
		otm1280a_write_1(0xFF, 0x00);
		mdelay(1);
		otm1280a_write_1(0xBA, 0x03);	////4LANE
		mdelay(1);
		otm1280a_write_1(0xC2, 0x03);	//Video mode
		mdelay(1);
	}

	return 0;
}

#ifdef LCD_CE_INTERFACE
static int ce_on_off(int value, int flag)
{
	int ret;
	if (flag)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, 0);
	if (lcd_id == LCD_LG) {
		if (value) {
			printk("%s: CE on for LG.\n", __func__);
#if 1
			//otm1280a_write_1(0x70, 0x00);
#if 1
			do{
				unsigned char buf[] = {0x74,0x05,0x03, 0x85};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);
#endif

			do{
				unsigned char buf[] = {0x75,0x03,0x00,0x03};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x76,0x07,0x00,0x03};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

#endif
		} else {
			printk("%s: CE off.\n", __func__);
#if 1
			//otm1280a_write_1(0x70, 0x00);
#if 1
			do{
				unsigned char buf[] = {0x74,0x00,0x00,0x06};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);
#endif

			do{
				unsigned char buf[] = {0x75,0x00,0x00,0x07};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x76,0x00,0x00,0x06};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

#endif

		}
	} 
#if 0
	else {
		if (value) {
			/*
			printk("%s boe ce on, value = %d\n", __func__, value);
			otm1280a_write_1(0xe6, 0x01);
			otm1280a_write_1(0xe4, value);
			*/
		} else {
			/*
			printk("%s boe ce off\n", __func__);
			otm1280a_write_1(0xe6, 0x00);
			otm1280a_write_1(0xe4, 0x00);
			*/
		}
	}
#endif
	if (flag) {
		mdelay(100);
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, current_brightness);
	}

	return 0;
}
#endif

int lcd_pannel_on(void)
{
	int ret = DSIM_TRUE;
	//printk("##%s#####\n", __func__);
	int i = 0;

	if (lcd_id == LCD_BOE) {
		//####################################################
		printk("####%s LCD_BOE####\n", __func__);
		//otm1280a_write_1(0xcc, 0x01);
#if 0
		do{
			//unsigned char buf[] = {0xB1,0x7C,0x00,0x34,0x09,0x01,0x11,0x11,0x36,0x3E,0x26,0x26,0x57,0x12,0x01,0xE6};
			unsigned char buf[] = {0xB1,0x7C,0x00,0x34,0x09,0x01,0x11,0x11,0x36,0x3e,0x26,0x26,0x57,0x12,0x01,0xe6};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
#else
		do{
			unsigned char buf[] = {0xB1,0x7C,0x00,0x24,0x09,0x01,0x11,0x11,0x36,0x3e,0x26,0x26,0x57,0x0a,0x01,0xe6};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		mdelay(5);
		do{
			unsigned char buf[] = {0xB2,0x0f,0xc8,0x04,0x04,0x00,0x81};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		mdelay(5);
#endif
		do{
			unsigned char buf[] = {0xBf,0x06,0x10};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);

#if 0

		do{
			unsigned char buf[] = {0xB4,0x00,0x00,0x00,0x05,0x06,0x41,0x42,0x02,0x41,0x42,0x43,0x47,0x19,0x58,0x60,0x08,0x85,0x10};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
#else
		do{
			unsigned char buf[] = {0xB4,0x00,0x00,0x00,0x05,0x06,0x41,0x42,0x02,0x41,0x42,0x43,0x47,0x19,0x58,0x60,0x08,0x85,0x10};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		mdelay(5);
#endif
#if 0
		do{
			unsigned char buf[] = {0xD5,0x4C,0x01,0x00,0x01,0xCD,0x23,0xEF,0x45,0x67,0x89,0xAB,0x11,0x00,0xDC,0x10,0xFE,0x32,0xBA,0x98,0x76,0x54,0x00,0x11,0x40};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
#else
		do{
			unsigned char buf[] = {0xD5,0x4C,0x01,0x07,0x01,0xCD,0x23,0xEF,0x45,0x67,0x89,0xAB,0x11,0x00,0xDC,0x10,0xFE,0x32,0xBA,0x98,0x76,0x54,0x00,0x11,0x40};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		mdelay(5);
#endif
#if 0
		do{
			unsigned char buf[] = {0xE0,0x24,0x33,0x36,0x3F,0x3f,0x3f,0x3c,0x56,0x05,0x0c,0x0e,0x11,0x13,0x12,0x14,0x12,0x1e,0x24,0x33,0x36,0x3f,0x3f,0x3f,0x3c,0x56,0x05,0x0c,0x0e,0x11,0x13,0x12,0x14,0x12,0x1e};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
#else
		do{
			unsigned char buf[] = {0xE0,0x24,0x33,0x36,
								0x3F,0x3f,0x3f,0x3c,
								0x56,0x05,0x0c,0x0e,
								0x11,0x13,0x12,0x14,
								0x12,0x1e,0x24,0x33,
								0x36,0x3f,0x3f,0x3f,
								0x3c,0x56,0x05,0x0c,
								0x0e,0x11,0x13,0x12,
								0x14,0x12,0x1e};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
#endif
#if 1
		do{
			unsigned char buf[] = {
0xC1,
0x01,
0x00,
0x07,
0x10,
0x16,
0x1B,
0x25,
0x2C,
0x34,
0x38,
0x44,
0x4B,
0x52,
0x5B,
0x61,
0x6A,
0x71,
0x79,
0x82,
0x89,
0x91,
0x9B,
0xA4,
0xAC,
0xB5,
0xBD,
0xC6,
0xCD,
0xD6,
0xDD,
0xE4,
0xEC,
0xF4,
0xFB,
0x05,
0xEA,
0xC9,
0xB0,
0x02,
0x5F,
0xFD,
0x73,
0xC0,
0x00,
0x03,
0x0E,
0x14,
0x19,
0x20,
0x28,
0x2F,
0x33,
0x3D,
0x43,
0x4A,
0x52,
0x57,
0x60,
0x66,
0x6E,
0x75,
0x7C,
0x83,
0x8B,
0x93,
0x9B,
0xA3,
0xAA,
0xB3,
0xBA,
0xC2,
0xC9,
0xCF,
0xD7,
0xDE,
0xE5,
0x05,
0xEA,
0xC9,
0xB1,
0x02,
0x5F,
0xFD,
0x73,
0xC0,
0x00,
0x01,
0x0F,
0x16,
0x1C,
0x26,
0x2E,
0x36,
0x3B,
0x47,
0x4E,
0x56,
0x5F,
0x65,
0x6F,
0x76,
0x7F,
0x88,
0x90,
0x99,
0xA2,
0xAC,
0xB4,
0xBD,
0xC5,
0xCD,
0xD5,
0xDE,
0xE5,
0xEB,
0xF4,
0xFA,
0xFF,
0x05,
0xEA,
0xC9,
0xB1,
0x02,
0x5F,
0xFD,
0x73,
0xC0
};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
#endif
		otm1280a_write_1(0xe3, 0x01);
		do{
			unsigned char buf[] = {0xE5,0x00,0x00,0x04,
								0x04,0x02,0x00,0x80,
								0x20,0x00,0x20,0x00,
								0x00,0x08,0x06,0x04,
								0x00,0x80,0x0E};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);		

		do{
			unsigned char buf[] = {0xC7,0x00,0x20};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

		//do{
		//	unsigned char buf[] = {0xcc,0x01};
		//	ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
		//	if (ret == 0){
		//		printk("cmmd_write error\n");
		//		return ret;
		//	}
		//}while(0);

		do{
			unsigned char buf[] = {0xb6,0x2a};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
#ifdef LCD_CE_INTERFACE
		ce_on_off(ce_mode, 0);
#endif
		//otm1280a_write_1(0x36, 0x01);	//only for command mode
		//otm1280a_write_1(0xcc, 0x01);

		/* Set Display ON */
		otm1280a_write_0(0x29);
		//otm1280a_write_1(0xcc, 0x01);
		mdelay(20);
		
	} else if (lcd_id == LCD_LG){
	
		//####################################################
		printk("####%s  LCD_LG####\n", __func__);
#if 1
		do{
			unsigned char buf[] = {0xE0,0x43,0x00,0x80,0x00,0x00};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);	
		//		mdelay(1);
		//Display mode
		do{
			//unsigned char buf[] = {0xB5,0x2A,0x20,0x40,0x00,0x20};
			unsigned char buf[] = {0xB5,0x34,0x20,0x40,0x00,0x20};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//GIP SDAC
		do{
			//unsigned char buf[] = {0xB1,0x7C,0x00,0x34,0x09,0x01,0x11,0x11,0x36,0x3E,0x26,0x26,0x57,0x12,0x01,0xE6};
			//unsigned char buf[] = {0xB6,0x04,0x34,0x0F,0x16,0x13};
			unsigned char buf[] = {0xB6,0x04,0x74,0x0F,0x16,0x13};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//internal ocsillator setting
		do{
			unsigned char buf[] = {0xc0,0x01,0x08};
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			unsigned char buf[] = {0xc1,0x00};
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//		mdelay(1);

		//Power setting 
		do{
			//unsigned char buf[] = {0xC3,0x01,0x09,0x10,0x02,0x00,0x66,0x20,0x03,0x02};
			unsigned char buf[] = {0xC3,0x00,0x09,0x10,0x02,0x00,0x66,0x20,0x13,0x00};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//		mdelay(1);

		do{
			//unsigned char buf[] = {0xC4,0x22,0x24,0x12,0x18,0x59};
			unsigned char buf[] = {0xC4,0x23,0x24,0x17,0x17,0x59};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		//Gamma setting
		do{
			//		unsigned char buf[] = {0xD0,0x10,0x60,0x67,0x35,0x00,0x06,0x60,0x21,0x02};
			unsigned char buf[] = {0xD0,0x21,0x13,0x67,0x37,0x0c,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif

		}while(0);
		do{
			//unsigned char buf[] = {0xD1,0x21,0x61,0x66,0x35,0x00,0x05,0x60,0x13,0x01};
			unsigned char buf[] = {0xD1,0x32,0x13,0x66,0x37,0x02,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			//unsigned char buf[] = {0xD2,0x10,0x60,0x67,0x40,0x1F,0x06,0x60,0x21,0x02};
			unsigned char buf[] = {0xD2,0x41,0x14,0x56,0x37,0x0c,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			//	unsigned char buf[] = {0xD3,0x21,0x61,0x66,0x40,0x1F,0x05,0x60,0x13,0x01};
			unsigned char buf[] = {0xD3,0x52,0x14,0x55,0x37,0x02,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			//	unsigned char buf[] = {0xD4,0x10,0x60,0x67,0x35,0x00,0x06,0x60,0x21,0x02};
			unsigned char buf[] = {0xD4,0x41,0x14,0x56,0x37,0x0c,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			//unsigned char buf[] = {0xD5,0x21,0x61,0x66,0x35,0x00,0x05,0x60,0x13,0x01};
			unsigned char buf[] = {0xD5,0x52,0x14,0x55,0x37,0x02,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//Gamma setting end
		do{
			unsigned char buf[] = {0x36,0x0B };
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//		otm1280a_write_0(0x11);
		//		mdelay(5);
		do{
			//unsigned char buf[] = {0xF9,0x80,0x00};
			unsigned char buf[] = {0xF9,0x00};
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
#endif

#ifdef LCD_CE_INTERFACE
			otm1280a_write_1(0x70, 0x00);

			do{
				unsigned char buf[] = {0x71,0x00,0x00, 0x01, 0x01};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x72,0x01,0x0e};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x73,0x34,0x52, 0x00};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);
#if 0
			do{
				unsigned char buf[] = {0x74,0x05,0x00, 0x06};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x75,0x03,0x00,0x07};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x76,0x07, 0x00 ,0x06};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);
#endif

			do{
				unsigned char buf[] = {0x77,0x3f,0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x78,0x40,0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x79,0x40,0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x7A,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x7B,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x7C,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);
			ce_on_off(ce_mode, 0);
#endif
	}else if (lcd_id == LCD_NOVA_CMI) {
	//####################################################
		printk("####%s  LCD_NOVA_CMI####\n", __func__);
		//i = 2;
		while(1)
		{
			if((yt_code[i].add == 0xff &&  yt_code[i].data == 0xff))
			        break;
			//lcd_init_cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA,  &(yt_code[i]), 2);
			otm1280a_write_1(yt_code[i].add, yt_code[i].data);
			udelay(10);
			//printk("write %x  %x\n", yt_code[i].add, yt_code[i].data);
			i++;
		}
		otm1280a_write_0(0x11);
		//  mdelay(200);
		mdelay(120);
		//otm1280a_write_1(0x51, 0xff);
		//mdelay(4);
		//otm1280a_write_1(0x53, 0x24);
		//mdelay(4);
		otm1280a_write_1(0x55, 0x80);
		//mdelay(4);
		otm1280a_write_0(0x29);
		// mdelay(50);
		mdelay(10);
		otm1280a_write_1(0xFF, 0x00);
		//mdelay(4);
		otm1280a_write_1(0x35, 0x00);
		//mdelay(4);	
	}else if (lcd_id == LCD_NOVA_YASSY) {
	//####################################################
		//i = 2;
		printk("####%s  LCD_NOVA_YASSY####\n", __func__);
		while(1)
		{
		    if((yt_code[i].add == 0xff &&  yt_code[i].data == 0xff))
		            break;
		    otm1280a_write_1(yt_code[i].add, yt_code[i].data);
		    udelay(10);
		    //printk("write %x  %x\n", yt_code[i].add, yt_code[i].data);
		    i++;
		}
		otm1280a_write_0(0x11);
		//  mdelay(200);
		mdelay(120);
		//  otm1280a_write_1(0x51, 0xff);
		// mdelay(4);
		//  otm1280a_write_1(0x53, 0x24);
		//  mdelay(4);
		otm1280a_write_1(0x55, 0x80);
		//mdelay(4);
		otm1280a_write_0(0x29);
		// mdelay(50);
		mdelay(10);
		otm1280a_write_1(0xFF, 0x00);
		// mdelay(4);
		otm1280a_write_1(0x35, 0x00);
		// mdelay(4);
	}
		
	return DSIM_TRUE;
}
int  lcd_panel_init(void)
{
	return  lcd_pannel_on();

}

static int stuttgart_panel_init(void)
{
	return lcd_panel_init();
}

static int otm1280a_set_link(void *pd, unsigned int dsim_base,
		unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
			unsigned int data1, unsigned int data2),
		unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
			unsigned int data1, unsigned int data2))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -1;
	}

	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
void otm1280a_early_suspend(void)
{
	//	struct s3cfb_global *fbdev =
	//		container_of(h, struct s3cfb_global, early_suspend);
#ifdef ESD_FOR_LCD
	if (lcd_id == LCD_BOE)
		;
		//cancel_delayed_work_sync(&lcd_delayed_work);
	else {
		disable_irq(TE_IRQ);
		del_timer(&te_timer);
		//cancel_delayed_work_sync(&lcd_init_work);
	}
#endif
	otm1280a_display_off(NULL);
#ifdef LCD_CABC_MODE
	if (lcd_id == LCD_LG) 
		otm1280a_cabc_control(CABC_OFF);
#endif
	printk("%s\n", __func__);
	if (lcd_id == LCD_BOE){
		otm1280a_write_0(0x10);
		mdelay(200);
		otm1280a_write_0(0x28);
		mdelay(20);
	}

	s5p_dsim_frame_done_interrupt_enable(0);
}

void otm1280a_late_resume(void)
{
	printk("%s\n", __func__);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, current_brightness);
	if (lcd_id == LCD_NOVA_CMI || lcd_id == LCD_NOVA_YASSY)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x53, 0x2c);
	else
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x53, 0x24);
	printk("%s current_bl = %d\n", __func__, current_brightness);
#ifdef LCD_CABC_MODE
	if (lcd_id == LCD_LG && cabc_mode) 
		otm1280a_cabc_control(cabc_mode);
#endif
	s5p_dsim_frame_done_interrupt_enable(1);
#ifdef ESD_FOR_LCD
	if (lcd_id == LCD_BOE)
	{
		schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
		//printk("%s, none ESD function\n", __func__);
	}
	else 
		enable_irq(TE_IRQ);
#endif
}
#else
#ifdef CONFIG_PM
static int otm1280a_suspend(void)
{
	//printk("%s\n", __func__);
	/*
	   otm1280a_write_0(0x28);
	   mdelay(20);
	   otm1280a_write_0(0x10);
	   mdelay(200);
	   */
	printk("%s\n", __func__);
	return 0;
}

static int otm1280a_resume(struct device *dev)
{
	printk("%s\n", __func__);
	return 0;
}
#else
#define otm1280a_suspend	NULL
#define otm1280a_resume	NULL
#endif
#endif

#define ID_PIN EXYNOS4_GPX3(3)
#ifdef ESD_FOR_LCD
extern void lcd_te_handle(void);
extern int fbdev_register_finished;
static int test_te_function(const char *val, struct kernel_param *kp)
{
	printk("%s\n", __func__);
	schedule_delayed_work(&lcd_init_work, 0);
	return 0;
}
static void lcd_work_handle(struct work_struct *work)
{
	otm1280a_write_0(0x38);
	schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
}

static void lcd_te_abnormal_handle(struct work_struct *work)
{
	printk("!!!!!!!!!!!!!!!! %s !!!!!!!!!!!!!!!!!\n", __func__);
	//otm1280a_early_suspend();
	lcd_te_handle();
	//otm1280a_late_resume();
	printk("!!!!!!!!!!!!!!!! %s finished!!!!!!!!!!!!!!!!!\n", __func__);
}

void te_timer_handle(unsigned long data)
{
	schedule_delayed_work(&lcd_init_work, 0);
}
irqreturn_t lcd_te_irq_handle(int irq, void *dev_id)
{
	if (fbdev_register_finished)
		mod_timer(&te_timer, TE_TIME * HZ / 1000 + jiffies);
	return IRQ_HANDLED;
}
#endif

static int otm1280a_probe(struct device *dev)
{
	struct s3cfb_global *fbdev;
	int ret, err;

	printk("%s\n", __func__);
	err = gpio_request(LCD_RESET, "GPF");
	if (err)
		printk(KERN_ERR "#### failed to request GPF0_0 ####\n");

	memset(&props, 0, sizeof(struct backlight_properties));
	props.max_brightness = 255;
	props.type = BACKLIGHT_RAW;
	backlight_device_register("pwm-backlight.0", dev, &props, &s5p_bl_ops, &props);
	fbdev = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
	if (!fbdev) {
		printk(KERN_ERR "failed to allocate for "
				"global fb structure\n");
		ret = -ENOMEM;
		return ret;
	}
#ifdef ESD_FOR_LCD
#if 1
	if (lcd_id == LCD_BOE) {
		INIT_DELAYED_WORK(&lcd_delayed_work, lcd_work_handle);
		schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
	} else
#endif
	{
		INIT_DELAYED_WORK(&lcd_init_work, lcd_te_abnormal_handle);
		init_timer(&te_timer);
		te_timer.function = te_timer_handle;
		ret = request_irq(TE_IRQ, lcd_te_irq_handle, IRQF_TRIGGER_RISING, "LCD_TE", NULL);
		if (ret) {
			printk(KERN_ERR "lcd te request irq(%d) failure, ret = %d\n", TE_IRQ, ret);
			return ret;
		}
	}
#endif
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	fbdev->early_suspend.suspend = otm1280a_early_suspend;
	fbdev->early_suspend.resume = otm1280a_late_resume;
	fbdev->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 30;
	register_early_suspend(&fbdev->early_suspend);
#endif
#endif


	return 0;
}

static s32 otm1280a_remove(struct device *dev)
{
	gpio_free(LCD_RESET);
	return 0;
}

static void  otm1280a_shutdown(struct device *dev)
{
	otm1280a_display_off(dev);
}
static struct mipi_lcd_driver otm1280a_mipi_driver = {
	.name = "stuttgart_mipi_lcd",
	.pre_init = lcd_pre_init,
	.init = stuttgart_panel_init,
	.display_on = otm1280a_display_on,
	.set_link = otm1280a_set_link,
	.probe = otm1280a_probe,
	.remove = otm1280a_remove,
	.shutdown = otm1280a_shutdown,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = otm1280a_suspend,
	.resume = otm1280a_resume,
#endif
	.display_off = otm1280a_display_off,
};

static int otm1280a_init(void)
{
	s5p_dsim_register_lcd_driver(&otm1280a_mipi_driver);
	return 0;
}

static void otm1280a_exit(void)
{
	return;
}

static struct s3cfb_lcd stuttgart_mipi_lcd_boe = {
	.width	= 720,
	.height	= 1280,
	.bpp	= 24,
	.freq	= 60,

	.timing = {
		.h_fp = 0x20,
		.h_bp = 0x3b,
		.h_sw = 0x09,
		.v_fp = 0x0b,
		.v_fpe = 1,
		.v_bp = 0x0b,
		.v_bpe = 1,
		.v_sw = 0x02,
		.cmd_allow_len = 0xf,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

static struct s3cfb_lcd stuttgart_mipi_lcd_lg = { 
	.width  = 720,
	.height = 1280,
	.bpp    = 24, 
	.freq   = 60, 

	.timing = { 
		.h_fp = 14, 
		.h_bp = 112,
		.h_sw = 4,
		.v_fp = 8, 
		.v_fpe = 1,
		.v_bp = 12, 
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 0xf,
	},  

	.polarity = { 
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},  
};

static struct s3cfb_lcd stuttgart_mipi_lcd_yassy = { 
	.width  = 720,
	.height = 1280,
	.bpp    = 24, 
	.freq   = 60, 

	.timing = { 
		.h_fp = 14, 
		.h_bp = 112,
		.h_sw = 4,
		.v_fp = 6, 
		.v_fpe = 2,
		.v_bp = 1, 
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 0xf,
	},  

	.polarity = { 
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},  
};

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	stuttgart_mipi_lcd_boe.init_ldi = NULL;
	stuttgart_mipi_lcd_lg.init_ldi = NULL;
	stuttgart_mipi_lcd_yassy.init_ldi = NULL;
	if (lcd_id == LCD_BOE)
		ctrl->lcd = &stuttgart_mipi_lcd_boe;
	else if(lcd_id == LCD_LG)
		ctrl->lcd = &stuttgart_mipi_lcd_lg;
	else if(lcd_id == LCD_NOVA_CMI)
		ctrl->lcd = &stuttgart_mipi_lcd_yassy;
	else//yassy
		ctrl->lcd = &stuttgart_mipi_lcd_yassy;
}

module_init(otm1280a_init);
module_exit(otm1280a_exit);
#ifdef LCD_CE_INTERFACE
static int ce_control_func(const char *val, struct kernel_param *kp)
{
	int value;
	int ret = param_set_int(val, kp);

	if(ret < 0)
	{   
		printk(KERN_ERR"%s Invalid argument\n", __func__);
		return -EINVAL;
	}   
	value = *((int*)kp->arg);

	if (flag == 0) {
		flag = 1;
		ce_on_off(value, 0);
	} else {
		ce_on_off(value, 1);
	}
	return 0;
}
#endif

#ifdef LCD_CABC_MODE
module_param_call(set_cabc_mode, otm1280a_cabc_mode, param_get_int, &cabc_mode, S_IRUSR | S_IWUSR);
#endif
#ifdef LCD_CE_INTERFACE
module_param_call(ce_control, ce_control_func, param_get_int, &ce_mode, S_IRUSR | S_IWUSR);
#endif
#ifdef ESD_FOR_LCD
module_param_call(test_te, test_te_function, param_get_int, &esd_test, S_IRUSR | S_IWUSR);
#endif



extern void print_fimd_regs(void);
extern void print_dsim_regs(void);
int lcdregs = 0;
void print_LCD_regs(const char *val, struct kernel_param *kp)
{
	print_fimd_regs();
	print_dsim_regs();
}
module_param_call(lcdregs, print_LCD_regs, param_get_int, &lcdregs, S_IRUSR | S_IWUSR);


MODULE_LICENSE("GPL");



#if 0	//read chip ID
	u32 read_buf[100];
	memset(read_buf, 0, sizeof(read_buf));
	u8 buf[] = {0xf4};
	//ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  (unsigned int )buf, sizeof buf);
	otm1280a_write_1(0xf4, 0x00);
	int read_size;
	ddi_pd->cmd_write(ddi_pd->dsim_base, SET_MAX_RTN_PKT_SIZE, 100, 0);
	s5p_dsim_clr_state(ddi_pd->dsim_base,RxDatDone);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_RD_NO_PARA, (u8)buf[0], 0);
	s5p_dsim_wait_state(ddi_pd->dsim_base, RxDatDone);
	s5p_dsim_read_rx_fifo(ddi_pd->dsim_base, read_buf, &read_size);
	u8 *p = (u8*)read_buf;
	for (i = 0; i < 1; i++)
	        printk("id = %#x \n", p[i]);
	printk("\n");
#endif

