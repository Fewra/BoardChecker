#include "json.h"
#include <string.h>
#include <stdio.h>
#include "main.h"

// Функция: разбирает JSON и кладёт данные в структуру
void parse_json(char *json_buf, struct JsonData *data)
{
    char *start, *end;

    // SENSOR
    start = strstr(json_buf, "\"sensor\"");
    if (start) {
        start = strchr(start, ':'); // найти двоеточие
        if (start) {
            start++; // перейти к кавычкам
            while (*start == ' ' || *start == '\"') start++;
            end = strchr(start, '\"');
            if (end) {
                size_t len = end - start;
                if (len >= MAX_SENSOR_LEN) len = MAX_SENSOR_LEN - 1;
                strncpy(data->sensor, start, len);
                data->sensor[len] = '\0';
            }
        }
    }

    // STATUS
    start = strstr(json_buf, "\"status\"");
    if (start) {
        start = strchr(start, ':');
        if (start) {
            start++;
            while (*start == ' ' || *start == '\"') start++;
            end = strchr(start, '\"');
            if (end) {
                size_t len = end - start;
                if (len >= MAX_STATUS_LEN) len = MAX_STATUS_LEN - 1;
                strncpy(data->status, start, len);
                data->status[len] = '\0';
            }
        }
    }

    // MESSAGE
    start = strstr(json_buf, "\"message\"");
    if (start) {
        start = strchr(start, ':');
        if (start) {
            start++;
            while (*start == ' ' || *start == '\"') start++;
            end = strchr(start, '\"');
            if (end) {
                size_t len = end - start;
                if (len >= MAX_MESSAGE_LEN) len = MAX_MESSAGE_LEN - 1;
                strncpy(data->message, start, len);
                data->message[len] = '\0';
            }
        }
    }
}

//заполнение пакета json
int json_fill (char* json, char* sensor_type, char* status, char* message)
{
	//проверка аргуметров
	if ((json == NULL) || (sensor_type == NULL) || (status == NULL) || (message == NULL)){
		return (-1);
	}

	sprintf(json, "{\r\n\"sensor\": \"%s\",\r\n\"status\": \"%s\",\r\n\"message\": \"%s\"\r\n}", sensor_type, status, message);

	return (0);
}

void json_clear(char* json)
{
	int i = 0;
	while (json[i] != '\0') {
		json[i] = 0;
	}
}

int json_send_report(char* json, char* sensor_type, char* status, char* message){

	//проверка аргуметров
		if ((json == NULL) || (sensor_type == NULL) || (status == NULL) || (message == NULL)){
			return (-1);
		}

	json_fill(json, sensor_type, status, message);

	HAL_UART_Transmit(&huart3, (uint8_t*)json, strlen((char*)json), 100);

	json_clear(json);

	return (0);
}

void clear_JsonData(struct JsonData *jd)
{
	memset(jd->sensor, 0, MAX_SENSOR_LEN);
	memset(jd->status, 0, MAX_STATUS_LEN);
	memset(jd->message, 0, MAX_MESSAGE_LEN);
}

// мб надо добавить проверку на переполнение буфера
void wait_for_json(struct JsonData *data)
{
	uint8_t rx_byte;
	char json_buf[1024] = {0};
	int idx = 0;

	while (1)
	{
		// данные принимаем по одному байту
		if (HAL_UART_Receive(&huart3, &rx_byte, 1, HAL_MAX_DELAY) == HAL_OK)
		{
			json_buf[idx++] = rx_byte;

			// условие окончания
			if (rx_byte == '}')
			{
				json_buf[idx] = '\0';
				break;  // JSON получен
			}
		}
	}

	parse_json(json_buf, data);
}
