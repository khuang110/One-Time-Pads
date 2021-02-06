#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include<sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define BUF_SIZE 69333

void send_file(char* fileName, int socketFD, int length)
{
    int num;
    FILE* fp = fopen(fileName, "r");
    char buffer[BUF_SIZE];
    memset(buffer, '\0', sizeof(buffer));

    while ((length = fread(buffer, sizeof(char), BUF_SIZE, fp)) > 0)
    {
        //send file
        if ((num = send(socketFD, buffer, length, 0)) < 0)
        {
            break;
        }
        memset(buffer, '\0', sizeof(buffer));
    }
    if (num == BUF_SIZE)
    { 
        // terminate connection
        send(socketFD, "0", 1, 0);
    }
    fclose(fp);

    return;
}

/*
* function to set up the address struct
*/
void setup_address_struct(struct sockaddr_in* address,
    int portNumber,
    char* hostname)
{

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname(hostname);

    if (hostInfo == NULL)
    {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*)&address->sin_addr.s_addr,
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

/*
* function to check for invalid chars in file
*/
void check_valid_chars(char *buffer, char *args, int validChar)
{
    while (read(validChar, buffer, 1) != 0) 
    {
        if (isspace(buffer[0]) || isalpha(buffer[0])) 
        {
        }
        else 
        {
            fprintf(stderr, "Invalid characters in %s\n", args);
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) 
{ 
    struct sockaddr_in serverAddress;
    struct hostent* host;
    int socketFD,
        portNum,
        value = 1,
        status;
    char buffer[BUF_SIZE],
        auth[] = "enc_auth",
        *arg1 = argv[1];
    
    memset(buffer, '\0', sizeof(buffer));
    
    if (argc != 4) 
    {  
        // Wrong num of args
        printf("Wrong Number of arguments.");
        exit(0);
    }
    else
    {
        //Last argument is the user entered port number
        portNum = atoi(argv[3]); 
        //Set up the socket
        socketFD = socket(AF_INET, SOCK_STREAM, 0);

        setup_address_struct(&serverAddress, portNum, "localhost");

        if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) 
        {  
            // connection error
            perror("Error connecting.");
            exit(2);
        }

        // write temp char then read it to validate good connection
        write(socketFD, auth, sizeof(auth));   
        read(socketFD, buffer, sizeof(buffer));

        // make sure connection is good
        if (strcmp(buffer, "enc_auth") != 0)
        {
            fprintf(stderr, "Error, can't use dec_clent.");
            exit(2);
        }

        int key = open(argv[2], O_RDONLY),
            keySize = lseek(key, 0, SEEK_END),
            chars = open(argv[1], O_RDONLY),    // var for entered characters   
            charsRead = lseek(chars, 0, SEEK_END);

        if (charsRead > keySize)
        {
            fprintf(stderr, "Error, key is too short");
            exit(1);
        }

        // check second cmd arg for invalid chars
        int validChar = open(argv[1], 'r');
        check_valid_chars(buffer, argv[1], validChar);

        memset(buffer, '\0', sizeof(buffer));

        //send file
        send_file(argv[1], socketFD, charsRead);
        send_file(argv[2], socketFD, keySize);

        // Check if socket is good
        if ((status = read(socketFD, buffer, sizeof(buffer) - 1)) < 0)
        {
           perror("Error reading from socket");
            exit(1);
        }
        //show contents of file
        printf("%s\n", buffer);

        close(socketFD);

        return 0;
    }
}
