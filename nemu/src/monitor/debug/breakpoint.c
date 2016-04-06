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
	free_->data=swaddr_read(addr,1);

	free_=free_->next;
	swaddr_write(addr, 1, 0xcc);
}
//first judge this is bp.If it is bp, end it.
void end_bp(uint32_t addr){
	BP *temp;
	bool isbp=0;
	for (temp=head;temp!=free_;temp=temp->next) {
		if (addr==temp->addr) {
			isbp=1;
			break;
		}
	}
	if (isbp==0)
		return;
	swaddr_write(addr,1,temp->data);
}

void info_bp(){
	printf("NO\taddress\t\twhat\n");
	BP *temp;
	for (temp=head;temp!=free_;temp=temp->next) {
		printf("%d\t0x%x\t%s\n",(temp-bp_list)/4,temp->addr,temp->expr);
	}
}
