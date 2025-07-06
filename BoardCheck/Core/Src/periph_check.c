//файл содержит функции проверки периферии платы

#include "periph_check.h"
#include "main.h"
#include "json.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <stdlib.h>
#include <math.h>

#define CNT_GYRO_CALIBR (10)
#define DEVIATION (1.1)
#define A_X_REF (0.0)
#define A_Y_REF (0.0)
#define A_Z_REF (0.0)
#define G_X_REF (0.0)
#define G_Y_REF (0.0)
#define G_Z_REF (0.0)

#define CNT_COMPASS_CALIBR (10)
#define MAGN_X_REF (0)
#define MAGN_Y_REF (0)
#define MAGN_Z_REF (0)

//#include "stm32f4xx_hal_uart.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

int pc_init(struct periph_check* pc)
{
	return 0;
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

// в зависимости от position проверяем состояние переключателей
int switch_check(struct periph_check* pc) {
	GPIO_PinState state = HAL_GPIO_ReadPin(GPIOE, KN_0_Pin) + HAL_GPIO_ReadPin(GPIOE, KN_1_Pin) + HAL_GPIO_ReadPin(GPIOE, KN_2_Pin) + HAL_GPIO_ReadPin(GPIOE, KN_3_Pin);

	if ((pc->position == GPIO_PIN_SET) && (state == 4)) {
			return 0;
		}
	else if ((pc->position == GPIO_PIN_RESET) && (state == 0)) {
		return 0;
	}
	else {
		return -1;
	}
}

void relay_check() {
	HAL_GPIO_WritePin(GPIOA, R1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, R2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, R3_Pin, GPIO_PIN_RESET);

	// отправляем json {"relay", "checking", "indication"}
//	while(1)
//	{
//		HAL_GPIO_TogglePin(GPIOA, R1_Pin);
//		HAL_GPIO_TogglePin(GPIOA, R2_Pin);
//		HAL_GPIO_TogglePin(GPIOA, R3_Pin);
//		HAL_Delay(100);
//
//		// если приходит json {"relay", "checking", "stop indication"}, то останавливаем
//	}
	//отправляем json {"relay", "valid", "relay valid"}
}

int gsm_check(struct periph_check* pc, struct JsonData* jd) {
	char *cmd;
	uint8_t len;

	// Отключаем эхо
	cmd = "ATE0\r\n";
	memset(pc->uart2_recv_buf, 0, sizeof(pc->uart2_recv_buf));
	HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 1000);
	HAL_UART_Receive(&huart2, pc->uart2_recv_buf, 10, 1000);

	if (strstr((char*)pc->uart2_recv_buf, "OK") == NULL) {
	        strncpy(jd->status, "error", MAX_STATUS_LEN - 1);
	        strncpy(jd->message, "ATE0 no OK", MAX_MESSAGE_LEN - 1);
	        return -1;
	    }

	memset(pc->uart2_recv_buf, 0, sizeof(pc->uart2_recv_buf));

	cmd = "AT\r\n";
	len = strlen(cmd);
	HAL_UART_Transmit(&huart2, (uint8_t*)cmd, len, 1000);
	HAL_UART_Receive(&huart2, pc->uart2_recv_buf, 10, 1000);
	if (strstr((char*)pc->uart2_recv_buf, "OK") == NULL) {
	        strncpy(jd->status, "error", MAX_STATUS_LEN - 1);
	        strncpy(jd->message, "AT no OK", MAX_MESSAGE_LEN - 1);
	        return -1;
	    }

	memset(pc->uart2_recv_buf, 0, sizeof(pc->uart2_recv_buf));
	// Ожидаем OK

	// Получаем инфо о модуле
	cmd = "ATI\r\n";
	len = strlen(cmd);
	HAL_UART_Transmit(&huart2, (uint8_t*)cmd, len, 1000);
	HAL_UART_Receive(&huart2, pc->uart2_recv_buf, 100, 1000);

	if (strstr((char*)pc->uart2_recv_buf, "OK") == NULL) {
	        strncpy(jd->status, "error", MAX_STATUS_LEN - 1);
	        strncpy(jd->message, "ATI no OK", MAX_MESSAGE_LEN - 1);
	        return -1;
	    }

	memset(pc->uart2_recv_buf, 0, sizeof(pc->uart2_recv_buf));

	return 0;
}

