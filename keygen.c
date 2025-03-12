#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int agrc, char* argv[])
{
    long keylength = strtol(argv[1], NULL, 10);
    int key; 
    char encrypted_key[keylength +1];
    srand(time(0)); //seed random number
    char valid[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    if(agrc != 2 || keylength <= 0)
    {
        fprintf(stderr, "Error: argv incorrect\n");
        exit(1);
    }
    else
    {
        for(int i = 0; i < keylength; i++)
        {
            key = rand() % 27;
            encrypted_key[i] = valid[key];
        } 
        encrypted_key[keylength] = '\0';
        fprintf(stdout, "%s\n", encrypted_key);
    } return 0;
} 
