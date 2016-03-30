#include "nemu.h"
#include <string.h>

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
	LBRACKET, RBRACKET,         // 22, 23
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

	{"0x[0-9a-f]+", HEX},              // heximal
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
	{"\\(", LBRACKET},
	{"\\)", RBRACKET}
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
				position += substr_len;

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
							&&tokens[nr_token-1].type!=REG)){
							if (rules[i].token_type==SUB)
								tokens[nr_token].type=NEG;
							else
								tokens[nr_token].type=POINTER;
						}
						break;
					default: tokens[nr_token].type=rules[i].token_type;
				}

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

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	panic("please implement me");
	return 0;
}

