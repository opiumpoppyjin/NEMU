#include "nemu.h"
#include <string.h>
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE,
	IDENTIFIER,NUM, HEX, REG,
	LB, RB, 
	NOT, POINTER, NEG, BIT_NOT,
	MUL, DIV, MOD, 
	ADD, SUB,
	RSHIFT, LSHIFT,
	NE, EQ, LE, LS, GE, GT, 
	BIT_OR, BIT_XOR, BIT_AND,
	OR, AND,
	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{"0[xX][0-9a-f]+", HEX},              // heximal
	{"[0-9]+", NUM},                // decimal
	{"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|al|ah|cl|ch|dl|dh|bl|bh)", REG},
	{"[A-Za-z_][A-Za-z_0-9]*", IDENTIFIER},  //标识符
	{"\\+", ADD},
	{"-", SUB},
	{"\\*", MUL},
	{"/", DIV},
	{"%", MOD},
	{"<<", LSHIFT},
	{">>", RSHIFT},
	{"<=", LE},
	{">=", GE},
	{"==", EQ},
	{"!=", NE},
	{"<", LS},
	{">", GT},
	{"&&", AND},
	{"\\|\\|", OR},
	{"!", NOT},
	{"&", BIT_AND},
	{"\\|", BIT_OR},
	{"\\^", BIT_XOR},
	{"-", NEG},
	{"\\~", BIT_NOT},
	{"\\*", POINTER},
	{"\\(", LB},
	{"\\)", RB}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 * t:let matching more effective.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
			//	char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

			//	Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case HEX:case NUM:case REG: 
						tokens[nr_token].type=rules[i].token_type;
						strncpy(tokens[nr_token].str,e+position,substr_len);
						break;
					case SUB:case MUL:
						if (nr_token==0||\
								(tokens[nr_token-1].type!=HEX\
								 &&tokens[nr_token-1].type!=NUM\
								 &&tokens[nr_token-1].type!=REG\
								 &&tokens[nr_token-1].type!=RB\
								 )){
							if (rules[i].token_type==SUB)
								tokens[nr_token].type=NEG;
							else
								tokens[nr_token].type=POINTER;
						}
						else {
							tokens[nr_token].type=rules[i].token_type;		
						}
						break;
					default: tokens[nr_token].type=rules[i].token_type;
				}
				position += substr_len;
				nr_token++;
				break;
			}
	}
		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
	return true; 
}

int	pair_of_LB[32];//记录在i处的LB所匹配的RB的位置 

static bool check_parentheses(int p,int q,bool *success){
	if (tokens[p].type==LB&&pair_of_LB[p]==q)
		return true;
	return false;
}

static int level(int op){
	switch(op){
		//先算优先级大的，数越小优先级越大
		case NOT:case POINTER:case NEG:case BIT_NOT:return 1;
		case MUL:case DIV: case MOD: return 2;
		case ADD:case SUB: return 3;						 
		case RSHIFT:case LSHIFT: return 4;
		case LE:case LS:case GE:case GT: return 5;
		case NE:case EQ: return 6;
		case BIT_AND: return 7;
		case BIT_XOR: return 8;
		case BIT_OR:return 9;
		case AND: return 10;			
		case OR: return 11;
		default:return 0;;
	}
}

static int find_op(int p, int q) {
	switch (tokens[p].type) {
		case NOT:case NEG:case BIT_NOT:case POINTER:return p;
	}
	int minlevel = -1,thislevel;
	int op=-1;
	int i;
	for (i=p;i<=q;i++){
		if (tokens[i].type==LB){
			i=pair_of_LB[i];

		}
		thislevel=level(tokens[i].type);
		if (thislevel>=minlevel){
				op=i;
				minlevel=thislevel;
		}
		else if(thislevel!=0){
			return op;
		}

	}
	return op;
}

