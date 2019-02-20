#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket(), bind(), connect() */
#include <arpa/inet.h>  /* sockaddr_in, inet_ntoa() */   
#include <string.h>    
#include <unistd.h>    

unsigned int MAX_CON = 5;
unsigned int REQUEST_BUFFER_SIZE = 32;

int serverSock;                   
int clientSock;            

struct sockaddr_in serverAddr; 
struct sockaddr_in clientAddr; 
unsigned short serverPort;  
unsigned int clientAddrLen;        

void setSocket (char** argv);
void closeSocket (int socket);
void fillServerAddr ();
void bindNetwork ();
void setListen ();
void listenRequest();
void HandleClient();

int main (int argc, char** argv) {

    setSocket(argv);

    fillServerAddr();

    bindNetwork();

    setListen();

    listenRequest();

    closeSocket(serverSock);
}

void setSocket (char** argv) {

    serverPort = atoi(argv[1]);

    serverSock = socket(PF_INET, SOCK_STREAM,IPPROTO_TCP);

    if (serverSock >= 0) printf("socket created!\n");
    else {
        perror("socket was not created");
        exit(EXIT_FAILURE);
    }
}

void closeSocket (int socket) {
    close(socket);
    printf("socket closed!\n");
}

void fillServerAddr () {
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;               
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);
}

void bindNetwork () {
    int bindResult = bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    
    if (bindResult >= 0) printf("socket binded!\n");
    else {
        perror("socket was not binded");
        exit(EXIT_FAILURE);
    }      
}

void setListen () {
    int listenResult = listen(serverSock, MAX_CON);
    
    if (listenResult >= 0) printf("socket set to listen!\n");
    else {
        perror("socket failed to listen");
        exit(EXIT_FAILURE);
    }      
} 

void listenRequest() {

     while(1) {
        clientAddrLen = sizeof(clientAddr);

        printf("\t\tWaiting for a request yo\n");
        /* Wait for a client to connect */
        clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &clientAddrLen);  
    
        if (clientSock >= 0) printf("request accepted!\n");
        else {
            perror("socket accept failed");
            exit(EXIT_FAILURE);
        }      

        /* clntSock is connected to a client! */

        printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));

        HandleClient();
    }
}

void HandleClient() {
    char requestBuffer[REQUEST_BUFFER_SIZE];       
    int requestMsgSize;                  

    /* Receive message from client */
    requestMsgSize = recv(clientSock, requestBuffer, REQUEST_BUFFER_SIZE, 0);
    if (requestMsgSize >=0) printf("msg found!\n");
    else {
        perror("no msg found");
        exit(EXIT_FAILURE);
    }      

    while (requestMsgSize > 0) {

        /* Send message back to client */
        if (send(clientSock, requestBuffer, requestMsgSize, 0) != requestMsgSize) {
            perror("msg failed to send");
            exit(EXIT_FAILURE);
        }
            
        /* Check for more data to receive */
        if ((requestMsgSize = recv(clientSock, requestBuffer, requestMsgSize, 0)) < 0) {
            perror("msg failed to send");
            exit(EXIT_FAILURE);
        }
    }

    /* Close client socket */
    close(clientSock);    
}