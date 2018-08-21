/*
 * img_test.c
 *
 *  Created on: 2018/06/28
 *      Author: e.sitnikov
 */

#include "stm32kiss.h"
#include "core_cmSimd.h"
#include "img_test.h"

void sad_simple(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count)
{
	while (count--)
	{
		res[0] = abs(a[0] - b[0]);
		a++;
		b++;
		res++;
	}
}

void sad_block4(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count)
{
	while (count >= 4)
	{
		res[0] = abs(a[0] - b[0]);
		res[1] = abs(a[1] - b[1]);
		res[2] = abs(a[2] - b[2]);
		res[3] = abs(a[3] - b[3]);
		a += 4;
		b += 4;
		res += 4;
		count -= 4;
	}
	sad_simple(a, b, res, count);
}

static inline uint32_t _sad_simd4_raw(uint32_t x, uint32_t y)
{
	uint32_t p = __USUB8(x, y);
	uint32_t m = __USUB8(y, x);
	return __SEL(m, p);
}

void sad_simd4(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count)
{
	const uint32_t *x = (const uint32_t *)a;
	const uint32_t *y = (const uint32_t *)b;
	uint32_t *r = (uint32_t *)res;
	size_t cnt = count / 4;

	while (cnt--)
	{
		r[0] = _sad_simd4_raw(x[0], y[0]);
		x++;
		y++;
		r++;
	}
}

void sad_simd16(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count)
{
	const uint32_t *x = (const uint32_t *)a;
	const uint32_t *y = (const uint32_t *)b;
	uint32_t *r = (uint32_t *)res;
	size_t cnt = count / 4;

	while (cnt >= 4)
	{
		r[0] = _sad_simd4_raw(x[0], y[0]);
		r[1] = _sad_simd4_raw(x[1], y[1]);
		r[2] = _sad_simd4_raw(x[2], y[2]);
		r[3] = _sad_simd4_raw(x[3], y[3]);

		x += 4;
		y += 4;
		r += 4;
		cnt -= 4;
	}
	sad_simd4((void*)x, (void*)y, (void*)r, (cnt * 4) + count % 4);
}

////////////////////////////////////////////////////////////////////////////////////

#define MONO_LEVEL 15
#define MONO_LEVEL_SIMD_TMP (MONO_LEVEL + 1)
#define MONO_LEVEL_SIMD \
		((MONO_LEVEL_SIMD_TMP << (0 * 8)) | \
	     (MONO_LEVEL_SIMD_TMP << (1 * 8)) | \
	     (MONO_LEVEL_SIMD_TMP << (2 * 8)) | \
	     (MONO_LEVEL_SIMD_TMP << (3 * 8)))

#define MONO_LEVEL_BIN 100
#define MONO_LEVEL_SIMD_BIN_TMP (MONO_LEVEL_BIN + 1)
#define MONO_LEVEL_SIMD_BIN \
		((MONO_LEVEL_SIMD_BIN_TMP << (0 * 8)) | \
	     (MONO_LEVEL_SIMD_BIN_TMP << (1 * 8)) | \
	     (MONO_LEVEL_SIMD_BIN_TMP << (2 * 8)) | \
	     (MONO_LEVEL_SIMD_BIN_TMP << (3 * 8)))

void mono_simple(const uint8_t *a, uint8_t *res, size_t count)
{
	while (count--)
	{
		res[0] = ((a[0] > MONO_LEVEL) ? 0xFF : 0x00);
		a++;
		res++;
	}
}

void mono_block4(const uint8_t *a, uint8_t *res, size_t count)
{
	while (count >= 4)
	{
		res[0] = ((a[0] > MONO_LEVEL) ? 0xFF : 0x00);
		res[1] = ((a[1] > MONO_LEVEL) ? 0xFF : 0x00);
		res[2] = ((a[2] > MONO_LEVEL) ? 0xFF : 0x00);
		res[3] = ((a[3] > MONO_LEVEL) ? 0xFF : 0x00);
		a += 4;
		res += 4;
		count -= 4;
	}
	mono_simple(a, res, count);
}

