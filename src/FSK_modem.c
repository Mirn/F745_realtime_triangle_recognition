#include "stm32kiss.h"
#include "FSK_modem_sintbl.h"

const char src_ver_fsk_modem[] = __DATE__"\t"__TIME__"\t"__FILE__"\r\n";
typedef int16_t tSample;

#ifdef TESTER_MODE
#define FSK_SAMPLES_FREQ   16000
#else
#define FSK_SAMPLES_FREQ    8000
#endif

//#undef FSK_SAMPLES_FREQ
//#define FSK_SAMPLES_FREQ   32000

#define FSK_BOD_RATE        1000
#define FSK_SEMPLES_PER_BIT  (FSK_SAMPLES_FREQ / FSK_BOD_RATE)
#define FSK_DC_FILT_SAMPLES  (FSK_SEMPLES_PER_BIT * 8)
#define PHASE_ADDER(freq)   (((uint64_t)freq * 0x100000000LL) / FSK_SAMPLES_FREQ)

#define FSK_FREQ_ONE        (1200 + 0)
#define FSK_FREQ_ZERO       (2200 + 0)

static const uint32_t adder_one  = PHASE_ADDER(FSK_FREQ_ONE);
static const uint32_t adder_zero = PHASE_ADDER(FSK_FREQ_ZERO);

#define CRC16_CCITT_INIT 0xFFFF

#define FSK_HEADER_START  0x55ULL
#define FSK_HEADER_FIRST  0x33ULL
#define FSK_HEADER_SECOND 0xCCULL
#define FSK_HEADER_FIND   ((FSK_HEADER_START << 16) | (FSK_HEADER_FIRST << 8) | FSK_HEADER_SECOND)
#define FSK_HEADER_COUNT_XOR_A 0xAA
#define FSK_HEADER_COUNT_XOR_B 0x55

static const uint8_t FSK_Scrambler[256] = {
		215, 87, 129, 97, 145, 125, 89, 127, 42, 218, 195, 250, 187, 177, 69, 60, 63, 94, 79, 222, 112, 154, 191, 9, 210, 229, 103, 165, 112, 186, 32, 40, 231, 188,
		165, 254, 94, 210, 21, 3, 192, 203, 53, 236, 95, 45, 34, 3, 204, 125, 111, 249, 183, 237, 155, 16, 0, 130, 38, 228, 0, 72, 33, 89, 143, 13, 144, 221, 236,
		140, 77, 254, 33, 4, 53, 211, 232, 255, 252, 159, 200, 32, 187, 217, 43, 164, 228, 20, 30, 189, 184, 37, 229, 149, 121, 6, 226, 80, 155, 254, 235, 12, 192,
		167, 4, 61, 33, 144, 111, 113, 98, 215, 87, 129, 94, 113, 236, 207, 27, 38, 190, 220, 55, 5, 168, 193, 182, 77, 227, 74, 95, 175, 59, 193, 35, 65, 252, 104,
		76, 174, 89, 68, 76, 114, 121, 15, 111, 57, 153, 83, 97, 88, 197, 209, 33, 208, 250, 62, 128, 176, 180, 130, 40, 240, 230, 89, 145, 246, 134, 81, 110, 22,
		25, 64, 163, 121, 125, 131, 103, 131, 31, 222, 196, 35, 199, 19, 3, 110, 142, 104, 198, 151, 36, 23, 165, 49, 58, 191, 192, 163, 18, 208, 188, 160, 34, 33,
		224, 112, 40, 65, 100, 224, 123, 72, 147, 92, 36, 99, 144, 106, 21, 177, 190, 137, 114, 187, 185, 95, 10, 86, 78, 99, 17, 5, 226, 17, 5, 21, 44, 252, 154,
		184, 183, 103, 222, 208, 203, 7, 13, 154, 235, 211, 59, 2, 211, 168,
};

