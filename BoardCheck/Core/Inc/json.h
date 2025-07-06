#ifndef JSON_H
#define JSON_H

#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart3;

#define MAX_SENSOR_LEN  32
#define MAX_STATUS_LEN  32
#define MAX_MESSAGE_LEN 64

struct JsonData{
    char sensor[MAX_SENSOR_LEN];
    char status[MAX_STATUS_LEN];
    char message[MAX_MESSAGE_LEN];
};

int json_fill (char* json, char* sensor_type, char* status, char* message);
void json_clear(char* json);
int json_send_report(char* json, char* sensor_type, char* status, char* message);

void parse_json(char *json_buf, struct JsonData *data);
void wait_for_json(struct JsonData *data);
void clear_JsonData(struct JsonData *jd);

#endif /* JSON_H */
