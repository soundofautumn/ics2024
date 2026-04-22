/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

// #define EXPR_DEBUG
#ifdef EXPR_DEBUG
#define EXPR_Log Log
#else
#define EXPR_Log(...) do { } while (0)
#endif

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NE, TK_AND, TK_OR,
  TK_NUM, TK_HEX, TK_REG,
  // unary minus and dereference
  TK_NEG, TK_DEREF,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus or unary minus
  {"\\*", '*'},         // multiply or dereference
  {"\\/", '/'},         // divide
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
  {"0[xX][0-9a-fA-F]+", TK_HEX}, // hexadecimal number
  {"[0-9]+", TK_NUM},   // decimal number
  {"==", TK_EQ},        // equal
  {"!=", TK_NE},        // not equal
  {"&&", TK_AND},        // logical and
  {"\\|\\|", TK_OR},    // logical or
  {"\\$[a-zA-Z_][a-zA-Z0-9_]*", TK_REG}, // register
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[256] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        EXPR_Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        bool skip = rules[i].token_type == TK_NOTYPE;
        if (skip) {
          break;
        }

        switch (rules[i].token_type) {
          case TK_EQ: case TK_NE: case TK_AND: case TK_OR:
            tokens[nr_token].type = rules[i].token_type; 
            break;
          case TK_NUM: case TK_HEX: case TK_REG:
            /* copy the substring as the token's string */
            if(substr_len >= 32) {
              EXPR_Log("Token is too long: %.*s\n", substr_len, substr_start);
              return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            tokens[nr_token].type = rules[i].token_type;
            break;
          case '+': case '-': case '*': case '/': case '(': case ')':
            tokens[nr_token].type = rules[i].token_type;
            break;
          default: 
            Assert(0, "Unknown token type: %d", rules[i].token_type);
        }
        nr_token ++;
        break;
      }
    }

    if (i == NR_REGEX) {
      EXPR_Log("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }
  int count = 0;
  for (int i = p + 1; i < q; i++) {
    if (tokens[i].type == '(') {
      count++;
    }
    else if (tokens[i].type == ')') {
      if (count == 0) {
        return false;
      }
      count--;
    }
  }
  return count == 0;
}

#ifdef EXPR_DEBUG

void token_type_to_str(int type, char *buf, size_t buf_size) {
  switch (type) {
    case TK_EQ: snprintf(buf, buf_size, "TK_EQ"); break;
    case TK_NE: snprintf(buf, buf_size, "TK_NE"); break;
    case TK_AND: snprintf(buf, buf_size, "TK_AND"); break;
    case TK_OR: snprintf(buf, buf_size, "TK_OR"); break;
    case TK_NUM: snprintf(buf, buf_size, "TK_NUM(%s)", tokens[nr_token].str); break;
    case TK_HEX: snprintf(buf, buf_size, "TK_HEX(%s)", tokens[nr_token].str); break;
    case TK_REG: snprintf(buf, buf_size, "TK_REG(%s)", tokens[nr_token].str); break;
    case TK_NEG: snprintf(buf, buf_size, "TK_NEG"); break;
    case TK_DEREF: snprintf(buf, buf_size, "TK_DEREF"); break;
    default: snprintf(buf, buf_size, "'%c'", type); break;
  }
}

void print_tokens() {
  for (int i = 0; i < nr_token; i++) {
    char type_str[64];
    token_type_to_str(tokens[i].type, type_str, sizeof(type_str));
    EXPR_Log("Token %d: type = %s", i, type_str);
  }
}

#endif

int find_main_op(int p, int q) {
  int main_op = -1;
  int balance = 0;
  int min_pri = 10; // higher than any operator precedence
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      balance++;
      continue;
    }
    if (tokens[i].type == ')') {
      balance--;
      continue;
    }
    if (balance != 0) {
      continue;
    }

    int pri = 0;
    switch (tokens[i].type) {
      case TK_AND: case TK_OR: pri = 1;break;
      case TK_EQ: case TK_NE: pri = 2; break;
      case '+': case '-': pri = 3; break;
      case '*': case '/': pri = 4; break;
      case TK_NEG: case TK_DEREF: pri = 5; break;
      default: continue;
    }

    if (pri <= min_pri) {
      main_op = i;
      min_pri = pri;
    }
  }
#ifdef EXPR_DEBUG
  char main_op_str[64];
  token_type_to_str(tokens[main_op].type, main_op_str, sizeof(main_op_str));
  EXPR_Log("Main operator at position %d: %s", main_op, main_op_str);
#endif
  return main_op;
}

static bool eval_success = true;

static word_t eval(int p, int q) {
  EXPR_Log("Evaluating tokens from %d to %d", p, q);
  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number. But we will add more
     * types of tokens later.
     */
     if(tokens[p].type == TK_HEX) {
      return strtoul(tokens[p].str, NULL, 16);
    }
    else if(tokens[p].type == TK_NUM) {
      return atoi(tokens[p].str);
    }
    else if(tokens[p].type == TK_REG) {
      bool success;
      word_t reg_value = isa_reg_str2val(tokens[p].str + 1, &success);
      if (!success) {
        printf("Unknown register: %s\n", tokens[p].str);
        eval_success = false;
        return 0;
      }
      return reg_value;
    }
    else {
      printf("Unexpected token type: %d\n", tokens[p].type);
    }
    assert(0);
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses. */
    return eval(p + 1, q - 1);
  }
  else {
    /* We should do more things here. */
    int op = find_main_op(p, q);
    assert(op != -1);
    // unary operator
    if (tokens[op].type == TK_NEG || tokens[op].type == TK_DEREF) {
      word_t val = eval(op + 1, q);
      word_t result;
      switch (tokens[op].type) {
        case TK_NEG: result = -val; break;
        case TK_DEREF: result = vaddr_read(val, 4); break;
        default: assert(0);
      }
      EXPR_Log("Evaluated %c %u = %u", tokens[op].type, val, result);
      return result;
    }
    word_t val1 = eval(p, op - 1);
    word_t val2 = eval(op + 1, q);
    word_t result;
    switch (tokens[op].type) {
      case '+': result = val1 + val2; break;
      case '-': result = val1 - val2; break;
      case '*': result = val1 * val2; break;
      case '/': result = val1 / val2; break;
      case TK_EQ: result = (val1 == val2); break;
      case TK_NE: result = (val1 != val2); break;
      case TK_AND: result = (val1 && val2); break;
      case TK_OR: result = (val1 || val2); break;
      default: assert(0);
    }
    EXPR_Log("Evaluated %u %c %u = %u", val1, tokens[op].type, val2, result);
    return result;
  }
}



word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  *success = true;
  
  for(int i = 0; i < nr_token; i ++) {
    if(tokens[i].type == '-' && (i == 0 || tokens[i - 1].type == '(')) {
      tokens[i].type = TK_NEG;
    }
    else if(tokens[i].type == '*' && (i == 0 || tokens[i - 1].type == '(')) {
      tokens[i].type = TK_DEREF;
    }
  }
#ifdef EXPR_DEBUG
  print_tokens();
#endif
  
  eval_success = true;
  word_t result = eval(0, nr_token - 1);
  if (!eval_success) {
    *success = false;
    return 0;
  }
  return result;
}