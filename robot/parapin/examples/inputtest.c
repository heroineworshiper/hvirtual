#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "parapin.h"

int main(int argc, char *argv[])
{
  int i;

  if (pin_init_user(LPT1) < 0)
    exit(0);

  pin_input_mode(LP_DATA_PINS);
  pin_input_mode(LP_SWITCHABLE_PINS);

  printf("\nstarting\n");
  for (i = 1; i <= 17; i++)
    printf("Pin %d: %s\n", i, pin_is_set(LP_PIN[i]) ? "HIGH":"LOW");
  printf("\n\n");
  return 0;
}
