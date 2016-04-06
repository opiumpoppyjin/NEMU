#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "monitor/breakpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_b(char *args){
	if (args==NULL){
		printf("Please querry where to break!\n");
		return -1;
	}
	add_bp(args);
	return 0;
}

static int cmd_info(char *args){
	if (args==NULL)
		return -1;
	else if (strcmp(args,"r")==0){
		printf("%-15s%#-15X%u\n","eax", cpu.eax, cpu.eax);
		printf("%-15s%#-15X%u\n","ecx", cpu.ecx, cpu.ecx); 
		printf("%-15s%#-15X%u\n","edx", cpu.edx, cpu.edx); 
		printf("%-15s%#-15X%u\n","ebx", cpu.ebx, cpu.ebx); 
		printf("%-15s%#-15X%u\n","esp", cpu.esp, cpu.esp); 
		printf("%-15s%#-15X%u\n","ebp", cpu.ebp, cpu.ebp); 
		printf("%-15s%#-15X%u\n","esi", cpu.esi, cpu.esi); 
		printf("%-15s%#-15X%u\n","edi", cpu.edi, cpu.edi); 
		printf("%-15s%#-15X%u\n","eip", cpu.eip, cpu.eip); 
		/*
		printf("CF PF AF ZF SF TF IF DF OF\n"
				"%2d %2d %2d %2d %2d %2d %2d %2d %2d\n",
				FLAG_VAL(CF),
				FLAG_VAL(PF),
				FLAG_VAL(AF),
				FLAG_VAL(ZF),
				FLAG_VAL(SF),
				FLAG_VAL(TF),
				FLAG_VAL(IF),
				FLAG_VAL(DF),
				FLAG_VAL(OF)
			  );
		printf("CS %x DS %x SS %x\n", cpu.cs, cpu.ds, cpu.ss);
		*/
	}
	else if (strcmp(args,"b")==0){
		info_bp();	
	}
	else if (strcmp(args,"w")==0){
		info_wp();	
	}
	return 0;
}

extern uint32_t expr(char *e, bool *success);  
static int nr_exprs=1;
static int cmd_p(char *args){
	bool success=true;
	if (args==NULL)
		return -1;
	int value=expr(args,&success);
	if (success){
		printf("$%d = %d\n",nr_exprs,value);
		nr_exprs++;
	}
	else{
		printf("Invalid expression!\n");
	}
	return 0;
}

static int cmd_si(char *args){
	if (args==NULL)	
		cpu_exec(1);
	else{
		int nr_steps=atoi(args);
		cpu_exec(nr_steps);
	}
	return 0;
}

static int cmd_w(char *args){
	if (args==NULL){
		printf("Please querry what to watching!\n");
		return -1;
	}
	add_wp(args);
	return 0;	
}

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },

	{ "b","Set breakpoint", cmd_b},
	{ "info", "-b Print information of breakpoint.\
		\n		 -r Print information of registers.\
		\n       -w Print information of watchpoints.", cmd_info},
	{ "p", "Expression evaluation", cmd_p},
	{ "si", "Single-step excution", cmd_si},
	{ "w", "Set watchpoint",cmd_w}

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
