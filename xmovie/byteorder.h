#ifndef BYTEORDER_H
#define BYTEORDER_H

#include "sizes.h"

inline int get_byte_order()
{                // 1 if little endian
	int32_t byteordertest;
	int byteorder;

	byteordertest = 0x00000001;
	byteorder = *((unsigned char *)&byteordertest);
	return byteorder;
}

#define SWAP_ITERATE byte1 = buffer[i++]; \
      							 byte2 = buffer[i--]; \
      							 buffer[i++] = byte2; \
      							 buffer[i++] = byte1;  
      							                                         
#define SWAP_24BIT_ITERATE \
								 byte1 = buffer[i]; \
      							 byte2 = buffer[j]; \
      							 byte3 = buffer[k]; \
      							 buffer[i] = byte3; \
      							 buffer[j] = byte2; \
      							 buffer[k] = byte1; \
      							 i += 3; j += 3; k += 3;

inline int swap_bytes(int wordsize, char *buffer, long len)
{
  register unsigned char byte1, byte2, byte3;
  register long i = 0, j = 0, k = 0;
  
	switch(wordsize)
	{
		case 1:
			return 0;
			break;
		
		case 2:
  		len -= 8;
  		while(i < len){
    		SWAP_ITERATE
    		SWAP_ITERATE
    		SWAP_ITERATE
    		SWAP_ITERATE
  		}

  		len += 8;
  		while(i < len){
    		SWAP_ITERATE
  		}
			return 0;
			break;
		
		case 3:
  		len -= 12;
  		while(i < len){
    		SWAP_24BIT_ITERATE
    		SWAP_24BIT_ITERATE
    		SWAP_24BIT_ITERATE
    		SWAP_24BIT_ITERATE
  		}

  		len += 12;
  		while(i < len){
    		SWAP_24BIT_ITERATE
  		}
			return 0;
			break;
	}
	return 0;
}

#endif