////////////////////////////////////////////////////// COMPASS //////////////////////////////////////////////////////////////////////

int compass_calibr_pos1(struct periph_check* pc){

	uint8_t reg = 0x00;
	int32_t sum_magn_x = 0, sum_magn_y = 0, sum_magn_z = 0;
	uint8_t qmc_raw[6];

	for (int i = 0; i < CNT_COMPASS_CALIBR; i++){
		HAL_I2C_Master_Transmit(&hi2c1, 0x0D << 1, &reg, 1, HAL_MAX_DELAY);
		HAL_I2C_Master_Receive(&hi2c1, 0x0D << 1, qmc_raw, 6, HAL_MAX_DELAY);
		sum_magn_x += (int16_t)((qmc_raw[1] << 8) | qmc_raw[0]);
		sum_magn_y += (int16_t)((qmc_raw[3] << 8) | qmc_raw[2]);
		sum_magn_z += (int16_t)((qmc_raw[5] << 8) | qmc_raw[4]);
	}
	pc->magn_x_calibr = sum_magn_x/CNT_COMPASS_CALIBR;
	pc->magn_y_calibr = sum_magn_y/CNT_COMPASS_CALIBR;
	pc->magn_z_calibr = sum_magn_z/CNT_COMPASS_CALIBR;

	/*
	char uart3_buf[64] = {0};
	sprintf(uart3_buf,
	"/r/n compass_calibr_pos1 \r\n"
	"magn_x_calibr = %d\r\n"
	"magn_y_calibr = %d\r\n"
	"magn_z_calibr = %d\r\n"
	pc->magn_x_calibr,
	pc->magn_y_calibr,
	pc->magn_z_calibr);
	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	*/

//проверка
	if ((pc->magn_x_calibr == 0) && (pc->magn_y_calibr == 0) && (pc->magn_z_calibr == 0)) {
		return (-1);
	}
	return (0);
}

int compass_calibr_pos2(struct periph_check* pc){

	uint8_t reg = 0x00;
	int32_t sum_magn_x = 0, sum_magn_y = 0, sum_magn_z = 0;
	uint8_t qmc_raw[6];

	for (int i = 0; i < CNT_COMPASS_CALIBR; i++){

		HAL_I2C_Master_Transmit(&hi2c1, 0x0D << 1, &reg, 1, HAL_MAX_DELAY);
		HAL_I2C_Master_Receive(&hi2c1, 0x0D << 1, qmc_raw, 6, HAL_MAX_DELAY);

		sum_magn_x += (int16_t)((qmc_raw[1] << 8) | qmc_raw[0]);
		sum_magn_y += (int16_t)((qmc_raw[3] << 8) | qmc_raw[2]);
		sum_magn_z += (int16_t)((qmc_raw[5] << 8) | qmc_raw[4]);
	}

	pc->magn_x_calibr = (pc->magn_x_calibr + sum_magn_x)/2;
	pc->magn_y_calibr = (pc->magn_y_calibr + sum_magn_y)/2;
	pc->magn_z_calibr = (pc->magn_z_calibr + sum_magn_z)/2;
	/*

	sprintf(uart3_buf,
	"/r/n compass_calibr_pos2 \r\n"
	"magn_x_calibr = %d\r\n"
	"magn_y_calibr = %d\r\n"
	"magn_z_calibr = %d\r\n"
	pc->magn_x_calibr,
	pc->magn_y_calibr,
	pc->magn_z_calibr);
	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	 */

	//проверка
	if ((pc->magn_x_calibr == 0) && (pc->magn_y_calibr == 0) && (pc->magn_z_calibr == 0)) {
		return (-1);
	}

	return (0);
}

