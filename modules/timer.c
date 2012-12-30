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

#include <string.h>

#include <openchronos.h>

#include <drivers/display.h>
#include <drivers/buzzer.h>

typedef enum timer_state {
  // no timers are running in the module
  MODULE_STOPPED,

  // at least one timer is running in the module
  MODULE_RUNNING,

  // at least one timer is running in the background, or at least
  // timer has expired
  MODULE_BACKGROUND,

  // current timer is being edited
  MODULE_EDITING,
} ModuleState;

typedef struct {
  // number of seconds to count down
  uint16_t interval;

  // number of seconds remaining
  uint16_t remaining;

  // whether we are running, either 1 or 0. Must never be any other
  // value
  uint8_t running;

  // for time keeping, increments at 20Hz, having one tick variable
  // per timer allows interval accuracy of 0.02
  uint8_t ticks;
} Timer;

static ModuleState _module_state;
static Timer _timers[CONFIG_MOD_TIMER_COUNT];
// index of current timer
static uint8_t _current_timer;

static uint8_t _alarm_ticks = 0xff;

// it is much easier to manipulate the interval when separated into
// hours, minutes and seconds, and do the conversion to interval in
// seconds at the end of editing
typedef struct {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} EditTimer;

static EditTimer _editTimer;

#define SECS_PER_MINUTE   (60)
#define MINUTES_PER_HOUR  (60)
#define SECS_PER_HOUR     (SECS_PER_MINUTE*MINUTES_PER_HOUR)

#define GET_HOURS(secs)   (secs/SECS_PER_HOUR)
#define GET_MINUTES(secs) ((secs/SECS_PER_MINUTE)%MINUTES_PER_HOUR)
#define GET_SECS(secs)    (secs%SECS_PER_MINUTE)

#define CURTIMERPTR   (_timers+_current_timer)

static void draw_current_timer();
  
// ------------------------------------------------------------------
// Timer Operation
// ------------------------------------------------------------------
static uint8_t timer_is_expired(Timer *tp) {
  return tp->interval && tp->remaining == 0;
}

static uint8_t number_of_running_timers() {
  uint8_t idx;
  uint8_t cnt;

  for (idx = cnt = 0; idx < CONFIG_MOD_TIMER_COUNT; ++idx) {
    cnt += _timers[idx].running;
  }

  return cnt;
}

static uint8_t number_of_expired_timers() {
  uint8_t idx;
  uint8_t cnt;

  for (idx = cnt = 0; idx < CONFIG_MOD_TIMER_COUNT; ++idx) {
    if (timer_is_expired(_timers+idx)) cnt += 1;
  }

  return cnt;
}

// resets a timer so it can start counting down again
static void timer_reset(Timer *tp) {
  tp->remaining = tp->interval;
  tp->ticks = 0;
}

// sets a new timer interval
static void timer_set(Timer *t, uint16_t interval) {
  t->interval = interval;
  timer_reset(t);
}

// ------------------------------------------------------------------
// Edit Mode 
// ------------------------------------------------------------------

static void setup_edit_timer() {
  Timer *curtimer = CURTIMERPTR;

  // XXX this is a bit of design decision... we can choose to edit
  // the timer's interval, or its current value.. I am choosing the
  // edit the current value because it is less surprising to the user,
  // and because you can always reset then edit to edit the interval.
  _editTimer.hours = GET_HOURS(curtimer->remaining);
  _editTimer.minutes = GET_MINUTES(curtimer->remaining);
  _editTimer.seconds = GET_SECS(curtimer->remaining);
}

static void save_edit_timer() {
  uint16_t interval =  _editTimer.hours * SECS_PER_HOUR;
  interval += _editTimer.minutes * SECS_PER_MINUTE;
  interval += _editTimer.seconds;

  timer_set(CURTIMERPTR, interval);
}

static void edit_h_sel(void) {
  display_symbol(0, LCD_SEG_L2_4, BLINK_ON);
}

static void edit_h_desel(void) {
  display_symbol(0, LCD_SEG_L2_4, BLINK_OFF);
}

