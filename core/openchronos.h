/**
	@file	openchronos.h
	@brief	Main include file

	@see		http://www.openchronos-ng.sourceforge.net
	@copyright

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __EZCHRONOS_H__
#define __EZCHRONOS_H__

// *************************************************************************************************
// Include section

#include <stdlib.h>

#ifndef TESTING
#include <msp430.h>
#else
#include "cc430f6137_testing.h"
#include <stdint.h>
#endif

#include <config/config.h>


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section

///	A item structure for menu_editmode_start.
struct menu_editmode_item {
	void (* select)(void);     /**< item selected callback */
	void (* deselect)(void);   /**< item deselected callback */
	void (* set)(int8_t step); /**< set value of item callback */
};

///	Handy prototype typedef for helpers_loop() function.
typedef void(* helpers_loop_fn_t)(uint8_t *, uint8_t, uint8_t, int8_t);

//* ************************************************************************************************
/// @brief	List of possible message types for the message bus.
/// @see	sys_messagebus_register()
/// @warning	The enum values are optimized to work with some drivers.
/// 			If you need to add a new entry, append it to the end !
//* ************************************************************************************************
enum sys_message
{
	// Drivers/rtca
	SYS_MSG_RTC_ALARM	= BIT0, /**< Alarm event from the hardware RTC. */
	SYS_MSG_RTC_SECOND	= BIT1, /**< Second event from the hardware RTC. */
	SYS_MSG_RTC_MINUTE	= BIT2, /**< Minute event from the hardware RTC. */
	SYS_MSG_RTC_HOUR	= BIT3, /**< Hour event from the hardware RTC. */
	SYS_MSG_RTC_DAY		= BIT4, /**< Day event from the hardware RTC. */
	SYS_MSG_RTC_MONTH	= BIT5, /**< Month event from the hardware RTC. */
	SYS_MSG_RTC_YEAR	= BIT6, /**< Year event from the hardware RTC. */

	// Drivers/timer
	SYS_MSG_TIMER_4S	= BIT7, /**< 4s (period) event from the hardware TIMER_0. */
	SYS_MSG_TIMER_20HZ	= BIT8, /**< 20HZ event from the hardware TIMER_0. */
	SYS_MSG_TIMER_PROG	= BIT9, /**< Programmable event from TIMER_0. */

	// Sensor/interrups
	SYS_MSG_AS_INT 		= BITA, /**<  Accelerometer event from the hardware Accel sensor. */
	SYS_MSG_PS_INT		= BITB,	/**<  Pressure event from the hardware Pressure sensor. */
	SYS_MSG_BATT		= BITC, /**<  Battery event from the hardware Voltage sensor. */
};

/// Linked list of nodes listening to the message bus.
struct sys_messagebus
{
	/// Callback for receiving messages from the system bus
	void (*fn)(enum sys_message);

	/// Bitfield of message types that the node wishes to receive
	enum sys_message listens;

	/// Pointer to the next node in the list
	struct sys_messagebus *next;
};


// *************************************************************************************************
// Prototypes section

//* ************************************************************************************************
/// @brief		Adds an entry to the main menu.
///
/// @details	This function is to be used by modules, so that they can be
/// 			visible in the main menu. A good place to call this function is from the
/// 			corresponding module's _init function.
/// @note		This function is NULL safe. You can set all of its parameters to NULL (except name)
/// 			if you don't need their functionality.
/// @note		The <i>name</i> string cannot be longer than 5 characters due to the LCD screen size.
//* ************************************************************************************************
void menu_add_entry(
	char const * name,			/**< Item name to be displayed in the menu */
	void (*up_btn_fn)(void),	/**< Callback for up button presses. */
	void (*down_btn_fn)(void),	/**< Callback for down button presses. */
	void (*num_btn_fn)(void),	/**< Callback for num button presses. */
	void (*lstar_btn_fn)(void),	/**< Callback for long star button presses. */
	void (*lnum_btn_fn)(void),	/**< Callback for long num button presses. */
	void (*updown_btn_fn)(void),/**< Callback for up&down button presses. */
	void (*activate_fn)(void),	/**< Callback for when the user switches into this entry in the menu. */
	void (*deactivate_fn)(void)	/**< Callback for when the user switches out from this entry in the menu. */
);

