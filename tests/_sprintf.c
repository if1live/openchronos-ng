#include <stdio.h>
#include <stdint.h>
#define SPRINTF_STR_LEN 8

/* storage for itoa function */
static char sprintf_str[SPRINTF_STR_LEN];

char *_sprintf(const char *fmt, int16_t n) {
	int8_t i = 0;
	int8_t j = 0;
		
	while (1) {
		/* copy chars until end of string or a int substitution is found */ 
		while (fmt[i] != '%') {
			if (fmt[i] == '\0' || j == SPRINTF_STR_LEN - 2) {
				sprintf_str[j] = '\0';
				return sprintf_str;
			}
			sprintf_str[j++] = fmt[i++];
		}
		i++;

		int digits = 0;
		int8_t zpad = ' ';
		int8_t showsign = 0;

		/* parse int substitution */
		while (fmt[i] != 'd' && fmt[i] != 'u' && fmt[i] != 'x') {
			if (fmt[i] == '0')
				zpad = '0';
			else if (fmt[i] == '+') 
			  showsign = 1;
      else
				digits = fmt[i] - '0';
			i++;
		}

    // XXX the doc says we must specify the field width, so if digits
    // is zero here, return "P". Why P? Because you would never get P
    // in any of the conversions. 
    if (digits == 0) return "P";

		/* show sign */
		if (fmt[i] == 'd') {
			if (n < 0) {
				sprintf_str[j++] = '-';
				n = (~n) + 1;
			} else
			  if (showsign)
				  sprintf_str[j++] = '+';
		}

		j += digits - 1;
		int8_t j1 = j + 1;

		/* convert int to string */
		if (fmt[i] == 'x') {
			do {
				sprintf_str[j--] = "0123456789ABCDEF"[n & 0x0F];
				n >>= 4;
				digits--;
			} while (n > 0);
		} else {
			do {
				sprintf_str[j--] = n % 10 + '0';
				n /= 10;
				digits--;
			} while (n > 0);
		}

		/* pad the remaining */
		while (digits > 0) {
			sprintf_str[j--] = zpad;
			digits--;
		}

		j = j1;
		i++;
	}

	return sprintf_str;
}

int main(void) {
  char *str = _sprintf("%02d", 20);
  printf(">%s<\n", str);
  return 0;
}