int compass_check_pos1(struct periph_check* pc){

	int retval = -1;
	uint8_t qmc_raw[6] = {0};
	int32_t sum_magn_x = 0, sum_magn_y = 0, sum_magn_z = 0;

	for (int i = 0; i < CNT_COMPASS_CALIBR; i++){

		MPU6050_Read_All(&hi2c1, &(pc->MPU6050));
		sum_magn_x += (int16_t)((qmc_raw[1] << 8) | qmc_raw[0]);
		sum_magn_y += (int16_t)((qmc_raw[3] << 8) | qmc_raw[2]);
		sum_magn_z += (int16_t)((qmc_raw[5] << 8) | qmc_raw[4]);
	}

	sum_magn_x = sum_magn_x/CNT_COMPASS_CALIBR;
	sum_magn_y = sum_magn_y/CNT_COMPASS_CALIBR;
	sum_magn_z = sum_magn_z/CNT_COMPASS_CALIBR;

	/*
	sprintf(uart3_buf,
	"/r/n compass_check_pos1 \r\n"
	"sum_magn_x = %d\r\n"
	"sum_magn_y = %d\r\n"
	"sum_magn_z = %d\r\n"
	sum_magn_x,
	sum_magn_y,
	sum_magn_z);
	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	*/

	//если величина a по оси x не выходит за пределы
	if (labs(sum_magn_x - pc->magn_x_calibr) < (MAGN_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси y не выходит за пределы
	if (labs(sum_magn_y - pc->magn_y_calibr) < (MAGN_Y_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси z не выходит за пределы
	if (labs(sum_magn_z - pc->magn_z_calibr) < (MAGN_Z_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	return (retval);
}

int compass_check_pos2(struct periph_check* pc)
{
	int retval = -1;
	uint8_t qmc_raw[6] = {0};
	int32_t sum_magn_x = 0, sum_magn_y = 0, sum_magn_z = 0;

	for (int i = 0; i < CNT_COMPASS_CALIBR; i++){

		MPU6050_Read_All(&hi2c1, &(pc->MPU6050));
		sum_magn_x += (int16_t)((qmc_raw[1] << 8) | qmc_raw[0]);
		sum_magn_y += (int16_t)((qmc_raw[3] << 8) | qmc_raw[2]);
		sum_magn_z += (int16_t)((qmc_raw[5] << 8) | qmc_raw[4]);
	}

	sum_magn_x = sum_magn_x/CNT_COMPASS_CALIBR;
	sum_magn_y = sum_magn_y/CNT_COMPASS_CALIBR;
	sum_magn_z = sum_magn_z/CNT_COMPASS_CALIBR;

	/*
	sprintf(uart3_buf,
	"/r/n compass_check_pos2 \r\n"
	"sum_magn_x = %d\r\n"
	"sum_magn_y = %d\r\n"
	"sum_magn_z = %d\r\n"
	sum_magn_x,
	sum_magn_y,
	sum_magn_z);
	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	 */

	//если величина a по оси x не выходит за пределы
	if (labs(sum_magn_x - pc->magn_x_calibr) < (MAGN_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси y не выходит за пределы
	if (labs(sum_magn_y - pc->magn_y_calibr) < (MAGN_Y_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси z не выходит за пределы
		if (labs(sum_magn_z - pc->magn_z_calibr) < (MAGN_Z_REF + DEVIATION)){
			retval = (0);
		} else {
			retval = (-1);
		}
			return (retval);
		}

////////////////////////////////////////////////////// GYRO //////////////////////////////////////////////////////////////////////

int gyro_calibr_pos1(struct periph_check* pc)
{
	MPU6050_t* mpu6050 = &(pc->MPU6050);
	double sum_a_x = 0, sum_a_y = 0, sum_a_z = 0;
	double sum_g_x = 0, sum_g_y = 0, sum_g_z = 0;

	for (int i = 0; i < CNT_GYRO_CALIBR; i++){
		MPU6050_Read_All(&hi2c1, &(pc->MPU6050));
		sum_a_x += mpu6050->Ax;
		sum_a_y += mpu6050->Ay;
		sum_a_z += mpu6050->Az;
		sum_g_x += mpu6050->Gx;
		sum_g_y += mpu6050->Gy;
		sum_g_z += mpu6050->Gz;
	}

	pc->a_x_calibr = sum_a_x/CNT_GYRO_CALIBR;
	pc->a_y_calibr = sum_a_y/CNT_GYRO_CALIBR;
	pc->a_z_calibr = sum_a_z/CNT_GYRO_CALIBR;

	pc->g_x_calibr = sum_g_x/CNT_GYRO_CALIBR;
	pc->g_y_calibr = sum_g_y/CNT_GYRO_CALIBR;
	pc->g_z_calibr = sum_g_z/CNT_GYRO_CALIBR;

	/*
	char uart3_buf[64] = {0};
	sprintf(uart3_buf,
	"/r/n gyro_calibr_pos1 \r\n"
	"a_x_calibr = %lf\r\n"
	"a_y_calibr = %lf\r\n"
	"a_z_calibr = %lf\r\n"
	"g_x_calibr = %lf\r\n"
	"g_y_calibr = %lf\r\n"
	"g_z_calibr = %lf\r\n",
	pc->a_x_calibr,
	pc->a_y_calibr,
	pc->a_z_calibr,
	pc->g_x_calibr,
	pc->g_y_calibr,
	pc->g_z_calibr);
	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	*/

	//проверка только по осям x и z
	if ((pc->a_x_calibr == 0) && (pc->a_z_calibr == 0) && (pc->g_x_calibr == 0) && (pc->g_z_calibr == 0)) {
		return (-1);
	}
	return (0);
}

int gyro_calibr_pos2(struct periph_check* pc){

	MPU6050_t* mpu6050 = &(pc->MPU6050);
	double sum_a_x = 0, sum_a_y = 0, sum_a_z = 0;
	double sum_g_x = 0, sum_g_y = 0, sum_g_z = 0;

	for (int i = 0; i < CNT_GYRO_CALIBR; i++){
		MPU6050_Read_All(&hi2c1, &(pc->MPU6050));
		sum_a_x += mpu6050->Ax;
		sum_a_y += mpu6050->Ay;
		sum_a_z += mpu6050->Az;
		sum_g_x += mpu6050->Gx;
		sum_g_y += mpu6050->Gy;
		sum_g_z += mpu6050->Gz;
	}

	sum_a_x = sum_a_x/CNT_GYRO_CALIBR;
	sum_a_y = sum_a_y/CNT_GYRO_CALIBR;
	sum_a_z = sum_a_z/CNT_GYRO_CALIBR;

	sum_g_x = sum_g_x/CNT_GYRO_CALIBR;
	sum_g_y = sum_g_y/CNT_GYRO_CALIBR;
	sum_g_z = sum_g_z/CNT_GYRO_CALIBR;

	pc->a_x_calibr = (pc->a_x_calibr + sum_a_x)/2;
	pc->a_y_calibr = (pc->a_y_calibr + sum_a_y)/2;
	pc->a_z_calibr = (pc->a_z_calibr + sum_a_z)/2;

	pc->g_x_calibr = (pc->g_x_calibr + sum_g_x)/2;
	pc->g_y_calibr = (pc->g_y_calibr + sum_g_y)/2;
	pc->g_z_calibr = (pc->g_z_calibr + sum_g_z)/2;

	/*
	char uart3_buf[64] = {0};
	sprintf(uart3_buf,
	"/r/n gyro_calibr_pos2 \r\n"
	"a_x_calibr = %lf\r\n"
	"a_y_calibr = %lf\r\n"
	"a_z_calibr = %lf\r\n"
	"g_x_calibr = %lf\r\n"
	"g_y_calibr = %lf\r\n"
	"g_z_calibr = %lf\r\n",
	pc->a_x_calibr,
	pc->a_y_calibr,
	pc->a_z_calibr,
	pc->g_x_calibr,
	pc->g_y_calibr,
	pc->g_z_calibr);

	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	*/

	//проверка только по осям x и z
	if ((pc->a_x_calibr == 0) && (pc->a_z_calibr == 0) && (pc->g_x_calibr == 0) && (pc->g_z_calibr == 0)) {
		return (-1);
	}
		return (0);
	}

int gyro_check_pos1(struct periph_check* pc){
	int retval = -1;
	MPU6050_t* mpu6050 = &(pc->MPU6050);

	double sum_a_x = 0, sum_a_y = 0, sum_a_z = 0;
	double sum_g_x = 0, sum_g_y = 0, sum_g_z = 0;

	for (int i = 0; i < CNT_GYRO_CALIBR; i++){
		MPU6050_Read_All(&hi2c1, &(pc->MPU6050));

		sum_a_x += mpu6050->Ax;
		sum_a_y += mpu6050->Ay;
		sum_a_z += mpu6050->Az;

		sum_g_x += mpu6050->Gx;
		sum_g_y += mpu6050->Gy;
		sum_g_z += mpu6050->Gz;
	}

	sum_a_x = sum_a_x/CNT_GYRO_CALIBR;
	sum_a_y = sum_a_y/CNT_GYRO_CALIBR;
	sum_a_z = sum_a_z/CNT_GYRO_CALIBR;

	sum_g_x = sum_g_x/CNT_GYRO_CALIBR;
	sum_g_y = sum_g_y/CNT_GYRO_CALIBR;
	sum_g_z = sum_g_z/CNT_GYRO_CALIBR;

	/*
	char uart3_buf[64] = {0};
	sprintf(uart3_buf,
	"/r/n gyro_check_pos1 \r\n"
	"sum_a_x = %lf\r\n"
	"sum_a_y = %lf\r\n"
	"sum_a_z = %lf\r\n"
	"sum_g_x = %lf\r\n"
	"sum_g_y = %lf\r\n"
	"sum_g_z = %lf\r\n",
	sum_a_x,
	sum_a_y,
	sum_a_z,
	sum_g_x,
	sum_g_y,
	sum_g_z);
	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	*/

	//если величина a по оси x не выходит за пределы
	if (fabs(sum_a_x - pc->a_x_calibr) < (A_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси y не выходит за пределы
	if (fabs(sum_a_y - pc->a_y_calibr) < (A_Y_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси z не выходит за пределы
	if (fabs(sum_a_z - pc->a_z_calibr) < (A_Z_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина g по оси x не выходит за пределы
	if (fabs(sum_g_x - pc->g_x_calibr) < (G_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина g по оси y не выходит за пределы
	if (fabs(sum_g_y - pc->g_y_calibr) < (G_Y_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина g по оси z не выходит за пределы
	if (fabs(sum_g_z - pc->g_z_calibr) < (G_Z_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	return (retval);
}

int gyro_check_pos2(struct periph_check* pc){

	int retval = -1;
	MPU6050_t* mpu6050 = &(pc->MPU6050);
	double sum_a_x = 0, sum_a_y = 0, sum_a_z = 0;
	double sum_g_x = 0, sum_g_y = 0, sum_g_z = 0;

	for (int i = 0; i < CNT_GYRO_CALIBR; i++){

		MPU6050_Read_All(&hi2c1, &(pc->MPU6050));

		sum_a_x += mpu6050->Ax;
		sum_a_y += mpu6050->Ay;
		sum_a_z += mpu6050->Az;
		sum_g_x += mpu6050->Gx;
		sum_g_y += mpu6050->Gy;
		sum_g_z += mpu6050->Gz;
	}

	sum_a_x = sum_a_x/CNT_GYRO_CALIBR;
	sum_a_y = sum_a_y/CNT_GYRO_CALIBR;
	sum_a_z = sum_a_z/CNT_GYRO_CALIBR;
	sum_g_x = sum_g_x/CNT_GYRO_CALIBR;
	sum_g_y = sum_g_y/CNT_GYRO_CALIBR;
	sum_g_z = sum_g_z/CNT_GYRO_CALIBR;

	/*
	char uart3_buf[64] = {0};
	sprintf(uart3_buf,
	"/r/n gyro_check_pos1 \r\n"
	"sum_a_x = %lf\r\n"
	"sum_a_y = %lf\r\n"
	"sum_a_z = %lf\r\n"
	"sum_g_x = %lf\r\n"
	"sum_g_y = %lf\r\n"
	"sum_g_z = %lf\r\n",
	sum_a_x,
	sum_a_y,
	sum_a_z,
	sum_g_x,
	sum_g_y,
	sum_g_z);
	HAL_UART_Transmit(&huart3, (uint8_t*)uart3_buf, strlen(uart3_buf), 3);
	*/

	//если величина a по оси x не выходит за пределы
	if (fabs(sum_a_x - pc->a_x_calibr) < (A_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси y не выходит за пределы
	if (fabs(sum_a_y - pc->a_y_calibr) < (A_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина a по оси z не выходит за пределы
	if (fabs(sum_a_z - pc->a_z_calibr) < (A_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина g по оси x не выходит за пределы
	if (fabs(sum_g_x - pc->g_x_calibr) < (A_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина g по оси y не выходит за пределы
	if (fabs(sum_g_y - pc->g_y_calibr) < (A_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	//если величина g по оси z не выходит за пределы
	if (fabs(sum_g_z - pc->g_z_calibr) < (A_X_REF + DEVIATION)){
		retval = (0);
	} else {
		retval = (-1);
	}

	return (retval);
}
