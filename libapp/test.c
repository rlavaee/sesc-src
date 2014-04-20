#include <stdio.h>

int main() {
  register unsigned int cp0count asm ("$c0r1");
  unsigned int d;

  d = cp0count + 3;

  printf("d=%d\n", d);
  
  return 0;
}
