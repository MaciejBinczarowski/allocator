#include <allocator.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    char* i = (char*) alloc(10 * sizeof(char));
    char* j = (char*) alloc(100 * sizeof(char));

    i[20] = 'd';

    dealloc(j);
    dealloc(i);

    return 0;
}
