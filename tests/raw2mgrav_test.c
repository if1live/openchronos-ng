#include <stdio.h>
#include <stdint.h>
int16_t raw_accel_to_mgrav(uint8_t value)
{
  /*
   * OK, here is the thing: if you look at the bit-to-mgrav tables, the mgrav
   * values approximately doubles for each higher bit. While we can use a
   * lookup table and compute the output "exactly" according to the datasheet,
   * we could also just multiply the 2s complement number by the unit mgrav
   * value, which is 18 mg for 2 g measurement range at 100 or 400Hz, and
   * 71 mg otherwise. While this leads to some lose of accuracy, the clarity
   * in code in my opinion more than makes up for it
   */

	// first determine which base unit of mg to use
	uint8_t unit_mgrav = 18;

  int16_t result = ((int8_t)value)*unit_mgrav;
  return result;
}

void p(uint8_t tval) {
  printf("0x%02X(%d) = %d\n", tval, (int8_t)tval,raw_accel_to_mgrav(tval));
}

int main(void) {
  p(0x81);
  p(0x01);
  p(0x7F);
  p(0xFF);
  p(0x00);
  p(0x0A);
  return 0;
}

