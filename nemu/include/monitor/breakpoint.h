#ifndef __BREAKPOINT_H__
#define __BREAKPOINT_H__

#include "common.h"
#include "memory/memory.h"

typedef struct breakpoint{
	int NO;
	struct breakpoint *next;
	char expr[32];
	swaddr_t addr;
	unsigned char data;

}BP;

void add_bp(char *e);

#endif