static inline uint32_t _mono_simd4_raw(uint32_t x, uint32_t lvl)
{
	__USUB8(x, lvl);
	return __SEL(0xFFFFFFFF, 0x00000000);
}

void mono_simd4(const uint8_t *a, uint8_t *res, size_t count)
{
	const uint32_t *x = (const uint32_t *)a;
	uint32_t *r = (uint32_t *)res;
	size_t cnt = count / 4;

	while (cnt--)
	{
		r[0] = _mono_simd4_raw(x[0], MONO_LEVEL_SIMD);
		x++;
		r++;
	}
}

void mono_simd16(const uint8_t *a, uint8_t *res, size_t count)
{
	const uint32_t *x = (const uint32_t *)a;
	uint32_t *r = (uint32_t *)res;
	size_t cnt = count / 4;

	while (cnt >= 4)
	{
		r[0] = _mono_simd4_raw(x[0], MONO_LEVEL_SIMD);
		r[1] = _mono_simd4_raw(x[1], MONO_LEVEL_SIMD);
		r[2] = _mono_simd4_raw(x[2], MONO_LEVEL_SIMD);
		r[3] = _mono_simd4_raw(x[3], MONO_LEVEL_SIMD);

		x += 4;
		r += 4;
		cnt -= 4;
	}
	mono_simd4((void*)x, (void*)r, (cnt * 4) + count % 4);
}

////////////////////////////////////////////////////////////////////////

uint32_t intrusion_simple(const uint8_t *a, const uint8_t *b, const uint8_t *m, size_t count)
{
	uint32_t result = 0;
	while (count--)
	{
		result += ((abs(a[0] - b[0]) & m[0]) > MONO_LEVEL) ? 1 : 0;
		a++;
		b++;
		m++;
	}
	return result;
}

uint32_t intrusion_block4(const uint8_t *a, const uint8_t *b, const uint8_t *m, size_t count)
{
	uint32_t result = 0;
	while (count >= 4)
	{
		result += ((abs(a[0] - b[0]) & m[0]) > MONO_LEVEL) ? 1 : 0;
		result += ((abs(a[1] - b[1]) & m[1]) > MONO_LEVEL) ? 1 : 0;
		result += ((abs(a[2] - b[2]) & m[2]) > MONO_LEVEL) ? 1 : 0;
		result += ((abs(a[3] - b[3]) & m[3]) > MONO_LEVEL) ? 1 : 0;
		a += 4;
		b += 4;
		m += 4;
		count -= 4;
	}
	return result + intrusion_simple(a, b, m, count);
}

//#define ERODE
#ifdef ERODE
static const uint8_t BITCNT_LUT[16] = {
		0, 0, 0, 0,
		0, 0, 0, 1,
		0, 0, 0, 0,
		0, 0, 1, 4
};
//static const uint8_t BITCNT_LUT[16] = {
//		0, 0, 0, 1,
//		0, 0, 1, 2,
//		0, 0, 1, 2,
//		1, 1, 2, 4
//};
#else
static const uint8_t BITCNT_LUT[16] = {
		0, 1, 1, 2,
		1, 2, 2, 3,
		1, 2, 2, 3,
		2, 3, 3, 4
};
#endif

static inline uint32_t _intrusion_simd4_raw(uint32_t x, uint32_t y, uint32_t mask)
{
	uint32_t p = __USUB8(x, y);
	uint32_t m = __USUB8(y, x);
	uint32_t a = __SEL(m, p) & mask;
	__USUB8(a, MONO_LEVEL_SIMD);
	return BITCNT_LUT[(__get_APSR() & APSR_GE_Msk) >> APSR_GE_Pos];
}

uint32_t intrusion_simd4(const uint8_t *a, const uint8_t *b, const uint8_t *mask, size_t count)
{
	const uint32_t *x = (const uint32_t *)a;
	const uint32_t *y = (const uint32_t *)b;
	const uint32_t *m = (const uint32_t *)mask;
	uint32_t result = 0;
	size_t cnt = count / 4;

	while (cnt--)
	{
		result += _intrusion_simd4_raw(x[0], y[0], m[0]);
		x++;
		y++;
		m++;
	}
	return result;
}

