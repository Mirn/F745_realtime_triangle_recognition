/*
 * triangle_find.c
 *
 *  Created on: 2018/08/02
 *      Author: e.sitnikov
 */

#include <string.h>  //for memset
#include <math.h>    //for sqrt
#include <stdlib.h>  //for abs

//#include <stdio.h> //for debug printf

#include "triangle_find.h"

#define UNUSED(expr) (void)(expr)
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

static uint8_t *stat_base = NULL;
static uint32_t stat_step = 0;
static uint32_t stat_sumx = 0;
static uint32_t stat_sumy = 0;
static tFillStat stat = {0};

void fillstat_reset()
{
	stat.cnt   = 0;
	stat_sumx  = 0;
	stat_sumy  = 0;
	stat.avrgx = 0;
	stat.avrgy = 0;
	stat.maxx  = 0;
	stat.maxy  = 0;
	stat.minx  = UINT16_MAX;
	stat.miny  = UINT16_MAX;
}

void fillstat_setup(uint8_t *base, uint32_t step)
{
	stat_base = base;
	stat_step = step;
	fillstat_reset();
}

static void fillstat_pointer(const uint8_t * const ptr_l, const uint8_t * const ptr_r)
{
#ifdef __BITFIELDFILL
	uint32_t l = ((uint32_t)ptr_l);
	uint32_t r = ((uint32_t)ptr_r);
#else
	uint32_t l = ((uint32_t)ptr_l) - ((uint32_t)stat_base);
	uint32_t r = ((uint32_t)ptr_r) - ((uint32_t)stat_base);
#endif
	uint32_t cnt = r - l + 1;

	uint32_t lx = l % stat_step;
	uint32_t  y = l / stat_step;
	uint32_t rx = lx + cnt - 1;//r % stat_step;//

	stat_sumx += ((rx + lx) / 2) * cnt;
	stat_sumy += y * cnt;
	stat.cnt += cnt;

	stat.maxy = MAX(stat.maxy, y);
	stat.miny = MIN(stat.miny, y);

	stat.maxx = MAX(stat.maxx, rx);
	stat.minx = MIN(stat.minx, lx);
}

const tFillStat * fillstat_calc()
{
	if (stat.cnt != 0) stat.avrgx = stat_sumx / stat.cnt; else stat.avrgx = 0;
	if (stat.cnt != 0) stat.avrgy = stat_sumy / stat.cnt; else stat.avrgy = 0;
	return &stat;
}

#ifdef STATRD
uint8_t *stat_read = NULL;
bool stat_show = false;
bool stat_filter = false;
#endif

//uint32_t cnt_scan = 0;

#ifdef __BITFIELDFILL
#define FILL_MARK 1
static inline bool bit_rd(uint8_t *ptr)
{
	uint32_t ofs = ((uint32_t)ptr);
	return (((uint32_t *)stat_base)[ofs / 32] & (1 << (ofs % 32))) != 0;
}

static inline void bit_clr(uint8_t *ptr)
{
	uint32_t ofs = ((uint32_t)ptr);
	(((uint32_t *)stat_base)[ofs / 32] &= (~(1 << (ofs % 32))));
}

static inline bool fill_check(uint8_t *ptr)
{
	return bit_rd(ptr);
}

static inline uint32_t ctz_func(uint32_t v)
{
	return ((v == 0) ? 32 : __builtin_ctz(v));
}

static inline uint32_t clz_func(uint32_t v)
{
	return ((v == 0) ? 32 : __builtin_clz(v));
}
#else
#define FILL_MARK 255
//#define FILL_MARK 60

static inline bool fill_check(uint8_t *ptr)
{
	return ((*ptr) < 100);
	//return ((*ptr) < FILL_MARK);
}

#endif

