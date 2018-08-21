/*
 * memtest.h
 *
 *  Created on: 2018/07/11
 *      Author: e.sitnikov
 */

#ifndef MEMTEST_H_
#define MEMTEST_H_


uint32_t memtest_fill(const uint32_t seed, uint32_t *ptr, const uint32_t size);
typedef uint32_t (*tMEMSETfunc)(void *block, int c, size_t size);

uint32_t memset_32bit_x1(void *block, int c, size_t size);
uint32_t memset_32bit_x8(void *block, int c, size_t size);
uint32_t memset_original(void *block, int c, size_t size);

uint32_t memread_32bit_x1(void *block, int c, size_t size);
uint32_t memread_32bit_x8(void *block, int c, size_t size);

void mem_speed_wr(const char *title, tMEMSETfunc func, void *data, const uint32_t size);
void mem_crc_test(const char *title, void *data, const uint32_t size);

extern uint32_t * const ptr_SDRAM;
extern uint32_t * const ptr_MCURAM;
#endif /* MEMTEST_H_ */
