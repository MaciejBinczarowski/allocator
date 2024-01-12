#include "../../src/allocator.h"

int main(int argc, char const *argv[])
{
    char* i = (char*) alloc(10 * sizeof(char));
    char* j = (char*) alloc(100 * sizeof(char));

    i[15] = 'd';

    dealloc(j);
    dealloc(i);

    return 0;
}
