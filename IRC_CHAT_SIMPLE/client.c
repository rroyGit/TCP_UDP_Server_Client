#include <stdio.h>
#include <sys/socket.h> /* socket(), connect(), send(), recv() */
#include <arpa/inet.h>  /* sockaddr_in and inet_addr() */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define REQUEST_BUFFER_SIZE 80 


int main(int argc, char *argv[]) {
    int clientSock;
    struct sockaddr_in serverAddr;
    unsigned short serverPort;
    char *serverPhyIP;
   
    char requestStringBuffer[REQUEST_BUFFER_SIZE];
    char requestBuffer[REQUEST_BUFFER_SIZE];
    unsigned int requestStringLen;
    int bytesReceived;

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
    printf("Enter \"EXIT\" to exit the program!\n");

    while (1) {
       
        if ((clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            perror("socket was not created");
            exit(EXIT_FAILURE);
        }

        if (connect(clientSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
            perror("socket was not connected");
            close(clientSock);
            exit(EXIT_FAILURE);
        }

        printf("Enter Msg: ");

        fgets(requestStringBuffer, REQUEST_BUFFER_SIZE, stdin);
        char* pch = strchr(requestStringBuffer, '\n');
        if (pch != NULL) *pch = '\0';

        if (strcmp(requestStringBuffer,"EXIT") == 0) break;
        

        requestStringLen = strlen(requestStringBuffer);
    //----------------------------------------------------------------------------------------------------//
        // printf("Sending: |%s|\n", requestStringBuffer);

        
        if (send(clientSock, requestStringBuffer, requestStringLen, 0) != requestStringLen) {
            perror("socket sent incorrect bytes");
            close(clientSock);
            exit(EXIT_FAILURE);
        }
      
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
        printf("Received: |%s|\nSize: %i\n", requestBuffer, bytesReceived);

        close(clientSock);
    }
   
   close(clientSock);
   return EXIT_SUCCESS;
}
