#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else

#include <editline/readline.h>

/* Must be compiled with cc -std=c99 -Wall evaluation.c mpc.c -ledit -lm -o evaluation */

#endif

/* Create Enumeration of Possible Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create Enumeration of Possible lval Types */
enum { LVAL_NUM, LVAL_ERR };

/* Declare New lval Struct */
typedef struct {
  int type;
  long num;
  int err;
} lval;

/* Create a new number type lval */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
    /* In the case the type is a number print it */
    /* Then 'break' out of the switch. */
    case LVAL_NUM: printf("%li", v.num); break;
    
    /* In the case the type is an error */
    case LVAL_ERR:
      /* Check what type of error it is and print it */
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      }
      if (v.err == LERR_BAD_OP)   {
        printf("Error: Invalid Operator!");
      }
      if (v.err == LERR_BAD_NUM)  {
        printf("Error: Invalid Number!");
      }
    break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

// take x to the y
int power_of(long x, long y) {
  int total = 1;
  for (int i = 0; i < y ; i++ ) {
    total = total * x;
  }
  return total;
}

lval eval_op(lval x, char* op, lval y) {
  
  /* If either value is an error return it */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* Otherwise do maths on the number values */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    /* If second operand is zero return error */
    return y.num == 0 
      ? lval_err(LERR_DIV_ZERO) 
      : lval_num(x.num / y.num);
  }
  if (strcmp(op, "%") == 0) { 
    return y.num == 0 
      ? lval_err(LERR_DIV_ZERO) 
      : lval_num(x.num - (y.num * (x.num / y.num))); }
  if (strcmp(op, "^") == 0) { return lval_num(power_of(x.num, y.num)); }
  if (strcmp(op, "min") == 0) { if (x.num > y.num) { return lval_num(y.num); } else {return lval_num(x.num); }}
  if (strcmp(op, "max") == 0) { if (x.num < y.num) { return lval_num(y.num); } else {return lval_num(x.num); }}

  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  
  if (strstr(t->tag, "number")) {
    /* Check if there is some error in conversion */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }
  
  char* op = t->children[1]->contents;  
  lval x = eval(t->children[2]);

  /* Iterate the remaining children and combine. */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  // If the negative sign has only one argument this will be called.
  if (strcmp(t->children[1]->contents, "-") == 0) { return lval_num(-x.num); }

  return lval_num(x.num);
}

/* Recursive function that computes the number of leaves in the tree */
int number_of_leaves(mpc_ast_t* t) {
  if (t->children_num == 0 && strcmp(t->tag, "expr|number|regex") == 0) { return 1; }
  if (t->children_num >= 1) {
    int total = 0;
    for (int i = 0; i < t->children_num; i++) {
      total = total + number_of_leaves(t->children[i]);
    }
    return total;
  }
  return 0;
}

/* Recursive function that computes the number of branches in the tree */
int number_of_branches(mpc_ast_t* t) {
  if (t->children_num == 0) { return 0; }
  if (t->children_num >= 1) {
    int total = 0;
    total++;
    for (int i = 0; i < t->children_num; i++) {
      total = total + number_of_branches(t->children[i]);
    }
    return total;
  }
  return 0;
}

int is_expr(mpc_ast_t* t) {
  mpc_ast_t** children = t->children;
  //This grabs the first example of expr|number|regex to test against expr in + 3 3
  if (strstr(children[2]->tag, "expr") != 0) { return 1; }
  else { return 0; }
}

int is_paren(mpc_ast_t* t) {
  mpc_ast_t** children = t->children;
  printf("%s\n", children[4]->contents);
  if (strcmp(children[4]->contents, "(") == 0 || strcmp(children[4]->contents, ")") == 0) { return 1; }
  else { return 0; }
}

int main(int argc, char** argv) {
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Function = mpc_new("function");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");
  
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                                       \
      number   : /-?[0-9]+/ ;                                                               \
      function : /min/ | /max/ ;                                        \
      operator : '+' | '-' | '*' | '/' | '%' | '^' | <function> ;                           \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;                                               \
    ",
    Number, Function, Operator, Expr, Lispy);
  
  puts("Lispy Version 0.0.0.0.4");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("lispy> ");
    add_history(input);
    
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispy, &r)) {

      lval result = eval(r.output);
      lval_println(result);

      // show the AST for debug purposes
      //mpc_ast_print(r.output);

      // show the number of leaves
      //int leaves = number_of_leaves(r.output);
      //printf("You have %d leaves\n", leaves);

      // show the number of branches
      //int branches = number_of_branches(r.output);
      //printf("You have %d branches\n", branches);

      // show if node is tagged as an 'expr', tested on + 3 3
      //int node = is_expr(r.output);
      //printf("Node is expr if this number is 1: %d\n", node);

      // show if a node's contents is a paren, test on + 3 ( + 3 3 )
      //int node = is_paren(r.output);
      //printf("Node is either ')' or '(' if this is 1: %d\n", node);

      mpc_ast_delete(r.output);
      
    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
    
  }
  
  mpc_cleanup(5, Number, Function, Operator, Expr, Lispy);
  
  return 0;
}