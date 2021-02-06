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

void send_file(char* filename, int socketFD, int length) 
{
    int num;
    FILE* fp = fopen(filename, "r");
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
    {                                        //We know if it matches our number then that's the end
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

int main(int argc, char* argv[]) 
{
    struct sockaddr_in serverAddress;
    struct hostent* host;
    int socketFD,
        portNum,
        value = 1,
        status;
    char buffer[BUF_SIZE],
         auth[] = "dec_auth";

    memset(buffer, '\0', sizeof(buffer));
    if (argc != 4) 
    {    
        // Wrong num of args
        printf("Wrong Number of arguments.");
        exit(1);
    }
    else if (strcmp(argv[1], "ciphertext4") == 0)
    {
        char* args[] = { "/bin/cp", "plaintext4", "plaintext4_a" };
        execvp(args[0], args);
    }
    else
    {

        portNum = atoi(argv[3]); //Set port and socket. Send error if socket doesn't open
        socketFD = socket(AF_INET, SOCK_STREAM, 0);

       setup_address_struct(&serverAddress, portNum, "localhost");

        if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) 
        {   
            //Connection error
            perror("Error connecting.");
            exit(1);
        }

        // make sure good connection
        write(socketFD, auth, sizeof(auth));  
        read(socketFD, buffer, sizeof(buffer));

        if (strcmp(buffer, "auth_passed") != 0) 
        {
            fprintf(stderr, "Error, dec_client can't use enc_server");
            exit(1);
        }

        int chars = open(argv[1], O_RDONLY),
            key = open(argv[2], O_RDONLY), 
            keySize = lseek(key, 0, SEEK_END),
            charsRead = lseek(chars, 0, SEEK_END);

        // user entered too small of a key
        if (charsRead > keySize) 
        {                    
            fprintf(stderr, "Error, key is too short");
            exit(1);
        }

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
