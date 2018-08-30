#include "stm32kiss.h"
#include "hw_init.h"
#include "usart_mini.h"
#include "img_test.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_camera.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "stm32746g_discovery_qspi.h"

#include "sysFastMemCopy.h"
#include "memtest.h"
#include "triangle_find.h"

//void img_test_main();
//int FSK_modem_test();

volatile bool running = false;
volatile int camera_line_cnt  = 0;
volatile int camera_vsync_cnt = 0;
volatile int camera_frame_cnt = 0;
volatile int camera_error_cnt = 0;

TS_StateTypeDef ts;
uint8_t render_mode = '1';
bool back_reset = false;
tBITimg bitsA;
tBITimg bitsB;

uint32_t lcd_drawing_address = LCD_FB_START_ADDRESS;

#define BASE_SHIFT_X   80
#define BASE_SHIFT_Y  104

void    BSP_CAMERA_LineEventCallback(void)  {/*send('.');*/ camera_line_cnt++;};
void    BSP_CAMERA_VsyncEventCallback(void) {/*send('!');*/ camera_vsync_cnt++;};
void    BSP_CAMERA_FrameEventCallback(void) {/*send('F');*/ camera_frame_cnt++; running = false; };
void    BSP_CAMERA_ErrorCallback(void)  {camera_error_cnt++;};

typedef struct {
	int32_t px;
	int32_t py;
	int32_t sx;
	int32_t sy;
	int32_t param;
	void (*callback)(int32_t param);
} tbutton;

tbutton buttons[8];
uint32_t buttons_cnt = 0;

uint32_t dbg_time = 0;

void button_draw(uint32_t px, uint32_t py, void (*callback)(int32_t param), int32_t param, const char *str)
{
	if (buttons_cnt >= LENGTH(buttons))
		return;

	int32_t szx = BSP_LCD_GetFont()->Width * strlen(str);
	int32_t szy = BSP_LCD_GetFont()->Height;

	int32_t cx = px + szx / 2;
	int32_t cy = py + szy / 2;

	buttons[buttons_cnt].px = cx - szx;
	buttons[buttons_cnt].py = cy - szy;
	buttons[buttons_cnt].sx = szx * 2;
	buttons[buttons_cnt].sy = szy * 2;
	buttons[buttons_cnt].param = param;
	buttons[buttons_cnt].callback = callback;
	buttons_cnt++;

	BSP_LCD_DisplayStringAt(px, py, (uint8_t*)str, LEFT_MODE);
}

void buttons_check(uint32_t tx, uint32_t ty)
{
	for (int pos = 0; pos < buttons_cnt; pos++)
		if ((tx >= buttons[pos].px) &&
			(ty >= buttons[pos].py) &&
			(tx <= buttons[pos].px + buttons[pos].sx) &&
			(ty <= buttons[pos].py + buttons[pos].sy))
		(*(buttons[pos].callback))(buttons[pos].param);
}

void ts_callback_mode(int32_t param)
{
	render_mode += param;
	if (render_mode < '1') render_mode = '7';
	if (render_mode > '7') render_mode = '1';
}

void ts_callback_setflag(int32_t param)
{
	*((bool *)param) = true;
}

static void LCD_Config(void)
{
  /* LCD Initialization */
  BSP_LCD_Init(); printf("BSP_LCD_Init;\n\n");
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_DisplayOn();
  BSP_LCD_SelectLayer(0);
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetTransparency(0, 100);
}

extern LTDC_HandleTypeDef  hLtdcHandler;

void LCD_switch()
{
	static bool mode = false;
	mode = !mode;
	if (mode)
	{
		lcd_drawing_address = LCD_FB_START_ADDRESS;
		BSP_LCD_SetLayerAddress(0, LCD_FB_START_ADDRESS_2);
		hLtdcHandler.LayerCfg[0].FBStartAdress = lcd_drawing_address;
	}
	else
	{
		lcd_drawing_address = LCD_FB_START_ADDRESS_2;
		BSP_LCD_SetLayerAddress(0, LCD_FB_START_ADDRESS);
		hLtdcHandler.LayerCfg[0].FBStartAdress = lcd_drawing_address;
	}
}

