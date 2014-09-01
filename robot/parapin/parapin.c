/*#define DEBUG*/
/*
 * parapin
 *
 * $Id: parapin.c,v 1.1.1.1 2003/10/14 07:54:38 heroine Exp $
 *
 * $Log: parapin.c,v $
 * Revision 1.1.1.1  2003/10/14 07:54:38  heroine
 *
 *
 * Revision 1.1  2003/03/01 09:48:05  amazon
 * *** empty log message ***
 *
 * Revision 1.1  2003/03/01 09:20:32  amazon
 * *** empty log message ***
 *
 * Revision 1.1  2002/09/01 23:29:49  amazon
 * *** empty log message ***
 *
 * Revision 1.10  2000/03/25 00:58:36  jelson
 * fixed bug in interrupt handling
 *
 * Revision 1.9  2000/03/24 21:13:46  jelson
 * more changes for interrupt handling
 *
 * Revision 1.8  2000/03/24 01:39:03  jelson
 * More functions in support of interrupt handling
 *
 * Revision 1.7  2000/03/23 01:25:09  jelson
 * Corrected bit number of IRQ enable line
 *
 * Revision 1.6  2000/03/22 05:06:28  jelson
 * Mostly minor interface changes (e.g., changes to constant names) to make
 * the interface cleaner... correcting problems that came up when I was writing
 * the documentation.
 *
 * Added a way to pass a pointer to an interrupt handler through parapin to
 * the kernel.
 *
 * Revision 1.5  2000/03/14 23:57:48  jelson
 * mostly minor changes -- cleanup for public release
 *
 * Revision 1.4  2000/03/14 21:12:42  jelson
 * Made parapin more kernel friendly -- it now has different initialization
 * functions for user-land use vs. kernel-land use.  The kernel version of
 * the library uses the "parport" functions to register itself as a parallel
 * device and claim exclusive access to the parallel port.
 *
 * Revision 1.3  2000/03/08 01:34:26  jelson
 * Minor changes to make parapin kernel-friendly
 *
 * Revision 1.2  1999/05/26 06:43:39  jelson
 * *** empty log message ***
 *
 * Revision 1.1  1999/05/24 03:59:30  jelson
 * first version of new parapin programming library
 *
 */

#ifdef __KERNEL__

#include <linux/config.h>
#include <linux/kernel.h>
#ifdef MODULE
#include <linux/module.h>
#endif /* MODULE */
#include <linux/stddef.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/parport.h>
#include <linux/parport_pc.h>
#include <linux/interrupt.h>

#define printf printk

/* parallel port we found and device we registered */
struct parport *parapin_port = NULL;
struct pardevice *parapin_dev = NULL;

/* was the system configured to use the interrupt before we got here? */
static int old_irq_mode = 0;
static int have_irq_handler = 0;

#else /* __KERNEL__ */

#include <stdio.h>
#include <linux/ioport.h>
#include <sys/io.h>

#endif /* __KERNEL__ */

#include "parapin.h"

/* base address of parallel port I/O */
static int lp_base;

/* most recently read or written values of the three parallel port I/O
 * registers.  kept in TRUE logic; i.e. the actual high/low electrical
 * state on the pins, NOT the values of register bits. */
static int lp_register[3];

/* masks of pins that we currently allow input and output on */
static int lp_input_pins;
static int lp_output_pins;



/************************* Basic Utilities *****************************/


#ifdef DEBUG
char *bin_to_char(int num)
{
  int i;
  static char buf[9];

  buf[8] = '\0';

  for (i = 0; i < 8; i++) {
    if (num & (1 << i))
      buf[7-i] = '1';
    else
      buf[7-i] = '0';
  }

  return buf;
      
}
#endif

/* read a byte from an I/O port (base + regnum), correct for inverted
 * logic on some bits, and store in the lp_register variable.  Return
 * the value read.  */
static inline int read_register(int regnum)
{
#ifdef DEBUG
  int result;

  result = (lp_register[regnum] =
          (inb(lp_base + (regnum)) ^ lp_invert_masks[regnum]));

  printf("read %s from register %d (addr %x)\n", bin_to_char(result),
         regnum, lp_base + regnum);

  return result;
#else
  return (lp_register[regnum] =
          (inb(lp_base + (regnum)) ^ lp_invert_masks[regnum]));
#endif
}


/* store the written value in the lp_register variable, correct for
 * inverted logic on some bits, and write a byte to an I/O port (base
 * + regnum) */
static inline void write_register(int value, int regnum)
{
#ifdef DEBUG
  printf("writing %s to register %d (addr %x)\n",
         bin_to_char(value ^ lp_invert_masks[regnum]),
         regnum, lp_base + regnum);
#endif

  outb((lp_register[regnum] = value) ^ lp_invert_masks[regnum],
       lp_base + regnum);
}



/********************* Initialization *************************************/