//* ************************************************************************************************
/// @brief		Enters edit mode.
///
/// @details	The edit mode is a mechanism that allows the user to change values being displayed
/// 			in the screen. For example, if a clock alarm is being displayed, then edit mode
/// 			can be used to increase/decrease the values of hours and minutes.
/// 			A good place to call this function is from the module's lstar_btn_fn function
/// 			(see menu_add_entry()).<br />See modules/alarm.c for an example how to use this.
//* ************************************************************************************************
void menu_editmode_start(
	/// Callback for when the user exits from the edit mode.
	void (* complete_fn)(void),
	/// A vector of #menu_editmode_item, it must be NULL terminated !
	struct menu_editmode_item *items
);

/// Include function defined in even_in_range.s @todo do we even need this??
unsigned short __even_in_range(unsigned short __value, unsigned short __bound);

//* ************************************************************************************************
/// @brief		Increment/decrements value by one without exiting the [lower, upper] interval.
///
/// @details	If the value meets the upper bound, it is restarted from lower bound.
/// 			If the value meets the lower bound, it is restarded from the upper bound.<br />
/// @note		For now, only steps of -1 and 1 are considered.
/// @see		menu_editmode_start
//* ************************************************************************************************
void helpers_loop(
	uint8_t *value,	/**< Value a pointer to the variable to be incremented. */
	uint8_t lower,	/**< Lower the lower bound for the loop interval. */
	uint8_t upper,	/**< Upper the upper bound for the loop interval. */
	int8_t step		/**< 1 for incrementing value, -1 for a decrement */
);
void helpers_loop_16(
	uint16_t *value,	/**< Value a pointer to the variable to be incremented. */
	uint16_t lower,	/**< Lower the lower bound for the loop interval. */
	uint16_t upper,	/**< Upper the upper bound for the loop interval. */
	int16_t step		/**< 1 for incrementing value, -1 for a decrement */
);


//* ************************************************************************************************
/// @brief	Registers a node in the message bus.
///
/// @details	Registers (add) a node to the message bus. A node can filter
/// 			what message(s) are to be received by setting the bitfield \b listens.
/// @see		sys_message, sys_messagebus, sys_messagebus_unregister
//* ************************************************************************************************
void sys_messagebus_register(

	/// Callback to receive messages from the message bus
	void (*callback)(enum sys_message),

	/// Only receive messages of this type
	enum sys_message listens
);

//* ************************************************************************************************
/// @brief	Unregisters a node from the message bus.
/// @see	sys_messagebus_register
//* ************************************************************************************************
void sys_messagebus_unregister(

	/// The same callback used on sys_messagebus_register()
	void (*callback)(enum sys_message)
);


#endif /* __EZCHRONOS_H__ */


// *************************************************************************************************
// Documentation section

/**
	@mainpage	openchronos-ng (new generation)

	@section contact_sec	Contact

	Project website: http://openchronos-ng.sourceforge.net
	IRC: \#openchronos @ chat.freenode.net

	<i>
		for mailing list, bug tracker and forum please go to the project website,
		you will find that info there.
	</i>

	@section devs_sec		For developers..

	If you are a module developer please take a look to the following topics,
	we choose them for you because they are the most important when starting a new module.

	<ol>
		<li>
			First have a look to <a href="http://sourceforge.net/p/
			openchronos-ng/wiki/Module%20build%20system/">our wiki</a> to understand how to
			create a module (including its sources) and make it appear in the openchronos menu config.
			It is really simple and should not take much of your time.
		</li>

		<li>
			Then have a look to menu_add_entry(), this is what you should call during
			module initialization to make it appear in the system menu. It is also in this function
			where you specify the module functions that are to be called
			when the user presses the ez430 chronos buttons.
		</li>

		<li>
			Your module is now receiving input but what about output?
			Have a look to drivers/display.h, you can display strings using #display_chars()
			and turn ON/OFF symbols using #display_symbol().
		</li>

		<li>
			Finally, if your module needs to execute some code periodically, then have a look to
			sys_messagebus_register() and #sys_message, on how to make your module listen
			for system events, like 1Hz events from the hardware timer.
		</li>
	</ol>

*/