void img_render(uint8_t *img)
{
	for (uint32_t y = 0; y < 272; y++)
	{
		volatile uint32_t *dst = (uint32_t *)(lcd_drawing_address + (y * 480) * 4);
		uint8_t *src = (uint8_t *)((uint32_t)img + ((y + BASE_SHIFT_Y) * BASE_SIZE_X) + BASE_SHIFT_X);
		for (uint32_t x = 0; x < 480; x += 4)
		{
			dst[x + 0] = 0xFF000000 | (src[x + 0] << 0) | (src[x + 0] << 8) | (src[x + 0] << 16);;
			dst[x + 1] = 0xFF000000 | (src[x + 1] << 0) | (src[x + 1] << 8) | (src[x + 1] << 16);;
			dst[x + 2] = 0xFF000000 | (src[x + 2] << 0) | (src[x + 2] << 8) | (src[x + 2] << 16);;
			dst[x + 3] = 0xFF000000 | (src[x + 3] << 0) | (src[x + 3] << 8) | (src[x + 3] << 16);;
		}
	}
}

void read_roi(const uint8_t *s, uint8_t *d)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		uint8_t *dst = (uint8_t *)((uint32_t)d + (y * IMG_X));
		const uint8_t *src = (const uint8_t *)((uint32_t)s + ((y + BASE_POS_Y) * BASE_SIZE_X) + BASE_POS_X);
		sysFastMemCopy(dst, src, IMG_X);
	}
}

void roi_update(const uint8_t *s, uint8_t *d)
{
	for (uint32_t y = 0; y < IMG_Y; y++)
	{
		uint8_t *dst = (uint8_t *)((uint32_t)d + ((y + BASE_POS_Y) * BASE_SIZE_X) + BASE_POS_X);
		const uint8_t *src = (const uint8_t *)((uint32_t)s + (y * IMG_X));
		sysFastMemCopy(dst, src, IMG_X);
	}
}

