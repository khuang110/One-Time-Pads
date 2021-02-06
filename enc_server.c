#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h> 

#define BUF_SIZE 69333

// chars that can be used
static const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

/*
* function to convert int to char, returns char at location in alphabet
*/
char int_to_char(int validLoc)
{

    if (validLoc < 0 || 27 < validLoc)
    {	                //If number is invalid, just return lowercase char	
        return 'a';
    }
    return characters[validLoc];                       //Else, return valid character
}

/*
* function to convert chars to ints, returns int
*/
int char_to_int(char character)
{
    int i;

    for (i = 0; i < 27; i++)
    {
        if (character == characters[i])
        {
            return i;
        }
    }

    // failed to convert char to int
    return -1;
}

void encrypt_message(char message[], char key[], int len)
{
    int i,
        charId,
        keyId,
        encId;
    len = (strlen(message) - 1);

    for (i = 0; i < len; i++)
    {
        // get ids to use
        keyId = char_to_int(key[i]);
        charId = char_to_int(message[i]);

        encId = (charId + keyId) % 27;
        // swap message with encrypted message
        message[i] = int_to_char(encId);
    }

    message[i] = '\0';
}

int main(int argc, char* argv[])
{  
    int value = 1,
        pipeFDs[2],
        portNumber;
    char buffer[BUF_SIZE];
    struct sockaddr_in clientAddress;
    struct sockaddr_in serverAddress;
    socklen_t clientLen;
    pid_t spawnPid;


    if (argc != 2)
    {                  //Not equal to two arguments will return an error
        fprintf(stderr, "Incorrect number of arguments");
        exit(1);
    }

    // Create the pipe with error check
    if (pipe(pipeFDs) == -1) {
        perror("Call to pipe() failed\n");
        exit(1);
    }

    pipeFDs[0] = socket(AF_INET, SOCK_STREAM, 0); //Create the socket
    if (pipeFDs[0] < 0)
    {                           //Error if not created correctly
        perror("Error opening socket.");
        exit(1);
    }



    setsockopt(pipeFDs[0], SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));

    memset((char*)&serverAddress,'\0', sizeof(serverAddress)); 
    portNumber = atoi(argv[1]); 
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = INADDR_ANY;


    if (bind(pipeFDs[0], (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {  //Enable listening
        perror("Error on binding");
        exit(1);
    }

    listen(pipeFDs[0], 5);        //Equivalent of turning it on

    while (1)
    {
        clientLen = sizeof(clientAddress);
        pipeFDs[1] = accept(pipeFDs[0], (struct sockaddr*)&clientAddress, &clientLen);  //Need to be able to accept sent calls
        if (pipeFDs[1] < 0)
        {            //Error if unable to accept
            perror("Error on accept");
            exit(1);
        }
        spawnPid = fork();

        if (spawnPid < 0)
        {
            perror("Error on fork");
            exit(1);
        }
        if (spawnPid == 0)
        {                       //This is how we'll handle all our connections
            memset(buffer, '\0', sizeof(buffer));

            int iter;
            int next;
            char* key;
            int numBytes;
            int strLoc;
            numBytes = sizeof(buffer);
            strLoc = 0;
            next = 0;

            read(pipeFDs[1], buffer, sizeof(buffer) - 1);      //Authenticate and exit with error if unable to
            if (strcmp(buffer, "enc_auth") != 0) {
                char output[] = "failed";
                write(pipeFDs[1], output, sizeof(output));
                exit(2);
            }
            else {
                char output[] = "enc_auth";
                write(pipeFDs[1], output, sizeof(output));
            }
            memset(buffer, '\0', sizeof(buffer));
            char* readBuff = buffer,
                * fixedBuff;
            while (1) 
            {
                strLoc = read(pipeFDs[1], readBuff, numBytes);
                
                if (numBytes == 0) 
                {                      //No need to read anymore, break here if bytes is 0
                    break;
                }
                if (strLoc == -1) 
                {
                    // -1 indicates an error
                    break;
                }
                // indicates end of file
                if (strLoc == 0)
                {
                    break;
                }

                for (iter = 0; iter < strLoc; iter++) 
                {        //Iterate to find new lines to know when to break
                    if (buffer[iter] == '\n')
                    {
                        ++next;                                 //Increment lines
                        if (next == 1) 
                        {
                            // add one to get null terminator
                            key = buffer + iter + 1;
                        }
          
                    }
                }
                if (next == 2) 
                {
                    break; 
                }
                numBytes = numBytes - strLoc;        //Subtract the number of bytes read from total number of bytes
                readBuff = strLoc + readBuff;             
            }
            int offset = key - buffer;
            char message[BUF_SIZE];                         //Set up message to be sent
            memset(message, '\0', sizeof(message));
            strncpy(message, buffer, offset);       //Copy to message
            encrypt_message(message, key, strlen(message)); //Encrypt it using earlier function
            write(pipeFDs[1], message, sizeof(message));   //Write to socket
            
        }
        close(pipeFDs[1]);
    }
    close(pipeFDs[0]);

    return 0;
}
