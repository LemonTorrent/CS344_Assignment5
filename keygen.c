#include <stdlib.h>
#include <stdio.h>
#include <math.h>


int main(int argc, char *argv[]) {
    time_t t;

    if (argc != 2) {
        printf("Enter the number of digits that the key should be\n");
        return 0;
    }

    srand((unsigned) time(&t));

    int numDigit = atoi(argv[1]);
    int numChar = atoi(argv[1]);

    int temp;
    char* genChar;

    for (int i = 0; i < numChar; i++) {
        temp = rand() % 27;
        temp += 65;

        

        if (temp == 91){
            printf(" ");
        } else {
            genChar = temp;
            printf("%c", genChar);
        }
        

    }

    printf("\n");
    return 0;

}