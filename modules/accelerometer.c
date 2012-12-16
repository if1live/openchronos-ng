/*
    modules/accelerometer.c: Accelerometer for Openchronos

    Copyright (C) 2011-2012 Paolo Di Prodi <paolo@robomotic.com>

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
#include <openchronos.h>

/* driver list */
#include <drivers/display.h>
#include <drivers/rtca.h>
#include <drivers/vti_as.h>
#include <drivers/buzzer.h>
// *************************************************************************************************
// Defines section

#define BIT(x) (1uL << (x))

enum ACCEL_MODE {
  ACCEL_MODE_OFF,
  ACCEL_MODE_ON,
  ACCEL_MODE_BACKGROUND
};

enum DISPLAY_AXIS {
  DISPLAY_AXIS_X,
  DISPLAY_AXIS_Y,
  DISPLAY_AXIS_Z,
  DISPLAY_AXIS_MAX,
};

// Stop acceleration measurement after 60 minutes to save battery
// This parameter is ignored if in background mode! Unit in minutes.
#define ACCEL_MEASUREMENT_TIMEOUT		(60u)

// *** Tunes for accelerometer synestesia
static note smb[] = {0x2588, 0x000F};

// *************************************************************************************************
// Global Variable section
struct accel
{
	enum ACCEL_MODE	mode;

	// Sensor raw data
	uint8_t	xyz[3];

	// Current acceleration data along one of the x, y or z axis, in mgrav
	int16_t data;

	// Same as above, but for the previous update. Used as part of an
	// exponential moving average to smooth the output
	int16_t data_prev;

	// Timeout: should be decreased with the 1 minute RTC event. When it reaches
	// 0 accelerometer is turned off. Unit is minutes.
	uint16_t timeout;

	// Determines which acceleration axis is displayed. This also affects which
	// axis' acceleration isn in data and data_prev
	enum DISPLAY_AXIS display_axis;
};

extern struct accel sAccel;

static enum {
	VIEW_SET_MODE = 0,
	VIEW_SET_PARAMS,
	VIEW_STATUS,
	VIEW_AXIS,
	VIEW_MAX
} submenu_state;


struct accel sAccel;

// Global flag for proper acceleration sensor operation
extern uint8_t as_ok;

// Prototypes
void display_data(uint8_t display_id);

// *************************************************************************************************
// @fn          is_measuring_acceleration
// @brief       Returns 1 if acceleration is currently measured.
// @param       none
// @return      u8		1 = acceleration measurement ongoing
// *************************************************************************************************
uint8_t is_measuring_acceleration(void)
{
	return ((sAccel.mode == ACCEL_MODE_ON) && (sAccel.timeout > 0));
}

// *************************************************************************************************
// @fn          raw_accel_to_mgrav
// @brief       Converts measured value to mgrav units
// @param       u8 value	g data from sensor 
// @return      u16			Acceleration (mgrav)
// *************************************************************************************************
int16_t raw_accel_to_mgrav(uint8_t value)
{
  /*
   * OK, here is the thing: if you look at the bit-to-mgrav tables, the mgrav
   * values approximately doubles for each successive bit. While we can use a
   * lookup table and compute the output _exactly_ according to the datasheet,
   * we could also just multiply the 2s complement number by the unit mgrav
   * value, which is 18 mg for 2 g measurement range at 100 or 400Hz, and
   * 71 mg otherwise. While this leads to some lose of accuracy, the clarity
   * of code in my opinion more than makes up for it
   */

	// first determine which base unit of mg to use
	uint8_t unit_mgrav = 71;

	if (as_config.range == 2 && 
	    (as_config.sampling == SAMPLING_100_HZ || 
	     as_config.sampling == SAMPLING_400_HZ)) {
	  unit_mgrav = 18;
  }

  int16_t result = ((int8_t)value)*unit_mgrav;
  return result;
}

void update_menu()
{
	// Depending on the state what do we do?
	switch (submenu_state) {
		case VIEW_SET_MODE:

			if(as_config.mode==AS_FALL_MODE)
				display_chars(0, LCD_SEG_L1_3_0 , "FALL", SEG_SET);
			else if(as_config.mode==AS_MEASUREMENT_MODE) 
				display_chars(0, LCD_SEG_L1_3_0 , "MEAS", SEG_SET);
      else if(as_config.mode==AS_ACTIVITY_MODE)
				display_chars(0, LCD_SEG_L1_3_0 , "ACTI", SEG_SET);
			
			display_chars(0, LCD_SEG_L2_4_0 , "MODE", SEG_SET);
			break;

		case VIEW_SET_PARAMS:
			display_chars(0, LCD_SEG_L2_4_0 , "SETS", SEG_SET);
			break;

		case VIEW_STATUS:
			display_chars(0,LCD_SEG_L2_4_0 , "STAT", SEG_SET);
			break;

		case VIEW_AXIS:
			display_chars(0,LCD_SEG_L2_4_0 , "DATA", SEG_SET);
			break;

		default:
			break;
	}

}