static inline uint8_t *scan(uint8_t *ptr, int dlt, bool pass)
{
#ifdef __BITFIELDFILL
	if ((dlt == +1) && pass)
	{
		uint32_t a = ((uint32_t)ptr) + 1;
		uint32_t idx = a / 32;
		uint8_t  ofs = a % 32;

		uint32_t ctz = ctz_func((~(((uint32_t *)stat_base)[idx])) & (0xFFFFFFFFU << ofs));
		ptr += ctz - ofs;
		if (ctz < 32)
			return ptr;
		idx++;

		while (true)
		{
			ctz = ctz_func(~(((uint32_t *)stat_base)[idx]));
			ptr += ctz;
			if (ctz < 32)
				return ptr;
			idx++;
		}
	}

	if ((dlt == -1) && pass)
	{
		uint32_t a = ((uint32_t)ptr) - 1;
		uint32_t idx = a / 32;
		uint8_t  ofs = 31 - (a % 32);

		uint32_t clz = clz_func((~(((uint32_t *)stat_base)[idx])) & (0xFFFFFFFFU >> ofs));
		ptr -= clz - ofs;
		if (clz < 32)
			return ptr;
		idx--;

		while (true)
		{
			clz = clz_func(~(((uint32_t *)stat_base)[idx]));
			ptr -= clz;
			if (clz < 32)
				return ptr;
			idx--;
		}
	}

	//printf("ERROR\n"); fflush(stdout);
	return ptr;
#else
//	while (((*(ptr + dlt)) == 0) == pass)
//		ptr += dlt;
	while (true)
	{
		//cnt_scan++;
		uint8_t *p = ptr + dlt;
#ifdef STATRD
		if ((dlt < 0) || (!stat_filter))
			stat_read[((uint32_t)p) - ((uint32_t)stat_base)]++;
#endif
		if (fill_check(p) != pass) break;
		ptr += dlt;
	}
	return ptr;
#endif
}

static inline bool scan_limit(uint8_t **ptr, uint8_t *limit, int dlt, bool pass)
{
#ifdef __BITFIELDFILL
	if ((dlt == +1) && !pass)
	{
		uint32_t a = ((uint32_t)(*ptr)) + 1;
		uint32_t idx = a / 32;
		uint8_t  ofs = a % 32;

		uint32_t ctz = ctz_func(((((uint32_t *)stat_base)[idx])) & (0xFFFFFFFFU << ofs));
		(*ptr) += ctz - ofs;
		if (((uint32_t)(*ptr)) > ((uint32_t)limit))	return true;
		if (ctz < 32) {(*ptr)++; return (((uint32_t)(*ptr)) > ((uint32_t)limit));}
		idx++;

		while (true)
		{
			ctz = ctz_func((((uint32_t *)stat_base)[idx]));
			(*ptr) += ctz;
			if (((uint32_t)(*ptr)) > ((uint32_t)limit))	return true;
			if (ctz < 32) {(*ptr)++; return (((uint32_t)(*ptr)) > ((uint32_t)limit));}
			idx++;
		}
	}

	if ((dlt == -1) && !pass)
	{
		uint32_t a = ((uint32_t)(*ptr)) - 1;
		uint32_t idx = a / 32;
		uint8_t  ofs = 31 - (a % 32);

		uint32_t clz = clz_func(((((uint32_t *)stat_base)[idx])) & (0xFFFFFFFFU >> ofs));
		(*ptr) -= clz - ofs;
		if (((uint32_t)(*ptr)) < ((uint32_t)limit))	return true;
		if (clz < 32) {(*ptr)--; return (((uint32_t)(*ptr)) < ((uint32_t)limit));}
		idx--;

		while (true)
		{
			clz = clz_func((((uint32_t *)stat_base)[idx]));
			(*ptr) -= clz;
			if (((uint32_t)(*ptr)) < ((uint32_t)limit))	return true;
			if (clz < 32) {(*ptr)--; return (((uint32_t)(*ptr)) < ((uint32_t)limit));}
			idx--;
		}
	}

	//printf("ERROR\n"); fflush(stdout);
	return ptr;
#else
//	*ptr = scan(*ptr, dlt, pass) + dlt;
//	if ((dlt > 0) && (((uint32_t)(*ptr)) > ((uint32_t)limit)))	return true;
//	if ((dlt < 0) && (((uint32_t)(*ptr)) < ((uint32_t)limit)))	return true;
//	return false;

	while (true)
	{
		//cnt_scan++;
		(*ptr) += dlt;
#ifdef STATRD
		if ((dlt < 0) || (!stat_filter))
			stat_read[((uint32_t)(*ptr)) - ((uint32_t)stat_base)]++;
#endif
		if ((dlt > 0) && (((uint32_t)(*ptr)) > ((uint32_t)limit)))	return true;
		if ((dlt < 0) && (((uint32_t)(*ptr)) < ((uint32_t)limit)))	return true;

		if (fill_check(*ptr) != pass)
			return false;
	}
	return false;
#endif
}

#ifdef CNT_CALC
uint32_t cnt_errmark = 0;
uint32_t cnt_itermax = 0;
uint32_t cnt_itercnt = 0;
uint32_t cnt_pixcnt  = 0;
uint32_t cnt_fifomax = 0;

