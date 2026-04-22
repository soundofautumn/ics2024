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

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

// ----------- watchpoint -----------

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char *expr;
  word_t value;
} WP;

void init_wp_pool();
WP* new_wp();
WP* find_wp(int no);
void free_wp(WP *wp);
void wp_display();

// ----------- iringbuf -----------

typedef struct {
  char **data;
  int head, tail;
  int size;
  int capacity;
} IRingBuf;

void init_iringbuf();
void write_iringbuf(const char *str);
void iringbuf_display();

// ----------- mtrace -----------

void set_mtrace_range(word_t start, word_t end);
void mtrace_read(word_t addr, int len, word_t data);
void mtrace_write(word_t addr, int len, word_t data);

#endif
