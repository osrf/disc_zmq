#include "common.h"

#ifdef TEST_MSP430
int putchar(int c)
{
  printf("%c", c);
}
#endif

