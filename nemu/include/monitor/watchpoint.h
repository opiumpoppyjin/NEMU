#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char expr[32];
	swaddr_t addr;
	unsigned char data;
	bool stop;
	uint32_t value;
	/* TODO: Add more members if necessary */
} WP;

void add_wp(char *e);
void info_wp();
bool check_wp(swaddr_t addr);
void end_wp(uint32_t addr);

#endif
