#include <stdio.h>
#include <stdint.h>
#include <NDL.h>

int main() {
    int time = 0;
    NDL_init();
    uint32_t old_time = NDL_GetTicks();
    while (1) {
        uint32_t new_time = NDL_GetTicks();
        if ((new_time - old_time) >= 500){
            printf("access 0.5s interval at %d times\n", time++);
            old_time = new_time;
        }
    }
    return 0;
}