/*
 * img_test.h
 *
 *  Created on: 2018/06/29
 *      Author: e.sitnikov
 */

#ifndef IMG_TEST_H_
#define IMG_TEST_H_

void sad_simple(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count);
void sad_block4(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count);
void sad_simd4 (const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count);
void sad_simd16(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count);

typedef void (*tSADfunc)(const uint8_t *a, const uint8_t *b, uint8_t *res, size_t count);


void mono_simple(const uint8_t *a, uint8_t *res, size_t count);
void mono_block4(const uint8_t *a, uint8_t *res, size_t count);
void mono_simd4 (const uint8_t *a, uint8_t *res, size_t count);
void mono_simd16(const uint8_t *a, uint8_t *res, size_t count);

typedef void (*tMONOfunc)(const uint8_t *a, uint8_t *res, size_t count);


uint32_t intrusion_simple(const uint8_t *a, const uint8_t *b, const uint8_t *m, size_t count);
uint32_t intrusion_block4(const uint8_t *a, const uint8_t *b, const uint8_t *m, size_t count);
uint32_t intrusion_simd4 (const uint8_t *a, const uint8_t *b, const uint8_t *mask, size_t count);
uint32_t intrusion_simd16(const uint8_t *a, const uint8_t *b, const uint8_t *mask, size_t count);

typedef uint32_t (*tIntrusionFunc)(const uint8_t *a, const uint8_t *b, const uint8_t *m, size_t count);

//#define IMG_X     652
//#define IMG_Y      83
#define IMG_X     300
#define IMG_Y     150
#define IMG_WORDS (((IMG_X + 31) & (0xFFFFFFFF - 31)) / 32)

#define BASE_SIZE_X 640
#define BASE_SIZE_Y 480
#define BASE_POS_X ((BASE_SIZE_X - IMG_X) / 2)
#define BASE_POS_Y ((BASE_SIZE_Y - IMG_Y) / 2)

typedef uint32_t tBITimg[IMG_WORDS * IMG_Y];

void bitimg_build(const uint8_t *a, tBITimg *res);
void bitimg_restore(const tBITimg *res, uint8_t *img);
void bitimg_send(const tBITimg *res);
void bitimg_absdiff_mono(const uint8_t *frame, const uint8_t *back, tBITimg *res);
void bitimg_erode(tBITimg *src, tBITimg *dst);
void bitimg_binarize(const uint8_t *frame, tBITimg *img);

uint32_t bitimg_bitcnt(const tBITimg *img);
uint32_t bitimg_mask_bitcnt(const tBITimg *img, const tBITimg *mask);

#endif /* IMG_TEST_H_ */
