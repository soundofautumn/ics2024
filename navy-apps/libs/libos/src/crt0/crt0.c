#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  int argc = (int)args[0];
  printf("argc = %d\n", argc);
  char **argv = (char **)&args[1];
  for (int i = 0; i < argc; i ++) {
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  char **envp = (char **)&args[argc + 2];
  for (int i = 0; envp[i] != NULL; i ++) {
    printf("envp[%d] = %s\n", i, envp[i]);
  }
  environ = envp;
  exit(main(argc, argv, envp));
  assert(0);
}
