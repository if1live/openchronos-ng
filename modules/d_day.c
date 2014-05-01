#include <core/openchronos.h>
#include <drivers/rtca.h>
#include <stdbool.h>
#include "drivers/display.h"

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
} date_t;

static date_t g_dday;

void fill_d_day_message(uint8_t *src, date_t *date);


static void display_d_day(void)
{
}

static void d_day_activate(void)
{
	// I don't need D-Day year.
#ifdef CONFIG_MOD_CLOCK_MONTH_FIRST
	_printf(0, LCD_SEG_L1_3_2, "%2u", g_dday.month);
	_printf(0, LCD_SEG_L1_1_0, "%2u", g_dday.day);
#else
	_printf(0, LCD_SEG_L1_3_2, "%2u", g_dday.day);
	_printf(0, LCD_SEG_L1_1_0, "%2u", g_dday.month);
#endif

	uint8_t line[7] = {0};
	fill_d_day_message(line, &g_dday);
	display_chars(0, LCD_SEG_L2_5_0, line, SEG_ON);
}

static void d_day_deactivate(void)
{
	display_clear(0, 1);
	display_clear(0, 2);
}

void mod_d_day_init(void)
{
	menu_add_entry("D-DAY", NULL, NULL, NULL, NULL, NULL, NULL,
				   &d_day_activate, &d_day_deactivate);

	g_dday.year = CONFIG_MOD_D_DAY_YEAR;
	g_dday.month = CONFIG_MOD_D_DAY_MONTH;
	g_dday.day = CONFIG_MOD_D_DAY_DAY;
}

bool is_leap_year(uint16_t year)
{
	// 1. A year that is divisible by 4 is a leap year. (Y % 4) == 0
	// 2. Exception to rule 1: a year that is divisible by 100 is not a leap year. (Y % 100) != 0
	// 3. Exception to rule 2: a year that is divisible by 400 is a leap year. (Y % 400) == 0
	return ((year%4==0) && ((year%100!=0) || (year%400==0)));
}

uint8_t get_number_of_days(uint8_t month, uint16_t year)
{
	switch(month)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return (31);

	case 4:
	case 6:
	case 9:
	case 11:
		return (30);

	case 2:
		return is_leap_year(year) ? (29) : (28);

	default:
		return (0);
	}
}

uint16_t get_number_of_days_in_year(uint16_t year)
{
	return is_leap_year(year) ? 366 : 365;
}

uint16_t get_nth_day_from_2000(uint16_t year, uint8_t month, uint8_t day)
{
	if(year < 2000)
	{
		return 0;
	}
	uint16_t sum_day = 0;

	uint8_t i = 0;
	for(i = 0 ; i < year - 2000 ; ++i)
	{
		uint16_t curr_year = 2000 + i;
		sum_day += get_number_of_days_in_year(curr_year);
	}
	uint8_t past_month = month - 1;
	for(i = 1 ; i < past_month ; ++i)
	{
		sum_day += get_number_of_days(i, year);
	}
	sum_day += day;
	return sum_day;
}

int16_t calc_d_day(uint16_t year, uint8_t month, uint8_t day)
{
	int16_t today = get_nth_day_from_2000(rtca_time.year, rtca_time.mon, rtca_time.day);
	int16_t dday = get_nth_day_from_2000(year, month, day);
	return today - dday;
}

void fill_d_day_message(uint8_t *src, date_t *date)
{
	/*
	  make src like " D-123"
	 */
	int16_t remain = calc_d_day(date->year, date->month, date->day);
	memset(src, 7, sizeof(uint8_t));
	src[0] = ' ';
	src[1] = 'D';
	if(remain <= 0) {
		src[2] = '-';
	} else {
		// cannot render '.'
		src[2] = '.';
	}

	remain = abs(remain);
	//usable index: 3, 4, 5
	src[3] = (remain / 100) % 10;
	src[4] = (remain % 100) / 10;
	src[5] = (remain % 10);

	uint8_t i = 0;
	for(i = 3 ; i <= 5 ; i++) {
		if(src[i] == 0) {
			src[i] = ' ';
		} else {
			src[i] += '0';
		}
	}
	if(remain == 0) {
		src[3] = 'D';
		src[4] = 'A';
		src[5] = 'Y';
	}
}