/* NUM (#) button pressed callback */
// This change the sub menu
static void num_pressed()
{
  submenu_state += 1;
  submenu_state %= VIEW_MAX;

  if (as_config.mode == AS_MEASUREMENT_MODE) {
    while ( submenu_state != VIEW_SET_MODE &&
            submenu_state != VIEW_AXIS ) {
      submenu_state += 1;
      submenu_state %= VIEW_MAX;
    }
  } 

  update_menu();
  lcd_screen_activate(0);
}

static void up_btn()
{
	// Depending on the state what do we do?
	switch (submenu_state) {
		case VIEW_SET_MODE:
			as_config.mode++;
			as_config.mode %= 3;
			change_mode(as_config.mode);
			update_menu();
			break;

		case VIEW_SET_PARAMS:
			_printf(0,LCD_SEG_L1_3_0 , "%04x", as_read_register(ADDR_CTRL));
			break;

		case VIEW_STATUS:
			_printf(0,LCD_SEG_L1_3_0, "%1u", as_status.all_flags);
			break;

		case VIEW_AXIS:
			if (as_config.mode == AS_MEASUREMENT_MODE) {
			  if (active_lcd_screen() != 1) lcd_screen_activate(1);
        else {
          sAccel.display_axis += 1;
          sAccel.display_axis %= DISPLAY_AXIS_MAX;
        }
      }
			break;

		default:
			break;
	}
}

static void down_btn()
{
	//not necessary at the moment
	/* update menu screen */
	lcd_screen_activate(0);
}

/* Star button long press callback. */
// This set/unset the background mode
static void star_long_pressed()
{
	if(sAccel.mode==ACCEL_MODE_ON)
	{
		sAccel.mode = ACCEL_MODE_BACKGROUND;
		//set the R symbol on so that is propagated when the user leaves the mode
 		display_symbol(0, LCD_ICON_RECORD, SEG_SET | BLINK_ON);
	}
	else if(sAccel.mode==ACCEL_MODE_BACKGROUND)
	{

		sAccel.mode = ACCEL_MODE_ON;
		//unset the R symbol
		display_symbol(0, LCD_ICON_RECORD, SEG_SET | BLINK_OFF);
	}
	/* update menu screen */
	lcd_screen_activate(0);
}

void display_data(uint8_t display_id)
{
	uint8_t raw_data=0;
	int16_t accel_data=0;

	display_clear(display_id,0);

	// Convert X/Y/Z values to mg
	switch (sAccel.display_axis)
	{
		case DISPLAY_AXIS_X: 	
			raw_data = sAccel.xyz[0];
			display_char(display_id,LCD_SEG_L1_3, 'X', SEG_SET);
			break;
		case DISPLAY_AXIS_Y: 	
			raw_data = sAccel.xyz[1];
			display_char(display_id,LCD_SEG_L1_3, 'Y', SEG_SET);
			break;
		case DISPLAY_AXIS_Z: 	
			raw_data = sAccel.xyz[2];
			display_char(display_id,LCD_SEG_L1_3, 'Z', SEG_SET);
			break;
		default:
		  display_chars2(display_id,1, "ERR", ALIGN_RIGHT, SEG_SET);
		  break;
	}
  
  accel_data = raw_accel_to_mgrav(raw_data);
	
	// update current and previous acceleration values
	sAccel.data_prev = sAccel.data;
	sAccel.data = accel_data;

	// Filter acceleration
	#ifdef FIXEDPOINT
	accel_data = (uint16_t)(((sAccel.data_prev * 2) + (sAccel.data * 8)) / 10);
	#else
	accel_data = (uint16_t)((sAccel.data_prev * 0.2) + (sAccel.data * 0.8));
	#endif

  display_chars2(
      display_id, 
      2, 
      _sprintf("%4d", accel_data), 
      ALIGN_RIGHT, 
      SEG_SET);

  /* This isn't really required b/c _sprintf puts in the sign anyway
  display_symbol(display_id,LCD_SYMB_ARROW_UP, SEG_OFF);
  display_symbol(display_id,LCD_SYMB_ARROW_DOWN, SEG_OFF);
	if (accel_data > 0) {
		display_symbol(display_id,LCD_SYMB_ARROW_UP, SEG_ON);
	} else if (accel_data < 0) {
		display_symbol(display_id,LCD_SYMB_ARROW_DOWN, SEG_ON);
	}
	*/
}

static void as_event(enum sys_message msg)
{
	if ( (msg & SYS_MSG_RTC_MINUTE) == SYS_MSG_RTC_MINUTE)
	{
		if(sAccel.mode == ACCEL_MODE_ON) sAccel.timeout--;
		//if timeout is over disable the accelerometer
		if(sAccel.timeout<1)
		{
			//disable accelerometer to save power			
			as_stop();
			//update the mode to remember
			sAccel.mode = ACCEL_MODE_OFF;
		}

	}

	if ( (msg & SYS_MSG_AS_INT) == SYS_MSG_AS_INT)
	{
		// Check the vti register for status information
		as_status.all_flags=as_get_status();

		// if we were in free fall or motion detection mode check for the event
		if(as_status.int_status.falldet || as_status.int_status.motiondet){

			// if such an event is detected enable the symbol
			display_symbol(0, LCD_ICON_ALARM , SEG_SET | BLINK_ON);
			
			/* update menu screen */
			lcd_screen_activate(0);

      // play a little song to indicate an event happened
      buzzer_play(smb);
		} else {
			as_get_data(sAccel.xyz);
		}
	}

	if (SYS_MSG_PRESENT(msg, SYS_MSG_TIMER_20HZ)) {
      if (as_config.mode == AS_MEASUREMENT_MODE) {
        display_data(1);
      }
  }	  
}

