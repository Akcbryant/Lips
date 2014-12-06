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

// take x to the y
int power_of(long x, long y) {
  int total = 1;
  for (int i = 0; i < y ; i++ ) {
    total = total * x;
  }
  return total;
}

/* Use operator string to see which operation to perform */
long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x - (y * (x / y)); }
  if (strcmp(op, "^") == 0) { return power_of(x, y); }
  if (strcmp(op, "min") == 0) { if (x > y) { return y; } else {return x; }}
  if (strcmp(op, "max") == 0) { if (x < y) { return y; } else {return x; }}
  return 0;
}

long eval(mpc_ast_t* t) {

  /* If tagged as number return it directly. */ 
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }
  
  /* The operator is always second child. */
  char* op = t->children[1]->contents;
  
  /* We store the third child in `x` */
  long x = eval(t->children[2]);

  
  
  /* Iterate the remaining children and combining. */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  return x;
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
  
  puts("Lispy Version 0.0.0.0.3");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("lispy> ");
    add_history(input);
    
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      
      long result = eval(r.output);
      printf("%li\n", result);

      // show the AST for debug purposes
      mpc_ast_print(r.output);

      // show the number of leaves
      int leaves = number_of_leaves(r.output);
      printf("You have %d leaves\n", leaves);

      // show the number of branches
      int branches = number_of_branches(r.output);
      printf("You have %d branches\n", branches);

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