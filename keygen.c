#include <stdio.h>
#include <stdlib.h>

void generateKey(FILE* secret_file, FILE* encyrpted_file, FILE* key_file)
{
    int c;
    while((c=fgetc(secret_file)) != EOF)
    {
        int key = rand();
        int encrypted_c = c ^ key;
        fputc(key, key_file);
        fputc(encrypted_c, encyrpted_file);
    }
}


int main(int agrc, char* argv[])
{
    if(agrc != 2)
    {
        printf("Provide file to encrypt");
    }
    else
    {
        char* secret_file_name = argv[1];
        FILE* secret_file = fopen(secret_file_name, "r");
        FILE* encyrpted_file = fopen("crypt.out", "w");
        FILE* key_file = fopen("key.out", "w");

        generateKey(secret_file, encyrpted_file, key_file);
        fclose(secret_file);
        fclose(encyrpted_file);
        fclose(key_file);
    }
}