/* Enter the accelerometer menu */
static void acc_activated()
{
	//register to the system bus for vti events as well as the RTC minute events
	sys_messagebus_register(
	    &as_event, 
	    SYS_MSG_AS_INT | SYS_MSG_RTC_MINUTE | SYS_MSG_TIMER_20HZ
	);


	/* Create two screens, the first is always the active one. Screen 0 
	 * will contain the menu structure and screen 1 the raw accelerometer 
	 * data 
	 * */
	lcd_screens_create(2);

	
	// Show warning if acceleration sensor was not initialised properly
	if (!as_ok) display_chars(0, LCD_SEG_L1_3_0, "ERR", SEG_SET);
	else {
		/* Initialization is required only if not in background mode! */
		if (sAccel.mode != ACCEL_MODE_BACKGROUND)
		{
			// Clear previous acceleration value
			sAccel.data = 0;
			//
			// Set timeout counter
			sAccel.timeout = ACCEL_MEASUREMENT_TIMEOUT;

			// Set mode for screen 
			sAccel.mode = ACCEL_MODE_ON;

			// Start with mode selection
			submenu_state=VIEW_SET_MODE;

			// Select Axis X
			sAccel.display_axis=DISPLAY_AXIS_X;

			// 2 g range
			as_config.range=2;
			// 100 Hz sampling rate
			as_config.sampling=SAMPLING_100_HZ;

			/* 
			 * In motion detection mode the G range is also 8 g, and lets set
			 * this to something about 2 g for convenience's sake
			 */ 
			as_config.MDTHR=0x20;

			/*
			 * In both motion detect and free fall mode, the g value on either
			 * the x,y,z axis must exceed +/-MDTHR (in MD mode), and fall
			 * below +/- FFTHR (in FF mode), for an amount of time determined
			 * by MDFFTMR before an interrupt is raised. This acts as a basic
			 * filter to get rid of jitters.
			 *
			 * The lower 4 bits sets the detection time threshold for free fall;
			 * The upper 4 bits sets the detection time threshold for motion
			 * detection.
			 *
			 * The LSB of each nibble is equal to 1/sampling rage. Therefore
			 * if sampling rate is 100Hz, then 0x01 sets a detection time 
			 * threshold of 10ms.
			 *
			 * Note that motion detect mode is fixed at 10Hz sampling rage.
			 *
			 * 0x1A should set motion and free fall detection time threshold to 
			 * 100ms.
			 */ 
			as_config.MDFFTMR=0x1A;

			// Start sensor in motion detection mode
			as_config.mode = AS_ACTIVITY_MODE;

			// After this call interrupts will be generated
			as_start(as_config.mode);
		}

		if (!as_ok) display_chars(0, LCD_SEG_L1_3_0, "FAIL", SEG_SET);
	  else update_menu();
	}
	/* return to main screen */
	lcd_screen_activate(0);
}

void print_debug()
{
		// check if that is really in the mode we set
		
		_printf(0, LCD_SEG_L1_3_0, "%03x", as_read_register(ADDR_CTRL));
		_printf(0, LCD_SEG_L2_5_0, "%05x", as_read_register(ADDR_MDFFTMR));

}


/* Exit the accelerometer menu. */
/* Here we could decide to keep it in the background or not and
** also check when activated if it was in background */

static void acc_deactivated()
{


	/* destroy virtual screens */
	lcd_screens_destroy();

	/* clean up screen */
	display_clear(0, 0);

	/* do not disable anything if in background mode */
	if	(sAccel.mode != ACCEL_MODE_BACKGROUND)  {
    /* clear symbols only if not in backround mode*/
    //display_symbol(0, LCD_ICON_ALARM , SEG_SET | BLINK_OFF);

    /* otherwise shutdown all the stuff
    ** deregister from the message bus */
    sys_messagebus_unregister(&as_event);
    /* Stop acceleration sensor */
    as_stop();

    /* Clear mode */
    sAccel.mode = ACCEL_MODE_OFF;
  }
}



void mod_accelerometer_init()
{

	//if this is called only one time after reboot there are some important things to initialise
	//Initialise sAccel struct?
	sAccel.data=0;
	sAccel.data_prev=0;
	// Set timeout counter
	sAccel.timeout = ACCEL_MEASUREMENT_TIMEOUT;
	/* Clear mode */
	sAccel.mode = ACCEL_MODE_OFF;

	menu_add_entry("ACC",&up_btn, &down_btn,
			&num_pressed,
			&star_long_pressed,
			NULL,NULL,
			&acc_activated,
			&acc_deactivated);

}
