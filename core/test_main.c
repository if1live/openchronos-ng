#ifdef TESTING
#include <core/openchronos.h>

#include <drivers/rtca.h>
#include <drivers/display.h>

#include "../greatest/greatest.h"

extern SUITE(the_suite);

// fake api
void sys_messagebus_unregister(void (*callback)(enum sys_message)) {}
void sys_messagebus_register(void (*callback)(enum sys_message), enum sys_message listens) {}
void menu_add_entry(char const * name,
					void (*up_btn_fn)(void),
					void (*down_btn_fn)(void),
					void (*num_btn_fn)(void),
					void (*lstar_btn_fn)(void),
					void (*lnum_btn_fn)(void),
					void (*updown_btn_fn)(void),
					void (*activate_fn)(void),
					void (*deactivate_fn)(void)) {}

void display_clear(uint8_t scr_nr, uint8_t line) {}

void display_char(uint8_t scr_nr,
				  enum display_segment segment,
				  char chr,
				  enum display_segstate state) {}
void display_chars(uint8_t scr_nr,
				   enum display_segment_array segments,
				   char const * str,
				   enum display_segstate state) {}

void display_bits(uint8_t scr_nr,
				  enum display_segment segment,
				  uint8_t bits,
				  enum display_segstate state) {}

char *_sprintf(const char *fmt, int16_t n) { return NULL; }


/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
	GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
	RUN_SUITE(the_suite);
	GREATEST_MAIN_END();        /* display results */
}


#endif
