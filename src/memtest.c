/*
 * memtest.c
 *
 *  Created on: 2018/07/11
 *      Author: e.sitnikov
 */

#include "stm32kiss.h"


uint32_t memtest_fill(uint32_t seed, uint32_t *ptr, const uint32_t size)
{
	//srand(seed);
	CRC->CR |= CRC_CR_RESET;

	uint32_t count = size / 4;
	while (count >= 8)
	{
		CRC->DR = ptr[0] = seed++;//rand();
		CRC->DR = ptr[1] = seed++;//rand();
		CRC->DR = ptr[2] = seed++;//rand();
		CRC->DR = ptr[3] = seed++;//rand();
		CRC->DR = ptr[4] = seed++;//rand();
		CRC->DR = ptr[5] = seed++;//rand();
		CRC->DR = ptr[6] = seed++;//rand();
		CRC->DR = ptr[7] = seed++;//rand();
		ptr += 8;
		count -= 8;
	};
	return CRC->DR;
}

typedef uint32_t (*tMEMSETfunc)(void *block, int c, size_t size);

uint32_t memset_32bit_x1(void *block, int c, size_t size)
{
	size /= 4;
	uint32_t *ptr = block;
	while (size--)
		*(ptr++) = c;
	return 0;
}

uint32_t memset_32bit_x8(void *block, int c, size_t size)
{
	size /= 4;
	uint32_t *ptr = block;
	while (size >= 8)
	{
		*(ptr++) = c;
		*(ptr++) = c;
		*(ptr++) = c;
		*(ptr++) = c;
		*(ptr++) = c;
		*(ptr++) = c;
		*(ptr++) = c;
		*(ptr++) = c;
		size -= 8;
	}
	memset_32bit_x1(ptr, c, size * 4);
	return 0;
}

uint32_t memset_original(void *block, int c, size_t size)
{
	memset(block, c, size);
	return 0;
}

////////////////////////////////////////////////////////////////////

uint32_t memread_32bit_x1(void *block, int c, size_t size)
{
	UNUSED(c);
	size /= 4;
	uint32_t *ptr = block;
	uint32_t result = 0;

	while (size--)
		result += *(ptr++);
	return result;
}

uint32_t memread_32bit_x8(void *block, int c, size_t size)
{
	UNUSED(c);
	size /= 4;
	uint32_t *ptr = block;
	uint32_t result = 0;
	while (size >= 8)
	{
		result += *(ptr++);
		result += *(ptr++);
		result += *(ptr++);
		result += *(ptr++);
		result += *(ptr++);
		result += *(ptr++);
		result += *(ptr++);
		result += *(ptr++);
		size -= 8;
	}
	return result + memread_32bit_x1(ptr, c, size * 4);
}

/////////////////////////////////////////////////////////////////////////////////////////

void mem_speed_wr(const char *title, tMEMSETfunc func, void *data, const uint32_t size)
{
	SCB_CleanDCache();
	//const uint32_t size = 0x20000;
	const int fill = 0x01;

	uint32_t t = DWT_CYCCNT;
	uint32_t result = (*func)(data, fill, size);
	t = DWT_CYCCNT - t;

	printf("%s\t%lu\t%9lu\t%5.2f\n",
			title,
			result,
			t,
			1.0 * t / size);
}

void mem_crc_test(const char *title, void *data, const uint32_t size)
{
	SCB_CleanDCache();

	uint32_t t = DWT_CYCCNT;
	uint32_t sram_norma = memtest_fill(0x1234567, data, size);
	uint32_t sram_real  = crc_calc(data, size);
	t = DWT_CYCCNT - t;

	printf("%s\t%08lX\t%s\t%9lu\t%5.2f\n",
			title,
			sram_norma,
			(sram_norma == sram_real) ? "OK" : "ERROR",
			t,
			1.0 * t / size);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

//uint32_t * const ptr_SDRAM  = (uint32_t *)0xC0000000;
uint32_t * const ptr_SDRAM  = (uint32_t *)0x60000000;
uint32_t * const ptr_MCURAM = (uint32_t *)0x20010000;

void memtest()
{

//	mem_speed_wr("memset_original",  memset_original,  ptr_MCURAM, 0x20000);
//	mem_speed_wr("memset_32bit_x1",  memset_32bit_x1,  ptr_MCURAM, 0x20000);
	mem_speed_wr("memset_32bit_x8",  memset_32bit_x8,  ptr_MCURAM, 0x20000);
//	mem_speed_wr("memread_32bit_x1", memread_32bit_x1, ptr_MCURAM, 0x20000);
	mem_speed_wr("memread_32bit_x8", memread_32bit_x8, ptr_MCURAM, 0x20000);
	printf("\n");

	mem_speed_wr("sdram_set_32bit_x8",  memset_32bit_x8,  ptr_SDRAM, 0x800000);
	mem_speed_wr("sdram_read_32bit_x8", memread_32bit_x8, ptr_SDRAM, 0x800000);
	printf("\n");

	mem_crc_test("mcu_ram", ptr_MCURAM, 0x20000);
	mem_crc_test("sdram", ptr_SDRAM, 0x800000);
	printf("\n");
}

