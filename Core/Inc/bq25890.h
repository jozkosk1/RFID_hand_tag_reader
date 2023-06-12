/*
 * bq25890.h
 *
 *  Created on: 19. 3. 2019
 *      Author: Jozo
 */

#ifndef BQ25890_H_
#define BQ25890_H_

#include "main.h"

#define I2C_ADR 0xd4

void BQ25890_write_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address, uint8_t data);
uint8_t BQ25890_read_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address);
void BQ25890_start_ADC(I2C_HandleTypeDef *i2c);
uint16_t BQ25890_read_bat_voltage (I2C_HandleTypeDef *i2c);
uint16_t BQ25890_read_chg_current (I2C_HandleTypeDef *i2c);

#endif /* BQ25890_H_ */
