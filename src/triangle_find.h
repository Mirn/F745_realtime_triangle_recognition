/*
 * triangle_find.h
 *
 *  Created on: 2018/08/02
 *      Author: e.sitnikov
 */

#ifndef TRIANGLE_FIND_H_
#define TRIANGLE_FIND_H_

#include <stdint.h>
#include <stdbool.h>

//#define STATRD
//#define CNT_CALC
#define __BITFIELDFILL

typedef struct {
	uint32_t cnt;
	uint16_t avrgx;
	uint16_t avrgy;
	uint16_t maxx;
	uint16_t maxy;
	uint16_t minx;
	uint16_t miny;
} tFillStat;

#ifdef STATRD
extern uint8_t *stat_read;
extern bool stat_show;
extern bool stat_filter;
#endif

#ifdef CNT_CALC
extern uint32_t cnt_errmark;
extern uint32_t cnt_itermax;
extern uint32_t cnt_itercnt;
extern uint32_t cnt_pixcnt;
extern uint32_t cnt_fifomax;

extern uint32_t total_itermax;
extern uint32_t total_itercnt;
extern uint32_t total_pixcnt;
extern uint32_t total_fifomax;
#endif


typedef void (*tStat_Func)(const tFillStat * stat, void *other);

typedef struct {
	int16_t x;
	int16_t y;
} tTriangleDot;

typedef struct {
	tTriangleDot a;
	tTriangleDot b;
	tTriangleDot c;
} tTriangle;


#ifdef __cplusplus
extern "C" {
#endif

void fillstat_reset();
void fillstat_setup(uint8_t *base, uint32_t step);
const tFillStat * fillstat_calc();

void fill(uint8_t * const img, const int32_t step, int iteration);

uint32_t FindConnectedComponentsWithStats(uint8_t * img, const uint32_t step_x, const uint32_t height, const tStat_Func func, void * const other);
bool triangle_find(uint8_t * const img, const uint32_t step_x, const uint32_t height, tTriangle *res);

#ifdef __cplusplus
}
#endif

#endif /* TRIANGLE_FIND_H_ */
