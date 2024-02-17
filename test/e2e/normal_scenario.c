#include <allocator.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    char* i = (char*) alloc(10 * sizeof(char));
    int* j = (int*) alloc(100 * sizeof(int));

    for (int k = 0; k < 100; k++)
    {
        j[k] = k;
    }

    for (int k = 0; k < 100; k++)
    {
        printf("%d\n", j[k]);
    }

    dealloc(j);
    dealloc(i);

    return 0;
}