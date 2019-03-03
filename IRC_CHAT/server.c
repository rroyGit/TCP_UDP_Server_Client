#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>  /* sockaddr_in, inet_ntoa() */
#include <sys/socket.h> /* socket(), bind(), connect() */
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <errno.h>
#include <string.h>    
#include <unistd.h>

unsigned int MAX_CON = 5;
unsigned int REQUEST_BUFFER_SIZE = 32;

int serverSock;                   
int clientSock;        

int rc, serverOn = 1;

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

void setSocketResuable ();
void setSocketNonblocking ();
void initPollFD ();

struct pollfd fds[5];
int nfds = 1, current_size = 0, i, j;
int timeout;

int main (int argc, char** argv) {

    if (argc != 2) {
       fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
       exit(1);
    }

    setSocket(argv);

    setSocketResuable();

    setSocketNonblocking();

    initPollFD();

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

void setSocketResuable () {
    rc = setsockopt(serverSock, SOL_SOCKET,  SO_REUSEADDR,(char *)&serverOn, sizeof(serverOn));
    if (rc < 0) {
        perror("socket failed to set to resuable");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }
} 

void setSocketNonblocking () {
    rc = ioctl(serverSock, FIONBIO, (char *)&serverOn);
    if (rc < 0) {
        perror("socket failed to set to nonblocking");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }
}

void initPollFD () {
    memset(fds, 0 , sizeof(fds));

    fds[0].fd = serverSock;
    fds[0].events = POLLIN;
  
    timeout = (1 * 60 * 1000);
}

void closeSocket (int socket) {
    close(socket);
    printf("\nsocket closed!\n");
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
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }      
}

void setListen () {
    int listenResult = listen(serverSock, MAX_CON);
    
    if (listenResult >= 0) printf("socket set to listen!\n");
    else {
        perror("socket failed to listen");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }      
} 

void listenRequest() {

     while(1) {
        printf("\t\tWaiting on polling!\n");
        rc = poll(fds, nfds, timeout);

        if (rc < 0){
            perror("polling failed");
            closeSocket(serverSock);
            exit(EXIT_FAILURE);
        }
    
        if (rc == 0) {
            printf("Polling timed out");
            closeSocket(serverSock);
            exit(EXIT_FAILURE);
        }


        clientAddrLen = sizeof(clientAddr);

        
        /* Wait for a client to connect */
        clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &clientAddrLen);  
    
        if (clientSock >= 0) printf("request accepted!\n");
        else {
            perror("socket accept failed");
            closeSocket(serverSock);
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

    if (requestMsgSize > 0) {
        printf("Msg found - size: %i\n", requestMsgSize);
        requestBuffer[requestMsgSize] = '\0';
        strcat(requestBuffer," yo");
        requestMsgSize = strlen(requestBuffer);
    } else {
        perror("no msg found");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }      

    while (requestMsgSize > 0) {

        /* Send message back to client */
        if (send(clientSock, requestBuffer, requestMsgSize, 0) != requestMsgSize) {
            perror("msg failed to send");
            exit(EXIT_FAILURE);
        }
        
        // clear dank memory
        memset(requestBuffer, 0, REQUEST_BUFFER_SIZE);

        /* Check for more data to receive */

        requestMsgSize = recv(clientSock, requestBuffer, REQUEST_BUFFER_SIZE, 0);

        if (requestMsgSize == -1) {
            perror("failed to receive msg");
            closeSocket(serverSock);
            exit(EXIT_FAILURE);
        } else if (requestMsgSize == 0) {
            printf("No msg meant be received - client disconnected");
        } else {
            printf("Extra Msg found - size: %i\n", requestMsgSize);
            requestBuffer[requestMsgSize] = '\0';
            strcat(requestBuffer," yo");
            requestMsgSize = strlen(requestBuffer);
        }
    }

    close(clientSock);    
}