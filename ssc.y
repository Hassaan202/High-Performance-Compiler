%code requires 
{
    #include <vector>
    #include <llvm/IR/Value.h>
    #include <llvm/IR/BasicBlock.h>
}

%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <vector>
    #include "IR.h"
    
    extern int yyparse();
    extern int yylex();
    extern FILE *yyin;
    extern int yylineno;
    
    #define DEBUGBISON
    #ifdef DEBUGBISON
        // Changed to print to stderr instead of stdout
        #define debugBison(a) (fprintf(stderr, "Rule %d: %s\n", a, __func__))
    #else
        #define debugBison(a)
    #endif
%}

%nonassoc LOWER_THAN_ELSE
%nonassoc tok_else

%union {
    char *identifier;
    double double_literal;
    char *string_literal;
    llvm::Value* value; 
    llvm::BasicBlock* block;
    std::vector<llvm::Value*>* value_list;
}

%token tok_printd
%token tok_prints
%token tok_if
%token tok_else
%token tok_for
%token tok_function
%token tok_return
%token <identifier> tok_identifier
%token <double_literal> tok_double_literal
%token <string_literal> tok_string_literal
%token tok_eq

%type <value> term expression condition
%type <block> if_statement if_else_statement statement statement_list
%type <block> for_statement
%type <block> function_definition
%type <value> function_call
%type <value> return_statement
%type <value_list> argument_list

%left '+' '-' 
%left '*' '/'
%left '(' ')'
%left '<' '>' tok_eq

%start program

%define parse.error verbose


%%


program:
    statement_list { debugBison(1); }

statement_list:
    /* empty */
    | statement_list statement { debugBison(5); }
    | statement_list function_definition { debugBison(6); }


statement:
    '{' statement_list '}'
  | prints    ';'
  | printd    ';'
  | assignment ';'
  | function_call ';'
  | expression ';'
  | for_statement { debugBison(12); }
  | return_statement ';'

  /* now the if/else cases: */
  | tok_if '(' condition ')' 
      {
        debugBison(31);
        handleIfElseStatement($3);
      }
    statement
      {
        endIfThenBlock();
      }
    tok_else
    statement
      {
        endIfElseStatement();
      }
  | tok_if '(' condition ')' 
      %prec LOWER_THAN_ELSE
      {
        debugBison(30);
        handleIfStatement($3);
      }
    statement
      {
        endIfStatement();
      }
    | error ';'  { yyerror("Statement error"); yyerrok; }
  ;

prints: 
    tok_prints '(' tok_string_literal ')' { debugBison(15); printString($3); free($3); } 
;

printd: 
    tok_printd '(' expression ')' { debugBison(16); printDouble($3); }
;

term: 
    tok_identifier { 
        debugBison(17); 
        Value* ptr = getFromSymbolTable($1); 
        $$ = builder.CreateLoad(builder.getDoubleTy(), ptr, "load_identifier"); 
        free($1); 
    }
    | tok_double_literal { debugBison(18); $$ = createDoubleConstant($1); }
    | '(' expression ')' { debugBison(26); $$ = $2; }
;

assignment: 
    tok_identifier '=' expression { debugBison(20); setDouble($1, $3); free($1); }
;

expression: 
    term { debugBison(21); $$ = $1; }
    | expression '+' expression { debugBison(22); $$ = performBinaryOperation($1, $3, '+'); }
    | expression '-' expression { debugBison(23); $$ = performBinaryOperation($1, $3, '-'); }
    | expression '/' expression { debugBison(24); $$ = performBinaryOperation($1, $3, '/'); }
    | expression '*' expression { debugBison(25); $$ = performBinaryOperation($1, $3, '*'); }
;

condition: 
    expression '>' expression { debugBison(27); $$ = createComparisonOperation($1, $3, '>'); }
    | expression '<' expression { debugBison(28); $$ = createComparisonOperation($1, $3, '<'); }
    | expression tok_eq expression { debugBison(29); $$ = createComparisonOperation($1, $3, '='); }
;

for_statement:
    tok_for '(' tok_identifier '=' expression ';' expression ')' '{'
        { 
          debugBison(32);
          startForLoop($5, $3, $7);
          free($3);
        }
      statement_list
        { 
          endForLoop();
        }
    '}'
    | tok_for error '{' statement_list '}' { 
        debugBison(40); 
        yyerror("Invalid for loop syntax"); 
        yyerrok; 
    }
;

function_definition:
    tok_function tok_identifier '(' ')' '{' statement_list '}' {
        debugBison(33);
        defineFunction($2);
        endFunctionDefinition(createDoubleConstant(0.0));
        free($2);
    }
    | tok_function tok_identifier '(' ')' '{' statement_list return_statement ';' '}' {
        debugBison(34);
        defineFunction($2);
        endFunctionDefinition($7);
        free($2);
    }
;

function_call:
    tok_identifier '(' argument_list ')' ';'
    {
        debugBison(35);
        $$ = callFunction($1, *$3);
        delete $3;
        free($1);
    }
    | tok_identifier '(' ')' ';'
    {
        debugBison(36);
        std::vector<Value*> emptyArgs;
        $$ = callFunction($1, emptyArgs);
        free($1);
    }
;

argument_list:
    expression {
        debugBison(37);
        $$ = new std::vector<Value*>();
        $$->push_back($1);
    }
    | argument_list ',' expression {
        debugBison(38);
        $1->push_back($3);
        $$ = $1;
    }
;

return_statement:
    tok_return expression {
        debugBison(39);
        $$ = $2;
    }
;

%%


void yyerror(const char *err) {
    fprintf(stderr, "\nError at line %d: %s\n", yylineno, err);
}

int main(int argc, char** argv) {
    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r");
        if (!fp) {
            fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
            return EXIT_FAILURE;
        }
        yyin = fp;
    } else {
        yyin = stdin;
    }
    
    initLLVM();
    int parserResult = yyparse();
    
    if (parserResult == 0) {
      addReturnInstr();
      printLLVMIR();
      return 0;
    } else {
      fprintf(stderr, "Parsing failed.\n");
      return 1;
    }
}