uint32_t intrusion_simd16(const uint8_t *a, const uint8_t *b, const uint8_t *mask, size_t count)
{
	const uint32_t *x = (const uint32_t *)a;
	const uint32_t *y = (const uint32_t *)b;
	const uint32_t *m = (const uint32_t *)mask;
	uint32_t result = 0;
	size_t cnt = count / 4;

	while (cnt >= 4)
	{
		result += _intrusion_simd4_raw(x[0], y[0], m[0]);
		result += _intrusion_simd4_raw(x[1], y[1], m[1]);
		result += _intrusion_simd4_raw(x[2], y[2], m[2]);
		result += _intrusion_simd4_raw(x[3], y[3], m[3]);

		x += 4;
		y += 4;
		m += 4;
		cnt -= 4;
	}

	return result + intrusion_simd4((void*)x, (void*)y, (void*)m, (cnt * 4) + count % 4);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void bitimg_build(const uint8_t *a, tBITimg *res)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		uint32_t *ptr = &((*res)[y * IMG_WORDS]);
		const uint8_t *b = &(a[y * IMG_X]);

		for (uint32_t x = 0; x < IMG_X; x++)
		{
			ptr[x >> 5] &= ~(1 << (x & 31));
			ptr[x >> 5] |= ((((uint32_t)(b[x])) & 0x01) << (x & 31));
		}
	}
}

void bitimg_send(const tBITimg *res)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		const uint32_t *ptr = &((*res)[y * IMG_WORDS]);
		for (uint32_t x = 0; x < IMG_X; x++)
			send(((ptr[x / 32] & (1 << (x % 32))) != 0) ? 0xFF : 00);
	}
}

void bitimg_restore(const tBITimg *res, uint8_t *img)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		const uint32_t *ptr = &((*res)[y * IMG_WORDS]);
		for (uint32_t x = 0; x < IMG_X; x++)
			*(img++) = ((ptr[x / 32] & (1 << (x % 32))) != 0) ? 0xFF : 00;
	}
}

//////////////////////////////////////////////////////////////////////////

static inline uint32_t _absdiff_mono_simd4_raw(uint32_t x, uint32_t y)
{
	uint32_t p = __USUB8(x, y);
	uint32_t m = __USUB8(y, x);
	uint32_t a = __SEL(m, p);
	__USUB8(a, MONO_LEVEL_SIMD);
	return (__get_APSR() & APSR_GE_Msk) >> APSR_GE_Pos;
}

