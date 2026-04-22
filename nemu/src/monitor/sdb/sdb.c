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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/vaddr.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char * args) {
  int n = 1;
  if(args != NULL) {
    sscanf(args, "%d", &n);
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  if(args == NULL) {
    printf("Usage: info r - display register status\n");
    printf("       info w - display watchpoint status\n");
    return 0;
  }
  if(strcmp(args, "r") == 0) {
    isa_reg_display();
  }
  else if(strcmp(args, "w") == 0) {
    wp_display();
  }
  else {
    printf("Unknown subcommand '%s'\n", args);
  }
  return 0;
}

static int cmd_x(char *args) {
  int n, offset;
  if(args == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  sscanf(args, "%d %n", &n, &offset);
  char *expression = args + offset;
  bool success;
  word_t addr = expr(expression, &success);
  if(!success) {
    printf("Invalid expression '%s'\n", expression);
    return 0;
  }
  for(int i = 0; i < n; i++) {
    printf(FMT_WORD ": ", addr + i * 4);
    for(int j = 0; j < 4; j++) {
      printf("%02x ", vaddr_read(addr + i * 4 + j, 1));
    }
    printf("\n");
  }
  return 0;
}

static int cmd_p(char *args) {
  if(args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }
  bool success;
  word_t result = expr(args, &success);
  if(success) {
    printf("%u\n", result);
  }
  else {
    printf("Invalid expression '%s'\n", args);
  }
  return 0;
}

static int cmd_w(char *args) {
  if(args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }
  bool success;
  word_t value = expr(args, &success);
  if(success) {
    WP *wp = new_wp();
    if(wp != NULL) {
      wp->expr = strdup(args);
      wp->value = value;
      printf("Watchpoint %d: %s = %u\n", wp->NO, wp->expr, wp->value);
    }
    else {
      printf("No free watchpoint\n");
    }
  }
  else {
    printf("Invalid expression '%s'\n", args);
  }
  return 0;
}

static int cmd_d(char *args) {
  if(args == NULL) {
    printf("Usage: d N\n");
    return 0;
  }
  int n;
  if(sscanf(args, "%d", &n) != 1) {
    printf("Invalid argument '%s'\n", args);
    return 0;
  }
  WP *wp = find_wp(n);
  if(wp == NULL) {
    printf("No such watchpoint %d\n", n);
    return 0;
  }
  free_wp(wp);
  return 0;
}

#ifdef CONFIG_MTRACE
static int cmd_mtrace(char *args) {
  if(args == NULL) {
    printf("Usage: mtrace START END\n");
    return 0;
  }
  word_t start, end;
  if(sscanf(args, FMT_WORD " " FMT_WORD, &start, &end) != 2) {
    printf("Invalid arguments '%s'\n", args);
    return 0;
  }
  set_mtrace_range(start, end);
  printf("Memory trace range set to [" FMT_WORD ", " FMT_WORD ")\n", start, end);
  return 0;
}
#endif

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step into instruction(s)", cmd_si },
  { "info", "Display register or watchpoint information", cmd_info },
  { "x", "Scan the memory", cmd_x },
  { "p", "Evaluate expression", cmd_p },
  { "w", "Set a watchpoint", cmd_w },
  { "d", "Delete a watchpoint", cmd_d },
#ifdef CONFIG_MTRACE
  { "mtrace", "Set memory trace range", cmd_mtrace },
#endif
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();

  /* Initialize the ring buffer. */
  init_iringbuf();
}
