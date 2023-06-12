/*
 * bq25890.c
 *
 *  Created on: 19. 3. 2019
 *      Author: Jozo
 */

#include "bq25890.h"

void BQ25890_write_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address, uint8_t data)
{
	uint8_t data_tx[2];

	data_tx[0] = reg_address;
	data_tx[1] = data;
	HAL_I2C_Master_Transmit(i2c, I2C_ADR, data_tx, 2, 100);
}
uint8_t BQ25890_read_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address)
{
	uint8_t data_tx[1];
	uint8_t data_rx[1];

	data_tx[0] = reg_address;
	HAL_I2C_Master_Transmit(i2c, I2C_ADR, data_tx, 1, 100);
	HAL_I2C_Master_Receive(i2c, I2C_ADR, data_rx, 1,100);
	return data_rx[0];
}
void BQ25890_start_ADC(I2C_HandleTypeDef *i2c)
{
	/*Conversion time NOM: 8ms, MAX 1000ms*/
	uint8_t data;
	data = BQ25890_read_reg(i2c, 0x02);
	data = data | 0x80;
	BQ25890_write_reg(i2c, 0x02, data);
}
uint16_t BQ25890_read_bat_voltage (I2C_HandleTypeDef *i2c)
{

	uint8_t data;
	uint16_t voltage;
	data = BQ25890_read_reg(i2c, 0x0E);
	data = data & 0x7F;
	data = data << 1;
	voltage = (data *10) + 2304;
	return voltage;
}
uint16_t BQ25890_read_chg_current (I2C_HandleTypeDef *i2c)
{
	uint8_t data, flag = 0;
	uint16_t current;
	data = BQ25890_read_reg(i2c, 0x12);
	flag = data & 0x01;

	data = data & 0x7F;
	data = data >> 1;
	current = (data *100);
	if (flag)
		current = current + 50;
	return current;
}




