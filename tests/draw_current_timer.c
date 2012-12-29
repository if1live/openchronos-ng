#include <stdio.h>
#include <stdint.h>

void draw_current_timer(uint16_t elapsed, const char *expected) {

  uint16_t hours = elapsed/3600;
  elapsed -= hours*3600;
  uint16_t minutes = elapsed/60;
  uint8_t seconds = elapsed%60;

  printf("%02d:%02d:%02d\t%s\n", hours, minutes, seconds, expected);
}

int main(void) {
  draw_current_timer(3600, "01:00:00");
  draw_current_timer(1800, "00:30:00");
  draw_current_timer(3*3600+2*60+44,"03:02:44");
  draw_current_timer(9*3600+18*60+24,"09:18:24");
  draw_current_timer(0, "00:00:00");
  draw_current_timer(35999,"09:59:59");

  return 0;
}
