#include "stm32kiss.h"
#include "img_data.h"
#include "img_test.h"

uint8_t result[56000]  __attribute__ ((section (".data_big"), used));;
uint8_t abin[56000]  __attribute__ ((section (".data_big"), used));;
uint8_t bbin[56000]  __attribute__ ((section (".data_big"), used));;
uint8_t *mask = result;

//#define bitimg ((tBITimg *)(((uint32_t)(&bbin)) + sizeof(tBITimg)*0))
//#define img2   ((tBITimg *)(((uint32_t)(&bbin)) + sizeof(tBITimg)*1))
//#define imgmsk ((tBITimg *)(((uint32_t)(&bbin)) + sizeof(tBITimg)*2))
tBITimg bitimg = {0};
tBITimg img2 = {0};
tBITimg imgmsk = {0};

//uint32_t crc_calc(void *ptr, size_t size)
//{
//	CRC_ResetDR();
//	return CRC_CalcBlockCRC(ptr, size / 4);
//}

void sad_test(const char *title, tSADfunc func)
{
	memset(result, 0, imgFrame_bmp_len);

	uint32_t t = DWT_CYCCNT;
	(*func)(abin, bbin, result, imgFrame_bmp_len);
	t = DWT_CYCCNT - t;
	printf("%s\t%lu\tTicks\n", title, t);
	printf("%s\t%5.2f\tTicks/Pixel \n", title, 1.0 * t / imgFrame_bmp_len);
	printf("%s\t%lu\tuSeconds\n", title, t / (SystemCoreClock / 1000000));
	printf("%s CRC\t%8lX\n", title, crc_calc(result, imgFrame_bmp_len));
	printf("\n");
}

void mono_test(const char *title, tMONOfunc func)
{
	memset(result, 0, imgFrame_bmp_len);

	uint32_t t = DWT_CYCCNT;
	(*func)(abin, result, imgFrame_bmp_len);
	t = DWT_CYCCNT - t;
	printf("%s\t%lu\tTicks\n", title, t);
	printf("%s\t%5.2f\tTicks/Pixel \n", title, 1.0 * t / imgFrame_bmp_len);
	printf("%s\t%lu\tuSeconds\n", title, t / (SystemCoreClock / 1000000));
	printf("%s CRC\t%8lX\n", title, crc_calc(result, imgFrame_bmp_len));
	printf("\n");
}

void intrusion_test(const char *title, tIntrusionFunc func, bool ignore_mask, bool rand_mask)
{
	memcpy(abin, imgFrame_bmp, imgFrame_bmp_len);
	memcpy(bbin, imgBack_bmp, imgBack_bmp_len);

	if (ignore_mask)
		memset(mask, 0xFF, mask_bmp_len);
	else
		for (uint32_t i = 0; i < mask_bmp_len; i++)
			mask[i] = ~mask_bmp[i];

	if (rand_mask)
	{
		srand(0x1223991);
		for (uint32_t i = 0; i < mask_bmp_len; i++)
		{
			abin[i] = rand();
			bbin[i] = rand();
			mask[i] ^= rand();
		}
	}

	//printf("intrusion_test\n");
	uint32_t t = DWT_CYCCNT;
	uint32_t value = (*func)(abin, bbin, mask, imgFrame_bmp_len);
	t = DWT_CYCCNT - t;
	float t_ticks = 1.0 * t / imgFrame_bmp_len;
	uint32_t t_uSec = t / (SystemCoreClock / 1000000);
#ifdef repfull
	printf("%s return:\t%lu\n", title, value);
	printf("%s    \t%lu\tTicks\n", title, t);
	printf("%s    \t%5.2f\tTicks/Pixel \n", title, t_ticks);
	printf("%s    \t%lu\tuSeconds\n", title, t_uSec);
	printf("\n");
#else
	printf("%s    \t%lu\t%5.2f\t%lu\n", title, value, t_ticks, t_uSec);
#endif
	//printf("%s CRC\t%8lX\n", title, crc_calc(result, imgFrame_bmp_len));
}

#define TIME_MEASURE_START() { uint32_t t = DWT_CYCCNT;
#define TIME_MEASURE_END(msg) \
	t = DWT_CYCCNT - t; \
	float t_ticks = 1.0 * t / imgFrame_bmp_len; \
	uint32_t t_uSec = t / (SystemCoreClock / 1000000);\
	printf("TIME: %s    \t%5.2f\tTicks/Pixel\t%lu\tuSec\n", msg, t_ticks, t_uSec); \
}

