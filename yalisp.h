/**
 * Copyright 2024 Adi Salimgereyev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _YALISP_H_
#define _YALISP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  node_type_int,
  node_type_symbol,
  node_type_string,
  node_type_list
} node_type;
typedef enum { value_type_int, value_type_string } value_type;

typedef struct value {
  value_type type;
  union {
    int int_value;
    char *string_value;
  };
} value;

value create_int_value(int int_value) {
  value val;
  val.type = value_type_int;
  val.int_value = int_value;
  return val;
}

value create_string_value(const char *string) {
  value val;
  val.type = value_type_string;
  val.string_value = strdup(string);
  return val;
}

void free_value(value val) {
  if (val.type == value_type_string) {
    free(val.string_value);
  }
}

void print_value(value val) {
  if (val.type == value_type_int) {
    printf("%d", val.int_value);
  } else if (val.type == value_type_string) {
    printf("\"%s\"", val.string_value);
  } else {
    fprintf(stderr, "Unknown value type\n");
    exit(1);
  }
}

typedef struct ast_node {
  node_type type;
  union {
    int int_value;
    char *symbol_value;
    char *string_value;
    struct {
      struct ast_node **items;
      size_t length;
    } list;
  };
} ast_node;

ast_node *create_int_node(int value) {
  ast_node *node = malloc(sizeof(ast_node));
  node->type = node_type_int;
  node->int_value = value;
  return node;
}

ast_node *create_symbol_node(const char *value) {
  ast_node *node = malloc(sizeof(ast_node));
  node->type = node_type_symbol;
  node->symbol_value = strdup(value);
  return node;
}

ast_node *create_string_node(const char *value) {
  ast_node *node = malloc(sizeof(ast_node));
  node->type = node_type_string;
  node->string_value = strdup(value);
  return node;
}

ast_node *create_list_node(ast_node **items, size_t length) {
  ast_node *node = malloc(sizeof(ast_node));
  node->type = node_type_list;
  node->list.items = items;
  node->list.length = length;
  return node;
}

void free_ast_node(ast_node *node) {
  if (node->type == node_type_symbol) {
    free(node->symbol_value);
  } else if (node->type == node_type_list) {
    for (size_t i = 0; i < node->list.length; i++) {
      free_ast_node(node->list.items[i]);
    }
    free(node->list.items);
  }
  free(node);
}

typedef struct result {
  int is_error; // 1 if there's an error, 0 otherwise
  union {
    value result_value;
    char *error_message;
  };
} result;

result create_success_result(value val) {
  result res;
  res.is_error = 0;
  res.result_value = val;
  return res;
}

result create_error_result(const char *message) {
  result res;
  res.is_error = 1;
  res.error_message = strdup(message);
  return res;
}

void free_result(result res) {
  if (res.is_error) {
    free(res.error_message);
  } else {
    if (res.result_value.type == value_type_string) {
      free(res.result_value.string_value);
    }
  }
}

typedef struct parse_result {
  int is_error; // 1 if there's a parsing error, 0 otherwise
  union {
    ast_node *node;
    char *error_message;
  };
} parse_result;

parse_result create_parse_success(ast_node *node) {
  parse_result res;
  res.is_error = 0;
  res.node = node;
  return res;
}

parse_result create_parse_error(const char *message) {
  parse_result res;
  res.is_error = 1;
  res.error_message = strdup(message);
  return res;
}

void free_parse_result(parse_result res) {
  if (res.is_error) {
    free(res.error_message);
  } else {
    free_ast_node(res.node);
  }
}

// TODO: Support utf8 encoded input
parse_result parse(const char *input, size_t *pos) {
  while (input[*pos] == ' ' || input[*pos] == '\n' || input[*pos] == '\t')
    (*pos)++;

  if (input[*pos] == '(') {
    (*pos)++;
    ast_node **items = NULL;
    size_t length = 0;

    while (input[*pos] != ')' && input[*pos] != '\0') {
      parse_result sub_result = parse(input, pos);
      if (sub_result.is_error) {
        for (size_t i = 0; i < length; i++) {
          free_ast_node(items[i]);
        }
        free(items);
        return sub_result;
      }
      items = realloc(items, sizeof(ast_node *) * (length + 1));
      items[length++] = sub_result.node;
    }

    if (input[*pos] == '\0') {
      for (size_t i = 0; i < length; i++) {
        free_ast_node(items[i]);
      }
      free(items);
      return create_parse_error("Unmatched '(' in input");
    }

    (*pos)++;
    return create_parse_success(create_list_node(items, length));
  } else if (input[*pos] >= '0' && input[*pos] <= '9') {
    int value = 0;
    while (input[*pos] >= '0' && input[*pos] <= '9') {
      value = value * 10 + (input[*pos] - '0');
      (*pos)++;
    }
    return create_parse_success(create_int_node(value));
  } else if (input[*pos] == '"') {
    (*pos)++;
    size_t start = *pos;
    while (input[*pos] != '"' && input[*pos] != '\0')
      (*pos)++;
    if (input[*pos] == '\0') {
      return create_parse_error("Unterminated string literal in input");
    }
    size_t length = *pos - start;
    char *string = strndup(input + start, length);
    (*pos)++;
    return create_parse_success(create_string_node(string));
  } else if (input[*pos] != '\0') {
    size_t start = *pos;
    while (input[*pos] != ' ' && input[*pos] != ')' && input[*pos] != '\n' &&
           input[*pos] != '\t') {
      (*pos)++;
    }
    char *symbol = strndup(input + start, *pos - start);
    return create_parse_success(create_symbol_node(symbol));
  }

  return create_parse_error("Unexpected end of input");
}

result eval_ast_node(ast_node *node) {
  if (node->type == node_type_int) {
    return create_success_result(create_int_value(node->int_value));
  } else if (node->type == node_type_string) {
    return create_success_result(create_string_value(node->symbol_value));
  } else if (node->type == node_type_symbol) {
    return create_error_result("Cannot evaluate a standalone symbol");
  } else if (node->type == node_type_list) {
    if (node->list.length == 0) {
      return create_error_result("Cannot evaluate an empty list");
    }

    ast_node *op = node->list.items[0];
    if (op->type != node_type_symbol) {
      return create_error_result(
          "First element of a list must be a symbol (operator)");
    }

    if (strcmp(op->symbol_value, "+") == 0) {
      int sum = 0;

      for (size_t i = 1; i < node->list.length; i++) {
        result arg_result = eval_ast_node(node->list.items[i]);

        if (arg_result.is_error)
          return arg_result;

        if (arg_result.result_value.type != value_type_int)
          return create_error_result("Non-integer argument to +");

        sum += arg_result.result_value.int_value;
        free_result(arg_result);
      }

      return create_success_result(create_int_value(sum));
    } else if (strcmp(op->symbol_value, "-") == 0) {
      result arg_result = eval_ast_node(node->list.items[1]);

      if (arg_result.is_error)
        return arg_result;

      if (arg_result.result_value.type != value_type_int)
        return create_error_result("Non-integer argument to +");

      int diff = arg_result.result_value.int_value;
      free_result(arg_result);

      for (size_t i = 2; i < node->list.length; i++) {
        result arg_result = eval_ast_node(node->list.items[i]);

        if (arg_result.is_error)
          return arg_result;

        if (arg_result.result_value.type != value_type_int)
          return create_error_result("Non-integer argument to +");

        diff -= arg_result.result_value.int_value;
        free_result(arg_result);
      }

      return create_success_result(create_int_value(diff));
    } else if (strcmp(op->symbol_value, "concat") == 0) {
      size_t total_length = 0;
      for (size_t i = 1; i < node->list.length; i++) {
        result arg_result = eval_ast_node(node->list.items[i]);
        if (arg_result.is_error)
          return arg_result;

        if (arg_result.result_value.type != value_type_string)
          return create_error_result("Non-string argument to concat");

        total_length += strlen(arg_result.result_value.string_value);
        free_result(arg_result);
      }

      char *result_string = malloc(total_length + 1);
      result_string[0] = '\0';

      for (size_t i = 1; i < node->list.length; i++) {
        result arg_result = eval_ast_node(node->list.items[i]);
        strcat(result_string, arg_result.result_value.string_value);
        free_result(arg_result);
      }

      return create_success_result(create_string_value(result_string));
    } else {
      return create_error_result("Unknown operator");
    }
  }

  return create_error_result("Unknown AST node type");
}

void process_yalisp_shell_input(const char *input) {
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

void run_yalisp_shell() {
  char input[1024];
  printf("Welcome to Yet Another Lisp (YALisp)!\n");
  printf("Type in lisp expressions, and I'll execute them :3\n");
  while (1) {
    printf("(yalisp) > ");
    if (!fgets(input, sizeof(input), stdin))
      break;

    process_yalisp_shell_input(input);
  }
}

#endif /* _YALISP_H_ */