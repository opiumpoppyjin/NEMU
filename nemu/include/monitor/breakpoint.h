#ifndef __BREAKPOINT_H__
#define __BREAKPOINT_H__

#include "common.h"


typedef struct breakpoint{
	int NO;
	struct breakpoint *next;
	char expr[32];
	swaddr_t addr;
	unsigned char data;

}BP;

void add_bp(char *e);
void end_bp(uint32_t addr);
void info_bp();

#endif
