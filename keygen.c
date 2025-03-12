#include <stdio.h>
#include <stdlib.h>

int main(int agrc, char* argv[])
{
    if(agrc > 2)
    {
        fprintf(stderr, "Error: argc over two\n");
    }
    else
    {
        int keylength = argv[1];
    }
}