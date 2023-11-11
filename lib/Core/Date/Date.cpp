#include <time.h>

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return 10 * 60 * 1000UL;  // 10 minutes
}