uint32_t total_itermax = 0;
uint32_t total_itercnt = 0;
uint32_t total_pixcnt  = 0;
uint32_t total_fifomax = 0;

static inline void memmark(uint8_t *ptr, uint8_t val, uint32_t count)
{
	while (count--)
	{
		if (!fill_check(ptr))
		{
			*ptr = 255;
#ifdef CNT_CALC
			cnt_errmark++;
#endif
		}
		else
			*ptr = val;
		ptr++;
	}
}
#endif

#ifdef __BITFIELDFILL
static inline void memset_fast(uint8_t *ptr, uint32_t val, uint32_t cnt)
{
	(void)val;
	uint32_t a = ((uint32_t)ptr);
	uint32_t idxA = a / 32;
	uint8_t  ofsA = a % 32;

	if (cnt == 1)
	{
		((uint32_t *)stat_base)[idxA] &= ~(1 << ofsA);
		return;
	}

	uint32_t b = a + cnt - 1;
	uint32_t idxB = b / 32;
	uint8_t  ofsB = b % 32;

	if (idxA == idxB)
	{
		uint32_t mask = (0xFFFFFFFFU >> (31 - (ofsB - ofsA))) << ofsA; //((ofsB == 31) && (ofsA == 0)) ? 0xFFFFFFFFU :((1 << (1 + ofsB - ofsA)) - 1) << ofsA;
		((uint32_t *)stat_base)[idxA] &= ~mask;
		return;
	}

	((uint32_t *)stat_base)[idxA] &= ~(0xFFFFFFFFU << ofsA);
	idxA++;

	while ((idxA + 3) < idxB)
	{
		((uint32_t *)stat_base)[idxA + 0] = 0;
		((uint32_t *)stat_base)[idxA + 1] = 0;
		((uint32_t *)stat_base)[idxA + 2] = 0;
		((uint32_t *)stat_base)[idxA + 3] = 0;
		idxA += 4;
	}

	while (idxA < idxB)
		((uint32_t *)stat_base)[idxA++] = 0;

	if (ofsB == 31)
		((uint32_t *)stat_base)[idxB] = 0;
	else
		((uint32_t *)stat_base)[idxA] &= (0xFFFFFFFFU << (1 + ofsB));//~((1 << (1 + ofsB)) - 1);//
}
#else
#if defined(STM32F4XX) || defined(STM32F745xx) || defined(STM32F746xx)
static inline void memset_fast(uint8_t *ptr, uint32_t val, uint32_t cnt)
{
	if (cnt < 8)
	{
		memset(ptr, val, cnt);
		return;
	}

	while ((((uint32_t)ptr) % 4) != 0)
	{
		*ptr = val;
		ptr++;
		cnt--;
	}

	uint32_t *pdw = (uint32_t *)ptr;
	uint32_t v4 = ((uint32_t)val << 0) | ((uint32_t)val << 8) | ((uint32_t)val << 16) | ((uint32_t)val << 24);

	while (cnt >= 16)
	{
		pdw[0] = v4;
		pdw[1] = v4;
		pdw[2] = v4;
		pdw[3] = v4;
		pdw += 4;
		cnt -= 16;
	}

	while (cnt >= 4)
	{
		*pdw = v4;
		pdw++;
		cnt -= 4;
	}

	ptr = (uint8_t *)pdw;
	while (cnt > 0)
	{
		*ptr = val;
		ptr++;
		cnt--;
	}
}
#else
static inline void memset_fast(uint8_t *ptr, uint32_t val, uint32_t cnt)
{
	memset(ptr, val, cnt);
}
#endif
#endif

typedef struct {
	uint8_t *ptr;
	int32_t step;
	//int32_t iter;
} tFIFOitem;

#define FIFO_LENGTH 1024
static tFIFOitem fifo[FIFO_LENGTH] = {0};
static uint32_t fifo_rd = 0;
static uint32_t fifo_wr = 0;

static inline void fifo_add(uint8_t *ptr, int32_t step, int32_t iter)
{
	UNUSED(iter);
	tFIFOitem *item = fifo + (fifo_wr % FIFO_LENGTH);
	item->ptr = ptr;
	item->step = step;
	//item->iter = iter;
	fifo_wr++;
#ifdef CNT_CALC
	cnt_fifomax = MAX(cnt_fifomax, fifo_wr - fifo_rd);
#endif
}

