#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket(), bind(), connect() */
#include <arpa/inet.h>  /* sockaddr_in, inet_ntoa() */   
#include <string.h>    
#include <unistd.h>    


unsigned int MAX_CON = 5;

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

int main (int argc, char** argv) {

    setSocket(argv);

    fillServerAddr();

    bindNetwork();

    setListen();

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
    
    if (listenResult >= 0) printf("socket listening!\n");
    else {
        perror("socket failed to listen");
        exit(EXIT_FAILURE);
    }      
} 