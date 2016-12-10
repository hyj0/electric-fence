#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    int SIZE = 4096;
    char *arr;    int i;
    arr = (char *)malloc(sizeof(char)*SIZE);
    for (int j = 0; j < 5; ++j) {
        arr[j] = 'a' + j;
    }
    char p;
//    p = arr[1];
//    arr[-1] = 'x';
//    p = arr[5];
//    arr[5] = 'a';
//    p = arr[6];
//    p = arr[7];
//    strcpy(arr,"amee is my name");
    free(arr);

    arr = (char *)malloc(24);
    arr[0] = 'a';
    p = arr[24];
    free(arr);
    free(arr);
    return 0;
}