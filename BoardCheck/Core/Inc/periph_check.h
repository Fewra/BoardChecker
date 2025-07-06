#ifndef PERIPH_CHECK_H
#define PERIPH_CHECK_H

#include <stdint.h>
#include "json.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"

#include "mpu6050.h"

extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart2;

struct periph_check
{
	uint8_t uart3_send_buf[80];
	uint8_t uart6_recv_buf[16];
	uint8_t uart2_recv_buf[100];

	//gps
	unsigned int received_checksum;
	uint8_t calculated_checksum;

	//compass
	int16_t magn_x_calibr;
	int16_t magn_y_calibr;
	int16_t magn_z_calibr;

	//gyro
	MPU6050_t MPU6050;
	double a_x_calibr;
	double a_y_calibr;
	double a_z_calibr;
	double g_x_calibr;
	double g_y_calibr;
	double g_z_calibr;

	//switch
	GPIO_PinState position;

};

int pc_init(struct periph_check* pc);

//gps
int pc_gps_parce_nmea(struct periph_check* pc, uint8_t* buf, size_t len);
int pc_gps_recv_nmea(struct periph_check* pc);

//compass
int pc_compass_check_addr();
uint8_t HMC5883L_IsReady(void);
int compass_calibr_pos1(struct periph_check* pc);
int compass_calibr_pos2(struct periph_check* pc);
int compass_check_pos1(struct periph_check* pc);
int compass_check_pos2(struct periph_check* pc);

//gyro
int gyro_calibr_pos1(struct periph_check* pc);
int gyro_calibr_pos2(struct periph_check* pc);
int gyro_check_pos1(struct periph_check* pc);
int gyro_check_pos2(struct periph_check* pc);

//switch
int switch_check(struct periph_check* periph_check);

//relay
void relay_check(void);

//gsm
int gsm_check(struct periph_check* pc, struct JsonData* jd);

#endif /* PERIPH_CHECK_H */
