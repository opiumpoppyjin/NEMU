#include "nemu.h"
#include <string.h>
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	OR, AND,                    // 0, 1
	BIT_OR, BIT_XOR, BIT_AND,   // 2, 3, 4
	NE, EQ, LE, LS, GE, GT,     // 5, 6, 7, 8, 9, 10
	RSHIFT, LSHIFT,             // 11, 12
	ADD, SUB,                   // 13, 14
	MUL, DIV, MOD,              // 15, 16, 17
	NOT, POINTER, NEG, BIT_NOT, // 18, 19, 20, 21
	LB, RB,         // 22, 23
	NUM, HEX, REG,              // 25, 26, 27
	IDENTIFIER,                 // 28
	NOTYPE                      // 29

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
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case HEX:case NUM:case REG: 
						tokens[nr_token].type=rules[i].token_type;
						strncpy(tokens[nr_token].str,e+position,substr_len);
						printf("num0=%s\n",tokens[nr_token].str);
						break;
					case SUB:case MUL:
						if (nr_token==0||\
								(tokens[nr_token-1].type!=HEX\
								 &&tokens[nr_token-1].type!=NUM\
								 &&tokens[nr_token-1].type!=REG)){
							if (rules[i].token_type==SUB)
								tokens[nr_token].type=NEG;
							else
								tokens[nr_token].type=POINTER;
						}
						break;
					default: tokens[nr_token].type=rules[i].token_type;
				}
				position += substr_len;
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


static bool check_parentheses(int p,int q,bool *success){
	if (tokens[p].type!=LB||tokens[q].type!=RB)
		return false;
	return true;
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
				printf("num1=%s\n",tokens[p].str);
				return atoi(tokens[p].str);break;
			case HEX:
				printf("num2=%s\n",tokens[p].str);
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
		printf("lala!!\n");
		return eval(p + 1, q - 1,success);
	} 
	else {
		/* We should do more things here. */
		return 0;
	}
}

static bool check_parentheses_matched(int p,int q){
	int count = 0;  // +1 when ( and -1 when ) should be 0 at the end
	int stack[32];                // to pair parentheses
	memset(stack, -1, 32);
	int i, j;
	for (i = j = 0; i < nr_token; i++) {
		if (tokens[i].type == LB) {
			count++;
			stack[j++] = i;//j处的右括号对应i处的左括号
			assert(tokens[stack[j-1]].type == LB);
		}
		else if (tokens[i].type == RB) {
			count--;
			stack[j] = -1;
		}
	}
	if (count == 0 && stack[0] == -1) return true;
	return false;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	if (check_parentheses_matched(0,nr_token)==false){
		*success = false;
		return 0;
	}

	return eval(0,nr_token,success);
	/* TODO: Insert codes to evaluate the expression. */
	//	panic("please implement me");
}

