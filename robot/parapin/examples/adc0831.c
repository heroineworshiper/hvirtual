#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "parapin.h"


#define VCC LP_PIN02
#define CS  LP_PIN03
#define CLK LP_PIN04
#define DBG LP_PIN05

#define D0  LP_PIN10


#define TPD0    2500
#define TSETUP  250

#define NUM_SAMPLES  200


void inline clock()
{
  set_pin(CLK);
  clear_pin(CLK);
}



void terminate(int signo)
{
  puts("Shutting down...");
  clear_pin(VCC | CS | CLK | DBG);
  exit(0);
}


int acquire()
{
  int sample, i, total, iterations, wait;

  iterations = NUM_SAMPLES;
  total = 0;

  while (iterations--) {

    clear_pin(CS); /* pull CS low */
    wait = TSETUP;
    while (wait--); /* set-up time */
    clock(); /* tell chip to acquire */

    sample = 0;
    for (i = 0; i < 8; i++) {
      clock();
      wait = TPD0;
      while (wait--); /* wait for output to stabilize */
      sample <<= 1;
      sample |= (pin_is_set(D0) == D0);
    }

    set_pin(CS);

    total += sample;
    /* delay before next sample */
    wait = 7000;
    while (wait--);
  }

  return total/NUM_SAMPLES;
}


int main(int argc, char *argv[])
{
  signal(SIGINT, terminate);
  signal(SIGTERM, terminate);
  signal(SIGHUP, terminate);

  pin_init_user(LPT1);

  set_pin(VCC | CS);

  sleep(1);

  if (argc != 1) {
    for (;;) {
      set_pin(DBG);
      acquire();
      clear_pin(DBG);
      acquire();
    }
  } else {
    for (;;) {
      printf("Got sample: %d\n", acquire());
      sleep(1);
    }
  }

  return 0;
}
