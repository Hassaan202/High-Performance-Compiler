%{
	#include <llvm/IR/Value.h>
	#include "ssc.tab.h"
	#include <stdlib.h>
	#include <stdio.h>
	#include <string>
	#include <vector>
	extern void yyerror(const char *msg);
	
	//#define DEBUGSSC
	#ifdef DEBUGSSC
		#define debugFlex(a) (printf("\n%d: Token=<ID, lexeme>=<%d, %s>\n",yylineno, a, yytext))
	#else
		#define debugFlex(a)
	#endif
%}

%option yylineno 
%option noyywrap

%%

"printd"				{debugFlex(tok_printd); return tok_printd;}
"prints"				{debugFlex(tok_prints); return tok_prints;}
"if"					{debugFlex(tok_if); return tok_if;}
"else"					{debugFlex(tok_else); return tok_else;}
"for"					{debugFlex(tok_for); return tok_for;}
"pfor"					{debugFlex(tok_pfor); return tok_pfor;}
"function"				{debugFlex(tok_function); return tok_function;}
"return"				{debugFlex(tok_return); return tok_return;}
[a-zA-Z][a-zA-Z0-9_]*			{debugFlex(tok_identifier); yylval.identifier=strdup(yytext); return tok_identifier;}
[0-9]+(\.[0-9]+)?			{debugFlex(tok_double_literal); yylval.double_literal=atof(yytext); return tok_double_literal;}
\"[^"]*\"				{debugFlex(tok_string_literal); yylval.string_literal=strdup(yytext); return tok_string_literal;}
\/\/.*					{/* ignore comments */}
[ \n\r\t]				{/* ignore whitespace */}
"=="					{debugFlex(tok_eq); return tok_eq;}
"+"|"-"|"*"|"\/"|"="|";"|"("|")"|","|"{"|"}"|">"|"<"	{debugFlex(yytext[0]); return yytext[0];}
.					{debugFlex(-1); yyerror("illegal lexeme"); exit(EXIT_FAILURE);}
%%