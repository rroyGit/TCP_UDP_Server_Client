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

#define MAX_CON  5
#define MAX_FDS  6

unsigned int REQUEST_BUFFER_SIZE = 32;

int serverSock = -1;                   
int clientSock = -1;        

int rc, serverOn = 1;

struct sockaddr_in serverAddr; 
struct sockaddr_in clientAddr[MAX_CON]; 
unsigned short serverPort;  
unsigned int clientAddrLen;        

void setSocket (char** argv);
void closeSocket (int socket);
void fillServerAddr ();
void bindNetwork ();
void setListen ();
void listenRequest();
void HandleClient(int index);

void setSocketResuable ();
void setSocketNonblocking ();
void initPollFD ();

struct pollfd fds[MAX_FDS];
int nfds = 1, current_size = 0, i, j;
int timeout;

int close_client = 0;
int close_server = 0;
int shift_pollfds = 0;

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

    return EXIT_SUCCESS;
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

    do {
        printf("\t\tWaiting on polling!\n");
        rc = poll(fds, nfds, timeout);

        if (rc < 0){
            perror("polling failed");
            break;
        }
    
        if (rc == 0) {
            printf("Polling timed out\n");
            break;
        }

        current_size = nfds;
        
        for (i = 0; i < current_size; i++) {

            if (fds[i].revents == 0) continue;


            if (fds[i].revents != POLLIN) {
                perror("No pollin");
                close_server = 1;
                break;
            }

            if (fds[i].fd == serverSock) {
                do {
                    // need a list of clientAddr
                    clientAddrLen = sizeof(clientAddr);

                    /* Wait for a client to connect */
                    clientSock = accept(serverSock, (struct sockaddr *) &clientAddr[nfds-1], &clientAddrLen);  
                
                    if (clientSock < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror("socket accept failed");
                            close_server = 1;
                        }
                        break;
                    }

                    printf("  New incoming connection - %i\n", clientSock);
                    fds[nfds].fd = clientSock;
                    fds[nfds].events = POLLIN;
                    nfds++;      
                } while(clientSock != -1);
                
            } else {
                printf("Handling client %s\n", inet_ntoa(clientAddr[i-1].sin_addr));

                HandleClient(i);
            }
        }

        if (shift_pollfds) {
            shift_pollfds = 0;

            for (i = 0; i < nfds; i++) {
                if(fds[i].fd == -1) {
                    for(j = i; j < nfds - 1; j++) {
                        fds[j].fd = fds[j+1].fd;

                        clientAddr[j] = clientAddr[j+1];

                    }
                    fds[j].fd = -1;
                    memset(&clientAddr[j], 0, sizeof(clientAddr[j]));
                    i--;
                    nfds--;
                }
            }
        }

    } while (close_server == 0);

    
    for (i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0)  closeSocket(fds[i].fd);
    }
}

void HandleClient(int index) {
    char requestBuffer[REQUEST_BUFFER_SIZE];       
    int requestMsgSize;

    do {
        close_client = 0;
        // clear dank memory
        memset(requestBuffer, 0, REQUEST_BUFFER_SIZE);
        requestMsgSize = recv(fds[index].fd, requestBuffer, REQUEST_BUFFER_SIZE, 0);

        if (requestMsgSize < 0){
            if (errno != EWOULDBLOCK) {
                perror("no msg found");
                close_client = 1;
            }
            break;
        } else if (requestMsgSize == 0){
            printf("  Connection closed\n");
            close_client = 1;
            break;
        }    

        printf("Msg found - size: %i\n", requestMsgSize);
        requestBuffer[requestMsgSize] = '\0';
        strcat(requestBuffer," yo");
        requestMsgSize = strlen(requestBuffer);

        if (send(fds[index].fd, requestBuffer, requestMsgSize, 0) < 0) {
            perror("msg failed to send");
            close_client = 1;
            break;
        }

    } while(1);  

    if (close_client) {
        close(fds[index].fd);
        fds[index].fd = -1;
        shift_pollfds = 1;
    }
}