static int eval(int p,int q,bool *success){
	if(p > q) {
		/* Bad expression */
		*success=false;
		return 0;
	} 
	else if(p == q) {
		/* Single token.
		 * * For now this token should be a number.
		 * * Return the value of the number.
		 * */
		switch(tokens[p].type){
			case NUM: 
				return atoi(tokens[p].str);break;
			case HEX:
				return strtoull(tokens[p].str, NULL, 16);
			case REG:
				if (!strcmp(tokens[p].str, "$eax")) return cpu.eax;
				else if (!strcmp(tokens[p].str, "$ecx")) return cpu.ecx;
				else if (!strcmp(tokens[p].str, "$edx")) return cpu.edx;
				else if (!strcmp(tokens[p].str, "$ebx")) return cpu.ebx;
				else if (!strcmp(tokens[p].str, "$esp")) return cpu.esp;
				else if (!strcmp(tokens[p].str, "$ebp")) return cpu.ebp;
				else if (!strcmp(tokens[p].str, "$esi")) return cpu.esi;
				else if (!strcmp(tokens[p].str, "$edi")) return cpu.edi;
				else if (!strcmp(tokens[p].str, "$eip")) return cpu.eip;
				/*
				   else if (!strcmp(temp, "$ax"))  return reg_w(R_AX);
				   else if (!strcmp(temp, "$al"))  return reg_b(R_AL);
				   else if (!strcmp(temp, "$ah"))  return reg_b(R_AH);
				   else if (!strcmp(temp, "$cx"))  return reg_w(R_CX);
				   else if (!strcmp(temp, "$cl"))  return reg_b(R_CL);
				   else if (!strcmp(temp, "$ch"))  return reg_b(R_CH);
				   else if (!strcmp(temp, "$dx"))  return reg_w(R_DX);
				   else if (!strcmp(temp, "$dl"))  return reg_b(R_DL);
				   else if (!strcmp(temp, "$dh"))  return reg_b(R_DH);
				   else if (!strcmp(temp, "$bx"))  return reg_w(R_BX);
				   else if (!strcmp(temp, "$bl"))  return reg_b(R_BL);
				   else if (!strcmp(temp, "$bh"))  return reg_b(R_BH);
				   */
			default: 
				*success=false;
				return 0;
		}
	} 
	else if(check_parentheses(p, q,success) == true) {
		/* The expression is surrounded by a matched pair of parentheses.
		 * * If that is the case, just throw away the parentheses.
		 * */
		return eval(p + 1, q - 1,success);
	} 
	else {
		int op = find_op(p, q);//找到优先级最高的操作符
		int eval2 = eval(op + 1, q,success);
		switch (tokens[op].type) {
			case NOT: return !eval2;
			case NEG: return -eval2;
			case BIT_NOT: return ~eval2;
			case POINTER: return swaddr_read(eval2, 4); //how long?
			default: assert(op != p);
		}

		int eval1 = eval(p, op - 1,success);
		switch(tokens[op].type) {
			case ADD: return eval1 + eval2;
			case SUB: return eval1 - eval2;
			case MUL: return eval1 * eval2;
			case DIV: return eval1 / eval2;
			case MOD: return eval1 % eval2;

			case AND: return eval1 && eval2;
			case OR:  return eval1 || eval2;

			case BIT_OR:  return eval1 | eval2;
			case BIT_XOR: return eval1 ^ eval2;
			case BIT_AND: return eval1 & eval2;

			case EQ: return eval1 == eval2;
			case NE: return eval1 != eval2;
			case LE: return eval1 <= eval2;
			case LS: return eval1 <  eval2;
			case GE: return eval1 >= eval2;
			case GT: return eval1 >  eval2;

			case RSHIFT: return eval1 >> eval2;
			case LSHIFT: return eval1 << eval2;

			default: assert(0);
		}
		exit(1);
	}
	return 0;
	/* We should do more things here. */
}


static bool check_parentheses_matched(){
	int count = 0;  // +1 when ( and -1 when ) should be 0 at the end
	int stack[33];
	int i,j;
	for (i=0;i<32;i++){
		stack[i]=-1;
		pair_of_LB[i]=-1;
	}
	for (i = j = 0; i < nr_token; i++) {
		if (tokens[i].type == LB) {
			count++;
			stack[count]=i;
		}
		else if (tokens[i].type == RB) {
			pair_of_LB[stack[count]]=i;
			stack[count]=-1;
			count--;
		}
		if (count<0)
			return false;
	}
	if (count!=0)
		return false;
	return true;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	if (check_parentheses_matched(0,nr_token-1)==false){
		*success = false;
		return 0;
	}

	return eval(0,nr_token-1,success);
	/* TODO: Insert codes to evaluate the expression. */
	//	panic("please implement me");
}

