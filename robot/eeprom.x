/*
 * $Id: eeprom.x,v 1.1.1.1 2003/10/14 07:54:38 heroine Exp $
 *
 * Boot block memory map for 68HC11 GNU tools
 */
MEMORY
{
  page0 (rwx) : ORIGIN = 0xb600, LENGTH = 0x200
  text  (rx)  : ORIGIN = 0xb600, LENGTH = 0x200
  data  (rwx) : ORIGIN = 0xb600, LENGTH = 0x200
}
