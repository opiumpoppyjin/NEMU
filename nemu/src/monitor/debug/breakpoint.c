#include "monitor/breakpoint.h"
#include "monitor/expr.h"

#define NR_BP 32

static BP bp_list[NR_BP];
static BP *head,*free_;

void init_bp_list(){
	int i;
	for (i=0;i<NR_BP;i++){
		bp_list[i].NO=i;
		bp_list[i].next=&bp_list[i+1];
	}
	bp_list[NR_BP-1].next=NULL;

	head=NULL;
	free_=bp_list;
}

void add_bp(char *e){
	if (head==NULL){
		if (free_==NULL){
			printf("Can not set more breakpoint.\n");
			return;
		}
		head=free_;
	}
	strncpy(free_->expr,e,32);
	bool success=1;
	uint32_t addr=expr(e,&success);
	if (success==1)
		free_->addr=addr;
	free_=free_->next;
printf("shit!\n");
	swaddr_write(addr, 1, 0xcc);
}




