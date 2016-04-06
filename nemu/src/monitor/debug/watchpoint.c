#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "memory/memory.h"

#define NR_WP 32

static WP wp_list[NR_WP];
static WP *head, *free_;

void init_wp_list() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_list[i].NO = i;
		wp_list[i].next = &wp_list[i + 1];
		wp_list[i].stop=false;
	}
	wp_list[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_list;
}


void add_wp(char *e){
	if (head==NULL){
		if (free_==NULL){
			printf("Can not set more breakpoint.\n");
			return;
		}
		head=free_;
	}
	strncpy(free_->expr,e,32);
	bool success=true;
	uint32_t value=expr(e,&success);
	if (success==false) {
		printf("Wrong expr!\n");
		return;
	}
	free_->value=value;
	free_=free_->next;
}

void check_wp(swaddr_t addr){
	bool success=true;
	uint32_t value;
	WP *temp;
	for (temp=head;temp!=free_;temp=temp->next) {
		value=expr(temp->expr,&success);
		if (success==true){
			if (value!=temp->value) {
				temp->addr=swaddr_read(addr,1);
				swaddr_write(addr, 1, 0xcc);
				temp->stop=true;
			}
		}
	}
}

//first judge this is bp.If it is bp, end it.
void end_wp(uint32_t addr){
	WP *temp;
	bool iswp=0;
	for (temp=head;temp!=free_;temp=temp->next) {
		if (addr==temp->addr) {
			iswp=1;
			break;
		}
	}
	if (iswp==0)
		return;
	swaddr_write(addr,1,temp->data);
}

void info_wp(){
	printf("NO\taddress\t\twhat\n");
	WP *temp;
	for (temp=head;temp!=free_;temp=temp->next) {
		printf("%d\t0x%x\t%s\n",(temp-wp_list)/4,temp->addr,temp->expr);
	}
}
/* TODO: Implement the functionality of watchpoint */


