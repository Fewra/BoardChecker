#include "json.h"
#include <string.h>
#include <stdio.h>

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

	HAL_UART_Transmit_IT(&huart3, (uint8_t*)json, strlen((char*)json));

	json_clear(json);

	return (0);
}
