/*
 * FSK_modem_test.c
 *
 *  Created on: 2018/07/06
 *      Author: e.sitnikov
 */

#include "stm32kiss.h"
#include "FSK_modem.h"

uint32_t sample_count = 0;

void modem_func()
{
#ifdef disabled
	for (uint32_t t=0; t < 7; t++)
	{
		sample_count++;
		if (sample_count == 128)
			FSK_modem_send_str("Testing BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG very SYPER big STRING");

		uint16_t sample = 0x8000 + FSK_modem_dac()/2;
		//sample += rand() % 0x400;
		//printf("%i\n", sample);
		FSK_modem_adc(sample);
	}
#else
	uint16_t sample[8];
	for (uint32_t t=0; t < 7; t++)
	{
		sample_count++;
		if (sample_count == 128)
			FSK_modem_send_str("Testing BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG very SYPER big STRING");

		sample[t] = 0x8000 + FSK_modem_dac()/2;
	}

	for (uint32_t t=0; t < 7; t++)
		FSK_modem_adc(sample[t]);
#endif
}

int FSK_modem_test()
{
	FSK_modem_init();
	FSK_modem_wait_func = modem_func;
	fsk_modem_debug_mode = true;

	//RUN_TIME_MEASURE_START();
	uint32_t avrg_sum = 0;
	uint32_t avrg_cnt = 0;

	for (uint32_t t=0; t < 100; t++)
	{
		uint64_t time = DWT_CYCCNT;
		sample_count = 0;
		FSK_modem_send_wait(1000, true);
		FSK_modem_recive_start_wait(1000, true);
		FSK_modem_recive_wait_end(1000, true, true);
		time = (DWT_CYCCNT - time);

		if (fsk_modem_recived_normal)
		{
			avrg_sum += time;
			avrg_cnt += sample_count;
		}

		printf("%i\t%i\t%lu\t", fsk_modem_stat_packet_ok, fsk_modem_recived_error, sample_count);
		//printf((const char *)fsk_modem_recive_data);
		printf("\tTime\t%i\n", (int)(time / sample_count));
	}
	printf("\t%lu\r\n", avrg_sum / avrg_cnt);
	printf("\r\n");
	//RUN_TIME_MEASURE_END("time:\t", 0);

	//cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	return 0;
}