void bitimg_absdiff_mono(const uint8_t *frame, const uint8_t *back, tBITimg *img)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		uint32_t *bits = &((*img)[y * IMG_WORDS]);
		uint32_t *f = (uint32_t*)(&(frame[y * IMG_X]));
		uint32_t *b = (uint32_t*)(&(back[y * IMG_X]));

		for (uint32_t cnt = 0; cnt < IMG_WORDS; cnt++)
		{
			uint32_t res = 0;
			res |= _absdiff_mono_simd4_raw(f[0], b[0]) << (0*4);
			res |= _absdiff_mono_simd4_raw(f[1], b[1]) << (1*4);
			res |= _absdiff_mono_simd4_raw(f[2], b[2]) << (2*4);
			res |= _absdiff_mono_simd4_raw(f[3], b[3]) << (3*4);
			res |= _absdiff_mono_simd4_raw(f[4], b[4]) << (4*4);
			res |= _absdiff_mono_simd4_raw(f[5], b[5]) << (5*4);
			res |= _absdiff_mono_simd4_raw(f[6], b[6]) << (6*4);
			res |= _absdiff_mono_simd4_raw(f[7], b[7]) << (7*4);
			*bits = res;

			bits++;
			f += 8;
			b += 8;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

static inline uint32_t _binary_simd4_raw(uint32_t x, uint32_t lvl)
{
	__USUB8(x, lvl);
	return (__get_APSR() & APSR_GE_Msk) >> APSR_GE_Pos;
}

void bitimg_binarize(const uint8_t *frame, tBITimg *img)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		uint32_t *bits = &((*img)[y * IMG_WORDS]);
		uint32_t *f = (uint32_t*)(&(frame[y * IMG_X]));

		for (uint32_t cnt = 0; cnt < IMG_WORDS; cnt++)
		{
			uint32_t res = 0;
			res |= _binary_simd4_raw(f[0], MONO_LEVEL_SIMD_BIN) << (0*4);
			res |= _binary_simd4_raw(f[1], MONO_LEVEL_SIMD_BIN) << (1*4);
			res |= _binary_simd4_raw(f[2], MONO_LEVEL_SIMD_BIN) << (2*4);
			res |= _binary_simd4_raw(f[3], MONO_LEVEL_SIMD_BIN) << (3*4);
			res |= _binary_simd4_raw(f[4], MONO_LEVEL_SIMD_BIN) << (4*4);
			res |= _binary_simd4_raw(f[5], MONO_LEVEL_SIMD_BIN) << (5*4);
			res |= _binary_simd4_raw(f[6], MONO_LEVEL_SIMD_BIN) << (6*4);
			res |= _binary_simd4_raw(f[7], MONO_LEVEL_SIMD_BIN) << (7*4);
			*bits = ~res;

			bits++;
			f += 8;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
static inline void bitimg_erode_X(tBITimg *src)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		uint32_t *res = &((*src)[y * IMG_WORDS]);
		uint32_t prev = 0;
		uint32_t curr = res[0];
		uint32_t next = res[1];

		for (uint32_t x = 0; x < IMG_WORDS; x++)
		{
			uint32_t left  = (curr << 1) | (prev >> 31);
			uint32_t right = (curr >> 1) | (next << 31);
			res[0] = left & right & curr;
			res++;

			prev = curr;
			curr = next;
			next = res[1];
		}
	}
}

static inline void bitimg_erode_zeroborder(uint32_t h, tBITimg *dst)
{
	uint32_t *d = &((*dst)[h * IMG_WORDS]);
	uint32_t cnt = IMG_WORDS;

	while (cnt >= 4)
	{
		d[0] = 0;
		d[1] = 0;
		d[2] = 0;
		d[3] = 0;
		d += 4;
		cnt -= 4;
	}

	while (cnt--)
	{
		d[0] = 0;
		d++;
	}
}


static inline void bitimg_erode_Y(tBITimg *src, tBITimg *dst)
{
	bitimg_erode_zeroborder(0, dst);

	for (uint32_t y = 1; y < (IMG_Y - 1); y++)
	{
		uint32_t *s = &((*src)[y * IMG_WORDS]);
		uint32_t *d = &((*dst)[y * IMG_WORDS]);

		uint32_t cnt = IMG_WORDS;

		while (cnt >= 4)
		{
			d[0] = s[0-IMG_WORDS] & s[0] & s[0+IMG_WORDS];
			d[1] = s[1-IMG_WORDS] & s[1] & s[1+IMG_WORDS];
			d[2] = s[2-IMG_WORDS] & s[2] & s[2+IMG_WORDS];
			d[3] = s[3-IMG_WORDS] & s[3] & s[3+IMG_WORDS];
			d += 4;
			s += 4;
			cnt -= 4;
		}

		while (cnt--)
		{
			d[0] = s[-IMG_WORDS] & s[0] & s[+IMG_WORDS];
			d++;
			s++;
		}
	}
	bitimg_erode_zeroborder(IMG_Y-1, dst);
}

void bitimg_erode(tBITimg *src, tBITimg *dst)
{
	bitimg_erode_X(src);
	bitimg_erode_Y(src, dst);
}

//////////////////////////////////////////////////////////////////////////

uint32_t bitimg_mask_bitcnt(const tBITimg *img, const tBITimg *mask)
{
	uint32_t result = 0;
	for (uint32_t i = 0; i < (IMG_WORDS * IMG_Y); i++)
		result += __builtin_popcount((*img)[i] & (*mask)[i]);
	return result;
}

uint32_t bitimg_bitcnt(const tBITimg *img)
{
	uint32_t result = 0;
	for (uint32_t i = 0; i < (IMG_WORDS * IMG_Y); i++)
		result += __builtin_popcount((*img)[i]);
	return result;
}
