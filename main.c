#include "yalisp.h"

void process_input(const char *input) {
  size_t pos = 0;
  parse_result parse_result = parse(input, &pos);
  if (parse_result.is_error) {
    printf("Error: %s\n", parse_result.error_message);
    free_parse_result(parse_result);
    return;
  }

  result eval_result = eval_ast_node(parse_result.node);
  if (eval_result.is_error) {
    printf("Error: %s\n", eval_result.error_message);
    free_parse_result(parse_result);
    free_result(eval_result);
    return;
  }

  print_value(eval_result.result_value);
  printf("\n");

  free_result(eval_result);
  free_parse_result(parse_result);
}

int main() {
  char input[1024];
  printf("Welcome to Yet Another Lisp (YALisp)!\n");
  printf("Type in lisp expressions, and I'll execute them :3\n");
  while (1) {
    printf("(yalisp) > ");
    if (!fgets(input, sizeof(input), stdin))
      break;

    process_input(input);
  }

  return 0;
}
