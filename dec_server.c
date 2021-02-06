#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h> 

#define BUF_SIZE 69333

// chars that can be used, 27 letters including space
static const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

/*
* function to convert int to char, returns char at location in alphabet
*/
char int_to_char(int validLoc) 
{

    if (validLoc < 0 || 27 < validLoc) 
    {
        return 'a';
    }
    return characters[validLoc];
}

/*
* function to convert chars to ints, returns int
*/
int char_to_int(char character) 
{
    int i;

    for (i = 0; i < 27; i++) 
    {
        if (characters[i] == character) 
        {
            return i;
        }
    }

    // failed to convert char to int
    return -1;
}

/*
* function to set up the address struct
*/
void setup_address_struct(struct sockaddr_in* address, int portNumber)
{
    memset((char*)address, '\0', sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_port = htons(portNumber);
    address->sin_addr.s_addr = INADDR_ANY;
}

void decrypt_message(char *message, char *key, int length) 
{
    int i, 
     charId,
     keyId,
     decId;
    length = (strlen(message) - 1);

    for (i = 0; i < length; i++)
    { 
        keyId = char_to_int(key[i]);
        charId = char_to_int(message[i]);

        // decription id to find positon in alphabet
        decId = (charId - keyId) % 27;

        if (decId < 0)
        {
            // shift decId to positive loc in alphabet
            decId += 27;
        }

        // convert to message
        message[i] = int_to_char(decId);
    }

    message[i] = '\0';
}


int main(int argc, char* argv[]) 
{
    pid_t spawnPid;
    int value = 1,
        socketFD,
        secondSocketFD;
    char buffer[BUF_SIZE];
    socklen_t clientLen;
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    if (argc != 2) 
    {              //Correct number of arguments needed
        fprintf(stderr, "Incorrect number of arguments\n");
        exit(1);
    }

    // open socket
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        // failed to open socket
        perror("Error opening socket.");
        exit(1);
    }

    setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));

    // setup the server address
    setup_address_struct(&serverAddress, atoi(argv[1]));


    // bind socket to server
    if (bind(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) 
    {  
        //Enable socket to listen
        perror("Binding error");
        exit(1);
    }

    listen(socketFD, 5);

    while (1) 
    {
        clientLen = sizeof(clientAddress);
        secondSocketFD = accept(socketFD, (struct sockaddr*)&clientAddress, &clientLen);  //new socket to use
        if (secondSocketFD < 0) 
        {
            perror("Error on accept.");
            exit(1);
        }
        spawnPid = fork();                   //Fork process and exit with error on failure
        if (spawnPid < 0) 
        {
            perror("Error on fork.");
            exit(1);
        }
        if (spawnPid == 0) 
        {
            memset(buffer, '\0', sizeof(buffer));
            int iter,
             next = 0,
             numBytes,
             bytesRead = 0;
            char* key;
            numBytes = sizeof(buffer);

            read(secondSocketFD, buffer, sizeof(buffer) - 1);  //Authenticate to make sure call is to correct file
            if (strcmp(buffer, "dec_auth") != 0) {
                char output[] = "failed";
                write(secondSocketFD, output, sizeof(output));
                exit(2);
            }
            else 
            {
                char output[] = "auth_passed";
                write(secondSocketFD, output, sizeof(output));
            }
            bzero(buffer, sizeof(buffer));
            char* buffHelper = buffer;

            while (1) {
                bytesRead = read(secondSocketFD, buffHelper, numBytes);
                if (numBytes == 0) {
                    break;
                }

                for (iter = 0; iter < bytesRead; iter++) 
                {    //Go until hit second new line
                    if (buffer[iter] == '\n') 
                    {
                        ++next;
                        if (next == 1) {
                            key = buffer + iter + 1;
                        }
                    }
                }
                if (next == 2) 
                {
                    break;
                }
                numBytes = numBytes - bytesRead;
                buffHelper = buffHelper + bytesRead;
            }
            char message[BUF_SIZE];                 //Set message and get ready to send
            memset(message, '\0', sizeof(message));
            strncpy(message, buffer, key - buffer);
            decrypt_message(message, key, strlen(message)); //Decrypt it, write it, and cleanup everything
            write(secondSocketFD, message, sizeof(message));
        }
        close(secondSocketFD);
    }
    close(socketFD);

    return 0;
}
