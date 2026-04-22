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

#include "sdb.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp() {
  if(free_ == NULL) {
    printf("No free watchpoint\n");
    return NULL;
  }
  WP *wp = free_;
  free_ = free_->next;
  wp->next = head;
  head = wp;
  return wp;
}

WP* find_wp(int no) {
  WP *wp = head;
  while(wp != NULL) {
    if(wp->NO == no) {
      return wp;
    }
    wp = wp->next;
  }
  return NULL;
}

void free_wp(WP *wp) {
  if(wp == NULL) {
    return;
  }
  wp->next = free_;
  free_ = wp;
}

void wp_display() {
  WP *wp = head;
  while(wp != NULL) {
    bool success;
    word_t value = expr(wp->expr, &success);
    if(!success) {
      printf("Failed to evaluate the expression of watchpoint %d: %s\n", wp->NO, wp->expr);
    } else {
      printf("Watchpoint %d: %s, value = " FMT_WORD "\n", wp->NO, wp->expr, value);
    }
    wp = wp->next;
  }
}

void check_watchpoints() {
  WP *wp = head;
  while(wp != NULL) {
    bool success;
    word_t value = expr(wp->expr, &success);
    if(!success) {
      printf("Failed to evaluate the expression of watchpoint %d: %s\n", wp->NO, wp->expr);
    } else if(value != wp->value) {
      printf("Watchpoint %d: %s, old value = " FMT_WORD ", new value = " FMT_WORD "\n", wp->NO, wp->expr, wp->value, value);
      nemu_state.state = NEMU_STOP;
      wp->value = value;
    }
    wp = wp->next;
  }
}
