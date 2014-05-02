#include <core/openchronos.h>
#include <drivers/rtca.h>
#include <stdbool.h>
#include <string.h>
#include "drivers/display.h"


#define EDIT_YY_FIELD LCD_SEG_L1_3_0
#define EDIT_MM_FIELD LCD_SEG_L2_4_3
#define EDIT_DD_FIELD LCD_SEG_L2_1_0

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
} date_t;

enum {
	D_DAY_VIEW_MMDD = 0,
	D_DAY_VIEW_YYYY,
	D_DAY_VIEW_COUNT
};

static date_t g_dday;
static date_t tmp_date;
static uint8_t g_view_state;

uint8_t get_number_of_days(uint8_t month, uint16_t year);
int16_t calc_d_day(date_t *target, date_t *today);

/******************** display functions *****************/
static void display_view()
{
	display_clear(0, 1);
	display_clear(0, 2);

	if(g_view_state == D_DAY_VIEW_MMDD) {
#ifdef CONFIG_MOD_CLOCK_MONTH_FIRST
		_printf(0, LCD_SEG_L1_3_2, "%2u", g_dday.month);
		_printf(0, LCD_SEG_L1_1_0, "%2u", g_dday.day);
#else
		_printf(0, LCD_SEG_L1_3_2, "%2u", g_dday.day);
		_printf(0, LCD_SEG_L1_1_0, "%2u", g_dday.month);
#endif
	} else if(g_view_state == D_DAY_VIEW_YYYY) {
		_printf(0, LCD_SEG_L1_3_0, "%4u", g_dday.year);
	}

	date_t today = { rtca_time.year, rtca_time.mon, rtca_time.day };
	int16_t remain = calc_d_day(&g_dday, &today);
	remain %= 1000;
	if(remain < 0) {
		_printf(0, LCD_SEG_L2_5_0, " D-%3d", remain);
	} else if(remain > 0) {
		_printf(0, LCD_SEG_L2_5_0, " D+3d", remain);
	} else {
		display_chars(0, LCD_SEG_L2_5_0, " D-DAY", SEG_ON);
	}
}

static void display_edit()
{
	display_clear(0, 1);
	display_clear(0, 2);

	_printf(0, EDIT_YY_FIELD, "%4u", tmp_date.year);
	_printf(0, EDIT_MM_FIELD, "%2u", tmp_date.month);
	_printf(0, EDIT_DD_FIELD, "%2u", tmp_date.day);
}

/*************************** edit mode callbacks **************************/
static void edit_yy_sel(void)
{
	display_chars(0, EDIT_YY_FIELD, NULL, BLINK_ON);
}

static void edit_yy_dsel(void)
{
	display_chars(0, EDIT_YY_FIELD, NULL, BLINK_OFF);
}

static void edit_yy_set(int8_t step)
{
	helpers_loop_16(&tmp_date.year, 2000, 2020, step);
	_printf(0, EDIT_YY_FIELD, "%4u", tmp_date.year);
}

static void edit_mm_sel(void)
{
	display_chars(0, EDIT_MM_FIELD, NULL, BLINK_ON);
}

static void edit_mm_dsel(void)
{
	display_chars(0, EDIT_MM_FIELD, NULL, BLINK_OFF);
}

static void edit_mm_set(int8_t step)
{
	helpers_loop(&tmp_date.month, 1, 12, step);
	_printf(0, EDIT_MM_FIELD, "%2u", tmp_date.month);
}

static void edit_dd_sel(void)
{
	display_chars(0, EDIT_DD_FIELD, NULL, BLINK_ON);
}

static void edit_dd_dsel(void)
{
	display_chars(0, EDIT_DD_FIELD, NULL, BLINK_OFF);
}

static void edit_dd_set(int8_t step)
{
	uint8_t day_limit = get_number_of_days(tmp_date.month, tmp_date.year);
	helpers_loop(&tmp_date.day, 1, day_limit, step);
	_printf(0, EDIT_DD_FIELD, "%2u", tmp_date.day);
}


static void edit_save(void)
{
	g_dday = tmp_date;
	display_view();
}


