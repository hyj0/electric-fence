#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    char *arr;    int i;
    arr = (char *)malloc(sizeof(char)*5);
    char p = arr[5];
    arr[5] = 'a';
    p = arr[6];
    p = arr[7];
    strcpy(arr,"amee is my name");

    return 0;
}