static uint32_t sin_adder = 0;
static uint32_t sin_pos = 0;
//static const uint32_t adder_0 = 1181116006;  //((2200 / 8000) * (2^32))
//static const uint32_t adder_1 =  644245094;  //((1200 / 8000) * (2^32))

static volatile bool dac_busy = false;
static uint32_t dac_pos = 0;
static uint8_t  dac_bit = 0;
static uint32_t dac_cnt = 0;
static const uint8_t *dac_buf = NULL;

int16_t FSK_modem_dac()
{
	int16_t result = fsk_sin_tbl_4k[sin_pos >> 20];
	sin_pos += sin_adder;

	dac_pos++;
	if ((dac_pos >= FSK_SEMPLES_PER_BIT) || (sin_adder == 0))
	{
		dac_pos = 0;

		const uint8_t *buf = dac_buf;
		if ((buf == NULL) || (dac_cnt == 0))
		{
			dac_busy = false;
			sin_adder = 0;
			sin_pos = 0;
		}
		else
		{
			sin_adder = (((*buf) & dac_bit) != 0) ? adder_one : adder_zero;
			dac_bit = dac_bit >> 1;
			if (dac_bit == 0)
			{
				dac_bit = 0x80;
				dac_buf++;
				dac_cnt--;
			}
		}
	}
	return result;
}

void FSK_modem_out_raw(const uint8_t *data, uint32_t size)
{
	dac_bit = 0x80;
	dac_cnt = size;
	dac_buf = data;
	dac_busy = true;
}

bool FSK_modem_send_busy()
{
	return dac_busy;
}

void FSK_modem_dac_init()
{
	dac_buf = 0;
	dac_cnt = 0;
	dac_bit = 0;
	dac_busy = false;
}