int process(uint8_t *img, uint8_t *tmp, uint8_t *back, bool reset)
{
	if (render_mode < '1') return 0;

	int level = -1;
	bool intrusion = false;

	read_roi(img, tmp);
	if (reset)
		sysFastMemCopy(back, tmp, IMG_X*IMG_Y);

	if (render_mode == '1') roi_update(tmp, img);
	if (render_mode == '2') roi_update(back, img);

	if (render_mode == '3')
	{
		sad_simd16(tmp, back, tmp, IMG_X * IMG_Y);
		roi_update(tmp, img);
	}

	if (render_mode == '4')
	{
		bitimg_absdiff_mono(tmp, back, &bitsA);
		bitimg_restore(&bitsA, tmp);
		roi_update(tmp, img);
	}

	if (render_mode == '5')
	{
		bitimg_absdiff_mono(tmp, back, &bitsA);
		bitimg_erode(&bitsA, &bitsB);
		bitimg_restore(&bitsB, tmp);
		roi_update(tmp, img);
	}

	if (render_mode == '6')
	{
		bitimg_absdiff_mono(tmp, back, &bitsA);
		bitimg_erode(&bitsA, &bitsB);
		level = bitimg_bitcnt(&bitsB);
		intrusion = (level > 100);
		if ((!intrusion) && (!reset))
			sysFastMemCopy(back, tmp, IMG_X*IMG_Y);
		roi_update(tmp, img);
	}

	tTriangle tri;
	bool tri_detect = false;
	if (render_mode == '7')
	{
#ifdef __BITFIELDFILL
		bitimg_binarize(tmp, &bitsA);
		//bitimg_restore(&bitsA, tmp);
		//roi_update(tmp, img);

		dbg_time = DWT_CYCCNT;
		tri_detect = triangle_find((uint8_t *)&bitsA, IMG_WORDS * 32, IMG_Y, &tri);
		dbg_time = DWT_CYCCNT - dbg_time;
#else
		dbg_time = DWT_CYCCNT;
		tri_detect = triangle_find(tmp, IMG_X, IMG_Y, &tri);
		dbg_time = DWT_CYCCNT - dbg_time;
		roi_update(tmp, img);
#endif
	}

	img_render(img);

	int cx = BASE_POS_X - BASE_SHIFT_X;
	int cy = BASE_POS_Y - BASE_SHIFT_Y;

	if (tri_detect)
	{
		BSP_LCD_SetTextColor(LCD_COLOR_RED);
		BSP_LCD_DrawLine(tri.a.x + cx, tri.a.y + cy, tri.b.x + cx, tri.b.y + cy);
		BSP_LCD_DrawLine(tri.b.x + cx, tri.b.y + cy, tri.c.x + cx, tri.c.y + cy);
		BSP_LCD_DrawLine(tri.c.x + cx, tri.c.y + cy, tri.a.x + cx, tri.a.y + cy);
	}

	if (intrusion)
	{
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		BSP_LCD_SetBackColor(LCD_COLOR_RED);
		BSP_LCD_SetFont(&Font24);
		BSP_LCD_DisplayStringAt(cx, cy + IMG_Y + 5, (uint8_t *)"!!! INTRUSION !!!", LEFT_MODE);
	}

	buttons_cnt = 0;
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_SetFont(&Font16);
	button_draw((480 - 16*3) / 2, 272 - 16, ts_callback_setflag, (int32_t)(&back_reset), "RST");

	BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_SetFont(&Font16);
	button_draw(0, cy -16 -5, ts_callback_mode, -1, "<<<");
	button_draw(480 - 11*3, cy -16 -5, ts_callback_mode, +1, ">>>");

	if (render_mode == '1') BSP_LCD_DisplayStringAt(cx, cy -16 -5, (uint8_t *)"Real time image", LEFT_MODE);
	if (render_mode == '2') BSP_LCD_DisplayStringAt(cx, cy -16 -5, (uint8_t *)"Saved back image", LEFT_MODE);
	if (render_mode == '3') BSP_LCD_DisplayStringAt(cx, cy -16 -5, (uint8_t *)"ABSsub output", LEFT_MODE);
	if (render_mode == '4') BSP_LCD_DisplayStringAt(cx, cy -16 -5, (uint8_t *)"ABSsub mono output", LEFT_MODE);
	if (render_mode == '5') BSP_LCD_DisplayStringAt(cx, cy -16 -5, (uint8_t *)"ABSsub mono lp-filter output", LEFT_MODE);
	if (render_mode == '6') BSP_LCD_DisplayStringAt(cx, cy -16*2 -5, (uint8_t *)"Intrusion detector", LEFT_MODE);
	if (render_mode == '7') BSP_LCD_DisplayStringAt(cx, cy -16 -5, (uint8_t *)"Triangle detector", LEFT_MODE);

	if (level >= 0)
	{
		char str[32];
		sprintf(str, "Intrusion level: %i", level);
		BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
		BSP_LCD_DisplayStringAt(cx, cy -16 -5, (uint8_t *)str, LEFT_MODE);
		BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
	}

	BSP_LCD_DrawRect(cx, cy, IMG_X, IMG_Y);

	SCB_CleanInvalidateDCache();
	LCD_switch();
	return level;
}

#include "testdata/bintest.h"
#include "testdata/testpoints.h"