static inline bool read_fifo(tFIFOitem **item)
{
	if ((fifo_wr - fifo_rd) >= FIFO_LENGTH)
	{
		fifo_rd = fifo_wr - FIFO_LENGTH + 1;
#ifdef CNT_CALC
		cnt_errmark++;
#endif
	}
	if (fifo_rd >= fifo_wr)
		return false;

	(*item) = fifo + (fifo_rd % FIFO_LENGTH);
	fifo_rd++;
	return true;
}

static inline void fill_raw(uint8_t * const img, const int32_t step, int iteration);

void fill(uint8_t * const img, const int32_t step, int iteration)
{
	UNUSED(iteration);
	fill_raw(img, step, 0);//iteration);

	tFIFOitem *item = NULL;
	while (read_fifo(&item))
		fill_raw(item->ptr, item->step, 0);//item->iter);
}

void fill_raw(uint8_t * const img, const int32_t step, int iteration)
{
	if (!fill_check(img))
		return;

#ifdef CNT_CALC
	cnt_itercnt++;
	cnt_itermax = MAX(cnt_itermax, ((uint32_t)iteration));
#endif

	iteration++;

	uint8_t *pl = NULL;
	uint8_t *pr = NULL;

	uint8_t *l = scan(img, -1, true);
	uint8_t *r = scan(img, +1, true);
	while (true)
	{
#ifdef CNT_CALC
		cnt_pixcnt += ((uint32_t)r - (uint32_t)l + 1);
#endif
		fillstat_pointer(l, r);
		memset_fast(l, FILL_MARK, (uint32_t)r - (uint32_t)l + 1);
		//memmark(l, FILL_MARK, (uint32_t)r - (uint32_t)l + 1);
		//memmark(l, FILL_MARK + ((pass > 0) ? +10 : -10), (uint32_t)r - (uint32_t)l + 1);
		//memmark(l, FILL_MARK + 10*sign(step) + ((pass > 0) ? 00 : 0), (uint32_t)r - (uint32_t)l + 1);
		//memmark(l, MIN(iteration*16, 255), (uint32_t)r - (uint32_t)l + 1);

		if (pl != NULL)
			if (((uint32_t)l) <= ((uint32_t)pl))
			{
				uint8_t *nl = pl - step;
				while (true)
				{
					if (!fill_check(nl))
						if (scan_limit(&nl, l - step, -1, false))
							break;
					fifo_add(nl, -step, iteration);
					nl = scan(nl, -1, true) - 1;
				}
			}

		if (pr == NULL)
			pr = l;

		if (((uint32_t)r) >= ((uint32_t)pr))
		{
			uint8_t *nr = pr - step;
			while (true)
			{
				if (!fill_check(nr))
					if (scan_limit(&nr, r - step, +1, false))
						break;
				fifo_add(nr, -step, iteration);
				nr = scan(nr, +1, true) + 1;
			}
		}

		uint8_t *dl = l + step;
		uint8_t *dr = r + step;

		pl = dl;
		pr = dr;

		if (!fill_check(dl))
		{
			if (scan_limit(&dl, dr, +1, false))
				break;
			l = dl;
		}
		else
			l = scan(dl, -1, true);
		r = scan(l, +1, true);

		dl = r;
		while (!scan_limit(&dl, dr, +1, false))
		{
			fifo_add(dl, step, iteration);
			dl = scan(dl, +1, true) + 1;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t FindConnectedComponentsWithStats(uint8_t * img, const uint32_t step_x, const uint32_t height, const tStat_Func func, void * const other)
{
#ifdef __BITFIELDFILL
	fillstat_setup(img, step_x);
	img = (uint8_t *)0;

	memset_fast(img, FILL_MARK, step_x);
	memset_fast(img + step_x * (height - 1), FILL_MARK, step_x);
	for (uint32_t i = 0; i < (height - 1); i++)
	{
		bit_clr(img + i*step_x + step_x - 1);
		bit_clr(img + i*step_x + step_x - 0);
	}
#else
	fillstat_setup(img, step_x);

	memset_fast(img, FILL_MARK, step_x);
	memset_fast(img + step_x * (height - 1), FILL_MARK, step_x);
	for (uint32_t i = 0; i < (height - 1); i++)
	{
		*(img + i*step_x + step_x - 1) = FILL_MARK;
		*(img + i*step_x + step_x - 0) = FILL_MARK;
	}
#endif

	uint8_t *endptr = img + (step_x * (height - 1));
	uint8_t *ptr = img + step_x + 1;

	uint32_t count = 0;
	while (!scan_limit(&ptr, endptr, +1, false))
	{
		fill(ptr, step_x, 0);
		(*func)(fillstat_calc(), other);
		fillstat_reset();
		count++;
	}
	return count;
}

const uint32_t TRIminAreaTh =   25;
const uint32_t TRImaxAreaTh = 1000;

const uint32_t TRIminAsp = 0.5 * 1024;
const uint32_t TRImaxAsp = 2.0 * 1024;
const uint32_t TRIANGLE_DEVIATION_MAX = 50;
const uint32_t TRIANGLE_LENGTH_MIN = 20;

const uint32_t TRIminAreaRatio = 0.5 * 1024;

#define TRIDOT_LENGTH 512
static tTriangleDot tridot_arr[TRIDOT_LENGTH];
static uint32_t tridot_cnt = 0;

void triangle_point_add(const tFillStat * const info, void *other)
{
	UNUSED(other);
	if (info->cnt < TRIminAreaTh) return;
	if (info->cnt > TRImaxAreaTh) return;

	uint32_t widthArea = info->maxx - info->minx + 1;
	uint32_t heightArea = info->maxy - info->miny + 1;
	uint32_t aspRatio = (heightArea * 1024) / widthArea;

	if (aspRatio < TRIminAsp) return;
	if (aspRatio > TRImaxAsp) return;

	uint32_t areaRatio = (info->cnt * 1024) / (widthArea * heightArea);
	if (areaRatio < TRIminAreaRatio) return;

	if (tridot_cnt < TRIDOT_LENGTH)
	{
		tridot_arr[tridot_cnt].x = info->avrgx;
		tridot_arr[tridot_cnt].y = info->avrgy;
		tridot_cnt++;
	}

//	if (other != NULL)
//	{
//		Mat *m = (Mat *)other;
//		cv::rectangle(*m, Point(minx, miny), Point(maxx, maxy), Scalar(255), 1);
//		m->at<uint8_t>(avrgy, avrgx) = 255;
//	}
}

static inline uint32_t pwr2(int16_t x)
{
	return x*x;
}

static inline uint32_t abs_func(int32_t val)
{
	return (val >= 0) ? val : -val;
}

#if defined(STM32F4XX) || defined(STM32F745xx) || defined(STM32F746xx)
inline float vsqrt (float value)
{
	float result ;
	asm volatile ( "vsqrt.f32 %0, %1 \r\n" : "=w" (result) : "w" (value) );
	return (result) ;
}

inline static uint32_t sqrt_func(uint32_t n)
{
	return vsqrt(n);
}
#else
#define sqrt_func(v) sqrt(v)
#endif

bool triangle_find(uint8_t * const img, const uint32_t step_x, const uint32_t height, tTriangle *res)
{
	tridot_cnt = 0;

	FindConnectedComponentsWithStats(img, step_x, height, triangle_point_add, NULL);

	if (tridot_cnt < 3)
		return false;

	int32_t lmax = 0;

	for (uint32_t i = 0 + 0; i < (tridot_cnt - 2); i++)
	for (uint32_t j = i + 1; j < (tridot_cnt - 1); j++)
	{
		uint32_t ab = sqrt_func(pwr2(tridot_arr[i].x - tridot_arr[j].x) + pwr2(tridot_arr[i].y - tridot_arr[j].y));
		if (ab < TRIANGLE_LENGTH_MIN) continue;

		for (uint32_t k = j + 1; k < (tridot_cnt - 0); k++)
		{
			uint32_t bc = sqrt_func(pwr2(tridot_arr[j].x - tridot_arr[k].x) + pwr2(tridot_arr[j].y - tridot_arr[k].y));
			if (bc < TRIANGLE_LENGTH_MIN) continue;

			uint32_t ca = sqrt_func(pwr2(tridot_arr[k].x - tridot_arr[i].x) + pwr2(tridot_arr[k].y - tridot_arr[i].y));
			if (ca < TRIANGLE_LENGTH_MIN) continue;

			uint32_t lavrg = (ab + bc + ca) / 3;

			uint32_t divAB = abs_func(lavrg - ab);
			uint32_t divBC = abs_func(lavrg - bc);
			uint32_t divCA = abs_func(lavrg - ca);

			uint32_t divMAX = (MAX(divAB, MAX(divBC, divCA)) * 1024) / (uint32_t)lavrg;
			if (divMAX > TRIANGLE_DEVIATION_MAX)
				continue;

			if (lmax < lavrg)
			{
				lmax = lavrg;
				res->a = tridot_arr[i];
				res->b = tridot_arr[j];
				res->c = tridot_arr[k];
			}
		}
	}
	return (lmax > 0);
}
