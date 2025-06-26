#ifndef JSON_H
#define JSON_H

#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart3;

int json_fill (char* json, char* sensor_type, char* status, char* message);
void json_clear(char* json);
int json_send_report(char* json, char* sensor_type, char* status, char* message);

#endif /* JSON_H */
