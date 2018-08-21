/*
 * hw_init.h
 *
 *      Author: Easy
 */

#ifndef HW_INIT_H_
#define HW_INIT_H_

extern UART_HandleTypeDef huart6;
extern CRC_HandleTypeDef hcrc;

void hw_init();

__attribute__ ((long_call))
void itcm_test(uint8_t * const test);
#endif /* HW_INIT_H_ */