#ifdef CNT_CALC
void filltest()
{
	const uint32_t bitstep = 1312;
	const uint32_t bitsize = 167936;
	uint8_t * const bindata = ((uint8_t *)0x20010000);

	uint32_t total_time = 0;
	total_itercnt = 0;
	total_itermax = 0;
	total_fifomax = 0;
	total_pixcnt  = 0;

	for (uint32_t i=0; i < (sizeof(test_points) / (2U*sizeof(test_points[0][0]))); i++)
	{
		uint32_t x = test_points[i][0];
		uint32_t y = test_points[i][1];

		cnt_errmark  = 0;
		cnt_itercnt = 0;
		cnt_itermax = 0;
		cnt_fifomax = 0;
		cnt_pixcnt = 0;

		memcpy(bindata, file_binary, bitsize);
		fillstat_setup(bindata, bitstep);

		uint32_t t = DWT_CYCCNT;
		fill((uint8_t *)0 + (bitstep * y) + x, bitstep, 0);
		t = (DWT_CYCCNT - t) / (SystemCoreClock / 1000000);

#ifdef CNT_CALC
		printf("%lu\t", cnt_errmark);
#endif
		printf("%lu\t", t);
#ifdef CNT_CALC
		printf("%lu\t", cnt_itercnt);
		printf("%lu\t", cnt_itermax);
		printf("%lu\t", cnt_fifomax);
		printf("%lu\t", cnt_pixcnt);

		total_itercnt += cnt_itercnt;
		total_itermax += cnt_itermax;
		total_fifomax += cnt_fifomax;
		total_pixcnt += cnt_pixcnt;
#endif
		printf("\n");
		total_time += t;
	}

	printf("%lu\t", total_time);
	printf("%lu\t", total_itercnt);
	printf("%lu\t", total_itermax);
	printf("%lu\t", total_fifomax);
	printf("%lu\t", total_pixcnt);
	printf("\n");
}
#endif