/* Initialization that kernel-mode and user-mode have in common */
static void pin_init_internal()
{
  /* initialize data register to all 0 */
  write_register(0, 0);

  /* read status register */
  read_register(1);

  /* initialize control register - data pins and switchable pins both
     in output mode */
  write_register(0, 2);

  /* initially, we can write to the datapins and switchable pins, and
   * can read from the "always input pins".  also, we are always
   * allowed to read from and write to the INPUT_MODE and IRQ_MODE
   * "pins" (control bits, really) */
  lp_output_pins = LP_DATA_PINS | LP_SWITCHABLE_PINS | 
    LP_INPUT_MODE | LP_IRQ_MODE;
  lp_input_pins = LP_ALWAYS_INPUT_PINS | LP_INPUT_MODE | LP_IRQ_MODE;
}


#ifdef __KERNEL__

/* kernel-mode parallel port initialization.  the 'lpt' argument is
 * the LP number of the port that you want.  this code will
 * EXCLUSIVELY claim the parallel port; i.e., so that other devices
 * connected to the same parallel port will not be able to use it
 * until you call the corresponding pin_release().  */
int pin_init_kernel(int lpt, void (*irq_func)(int, void *, struct pt_regs *))
{
  /* find the port */
  for (parapin_port = parport_enumerate(); parapin_port != NULL;
       parapin_port = parapin_port->next)
    if (parapin_port->number == lpt)
      break;

  if (parapin_port == NULL) {
    printk("parapin: init failed, parallel port lp%d not found\n", lpt);
    return -ENODEV;
  }

  /* now register the device on the port for exclusive access */
  parapin_dev = parport_register_device(parapin_port,
					"parapin", /* name for debugging */
					NULL, /* preemption callback */
					NULL, /* wakeup callback */
					irq_func, /* interrupt callback */
					PARPORT_DEV_EXCL, /* flags */
					NULL); /* user data */
  if (parapin_dev == NULL)
    return -ENODEV;

  /* ok - all systems go.  claim the parallel port. */
  parport_claim(parapin_dev);

  /* remember the LP base of our parallel port */
  lp_base = parapin_port->base;

  /* put us into bidir mode if we have an ECR */
//  if (parapin_port->modes & PARPORT_MODE_PCECR)
    parport_pc_write_econtrol(parapin_port, 0x20);

  /* initialize the state of the registers */
  pin_init_internal();

  /* remember the current state of the interrupt enable flag */
  old_irq_mode = pin_is_set(LP_IRQ_MODE);
  have_irq_handler = (irq_func != NULL);

  /* disable interrupts */
  pin_disable_irq();

  /* tell the user what's happening */
  printk("parapin: claiming %s at 0x%lx, irq %d\n", parapin_port->name,
         parapin_port->base, parapin_port->irq);

  return 0;
}


/* this must be called by kernel programs when you're done using the
   parallel port.  it releases the port to be used by other apps. */
void pin_release()
{
  /* restore interrupts to their former state */
  change_pin(LP_IRQ_MODE, old_irq_mode ? LP_SET : LP_CLEAR);

  /* release and unregister the parallel port */
  parport_release(parapin_dev);
  parport_unregister_device(parapin_dev);
}


/* are interrupts available? */
int pin_have_irq()
{
  return (parapin_port && have_irq_handler && (parapin_port->irq >= 0));
}

/* turn interrupts on */
void pin_enable_irq()
{
  if (pin_have_irq()) {
    set_pin(LP_IRQ_MODE);
    udelay(10);
    enable_irq(parapin_port->irq);
  }
}

/* turn interrupts off */
void pin_disable_irq()
{
  if (parapin_port && parapin_port->irq >= 0)
    disable_irq(parapin_port->irq);
  clear_pin(LP_IRQ_MODE);
}


#else  /* user-space-only functions */

/* user-land initialization */
int pin_init_user(int base)
{
  lp_base = base;

  /* get write permission to the I/O port */
  if (ioperm(lp_base, 3, 1) < 0) {
    perror("can't get IO permissions!");
    return -1;
  }

  pin_init_internal();
  return 0;
}

#endif /* __KERNEL__ */


/******************** Pin Setting and Clearing **************************/


/* set output pins in the data register */
static inline void set_datareg(int pins)
{
  write_register(lp_register[0] | pins, 0);
}

/* clear output pins in the data register */
static inline void clear_datareg(int pins)
{
  write_register((lp_register[0] & (~pins)), 0);
}

/* set output pins in the control register */
static inline void set_controlreg(int pins)
{
  /* the control register requires:
   *    - switchable pins that are currently being used as inputs must be 1
   *    - all other pins may be either set or cleared
   */

  /* read existing register into lp_register[2] */
  read_register(2);

  /* set the requested bits to 1, leaving the others unchanged */
  lp_register[2] |= pins;

  /* set all inputs to one (they may have been read as 0's!) 1 */
  lp_register[2] |= (0x0f & ((lp_input_pins & LP_SWITCHABLE_PINS) >> 16));

  /* write it back */
  write_register(lp_register[2], 2);
}