/*Name  : CRC-16 CCITT
  Poly  : 0x1021    x^16 + x^12 + x^5 + 1
  Init  : 0xFFFF
  Revert: false
  XorOut: 0x0000
  Check : 0x29B1 ("123456789")
  MaxLen: 4095
*/
static uint16_t Crc16_CCITT(uint16_t crc, const uint8_t *pcBlock, size_t len)
{
    while (len--)
    {
        crc ^= *(pcBlock++) << 8;

        uint32_t i;
        for (i = 0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

static uint8_t  send_buf[128 + 10];
static uint32_t send_cnt  = 0;
static uint8_t  send_body = 0;
static uint8_t  send_num  = 0;

static void send_begin()
{
	send_cnt = 0;
	send_body = 0;
	memset(send_buf, 0, sizeof(send_buf));
}

static void send_end()
{
	FSK_modem_out_raw(send_buf, send_cnt);
}

static void send_add(uint8_t val)
{
	if (send_cnt >= LENGTH(send_buf))
	{
		printf("\nERROR: send_add: if (send_cnt >= LENGTH(send_buf))\n");
		return;
	}

	send_buf[send_cnt] = val;
	send_cnt++;
}

static void send_add_body(uint16_t *crc, uint8_t val)
{
	*crc = Crc16_CCITT(*crc, &val, 1);

	val = val ^ FSK_Scrambler[send_body];
	send_body++;

	send_add(val);
}


void FSK_modem_send(const uint8_t *buf, uint8_t count)
{
	if ((uint32_t)(count + 10) >= LENGTH(send_buf))
	{
		printf("\nFSK_modem_send ERROR: count >= 128\n");
		while (1)
			;
	}

	send_begin();

	send_add(FSK_HEADER_START);
	send_add(FSK_HEADER_START);
	send_add(FSK_HEADER_START);
	send_add(FSK_HEADER_FIRST);
	send_add(FSK_HEADER_SECOND);

	send_add(count ^ FSK_HEADER_COUNT_XOR_A);
	send_add(count ^ FSK_HEADER_COUNT_XOR_B);

	uint16_t crc = CRC16_CCITT_INIT;
	while (count--)
		send_add_body(&crc, *(buf++));

	send_add_body(&crc, send_num);
	send_num++;

	send_add((crc & 0x00FF) >> 0);
	send_add((crc & 0xFF00) >> 8);

	send_end();
}

void FSK_modem_send_str(const char *buf)
{
	FSK_modem_send((const uint8_t *)buf, strlen(buf));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FSK_THRESHOLD_DIV      4
#define FSK_AUTO_TAU          32 //64

#ifdef DEBUG_LOG
#include <stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	uint16_t data[FSK_DC_FILT_SAMPLES];
	uint32_t pos;
	uint32_t sum;
} tDC_filter;

inline static uint16_t dc_filt(tDC_filter *filter, const uint16_t sample)
{
	filter->pos = (filter->pos + 1) % LENGTH(filter->data);
	uint16_t *item = &filter->data[filter->pos];

	filter->sum -= *item;
	filter->sum += sample;
	*item = sample;

	return filter->sum / LENGTH(filter->data);
}

static void dc_filter_init(tDC_filter *filter)
{
	memset(filter->data, 0, sizeof(filter->data));
	filter->sum = 0;
	filter->pos = 0;
	for (uint32_t pos = 0; pos < LENGTH(filter->data); pos++)
		dc_filt(filter, 0x8000);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	tSample  data[FSK_SEMPLES_PER_BIT];
	uint32_t pos;
	uint32_t sum;
} tAVRG_filter;

inline static tSample avrg_filt(tAVRG_filter *filter, const tSample sample)
{
	filter->pos = (filter->pos + 1) % LENGTH(filter->data);
	tSample *item = &filter->data[filter->pos];

	filter->sum -= *item;
	filter->sum += sample;
	*item = sample;

	return filter->sum / LENGTH(filter->data);
}

static void avrg_filter_init(tAVRG_filter *filter)
{
	memset(filter->data, 0, sizeof(filter->data));
	filter->sum = 0;
	filter->pos = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	tAVRG_filter filter_sin;
	tAVRG_filter filter_cos;
	uint32_t  phase;
} tQuadrator;

static void quadrator_init(tQuadrator *q)
{
	avrg_filter_init(&q->filter_sin);
	avrg_filter_init(&q->filter_cos);

	q->phase = 0;
}

#ifdef __i386__
#define sqrt_func(v) sqrt(v)
#else
#ifdef STM32F7
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
#define sqrt_iter1(N) \
	tmp = root + (1 << (N)); \
    if (n >= tmp << (N))   \
    {   n -= tmp << (N);   \
        root |= 2 << (N); \
    }

inline static uint32_t sqrt_func(uint32_t n)
{
	uint32_t root = 0, tmp;
    sqrt_iter1 (15);    sqrt_iter1 (14);    sqrt_iter1 (13);    sqrt_iter1 (12);
    sqrt_iter1 (11);    sqrt_iter1 (10);    sqrt_iter1 ( 9);    sqrt_iter1 ( 8);
    sqrt_iter1 ( 7);    sqrt_iter1 ( 6);    sqrt_iter1 ( 5);    sqrt_iter1 ( 4);
    sqrt_iter1 ( 3);    sqrt_iter1 ( 2);    sqrt_iter1 ( 1);    sqrt_iter1 ( 0);
    return root >> 1;
}
#endif
#endif

inline static tSample quadrator(tQuadrator *q, const tSample sample, const uint32_t adder)
{
	tSample sin_val = fsk_sin_tbl_4k[q->phase >> 20];
	tSample cos_val = fsk_sin_tbl_4k[((q->phase >> 20) + (LENGTH(fsk_sin_tbl_4k) / 4)) % LENGTH(fsk_sin_tbl_4k)];
	q->phase += adder;

	int32_t sin_avrg = avrg_filt(&q->filter_sin, ((int32_t)sample*(int32_t)sin_val) / 0x10000);
	int32_t cos_avrg = avrg_filt(&q->filter_cos, ((int32_t)sample*(int32_t)cos_val) / 0x10000);

	return sqrt_func(sin_avrg*sin_avrg + cos_avrg*cos_avrg);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	int32_t raw;
	tSample result;
} tRC_filter;

static void rc_filter_init(tRC_filter *filter)
{
	memset(filter, 0, sizeof(*filter));
}

inline static tSample rc_filter(tRC_filter *filter, const tSample sample, const int tau)
{
	filter->raw = (filter->raw * (tau - 1) / tau) + sample;
	filter->result = filter->raw / tau;
	return filter->result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	tRC_filter plus;
	tRC_filter minus;
	bool state;
} tShmitt_trigger;

static void shmitt_trigger_init(tShmitt_trigger *trigger)
{
	trigger->state = false;
	rc_filter_init(&trigger->plus);
	rc_filter_init(&trigger->minus);
}

inline static bool shmitt_trigger(tShmitt_trigger *trigger, const tSample sample, const int16_t threshold_divider)
{
	if (sample > 0) rc_filter(&trigger->plus,  +sample, FSK_AUTO_TAU);
	if (sample < 0) rc_filter(&trigger->minus, -sample, FSK_AUTO_TAU);

	tSample limit_plus  = +(trigger->plus.result  / threshold_divider);
	tSample limit_minus = -(trigger->minus.result / threshold_divider);

	bool bit = trigger->state;
	if ((bit == false) && (sample > limit_plus))  bit = true;
	if ((bit == true)  && (sample < limit_minus)) bit = false;
	trigger->state = bit;

	return trigger->state;
}

inline static bool simple_trigger(tShmitt_trigger *trigger, tSample sample)
{
	trigger->state = (sample > 0);
	return trigger->state;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

typedef uint8_t tGlitch_filter;

static void glitch_filter_init(tGlitch_filter *filter)
{
	*filter = 0;
}

inline static bool glitch_filter(tGlitch_filter *filter, bool bit)
{
	uint8_t shift_reg = *filter;
	shift_reg = (shift_reg << 1) | ((bit) ? 1 : 0);

	if ((shift_reg & 0x07) == 0x02) shift_reg &= ~0x02; //find pattern "010" and change to "000"
	if ((shift_reg & 0x07) == 0x05) shift_reg |=  0x02; //find pattern "101" and change to "111"
	if ((shift_reg & 0x0F) == 0x06) shift_reg &= ~0x06; //find pattern "0110" and change to "0000"
	if ((shift_reg & 0x0F) == 0x09) shift_reg |=  0x06; //find pattern "1001" and change to "1111"

	*filter = shift_reg;
	return (shift_reg & 0x08) != 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	bool cur;
	bool old;
	uint8_t cnt;
} tBit_sync;

static void bit_sync_init(tBit_sync *bit_sync)
{
	bit_sync->cur = false;
	bit_sync->old = bit_sync->cur;
	bit_sync->cnt = 0;
}

inline static bool bit_sync(tBit_sync *bit_sync, bool bit)
{
	bit_sync->old = bit_sync->cur;
	bit_sync->cur = bit;

	if (bit_sync->cur != bit_sync->old)
		bit_sync->cnt = 0;
	else
		bit_sync->cnt = (bit_sync->cnt + 1) % FSK_SEMPLES_PER_BIT;

	return (bit_sync->cnt == (FSK_SEMPLES_PER_BIT / 2) - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////

static tDC_filter dc_filter;
static tQuadrator q_mark;
static tQuadrator q_space;

static tShmitt_trigger trigger;
static tGlitch_filter  antiglitch;
static tBit_sync  sync;

bool fsk_modem_debug_mode = false;

uint16_t fsk_modem_stat_header_ok  = 0;
uint16_t fsk_modem_stat_packet_ok  = 0;
uint16_t fsk_modem_stat_err_header = 0;
uint16_t fsk_modem_stat_err_crc    = 0;
uint16_t fsk_modem_stat_err_num    = 0;

uint8_t fsk_modem_recive_data[256] = {0};
uint8_t fsk_modem_recive_cnt = 0;
bool fsk_modem_recived_normal = false;
bool fsk_modem_recived_error = false;

static uint64_t shift_reg = 0;

static volatile bool recive_processing = false;
static volatile bool recive_ready = false;
static uint8_t recive_pos = 0;
static uint8_t recive_val = 0;
static uint8_t recive_bit = 0;
static uint8_t recive_num = 0;
static bool recive_num_first = true;

void bit_func_syncro(bool bit);
void bit_func_body(bool bit);

void (*bit_func)(bool bit) = bit_func_syncro;

void bit_func_syncro(bool bit)
{
	shift_reg = shift_reg << 1;
	if (bit)
		shift_reg |= 1;

	if (dac_busy && (fsk_modem_debug_mode == false))
		return;

	if (((shift_reg >> 16) & 0xFFFFFF) == FSK_HEADER_FIND)
	{
		uint8_t cnt_p = (shift_reg >> 8) ^ FSK_HEADER_COUNT_XOR_A;
		uint8_t cnt_n = (shift_reg >> 0) ^ FSK_HEADER_COUNT_XOR_B;

		if ((cnt_p != cnt_n))// && (cnt_p < 128))
			fsk_modem_stat_err_header++;
		else
		{
			fsk_modem_stat_header_ok++;
			fsk_modem_recive_cnt = cnt_p;
			recive_pos = 0;
			recive_bit = 0;
			recive_val = 0;
			recive_processing = true;
			bit_func = bit_func_body;
		}
	}
	else
		recive_processing = false;
}

void bit_func_body(bool bit)
{
	recive_val = recive_val << 1;
	if (bit)
		recive_val |= 1;

	recive_bit++;
	if (recive_bit >= 8)
	{
		recive_bit = 0;

		if (recive_pos < (fsk_modem_recive_cnt + 1))
			recive_val ^= FSK_Scrambler[recive_pos];

		fsk_modem_recive_data[recive_pos] = recive_val;
		recive_val = 0;

		recive_pos++;
		if (recive_pos >= (fsk_modem_recive_cnt + 3))
		{
			recive_pos = 0;
			recive_bit = 0;
			recive_val = 0;
			recive_ready = true;
			recive_processing = false;
			bit_func = bit_func_syncro;
		}
	}
}

bool fsk_modem_recive_check(bool error_allow)
{
	if (!recive_ready) return false;
	recive_ready = false;

	uint16_t crc_need = Crc16_CCITT(CRC16_CCITT_INIT, fsk_modem_recive_data, fsk_modem_recive_cnt + 1);

	uint8_t  packet_num   = fsk_modem_recive_data[fsk_modem_recive_cnt + 0];
	uint16_t packet_crc_l = fsk_modem_recive_data[fsk_modem_recive_cnt + 1];
	uint16_t packet_crc_h = fsk_modem_recive_data[fsk_modem_recive_cnt + 2];

	memset(fsk_modem_recive_data + fsk_modem_recive_cnt, 0, sizeof(fsk_modem_recive_data) - fsk_modem_recive_cnt);

	uint16_t packet_crc = packet_crc_l + (packet_crc_h << 8);


	bool sum_ok = (crc_need == packet_crc);
	bool num_ok = (packet_num == recive_num) || recive_num_first;
	recive_num_first = false;

	if (sum_ok)
		recive_num = packet_num + 1;
	else
		recive_num++;

	if (!sum_ok) fsk_modem_stat_err_crc++;
	if (!num_ok) fsk_modem_stat_err_num++;

	fsk_modem_recived_normal = sum_ok;
	fsk_modem_recived_error = !fsk_modem_recived_normal;
	if (fsk_modem_recived_normal) fsk_modem_stat_packet_ok++;

	return error_allow || fsk_modem_recived_normal;
}

/*bool fsk_debug_enable = false;
int16_t fsk_debug_data[2048];
int16_t fsk_debug_cnt = 0;

void fsk_debug(int16_t value)
{
	if (!fsk_debug_enable) return;
	if (fsk_debug_cnt >= LENGTH(fsk_debug_data)) return;
	fsk_debug_data[fsk_debug_cnt] = value;
	fsk_debug_cnt++;
}*/

bool FSK_modem_adc(uint16_t raw_data)
{
	uint16_t dc = dc_filt(&dc_filter, raw_data);
	int32_t v = dc - raw_data;
	v = MIN(v, +0x7FFF);
	v = MAX(v, -0x8000);
	tSample sample = v;
	//fsk_debug(sample);

	sample = quadrator(&q_mark, sample, adder_one)
			 -
			 quadrator(&q_space, sample, adder_zero);
	//fsk_debug(sample);

	//if (dac_busy && (fsk_modem_debug_mode == false))
	//	return false;

	bool bit = (sample > 0);
	//bool bit = shmitt_trigger(&trigger, sample, FSK_THRESHOLD_DIV);
	//fsk_debug(bit);

	bit = glitch_filter(&antiglitch, bit);

	bool strobe = bit_sync(&sync, bit);
	if (strobe)
		(*bit_func)(bit);

	//if (strobe)	fsk_debug(bit);
	return strobe;
}

//126 simple_trigger
//108 shmitt_trigger FSK_THRESHOLD_DIV*1
//121 shmitt_trigger FSK_THRESHOLD_DIV*2
//117 shmitt_trigger no_glitch_filter

//remote:
//93  normal
//165 no_glitch_filter

//remote small rnd 5sec volume: 71/1000
//25 errors in 16k normal simple_trigger
//55 errors in 16k no_glitch_filter
//26 errors in 16k normal shmitt_trigger
//70 errors in 16k normal simple_trigger no_glitch_filter
//61 errors in  8k normal simple_trigger
//30 errors in 16k normal simple_trigger
//18 errors in 16k normal simple_trigger fsk_sin_tbl_4k

void FSK_modem_init()
{
	FSK_modem_dac_init();

	dc_filter_init(&dc_filter);
	quadrator_init(&q_mark);
	quadrator_init(&q_space);

	shmitt_trigger_init(&trigger);
	glitch_filter_init(&antiglitch);
	bit_sync_init(&sync);
}

void (*FSK_modem_wait_func)() = NULL;

bool FSK_modem_recive_wait_end(uint32_t timeout_ms, bool error_allow, bool verbose)
{
	uint64_t timeout = timeout_ms * (SystemCoreClock / 1000);

	uint64_t time = DWT_CYCCNT;
	while ((DWT_CYCCNT - time) < timeout)
		if (fsk_modem_recive_check(error_allow))
			return true;
		else
			if (FSK_modem_wait_func != NULL)
				(*FSK_modem_wait_func)();

	if (verbose)
		printf("\nFSK_modem_recive_wait ERROR: timeout\n");
	return false;
}

bool FSK_modem_recive_start_wait(uint32_t timeout_ms, bool verbose)
{
	uint64_t timeout = timeout_ms * (SystemCoreClock / 1000);

	uint64_t time = DWT_CYCCNT;
	while ((DWT_CYCCNT - time) < timeout)
		if (recive_processing)
			return true;
		else
			if (FSK_modem_wait_func != NULL)
				(*FSK_modem_wait_func)();

	if (verbose)
		printf("\nFSK_modem_recive_start_wait ERROR: timeout\n");
	return false;
}

bool FSK_modem_send_wait(uint32_t timeout_ms, bool verbose)
{
	uint64_t timeout = timeout_ms * (SystemCoreClock / 1000);

	uint64_t time = DWT_CYCCNT;
	while ((DWT_CYCCNT - time) < timeout)
		if (FSK_modem_send_busy() == false)
			return true;
		else
			if (FSK_modem_wait_func != NULL)
				(*FSK_modem_wait_func)();

	if (verbose)
		printf("\nFSK_modem_send_wait ERROR: timeout\n");
	return false;
}

