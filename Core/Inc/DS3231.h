/*
 * DS3231.h
 *
 *  Created on: 8. 9. 2019
 *      Author: Jozo
 */

#ifndef DS3231_H_
#define DS3231_H_

#include "stm32f0xx_hal.h"

#define DS3231_ADR 0xD0

typedef struct __RealTime_structure{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t date;
	uint8_t month;
	uint8_t year;
	uint8_t A1_seconds;
	uint8_t A1_minutes;
	uint8_t A1_hours;
	uint8_t A1_date;
	uint8_t A1_month;
	uint8_t A2_minutes;
	uint8_t A2_hours;
	uint8_t A2_date;
	uint8_t A2_month;
}RealTime_structure;

extern RealTime_structure RealTime;


void i2c_write_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address, uint8_t data);
uint8_t i2c_read_reg (I2C_HandleTypeDef *i2c, uint8_t reg_address);
void time_read(I2C_HandleTypeDef *i2c);
void time_set(I2C_HandleTypeDef *i2c);
void time_alarm_set (I2C_HandleTypeDef *i2c, uint8_t delta_sec, uint8_t delta_min, uint8_t delta_hour);
void time_alarm_reset(I2C_HandleTypeDef *i2c);
void time_oscilator_stop_flag_reset(I2C_HandleTypeDef *i2c);


#endif /* DS3231_H_ */
