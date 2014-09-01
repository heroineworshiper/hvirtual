#ifndef ENC_PORTAB_H_
#define ENC_PORTAB_H_

#if defined(WIN32)

#include <windows.h>

#define int8_t char
#define uint8_t unsigned char
#define int16_t short
#define uint16_t unsigned short
#define int32_t int
#define uint32_t unsigned int
#define int64_t __int64
#define uint64_t unsigned __int64

#if defined(_MMX_)
#define EMMS __asm {emms}
#else
#define EMMS
#endif

// needed for bitstream.h
#define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax

// needed for timer.c
static __inline int64_t read_counter() {
	int64_t ts;
	uint32_t ts1, ts2;

	__asm {
		rdtsc
		mov  ts1, eax
		mov  ts2, edx
	}
	
	ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
    
	return ts;
}

#elif defined(LINUX)

#include <stdint.h>

#if defined(_MMX_)
#define EMMS __asm__("emms\n\t")
#else
#define EMMS
#endif

// needed for bitstream.h
#define BSWAP(a) __asm__ ( "bswapl %0\n" : "=r" (a) : "0" (a) )

// needed for timer.c
static __inline int64_t read_counter() {
    int64_t ts;
    uint32_t ts1, ts2;

//    __asm__ __volatile__("rdtsc\n\t":"=a"(ts1), "=d"(ts2));

//    ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);

    return ts;
}

#else // OTHER OS

#include <inttypes.h>

#define EMMS

// needed for bitstream.h
#define BSWAP(a) \
	 ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))

// rdtsc command most likely not supported,
// so just dummy code here
static __inline int64_t read_counter() {
	return 0;
}

#endif

#endif // _PORTAB_H_

