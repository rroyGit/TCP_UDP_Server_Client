#include <stdio.h>      
#include <sys/socket.h> /* socket(), connect(), send(), recv() */
#include <arpa/inet.h>  /* sockaddr_in and inet_addr() */
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>    

#define REQUEST_BUFFER_SIZE 32   /* Size of receive buffer */


int main(int argc, char *argv[])
{
    int clientSock;                     
    struct sockaddr_in serverAddr; 
    unsigned short serverPort; 
    char *serverPhyIP;                   
    char *requestString;                
    char requestBuffer[REQUEST_BUFFER_SIZE];     
    unsigned int requestStringLen;      
    int bytesReceived, totalBytesReceived;

    if (argc != 4) {
       fprintf(stderr, "Usage: %s <Server IP> <A Word> <Echo Port>\n", argv[0]);
       exit(1);
    }

    serverPhyIP = argv[1];
    requestString = argv[2];
    serverPort = atoi(argv[3]);
    

    /* Create a reliable, stream socket using TCP */
    if ((clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket was not created");
        exit(EXIT_FAILURE);
    }

    /* Construct the server address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));    
    serverAddr.sin_family = AF_INET;             
    serverAddr.sin_addr.s_addr = inet_addr(serverPhyIP);   
    serverAddr.sin_port = htons(serverPort);

    /* Establish the connection to the server */
    if (connect(clientSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("socket was not connected");
        exit(EXIT_FAILURE);
    }

    requestStringLen = strlen(requestString);

    /* Send the string to the server */
    if (send(clientSock, requestString, requestStringLen, 0) != requestStringLen) {
        perror("socket sent incorrect bytes");
        exit(EXIT_FAILURE);
    }

    /* Receive the same string back from the server */
    totalBytesReceived = 0;
    printf("Received: ");

    while (totalBytesReceived < requestStringLen) {
        /* Receive up to the buffer size (minus 1 to leave space for a null terminator) bytes from the sender */
        if ((bytesReceived = recv(clientSock, requestBuffer, REQUEST_BUFFER_SIZE - 1, 0)) <= 0) {
            perror("recv() failed or connection closed prematurely");
            exit(EXIT_FAILURE);
        }
    
        totalBytesReceived += bytesReceived;
        requestBuffer[bytesReceived] = '\0';  /* Terminate the string! */
        printf("%s", requestBuffer);
    }

    printf("\n");    /* Print a final linefeed */

    close(clientSock);
    exit(0);
}