static void edit_h_set(int8_t step) {
  helpers_loop(&_editTimer.hours, 0, 9, step);
  save_edit_timer();
}

static void edit_mm_sel(void) {
  display_chars(0, LCD_SEG_L2_3_2, NULL, BLINK_ON);
}

static void edit_mm_desel(void) {
  display_chars(0, LCD_SEG_L2_3_2, NULL, BLINK_OFF);
}

static void edit_mm_set(int8_t step) {
  helpers_loop(&_editTimer.minutes, 0, 59, step);
  save_edit_timer();
}

static void edit_ss_sel(void) {
  display_chars(0, LCD_SEG_L2_1_0, NULL, BLINK_ON);
}

static void edit_ss_desel(void) {
  display_chars(0, LCD_SEG_L2_1_0, NULL, BLINK_OFF);
}

static void edit_ss_set(int8_t step) {
  helpers_loop(&_editTimer.seconds, 0, 59, step);
  save_edit_timer();
}

static void edit_save() {
  save_edit_timer();
  _module_state = MODULE_STOPPED;
}

// --------------------------------------------------------------------
// Module Operation
// -------------------------------------------------------------------

// draws the current timer to the screen
static void draw_current_timer() {
  // do not draw if we are in background
  if (_module_state == MODULE_BACKGROUND) return;
  
  display_clear(0, 0);

  Timer *curtimer = CURTIMERPTR;

  char *dispstr = NULL;
  if (_current_timer < 10) dispstr = _sprintf("TMR%1d", _current_timer);
  else if (_current_timer < 100) {
    dispstr = _sprintf("T %02d", _current_timer);
  } else dispstr = _sprintf("%04d", _current_timer);

  display_chars2(0, 1, dispstr, ALIGN_CENTER, SEG_ON);

  // it is up to the code that sets the interval to ensure hours is 
  // never greater than 9. It is also up to the code that updates
  // the timers to ensure that remaining never overflows
  uint16_t hours = GET_HOURS(curtimer->remaining);
  uint8_t minutes = GET_MINUTES(curtimer->remaining);
  uint8_t seconds = GET_SECS(curtimer->remaining);

  dispstr = _sprintf("%1d", hours);
  display_chars2(0, 2, dispstr, ALIGN_LEFT, SEG_ON);

  dispstr = _sprintf("%02d", minutes);
  display_chars2(0, 2, dispstr, ALIGN_CENTER, SEG_ON);

  dispstr = _sprintf("%02d", seconds);
  display_chars2(0, 2, dispstr, ALIGN_RIGHT, SEG_ON);

  enum display_segstate colstate = SEG_ON;
  if (curtimer->running) colstate |= BLINK_ON;
  else colstate |= BLINK_OFF;

 display_symbol(0, LCD_SEG_L2_COL0, colstate); 
 display_symbol(0, LCD_SEG_L2_COL1, colstate); 
}

// Updates _current_timer to the highest numbered expired timer, if
// any. If none, _current_timer is unchanged
static void show_expired_timer() {
  uint8_t idx;
  for (idx = 0; idx < CONFIG_MOD_TIMER_COUNT; ++idx) {
    if (timer_is_expired(_timers+idx)) _current_timer = idx;
  }
  draw_current_timer();
}