/* clear output pins in the control register */
static inline void clear_controlreg(int pins)
{
  /* the control register requires:
   *    - switchable pins that are currently being used as inputs must be 1
   *    - all other pins may be either set or cleared
   */

  /* read existing register into lp_register[2] */
  read_register(2);

  /* clear the requested pins, leaving others unchanged */
  lp_register[2] &= (~pins);

  /* set all inputs to one (they may have been read as 0's!) 1 */
  lp_register[2] |= (0x0f & ((lp_input_pins & LP_SWITCHABLE_PINS) >> 16));

  /* write it back */
  write_register(lp_register[2], 2);
}



/*
 * externally visible function: set_pin(int pins)
 *
 * this can be used to set any number of pins without disturbing other pins.
 *
 * example: set_pin(LP_PIN02 | LP_PIN05 | LP_PIN07)
 */
void set_pin(int pins)
{
  /* make sure the user is only trying to set an output pin */
  pins &= lp_output_pins;

  /* is user trying to set pins high that are data-register controlled? */
  if (pins & LPBASE0_MASK)
    set_datareg(pins & LPBASE0_MASK);

  /* is user trying to set pins high that are control-register controlled? */
  if (pins & LPBASE2_MASK)
    set_controlreg((pins & LPBASE2_MASK) >> 16);
}

/*
 * externally visible function: clear_pin(int pins)
 *
 * same interface as set_pin, except that it clears instead
 */
void clear_pin(int pins)
{
  /* make sure the user is only trying to set an output pin */
  pins &= lp_output_pins;

  /* is user trying to clear pins that are data-register controlled? */
  if (pins & LPBASE0_MASK)
    clear_datareg(pins & LPBASE0_MASK);

  /* is user trying to clear pins that are control-register controlled? */
  if (pins & LPBASE2_MASK)
    clear_controlreg((pins & LPBASE2_MASK) >> 16);
}


/*
 * a different interface to set_pin and change_pin;
 *
 * change_pin(X, LP_SET) is the same as set_pin(X);
 * change_pin(X, LP_CLEAR) is the same as clear_pin(X);
 */
void change_pin(int pins, int mode)
{
  if (mode == LP_SET)
    set_pin(pins);
  else if (mode == LP_CLEAR)
    clear_pin(pins);
}


/* pin_is_set: takes any number of pins to check, and returns a
 * corresponding bitvector with 1's set on pins that are electrically
 * high. */
int pin_is_set(int pins)
{
  int result = 0;

  /* make sure the user is only trying to read an output pin */
  pins &= lp_input_pins;

  if (pins & LPBASE0_MASK) {
    result |= (read_register(0) & (pins & LPBASE0_MASK));
  }

  if (pins & LPBASE1_MASK) {
    result |= ((read_register(1) & ((pins & LPBASE1_MASK) >> 8)) << 8);
  }

  if (pins & LPBASE2_MASK) {
    result |= ((read_register(2) & ((pins & LPBASE2_MASK) >> 16)) << 16);
  }

  return result;
}



/************************* Direction Changing *****************************/

/* change the data pins (pins 2-9) to input mode */
static void dataline_input_mode()
{
#ifdef DEBUG
  printf("setting input mode on data pins\n");
#endif
  lp_input_pins |= LP_DATA_PINS;
  lp_output_pins &= (~LP_DATA_PINS);
  set_pin(LP_INPUT_MODE);
}


static void dataline_output_mode()
{
#ifdef DEBUG
  printf("setting output mode on data pins\n");
#endif
  lp_input_pins &= (~LP_DATA_PINS);
  lp_output_pins |= LP_DATA_PINS;
  clear_pin(LP_INPUT_MODE);
}


static void controlreg_input_mode(int pins)
{
#ifdef DEBUG
  printf("setting control-reg input mode\n");
#endif
  lp_input_pins |= pins;
  lp_output_pins &= (~pins);
  set_controlreg(0);
}

static void controlreg_output_mode(int pins)
{
#ifdef DEBUG
  printf("setting control-reg output mode\n");
#endif
  lp_input_pins &= (~pins);
  lp_output_pins |= pins;
  set_controlreg(0);
}


/* user-visible function to change pins to input mode */
void pin_input_mode(int pins)
{
  if (pins & LP_DATA_PINS)
    dataline_input_mode();
  if (pins & LP_SWITCHABLE_PINS)
    controlreg_input_mode(pins & LP_SWITCHABLE_PINS);
}

/* user-visible function to change pins to output mode */
void pin_output_mode(int pins)
{
  if (pins & LP_DATA_PINS)
    dataline_output_mode();
  if (pins & LP_SWITCHABLE_PINS)
    controlreg_output_mode(pins & LP_SWITCHABLE_PINS);
}

/* another interface to pin_input_mode and pin_output_mode */
void pin_mode(int pins, int mode)
{
  if (mode == LP_INPUT)
    pin_input_mode(pins);
  if (mode == LP_OUTPUT)
    pin_output_mode(pins);
}

