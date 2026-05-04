#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  int argc = (int)args[0];
  char **argv = (char **)&args[1];
  char **envp = (char **)&args[argc + 2];
  printf("argc = %d\n", argc);
  for (int i = 0; i < argc; i ++) {
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  for (int i = 0; envp[i] != NULL; i ++) {
    printf("envp[%d] = %s\n", i, envp[i]);
  }
  environ = envp;
  exit(main(argc, argv, envp));
  assert(0);
}