void img_test_main()
{
	printf("test\timgFrame_bmp_len\t%lu\n", imgFrame_bmp_len);
	memcpy(abin, imgFrame_bmp, imgFrame_bmp_len);
	printf("test1\n");
	memcpy(bbin, imgBack_bmp, imgBack_bmp_len);
	printf("test2\n");

	sad_test("sad_simple", sad_simple);
	sad_test("sad_block4", sad_block4);
	sad_test("sad_simd4 ", sad_simd4);
	sad_test("sad_simd16", sad_simd16);

//	memcpy(result, imgBack_bmp, HEADER_SHIFT);
//	send_file("sad.bmp", result, imgFrame_bmp_len);

	///////////////////////////////////////////////////////

	memcpy(abin, result, imgFrame_bmp_len);

	mono_test("mono_simple", mono_simple);
	mono_test("mono_block4", mono_block4);
	mono_test("mono_simd4 ", mono_simd4);
	mono_test("mono_simd16", mono_simd16);

//	memcpy(result, imgBack_bmp, HEADER_SHIFT);
//	send_file("mono.bmp", result, imgFrame_bmp_len);

	///////////////////////////////////////////////////////

	memset(bitimg, 0, sizeof(tBITimg));
	memcpy(abin, result, imgFrame_bmp_len);

	TIME_MEASURE_START();
	bitimg_build(abin + HEADER_SHIFT, &bitimg);
	TIME_MEASURE_END("bitimg_build");
	printf("bitimg_build CRC\t%8lX\n", crc_calc(bitimg,  sizeof(tBITimg) - 4));

//	send_file_header("bitimg.bmp", imgFrame_bmp_len);
//	send_block(imgBack_bmp, HEADER_SHIFT);
//	bitimg_send(&bitimg);

	///////////////////////////////////////////////////////

	memset(bitimg, 0, sizeof(tBITimg));
	memcpy(abin, imgFrame_bmp, imgFrame_bmp_len);
	memcpy(bbin, imgBack_bmp, imgBack_bmp_len);

	//for (int y=1; y < 50; y++)
	//	memset(abin + (IMG_X * y) + 300 + HEADER_SHIFT - y, 0xFF, y * 2);

	TIME_MEASURE_START();
	bitimg_absdiff_mono(abin + HEADER_SHIFT, bbin + HEADER_SHIFT, &bitimg);
	TIME_MEASURE_END("absdiff_mono");
	printf("absdiff_mono CRC\t%8lX\n", crc_calc(bitimg,  sizeof(tBITimg) - 4));

//	send_file_header("bitimg_simd32.bmp", imgFrame_bmp_len);
//	send_block(imgBack_bmp, HEADER_SHIFT);
//	bitimg_send(&bitimg);

	///////////////////////////////////////////////////////

	memset(img2, 0, sizeof(tBITimg));

	TIME_MEASURE_START();
	bitimg_erode(&bitimg, &img2);
	TIME_MEASURE_END("bitimg_erode");

//	send_file_header("bitimg_erode.bmp", imgFrame_bmp_len);
//	send_block(imgBack_bmp, HEADER_SHIFT);
//	//bitimg_send(bitimg);
//	bitimg_send(img2);

	///////////////////////////////////////////////////////

	memset(bitimg, 0, sizeof(tBITimg));
	bitimg_build(mask_bmp + HEADER_SHIFT, &bitimg);

//	send_file_header("mask.bmp", imgFrame_bmp_len);
//	send_block(imgBack_bmp, HEADER_SHIFT);
//	bitimg_send(bitimg);

	uint32_t bitcnt = 0;
	TIME_MEASURE_START();
	bitcnt = bitimg_mask_bitcnt(&img2, &bitimg);
	TIME_MEASURE_END("bitimg_mask_bitcnt");

	printf("bitimg_mask_bitcnt return:\t%lu\n", bitcnt);


	///////////////////////////////////////////////////////

//	memcpy(abin, imgFrame_bmp, imgFrame_bmp_len);
//	memcpy(bbin, imgBack_bmp, imgBack_bmp_len);
//
//	intrusion_test("intrusion_simple", intrusion_simple, false, false);
//	intrusion_test("intrusion_simple", intrusion_simple, true, false);
//	intrusion_test("intrusion_simple", intrusion_simple, true, true);
//
//	intrusion_test("intrusion_block4", intrusion_block4, false, false);
//	intrusion_test("intrusion_block4", intrusion_block4, true, false);
//	intrusion_test("intrusion_block4", intrusion_block4, true, true);
//
//	intrusion_test("intrusion_simd4 ", intrusion_simd4, false, false);
//	intrusion_test("intrusion_simd4 ", intrusion_simd4, true, false);
//	intrusion_test("intrusion_simd4 ", intrusion_simd4, true, true);
//
//	intrusion_test("intrusion_simd16", intrusion_simd16, false, false);
//	intrusion_test("intrusion_simd16", intrusion_simd16, true, false);
//	intrusion_test("intrusion_simd16", intrusion_simd16, true, true);

	printf("done\n");
}
