#ifndef __FSK_MODEM__
#define __FSK_MODEM__

#ifdef __cplusplus
extern "C" {
#endif

/*extern bool fsk_debug_enable;
extern int16_t fsk_debug_data[2048];
extern int16_t fsk_debug_cnt;*/
extern bool fsk_modem_debug_mode;

extern uint16_t fsk_modem_stat_header_ok;
extern uint16_t fsk_modem_stat_packet_ok;
extern uint16_t fsk_modem_stat_err_header;
extern uint16_t fsk_modem_stat_err_crc;
extern uint16_t fsk_modem_stat_err_num;

extern bool fsk_modem_recived_normal;
extern bool fsk_modem_recived_error;

extern uint8_t fsk_modem_recive_data[256];
extern uint8_t fsk_modem_recive_cnt;

extern void (*FSK_modem_wait_func)();

void FSK_modem_init();

int16_t FSK_modem_dac();
   bool FSK_modem_adc(uint16_t raw_data);

bool fsk_modem_recive_check(bool error_allow);

void FSK_modem_out_raw(const uint8_t *data, uint32_t count);
void FSK_modem_send(const uint8_t *buf, uint8_t count);
void FSK_modem_send_str(const char *buf);

bool FSK_modem_send_busy();
bool FSK_modem_send_wait(uint32_t timeout_ms, bool verbose);
bool FSK_modem_recive_wait_end(uint32_t timeout_ms, bool error_allow, bool verbose);
bool FSK_modem_recive_start_wait(uint32_t timeout_ms, bool verbose);

#ifdef __cplusplus
}
#endif

#endif
