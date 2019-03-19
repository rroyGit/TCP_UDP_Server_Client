#include <stdio.h>
#include <sys/socket.h> /* socket(), connect(), send(), recv() */
#include <arpa/inet.h>  /* sockaddr_in and inet_addr() */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// maximum buffer size including null terminator
#define REQUEST_BUFFER_SIZE 80 


int main(int argc, char *argv[]) {
    // client file descriptor
    int clientSock;
    // server address struct
    struct sockaddr_in serverAddr;
    // server port number
    unsigned short serverPort;
    // physical network IP address
    char *serverPhyIP;
    // buffer to store messages
    char requestStringBuffer[REQUEST_BUFFER_SIZE];
    char requestBuffer[REQUEST_BUFFER_SIZE];
    // length of string
    unsigned int requestStringLen;
    // bytes received from recv(...) calls
    int bytesReceived;

    if (argc != 3) {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
       exit(1);
    }
    
    // get physical network IP and port number from command line arguments
    serverPhyIP = argv[1];
    serverPort = atoi(argv[2]);

    // construct the server address structure
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

    while (1) {
        // create a client socket
        if ((clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            perror("socket was not created");
            exit(EXIT_FAILURE);
        }
        // connect to server using the server address struct
        if (connect(clientSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
            perror("socket was not connected");
            close(clientSock);
            exit(EXIT_FAILURE);
        }
        printf("\tEnter \"EXIT\" to exit the program\n");
        printf("Enter Msg: ");

        // get user input
        fgets(requestStringBuffer, REQUEST_BUFFER_SIZE, stdin);
        char* pch = strchr(requestStringBuffer, '\n');
        if (pch != NULL) *pch = '\0';

        if (strcmp(requestStringBuffer,"EXIT") == 0) break;
        
        // get length of user input message
        requestStringLen = strlen(requestStringBuffer);

        // printf("Sending: |%s|\n", requestStringBuffer);

        printf("\tGetting response from server...\n");
        // send user message to server using the client socket
        if (send(clientSock, requestStringBuffer, requestStringLen, 0) != requestStringLen) {
            perror("socket sent incorrect bytes");
            close(clientSock);
            exit(EXIT_FAILURE);
        }

        // receive response message from server
        bytesReceived = recv(clientSock, requestBuffer, REQUEST_BUFFER_SIZE - 1, 0);

        if (bytesReceived < 0){
            if (errno != EWOULDBLOCK) {
                perror("no msg found");
            }
            break;
        } else if (bytesReceived == 0) {
            printf("connection closed\n");
            break;
        }

        requestBuffer[bytesReceived] = '\0'; 

        // print received message
        printf("Received: |%s|\nSize: %i\n", requestBuffer, bytesReceived);

        // close client socket
        close(clientSock);
    }

   // close client socket
   close(clientSock);
   return EXIT_SUCCESS;
}