/* edit mode item table */
static struct menu_editmode_item edit_items[] = {
	{&edit_yy_sel, &edit_yy_dsel, &edit_yy_set},
	{&edit_mm_sel, &edit_mm_dsel, &edit_mm_set},
	{&edit_dd_sel, &edit_dd_dsel, &edit_dd_set},
	{ NULL },
};


/******************** menu callbacks **************************************/
static void d_day_activate(void)
{
	display_symbol(0, LCD_SEG_L1_COL, SEG_OFF);
	g_view_state = D_DAY_VIEW_MMDD;

	display_view();
}

static void d_day_deactivate(void)
{
	display_clear(0, 1);
	display_clear(0, 2);
}

static void star_long_pressed()
{
	tmp_date = g_dday;
	display_edit();
	menu_editmode_start(&edit_save, edit_items);
}

static void up_btn_pressed()
{
	g_view_state = (g_view_state + 1) % D_DAY_VIEW_COUNT;
	display_view();
}

void mod_d_day_init(void)
{
	menu_add_entry("D-DAY",
		&up_btn_pressed,
		NULL, NULL,
		&star_long_pressed,
		NULL, NULL,
		&d_day_activate, &d_day_deactivate);

	g_dday.year = CONFIG_MOD_D_DAY_YEAR;
	g_dday.month = CONFIG_MOD_D_DAY_MONTH;
	g_dday.day = CONFIG_MOD_D_DAY_DAY;
}

/****************** date library *********************************/
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
	for(i = 1 ; i < month ; ++i)
	{
		sum_day += get_number_of_days(i, year);
	}
	sum_day += day;
	return sum_day;
}

int16_t calc_d_day(date_t *target, date_t *today)
{
	int16_t today_count = get_nth_day_from_2000(today->year, today->month, today->day);
	int16_t dday_count = get_nth_day_from_2000(target->year, target->month, target->day);
	return today_count - dday_count;
}


#ifdef TESTING
#include "../greatest/greatest.h"

TEST test_is_leap_year()
{
	ASSERT_EQ(is_leap_year(2012), true);
	ASSERT_EQ(is_leap_year(2013), false);
	ASSERT_EQ(is_leap_year(2014), false);
	ASSERT_EQ(is_leap_year(2015), false);
	ASSERT_EQ(is_leap_year(2016), true);
	ASSERT_EQ(is_leap_year(2017), false);
	PASS();
}

TEST test_get_number_of_days_in_year()
{
	ASSERT_EQ(get_number_of_days_in_year(2012), 366);
	ASSERT_EQ(get_number_of_days_in_year(2013), 365);
	PASS();
}

TEST test_get_number_of_days()
{
	ASSERT_EQ(get_number_of_days(2, 2012), 29);
	ASSERT_EQ(get_number_of_days(2, 2013), 28);
	PASS();
}

TEST test_get_nth_day_from_2000()
{
	ASSERT_EQ(get_nth_day_from_2000(2000, 1, 1), 1);

	ASSERT_EQ(get_nth_day_from_2000(2000, 1, 31), 31);
	ASSERT_EQ(get_nth_day_from_2000(2000, 2, 1), 31+1);

	ASSERT_EQ(get_nth_day_from_2000(2001, 1, 1), 366+1);
	ASSERT_EQ(get_nth_day_from_2000(2002, 1, 1), 366+365+1);

	PASS();
}

TEST test_calc_d_day()
{
	date_t today = {2014, 5, 2};
	date_t day_7_31 = {2014, 7, 31};
	date_t day_8_1 = {2014, 8, 1};

	ASSERT_EQ(calc_d_day(&day_7_31, &today), -90);
	ASSERT_EQ(calc_d_day(&day_8_1, &today), -91);

	ASSERT_EQ(calc_d_day(&today, &today), 0);

	ASSERT_EQ(calc_d_day(&today, &day_7_31), +90);

	PASS();
}


TEST test_foo()
{
	PASS();
}

SUITE(d_day_suite)
{
	RUN_TEST(test_is_leap_year);
	RUN_TEST(test_get_number_of_days_in_year);
	RUN_TEST(test_get_number_of_days);
	RUN_TEST(test_get_nth_day_from_2000);
	RUN_TEST(test_calc_d_day);
}

#endif