void main(void)
{
	hw_init();

#ifdef SystemCoreClockUpdate_ENABLED
	SystemCoreClockUpdate();
#endif

	ticks_init();
	usart_init();
	delay_ms(1000);

	printf("================= RESTART ================= \n");
	printf("SystemCoreClock\t%lu\n", SystemCoreClock);
	printf("GCC version\t%i.%i.%i\n", __GNUC__, __GNUC_MINOR__ , __GNUC_PATCHLEVEL__);
	printf("\n");

//	for (uint32_t i=0; i < 20; i++)
//		printf("%08lX\t%08lX\t%08lX\t%08lX\n", i, __builtin_clz(i), __builtin_ctz(i), __RBIT(i));
//	filltest();
//	while (1)
//		__WFI();

	//SCB_DisableDCache();
	//FSK_modem_test();

	BSP_SDRAM_Init();               printf("BSP_SDRAM_Init();\n");
	HAL_EnableFMCMemorySwapping();  printf("HAL_EnableFMCMemorySwapping();\n");
	printf("\n");

	BSP_QSPI_Init();
	QSPI_Info info;
	BSP_QSPI_GetInfo(&info);
	printf("BSP_QSPI_GetStatus()          %i\n", BSP_QSPI_GetStatus());
	printf("QSPI_Info.FlashSize           %i\n", info.FlashSize);
	printf("QSPI_Info.EraseSectorSize     %i\n", info.EraseSectorSize);
	printf("QSPI_Info.EraseSectorsNumber  %i\n", info.EraseSectorsNumber);
	printf("QSPI_Info.ProgPageSize        %i\n", info.ProgPageSize);
	printf("QSPI_Info.ProgPagesNumber     %i\n", info.ProgPagesNumber);
	printf("\n");

	BSP_QSPI_EnableMemoryMappedMode();
	printf("BSP_QSPI_EnableMemoryMappedMode():OK\n");

	uint8_t *ptr = (uint8_t *)0x90000000;
	//send_file("qspi_mm.bin", ptr, info.FlashSize);
	//printf("qspi send ok\n");

	static uint8_t tmp[0x10000]  __attribute__ ((section (".data_big"), used));
	uint32_t t0 = DWT_CYCCNT;
	sysFastMemCopy((uint8_t *)tmp, (const uint8_t *)0x20000000, sizeof(tmp));
	t0 = DWT_CYCCNT - t0;

	uint32_t t1 = DWT_CYCCNT;
	sysFastMemCopy((uint8_t *)tmp, (const uint8_t *)ptr_SDRAM, sizeof(tmp));
	t1 = DWT_CYCCNT - t1;

	uint32_t t2 = DWT_CYCCNT;
	sysFastMemCopy((uint8_t *)tmp, (const uint8_t *)ptr, sizeof(tmp));
	t2 = DWT_CYCCNT - t2;
	printf("%i\t%i\t%i\n", t0, t1, t2);


//	send_file_header("qspi.bin", info.FlashSize);
//	uint32_t pos = 0;
//	uint8_t buf[256];
//	while (pos < info.FlashSize)
//	{
//		BSP_QSPI_Read(buf, pos, sizeof(buf));
//		send_block(buf, sizeof(buf));
//		pos += sizeof(buf);
//	}
//	printf("qspi send ok\n");


	while (1)
		__WFI();

#ifdef disabled
#ifndef disabled
	LCD_Config();
	BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

//	while (true)
//	{
//		uint32_t t = DWT_CYCCNT;
//		BSP_TS_GetState(&ts);
//		t = DWT_CYCCNT - t;
//		printf("\n%lu\t%i\t%i", t, ts.touchDetected, ts.gestureId);
//		for (int pos = 0; pos < ts.touchDetected; pos++)
//			printf("\t#%i-%i:%i", ts.touchEventId[pos], ts.touchX[pos], ts.touchY[pos]);
//		delay_ms(60);
//	}

	uint32_t result = BSP_CAMERA_Init(RESOLUTION_R640x480);
	printf("BSP_CAMERA_Init:\t%i\n", result);

	uint16_t *img = (uint16_t *)ptr_SDRAM;
	uint8_t *img8bit = (uint8_t *)ptr_SDRAM;
	const size_t img_count = BASE_SIZE_X * BASE_SIZE_Y; //0x25800 * 2;
	const size_t img_size = img_count * 2;
	//uint8_t *tmp = ((uint8_t *)img) + img_size;
	//uint8_t *back = ((uint8_t *)tmp) + img_size;
	static uint8_t tmp[56000]  __attribute__ ((section (".data_big"), used));;
	uint8_t *back = ((uint8_t *)img) + img_size;

	BSP_CAMERA_ContinuousStart((uint8_t *)img); running = true;
	while (1)
	{
		//BSP_CAMERA_SnapshotStart((uint8_t *)img);
		//running = true;

		while (running)
			__WFI();
		running = true;

		uint32_t time = DWT_CYCCNT;

		for (uint32_t y = 0; y < 272; y++)
		{
			uint8_t  *dst = (uint8_t  *)((uint32_t)img8bit + (((y + BASE_SHIFT_Y) * BASE_SIZE_X) + BASE_SHIFT_X) * 1);
			uint16_t *src = (uint16_t *)((uint32_t)img     + (((y + BASE_SHIFT_Y) * BASE_SIZE_X) + BASE_SHIFT_X) * 2);
			for (uint32_t x = 0; x < 480; x += 4)
			{
				dst[x + 0] = src[x + 0] >> 8;
				dst[x + 1] = src[x + 1] >> 8;
				dst[x + 2] = src[x + 2] >> 8;
				dst[x + 3] = src[x + 3] >> 8;
			}
		}

		int level = process(img8bit, tmp, back, back_reset);

		back_reset = false || (camera_frame_cnt == 30);
		if (recive_count() > 0)
		{
			uint8_t byte;
			while (recive_count() > 0)
			{
				recive_byte(&byte);
				if (byte == 'R')
					back_reset = true;
				else
					render_mode = byte;
			}
		};

		BSP_TS_GetState(&ts);
		static bool last_touched = false;
		if ((ts.touchDetected > 0) && (!last_touched))
			buttons_check(ts.touchX[0], ts.touchY[0]);
		last_touched = (ts.touchDetected > 0);

		time = DWT_CYCCNT - time;
		printf("%i\t%c\t%lu\t%i\t\t%lu\t%lu\t\t%lu\n",
				back_reset,
				render_mode,
				time / (SystemCoreClock / 1000000),
				level,
				camera_error_cnt,
				camera_frame_cnt,
				dbg_time / (SystemCoreClock / 1000000)
				);

		dbg_time = 0;
	}

	while (1)
		__WFI();
#else
	memtest();

	printf("done\n");
	while (1)
	{
		for (volatile int cnt = 0; cnt < 10000; cnt++)
			__WFI();

		printf("sdram crc\t%08lX\n", crc_calc(ptr_SDRAM, 0x800000));
		//while (1) __WFI();
	}

	img_test_main();
#endif
#endif
}
