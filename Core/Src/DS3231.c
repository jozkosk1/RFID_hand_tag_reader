/*
 * DS3231.c
 *
 *  Created on: 8. 9. 2019
 *      Author: Jozo
 */

#include "DS3231.h"

RealTime_structure RealTime;

void i2c_write_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address, uint8_t data)
{
	uint8_t data_tx[2];

	data_tx[0] = reg_address;
	data_tx[1] = data;
	HAL_I2C_Master_Transmit(i2c, DS3231_ADR, data_tx, 2, 100);
}
uint8_t i2c_read_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address)
{
	uint8_t data_tx[1];
	uint8_t data_rx[1];

	data_tx[0] = reg_address;
	HAL_I2C_Master_Transmit(i2c, DS3231_ADR, data_tx, 1, 100);
	HAL_I2C_Master_Receive(i2c, DS3231_ADR, data_rx, 1,100);
	return data_rx[0];
}
void time_read(I2C_HandleTypeDef *i2c)
{
	uint8_t rx_data;
	rx_data = i2c_read_reg(i2c, 0x00);
	RealTime.seconds = (rx_data & 0x0f)+(((rx_data & 0x70)>>4)*10);

	rx_data = i2c_read_reg(i2c, 0x01);
	RealTime.minutes = (rx_data & 0x0f)+(((rx_data & 0x70)>>4)*10);

	rx_data = i2c_read_reg(i2c, 0x02);
	RealTime.hours = (rx_data & 0x0f);
	if (rx_data & 0x10)
		RealTime.hours = RealTime.hours + 10;
	if (rx_data & 0x20)
		RealTime.hours = RealTime.hours + 20;

	rx_data = i2c_read_reg(i2c, 0x4);
	RealTime.date = (rx_data & 0x0f)+(((rx_data & 0x30)>>4)*10);

	rx_data = i2c_read_reg(i2c, 0x5);
	RealTime.month = (rx_data & 0x0f)+(((rx_data & 0x10)>>4)*10);

	rx_data = i2c_read_reg(i2c, 0x6);
	RealTime.year = (rx_data & 0x0f)+(((rx_data & 0xf0)>>4)*10);
}
void time_set(I2C_HandleTypeDef *i2c)
{
	uint8_t tx_data=0;
	tx_data = ((RealTime.seconds%10)&0x0f) + ((RealTime.seconds/10)<<4);
	i2c_write_reg(i2c, 0x00, tx_data);

	tx_data = 0;
	tx_data = ((RealTime.minutes%10)&0x0f) + ((RealTime.minutes/10)<<4);
	i2c_write_reg(i2c, 0x01, tx_data);

	tx_data = 0;
	tx_data = ((RealTime.hours%10)&0x0f);
	if (RealTime.hours >19)
		tx_data = tx_data + 0x20;
	else
		if (RealTime.hours > 9)
			tx_data = tx_data +0x10;
	i2c_write_reg(i2c, 0x02, tx_data);

	tx_data=0;
	tx_data = ((RealTime.date%10)&0x0f) + ((RealTime.date/10)<<4);
	i2c_write_reg(i2c, 0x04, tx_data);

	tx_data=0;
	tx_data = ((RealTime.month%10)&0x0f) + ((RealTime.month/10)<<4);
	i2c_write_reg(i2c, 0x05, tx_data);

	tx_data=0;
	tx_data = ((RealTime.year%10)&0x0f) + ((RealTime.year/10)<<4);
	i2c_write_reg(i2c, 0x06, tx_data);
}
void time_alarm_set (I2C_HandleTypeDef *i2c, uint8_t delta_sec, uint8_t delta_min, uint8_t delta_hour)
{
	uint8_t temporary;
	time_alarm_reset(i2c);
	time_read(i2c);

	RealTime.A1_seconds = RealTime.seconds + delta_sec;
	if (RealTime.A1_seconds > 59)
	{
		delta_min = delta_min + (RealTime.A1_seconds/60);
		RealTime.A1_seconds = RealTime.A1_seconds % 60;
	}

	RealTime.A1_minutes = RealTime.minutes + delta_min;
	if (RealTime.A1_minutes > 59)
	{
		delta_hour = delta_hour + (RealTime.A1_minutes/60);
		RealTime.A1_minutes = RealTime.A1_minutes % 60;
	}

	RealTime.A1_hours = RealTime.hours + delta_hour;
	if (RealTime.A1_hours > 23)
	{
		RealTime.A1_date = RealTime.date + (RealTime.A1_hours/24);
		RealTime.A1_hours = RealTime.A1_hours % 60;
	}
	else
	{
		RealTime.A1_date = RealTime.date;
	}

	temporary = (((RealTime.A1_seconds/10) << 4)+(RealTime.A1_seconds % 10));
	temporary &= ~(1<<7);
	i2c_write_reg(i2c, 0x07, temporary);

	temporary = (((RealTime.A1_minutes/10) << 4)+(RealTime.A1_minutes % 10));
	temporary &= ~(1<<7);
	i2c_write_reg(i2c, 0x08, temporary);

	i2c_write_reg(i2c, 0x09, 0x80);
	i2c_write_reg(i2c, 0x0a, 0x80);

	temporary = i2c_read_reg(i2c, 0x0e);
	temporary |= (1<<0);
	i2c_write_reg(i2c, 0x0e, temporary);
}
void time_alarm_reset(I2C_HandleTypeDef *i2c)
{
	uint8_t temporary;
	temporary = i2c_read_reg(i2c, 0x0e);
	temporary &= ~(1<<0);
	temporary &= ~(1<<1);
	i2c_write_reg(i2c, 0x0e, temporary);


	temporary = i2c_read_reg(i2c, 0x0f);
	temporary &= ~(1<<0);
	temporary &= ~(1<<1);
	i2c_write_reg(i2c, 0x0f, temporary);
}
void time_oscilator_stop_flag_reset(I2C_HandleTypeDef *i2c)
{
	uint8_t temporary;
	temporary = i2c_read_reg(i2c, 0x0f);
	temporary &= ~(1<<7);
	i2c_write_reg(i2c, 0x0f, temporary);
}








