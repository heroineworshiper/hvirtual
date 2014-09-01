#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "parapin.h"

int main(int argc, char *argv[])
{
  int pin_sequence[] = { 1, 2, 3, 4, 5, 6, 7, 8, 17, 14, 16, 9, -1};
  int i;
  int prev = 0;
  char buf[240];

  if (pin_init_user(LPT1) < 0)
    exit(0);

  i = -1;
  while (1) {
    if (pin_sequence[++i] == -1)
      i = 0;
    printf("setting pin %d\n", pin_sequence[i]);
    set_pin(LP_PIN[pin_sequence[i]] | prev);
    printf("Hit return...\n");
    fgets(buf, 5, stdin);
    printf("clearing pin %d\n", pin_sequence[i]);
    clear_pin(LP_PIN[pin_sequence[i]] | prev);
    /*    prev = LP_PIN[pin_sequence[i]]; */
  } while (0);

}
