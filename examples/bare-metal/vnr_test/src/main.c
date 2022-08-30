
#include <stdio.h>
#include "wrapper.h"

int main(int argc, char **argv) {
    vnr_init();
    while(1) {
        vnr_inference_invoke();
    }
    return 0;
}
