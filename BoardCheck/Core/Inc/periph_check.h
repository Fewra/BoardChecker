#ifndef PERIPH_CHECK_H
#define PERIPH_CHECK_H

#include <stdint.h>

//#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
//#include "stm32f4xx_hal_uart.h"

extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;

struct periph_check
{
	uint8_t uart3_send_buf[80];
	uint8_t uart6_recv_buf[16];

	//gps
	unsigned int received_checksum;
	uint8_t calculated_checksum;
	//uint8_t j;
};

int pc_init(struct periph_check* pc);

//gps
int pc_gps_parce_nmea(struct periph_check* pc, uint8_t* buf, size_t len);
int pc_gps_recv_nmea(struct periph_check* pc);

//compass
int pc_compass_check_addr();
uint8_t HMC5883L_IsReady(void);

#endif /* PERIPH_CHECK_H */
