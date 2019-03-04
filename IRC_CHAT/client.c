#include <stdio.h>
#include <sys/socket.h> /* socket(), connect(), send(), recv() */
#include <arpa/inet.h>  /* sockaddr_in and inet_addr() */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REQUEST_BUFFER_SIZE 160   /* Size of receive buffer */


int main(int argc, char *argv[]) {
    int clientSock;
    struct sockaddr_in serverAddr;
    unsigned short serverPort;
    char *serverPhyIP;
    char requestString[REQUEST_BUFFER_SIZE];
    char requestStringBuffer[REQUEST_BUFFER_SIZE];
    char requestBuffer[REQUEST_BUFFER_SIZE];
    unsigned int requestStringLen;
    int bytesReceived, totalBytesReceived;

    if (argc != 3) {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
       exit(1);
    }

    serverPhyIP = argv[1];
    serverPort = atoi(argv[2]);

    /* Construct the server address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    if (inet_addr(serverPhyIP) == -1) {
        perror("IP could not be converetd to network bytes");
        close(clientSock);
        exit(EXIT_FAILURE);
    } else {
        serverAddr.sin_addr.s_addr = inet_addr(serverPhyIP);
    }
    
    serverAddr.sin_port = htons(serverPort);

    for (unsigned short i = 0; i < 7; i++) {

        /* Create a reliable, stream socket using TCP */
        if ((clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            perror("socket was not created");
            exit(EXIT_FAILURE);
        }

        /* Establish the connection to the server */
        if (connect(clientSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
            perror("socket was not connected");
            exit(EXIT_FAILURE);
        }

        printf("Enter %u chars text:", 31);

        fgets(requestStringBuffer, REQUEST_BUFFER_SIZE, stdin);
        char* pch = strchr(requestStringBuffer, '\n');
        if (pch != NULL) *pch = '\0';

        memcpy(&requestString, &requestStringBuffer, (unsigned)(strlen(requestStringBuffer)+1));
        requestStringLen = strlen(requestString);
    //----------------------------------------------------------------------------------------------------//
        printf("Sending: |%s|\n", requestString);

        /* Send the string to the server */
        if (send(clientSock, requestStringBuffer, requestStringLen, 0) != requestStringLen) {
            perror("socket sent incorrect bytes");
            exit(EXIT_FAILURE);
        }

    
        totalBytesReceived = 0;
        
        printf("Received:\n");
        while (totalBytesReceived < REQUEST_BUFFER_SIZE) {
            /* Receive up to the buffer size (minus 1 to leave space for a null terminator) bytes from the sender */
            if ((bytesReceived = recv(clientSock, requestBuffer, REQUEST_BUFFER_SIZE - 1, 0)) <= 0) {
                perror("recv() failed or connection closed prematurely");
                exit(EXIT_FAILURE);
            }

            totalBytesReceived += bytesReceived;
            requestBuffer[bytesReceived] = '\0';  /* Terminate the string! */
            printf("%s", requestBuffer);
        }

        printf("\n"); 
        close(clientSock);
    }
   
   return EXIT_SUCCESS;
}
