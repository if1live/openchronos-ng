/*
 modules/timer.c

 Copyright (C) 2012   S. Bian <freespace@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * timer.c
 * 
 * Created on: Dec 28, 2012
 *      Author: S. Bian
 *
 *      Use:
 *        * Long press on STAR to edit minutes,hours and secods
 *
 */

#include <openchronos.h>
/* driver */
#include <drivers/display.h>

/* Defines */
typedef static enum timer_state {
  MODULE_STOPPED,
  MODULE_RUNNING,
  MODULE_BACKGROUND,
  MODULE_EDIT,
} ModuleState;

typedef struct {
  // number of seconds to count down
  uint16_t interval;

  // number of seconds remaining
  uint16_t remaining;

  // whether we are running
  uint8_t running;
} Timer;

ModuleState _module_state;
Timer _timers[CONFIG_MOD_TIMER_COUNT];
// index of current timer
uint8_t _current_timer;

// draws the current timer to the screen
void draw_current_timer(void) {
  // do not draw if we are in background
  if (_module_state == MODULE_BACKGROUND) return;
  
  Timer *curtimer = _timers[_current_timer];

  char *dispstr = NULL;
  if (_current_timer < 10) dispstr = _sprintf("TMR%d", _current_timer);
  else if (_current_timer < 100) {
    dispstr = _sprintf("T %d", _current_timer);
  } else dispstr = _sprintf("%d", _current_timer);

  display_chars2(0, 1, dispstr, ALIGN_CENTER, SEG_SET);

  // it is up to the code that sets the interval to ensure hours is 
  // never greater than 9. It is also up to the code that updates
  // the timers to ensure that remaining never overflows
  uint16_t hours = curtimer->remaining/3600;
  uint8_t minutes = curtimer->remaining/60;
  uint8_t seconds = curtimer->remaining%60;

  dispstr = _sprintf("%1d%02d%02d",hours,minutes,seconds);

  display_chars2(0, 1, dispstr, ALIGN_CENTER, SEG_SET);
}

void timer_tick1second() {
  int idx;
  for (idx = 0; idx < CONFIG_MOD_TIMER_COUNT; ++idx) {
    Timer *t = _timers[idx];
    if (t->running) {
      if (t->remaining) t->remaining -= 1;
      else {
        t->running = 0;
        // alert here
      }
    }
  }
}

/* Activation of the module */
static void stopwatch_activated() {
	display_symbol(0, LCD_SEG_L2_COL0, SEG_ON|BLINK_ON);
	display_symbol(0, LCD_SEG_L2_COL1, SEG_ON|BLINK_ON);
	if (sSwatch_conf.state == SWATCH_MODE_BACKGROUND) {
		sSwatch_conf.state = SWATCH_MODE_ON;
		return;
	}

	sys_messagebus_register(&stopwatch_event, SYS_MSG_TIMER_20HZ);
	drawStopWatchScreen();
}

/* Deactivation of the module */
static void stopwatch_deactivated() {
	/* clean up screen */
	display_clear(0, 1);
	display_clear(0, 2);
	if (sSwatch_conf.state == SWATCH_MODE_ON) {
		sSwatch_conf.state = SWATCH_MODE_BACKGROUND;
		return;
	} else {

		sys_messagebus_unregister(&stopwatch_event);
		display_symbol(0, LCD_ICON_STOPWATCH, SEG_OFF);
		display_symbol(0, LCD_SEG_L2_COL0, SEG_OFF);
		display_symbol(0, LCD_SEG_L2_COL1, SEG_OFF);
	}
}

static void down_press() {
	if (sSwatch_conf.state == SWATCH_MODE_ON) {
		increment_lap_stopwatch();

	} else if (sSwatch_conf.laps != 0) {
		if (sSwatch_conf.lap_act > sSwatch_conf.laps - 1) {
			sSwatch_conf.lap_act = 0;
		} else if (sSwatch_conf.lap_act > 0) {
			sSwatch_conf.lap_act--;
		}
		drawStopWatchScreen();
	}
}
static void up_press() {
	if (sSwatch_conf.state == SWATCH_MODE_ON) {
		increment_lap_stopwatch();
	} else if (sSwatch_conf.laps != 0) {
		if (sSwatch_conf.lap_act < sSwatch_conf.laps - 1) {
			sSwatch_conf.lap_act++;
		} else if (sSwatch_conf.lap_act > sSwatch_conf.laps - 1) {
			sSwatch_conf.lap_act = sSwatch_conf.laps - 1;
		}
		drawStopWatchScreen();
	}
}
static void num_press() {
	if (sSwatch_conf.state == SWATCH_MODE_OFF) {
		sSwatch_conf.state = SWATCH_MODE_ON;
		sSwatch_conf.lap_act = SW_COUNTING;
	} else {
		sSwatch_conf.state = SWATCH_MODE_OFF;
	}
	drawStopWatchScreen();
}

static void num_long_pressed() {

	if (sSwatch_conf.state == SWATCH_MODE_OFF) {
		clear_stopwatch();
		drawStopWatchScreen();
	}
}

void mod_stopwatch_init(void) {
	sSwatch_conf.state = SWATCH_MODE_OFF;
	clear_stopwatch();

	menu_add_entry("ST WH", &up_press, &down_press, &num_press, NULL,
			&num_long_pressed, NULL, &stopwatch_activated,
			&stopwatch_deactivated);
}

/*
 * Helper Functions
 */

void clear_stopwatch(void) {
	sSwatch_time[SW_COUNTING].cents = 0;
	sSwatch_time[SW_COUNTING].hours = 0;
	sSwatch_time[SW_COUNTING].minutes = 0;
	sSwatch_time[SW_COUNTING].seconds = 0;
	sSwatch_conf.laps = 0;
	sSwatch_conf.lap_act = SW_COUNTING;
}

void increment_lap_stopwatch(void) {
	sSwatch_time[sSwatch_conf.laps] = sSwatch_time[SW_COUNTING];
	if (sSwatch_conf.laps < (MAX_LAPS - 1)) {
		sSwatch_conf.laps++;
	}
}
