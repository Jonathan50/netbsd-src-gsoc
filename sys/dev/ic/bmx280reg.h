/*	$NetBSD: bmx280reg.h,v 1.1 2022/12/03 01:04:43 brad Exp $	*/

/*
 * Copyright (c) 2022 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DEV_IC_BMX280REG_H_
#define _DEV_IC_BMX280REG_H_

#define BMX280_TYPICAL_ADDR_1	0x76
#define BMX280_TYPICAL_ADDR_2	0x77

#define BMX280_REGISTER_DIG_T1		0x88
#define BMX280_REGISTER_DIG_T2		0x8A
#define BMX280_REGISTER_DIG_T3		0x8C
#define BMX280_REGISTER_DIG_P1		0x8E
#define BMX280_REGISTER_DIG_P2		0x90
#define BMX280_REGISTER_DIG_P3		0x92
#define BMX280_REGISTER_DIG_P4		0x94
#define BMX280_REGISTER_DIG_P5		0x96
#define BMX280_REGISTER_DIG_P6		0x98
#define BMX280_REGISTER_DIG_P7		0x9A
#define BMX280_REGISTER_DIG_P8		0x9C
#define BMX280_REGISTER_DIG_P9		0x9E
#define BMX280_REGISTER_DIG_H1		0xA1
#define BMX280_REGISTER_DIG_H2		0xE1
#define BMX280_REGISTER_DIG_H3		0xE3
#define BMX280_REGISTER_DIG_H4		0xE4
#define BMX280_REGISTER_DIG_H5		0xE5

#define BMX280_REGISTER_ID		0xD0
#define BMX280_ID_BMP280		0x58
#define BMX280_ID_BME280		0x60

#define BMX280_REGISTER_RESET		0xE0
#define BMX280_TRIGGER_RESET		0xB6

#define BMX280_REGISTER_CTRL_HUM	0xF2

#define BMX280_REGISTER_STATUS		0xF3
#define BMX280_STATUS_MEASURING_MASK	0x08
#define BMX280_STATUS_IM_UPDATE_MASK	0x01

#define BMX280_REGISTER_CTRL_MEAS	0xF4
#define BMX280_CTRL_OSRS_T_MASK		0xE0
#define BMX280_CTRL_OSRS_P_MASK		0x1C
#define BMX280_CTRL_OSRS_T_SHIFT	5
#define BMX280_CTRL_OSRS_P_SHIFT	2
#define BMX280_OSRS_TP_VALUE_SKIPPED	0x00
#define BMX280_OSRS_TP_VALUE_X1		0x01
#define BMX280_OSRS_TP_VALUE_X2		0x02
#define BMX280_OSRS_TP_VALUE_X4		0x03
#define BMX280_OSRS_TP_VALUE_X8		0x04
#define BMX280_OSRS_TP_VALUE_X16	0x05
#define BMX280_CTRL_MODE_MASK		0x03
#define BMX280_MODE_SLEEP		0x00
#define BMX280_MODE_FORCED		0x01
#define BMX280_MODE_NORMAL		0x03

#define BMX280_REGISTER_CONFIG		0xF5
#define BMX280_CONFIG_T_SB_MASK		0xE0
#define BMX280_CONFIG_FILTER_MASK	0x1C
#define BMX280_CONFIG_FILTER_SHIFT	2
#define BMX280_FILTER_VALUE_OFF		0x00
#define BMX280_FILTER_VALUE_2		0x01
#define BMX280_FILTER_VALUE_5		0x02
#define BMX280_FILTER_VALUE_11		0x04
#define BMX280_FILTER_VALUE_22		0x05
#define BMX280_CONFIG_SPI3W_EN_MASK	0x01

#define BMX280_REGISTER_PRESS_MSB	0xF7
#define BMX280_REGISTER_PRESS_LSB	0xF8
#define BMX280_REGISTER_PRESS_XLSB	0xF9

#define BMX280_REGISTER_TEMP_MSB	0xFA
#define BMX280_REGISTER_TEMP_LSB	0xFB
#define BMX280_REGISTER_TEMP_XLSB	0xFC

#define BMX280_TEMPPRES_XLSB_MASK	0xF0

#define BMX280_REGISTER_HUM_MSB		0xFD
#define BMX280_REGISTER_HUM_LSB		0xFE

#endif