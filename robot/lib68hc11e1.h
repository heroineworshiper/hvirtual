#ifndef LIB68HC11E1_H
#define LIB68HC11E1_H


#include <stdio.h>

#define RAM_SIZE 512

void fputw(int data, FILE *fd);
int fgetw(FILE *fd);
int read_s19(char *prog, int *address, char *path);
void do_serial(FILE *fd, char *serial_path);
FILE* run_68hc11e1(char *serial_path, 
	unsigned char *prog, 
	int size, 
	int debug);

// Functions for interpreting analog data

typedef struct
{
	unsigned char *start;
	unsigned char *pointer;
	unsigned char *end;
	unsigned char min;
	unsigned char max;
	int size;
} limit_buffer_t;

limit_buffer_t* new_limit_buffer(int size);
void delete_limit_buffer(limit_buffer_t *ptr);
void update_limit_buffer(limit_buffer_t *ptr, unsigned char value);
unsigned char limit_buffer_min(limit_buffer_t *ptr);
unsigned char limit_buffer_max(limit_buffer_t *ptr);





#endif
