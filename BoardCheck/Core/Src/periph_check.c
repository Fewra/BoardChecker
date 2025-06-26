//файл содержит функции проверки периферии платы

#include "periph_check.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

//#include "stm32f4xx_hal_uart.h"

int pc_init(struct periph_check* pc)
{
	return (0);
}

int pc_gps_parce_nmea(struct periph_check* periph_check, uint8_t* buf, size_t len)
{
	// ищем '*' в сообщении
	char* asterisk = strchr((char*)buf, '*');
	if (asterisk == NULL) {
		//return 0;
	}

	// Извлекаем переданную контрольную сумму
	if (sscanf(asterisk + 1, "%02x", &periph_check->received_checksum) != 1) {
		//return 0;
	}

	int j = 0;
	while(buf[j] != '*'){
		j++;
	}

	periph_check->calculated_checksum = 0x00;
	//Вычисляем контрольную сумму самостоятельно (XOR всех символов от $ до *)
	for (int i = 0; i < j; i++){
		periph_check->calculated_checksum ^= buf[i];
	}

	if (periph_check->received_checksum == periph_check->calculated_checksum) return (0);
	else return (-1);
}

int pc_gps_recv_nmea(struct periph_check* pc)
{
   uint8_t* uart6_recv_buf = pc->uart6_recv_buf;

   uint8_t nmea_message[48] = {0};
   int i = 0;
   int retval = -1;

   //принимаем один байт данных
   while(HAL_UART_Receive(&huart6, uart6_recv_buf, 1, 3) != HAL_OK ) {}

   //если обнаружили начало пакета NMEA
   if (uart6_recv_buf[0] == '$'){

      //накапливаем символы пакета NMEA в массив nmea_message за исключением '$' пока не достигнем конца строки
      while (uart6_recv_buf[0] != '\r'){
         while(HAL_UART_Receive(&huart6, uart6_recv_buf, 1, 3) != HAL_OK ) {}
	        nmea_message[i] = uart6_recv_buf[0];
		    i++;
      }
      i = 0;

      //вычисляем контрольную сумму пакета и сверяем её с указанной в конце пакета
      if (pc_gps_parce_nmea(pc, nmea_message, sizeof(nmea_message)/sizeof(nmea_message[0])) != 0){
    	 char error_text[] = "checksum error!\r\n";
         HAL_UART_Transmit(&huart3, (uint8_t*)error_text, strlen((char*)error_text), 3);
      } else {
         char match_text[] = "checksum match!\r\n";
         HAL_UART_Transmit(&huart3, (uint8_t*)match_text, strlen((char*)match_text), 3);
         retval = 0;
      }

      char text_devide[] = "\r\n\r\n";
      HAL_UART_Transmit(&huart3, (uint8_t*)text_devide, strlen((char*)text_devide), 3);

      for (int j = 0; j < (sizeof(nmea_message)/sizeof(nmea_message[0])); j++){
         nmea_message[j] = 0;
      }
   }
   return (retval);
}

#define HMC5883L_ADDRESS (0x3D)

int pc_compass_check_addr() {

   //HAL_I2C_Master_Receive
   return(0);
}




uint8_t HMC5883L_IsReady(void) {
    // Проверка занятости шины
    if(LL_I2C_IsActiveFlag_BUSY(I2C1)) {
        return 0;
    }

    // Генерация START условия
    LL_I2C_GenerateStartCondition(I2C1);

    // Ожидание флага SB
    uint32_t timeout = 10000;
    while(!LL_I2C_IsActiveFlag_SB(I2C1)) {
        if(--timeout == 0) return 0;
    }

    // Отправка адреса устройства
    LL_I2C_TransmitData8(I2C1, HMC5883L_ADDRESS);

    // Ожидание флага ADDR или AF
    timeout = 10000;
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1) && !LL_I2C_IsActiveFlag_AF(I2C1)) {
        if(--timeout == 0) return 0;
    }

    // Проверка подтверждения
    uint8_t result = LL_I2C_IsActiveFlag_ADDR(I2C1);

    // Генерация STOP условия
    LL_I2C_GenerateStopCondition(I2C1);

    // Очистка флага ADDR
    if(result) {
        LL_I2C_ClearFlag_ADDR(I2C1);
    }

    return result;
}
