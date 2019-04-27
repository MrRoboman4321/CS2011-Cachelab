#include <stdio.h>

int main(int argc, char *argv[]) {
    int arr[8][64];
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 64; j++) {
            printf("r: %d, c: %d, addr: %p\n", i, j, &arr[i][j]);
        }
    }

    return 0;
}