void timer_tick() {
  int idx;
  for (idx = 0; idx < CONFIG_MOD_TIMER_COUNT; ++idx) {
    Timer *t = _timers+idx;
    if (t->running) {
      t->ticks += 1;
      if (t->ticks >= 20) {
        t->ticks = 0;
        if (t->remaining) t->remaining -= 1;
      }

      // XXX no, this code isn't redundant. We want the alarm to go off
      // as soon as it expires, not in the next second
      if (t->remaining == 0) t->running = 0;
    }
  }

  if (_module_state == MODULE_EDITING) draw_current_timer();
  else if (number_of_expired_timers()) {
    if (_alarm_ticks == 0xff) _alarm_ticks = 0;
    else _alarm_ticks = (_alarm_ticks+1)%20;

    if (_alarm_ticks == 0) buzzer_play_alert2x();

    if (_module_state != MODULE_BACKGROUND) {
      show_expired_timer();

      // now flash all the things to draw attension
      display_chars(0, LCD_SEG_L1_3_0, NULL, BLINK_ON);
      display_chars(0, LCD_SEG_L2_4_0, NULL, BLINK_ON);
    }
  } else if (number_of_running_timers() == 0) {
    _module_state = MODULE_STOPPED;
    _alarm_ticks = 0xff;
    display_chars(0, LCD_SEG_L1_3_0, NULL, BLINK_OFF);
    display_chars(0, LCD_SEG_L2_4_0, NULL, BLINK_OFF);
    draw_current_timer();
  } else {
    _module_state = MODULE_RUNNING; 
    draw_current_timer();
  }
}

void timer_activate() {
  sys_messagebus_register(&timer_tick, SYS_MSG_TIMER_20HZ);

  // if we were in the background, bring ourselves into the running
  // state
  if (_module_state == MODULE_BACKGROUND) {
    _module_state = MODULE_RUNNING;
  } else _module_state = MODULE_STOPPED;

  draw_current_timer();
}

void timer_deactivated() {
	display_clear(0, 0);
	if (_module_state == MODULE_RUNNING) _module_state = MODULE_BACKGROUND;
  else {
    _module_state = MODULE_STOPPED;
		sys_messagebus_unregister(&timer_tick);
	}
}

void down_press() {
  if (_module_state == MODULE_STOPPED) {
    if (_current_timer == 0) _current_timer = CONFIG_MOD_TIMER_COUNT-1;
    else _current_timer -= 1;
    draw_current_timer();
  }
}

void up_press() {
  if (_module_state == MODULE_STOPPED) {
    if (_current_timer == CONFIG_MOD_TIMER_COUNT-1) _current_timer = 0;
    else _current_timer += 1;
    draw_current_timer();
  }
}

void num_press() {
  if (_module_state != MODULE_EDITING) {
    Timer *curtimer = _timers + _current_timer;

    // if the timer is expired, reset it
    if (timer_is_expired(curtimer)) timer_reset(curtimer);
    // otherwise if it has a non-zero interval and not expired, start
    // or stop it
    else if (curtimer->interval) curtimer->running = !curtimer->running;
  }
}

void num_long_press() {
  Timer *curtimer = _timers + _current_timer;
  if (curtimer->running == 0) {
    timer_reset(curtimer);
		draw_current_timer();
	}
}

void star_long_press() {
  static struct menu_editmode_item edit_items[] = {
    {&edit_h_sel, &edit_h_desel, &edit_h_set},
    {&edit_mm_sel, &edit_mm_desel, &edit_mm_set},
    {&edit_ss_sel, &edit_ss_desel, &edit_ss_set},
    {NULL},
  };

  if (_module_state == MODULE_STOPPED) {
    _module_state = MODULE_EDITING;
    setup_edit_timer();
    menu_editmode_start(&edit_save, edit_items);
  }
}

void mod_timer_init(void) {
  _module_state = MODULE_STOPPED;
  // XXX we should if possible read timers configs out of simulated
  // EEPROM, otherwise the timers are wiped each timer we change the
  // batteries
  memset(_timers, 0, sizeof(_timers));
  _current_timer = 0;

	menu_add_entry("TIMER", 
	    &up_press, 
	    &down_press, 
	    &num_press, 
	    &star_long_press,
			&num_long_press, 
			NULL, 
			&timer_activate,
			&timer_deactivated);

#ifdef CONFIG_MOD_TIMER_PRESETS
  uint8_t presets[] = {5, 15, 20, 30, 60};
  uint8_t idx;
  for (idx = 0; idx < sizeof(presets) && 
                idx < CONFIG_MOD_TIMER_COUNT; ++idx) {
    timer_set(_timers+idx, presets[idx]*SECS_PER_MINUTE);
  }
#endif

}
