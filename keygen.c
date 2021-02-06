#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
* function to create a key file of user specified length
*/
int main(int argc, char* argv[]) 
{
    srand(time(NULL)); 

    // letters to choose from
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    int randLetter,
        keyLen,
        i = 0;

    if (argc < 2) 
    { 
        fprintf(stderr, "Error, key is too short.");
        exit(1);
    }

    // keyLen is set to user specified key length
    keyLen = atoi(argv[1]);  

    // loop to generate a random key
    for (; i < keyLen; i++) 
    {                         
        // randomly select letters
        randLetter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 27];
        if (randLetter == 'A' + 26) {
            randLetter = ' ';
        }
        // store in stdout
        fprintf(stdout, "%c", randLetter);
    }

    // add endline
    fprintf(stdout, "\n");

    return 0;
}
