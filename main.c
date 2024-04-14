#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <side1> <side2> <side3>\n", argv[0]);
        return 1;
    }

    unsigned int a = atoi(argv[1]);
    unsigned int b = atoi(argv[2]);
    unsigned int c = atoi(argv[3]);

    if (a <= 0 || b <= 0 || c <= 0) {
        printf("Sides must be positive integers.\n");
        return 1;
    }

    // Check if a^2 + b^2 = c^2
    if (a * a + b * b == c * c ||
        a * a + c * c == b * b ||
        b * b + c * c == a * a) {
        printf("YES\n");
    } else {
        printf("NO\n");
    }

    return 0;
}