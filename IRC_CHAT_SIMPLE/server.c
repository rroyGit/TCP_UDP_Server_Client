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

// max number of clients that can connect to a server
#define MAX_CON  5
// max number of file descriptors (5 clients + 1 server)
#define MAX_FDS  6

// max buffer size
unsigned int REQUEST_BUFFER_SIZE = 80;

// init descriptors to default
int serverSock = -1;                   
int clientSock = -1;    

// temp variable to hold returned values
int rc;

// track server status while socket set to reusable
int serverOn = 1;

// server address struct
struct sockaddr_in serverAddr;
// client(s) address struct
struct sockaddr_in clientAddr[MAX_CON];
// server port number
unsigned short serverPort;
// length of client address for calls like accept(...)
unsigned int clientAddrLen;        

void setSocket ();
void closeSocket (int socket);
void fillServerAddr (char** argv);
void bindNetwork ();
void setListen ();
void listenRequest ();
void handleClient (int index);

void setSocketResuable ();
void setSocketNonblocking ();
void initPollFD ();

struct pollfd fds[MAX_FDS];
int nfds = 1, current_size = 0, i, j;
int timeout;

int close_client = 0;
int close_server = 0;
int shift_pollfds = 0;

int currentIndex = 0;

int main (int argc, char** argv) {

    if (argc != 2) {
       fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
       exit(1);
    }

    setSocket();

    setSocketResuable();

    setSocketNonblocking();

    initPollFD();

    fillServerAddr(argv);

    bindNetwork();

    setListen();

    listenRequest();

    return EXIT_SUCCESS;
}
/*
    create a TCP socket with guranteed packet delivery service via sock stream
*/
void setSocket () {

    serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSock < 0) {
        perror("socket was not created");
        exit(EXIT_FAILURE);
    }
}
/*
    change server socket's options - make the socket reusable to avoid EADDRINUSE
    from a bind call when handling concurrent clients to a server
*/
void setSocketResuable () {
    rc = setsockopt(serverSock, SOL_SOCKET,  SO_REUSEADDR, (char *)&serverOn, sizeof(serverOn));
    if (rc < 0) {
        perror("socket failed to set to resuable");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }
} 
/*
    set server socket to non-binding to prevent it from blocking the running thread with calls
    like accept(...) and recv(...)
*/
void setSocketNonblocking () {
    rc = ioctl(serverSock, FIONBIO, (char *)&serverOn);
    if (rc < 0) {
        perror("socket failed to set to nonblocking");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }
}
/*
    set the first poll item to server socket and set timeout to 1 minute
*/
void initPollFD () {
    memset(fds, 0 , sizeof(fds));

    fds[0].fd = serverSock;
    fds[0].events = POLLIN;
  
    timeout = (1 * 60 * 1000);
}
/*
    close the incoming socket descriptor
*/
void closeSocket (int socket) {
    close(socket);
    printf("\nsocket closed!\n");
}
/*
    fill in the server address struct with port number and set the socket to bind to all
    network interfaces on the host machine using INADDR_ANY
*/
void fillServerAddr (char** argv) {
    serverPort = atoi(argv[1]);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;               
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);
}
/*
    bind the socket to the network connection with the specified socket address struct  
*/
void bindNetwork () {
    int bindResult = bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    
    if (bindResult < 0) {
        perror("socket was not binded");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }      
}
/*
    set the socket's property to listen to maximum number of connections at any given time specified
    by MAX_CON
*/
void setListen () {
    int listenResult = listen(serverSock, MAX_CON);
    
    if (listenResult < 0) {
        perror("socket failed to listen");
        closeSocket(serverSock);
        exit(EXIT_FAILURE);
    }      
} 
/*
    use polling to concurrently listen for requests sent from clients
*/
void listenRequest() {

    do {
        printf("\t\tWaiting on polling - Timeout: %i min(s)\n", timeout/60000);

        // wait for a file descriptor to be ready for I/O operation
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
        
        // check if there is data to be read from stored file descriptors and handle it
        for (i = 0; i < current_size; i++) {

            if (fds[i].revents == 0) continue;

            // check if the kernel identified the file descriptor ready for reading data 
            if (fds[i].revents != POLLIN) {
                perror("No pollin");
                close_server = 1;
                break;
            }

            // if it is server descriptor, then get client descriptor and store it for polling
            // else, handle client's request
            if (fds[i].fd == serverSock) {
                do {
                    
                    clientAddrLen = sizeof(clientAddr);

                    // get client descriptor from clients requesting to server socket
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
                printf("\t\tHandling client @ %s\n", inet_ntoa(clientAddr[i-1].sin_addr));

                handleClient(i);
            }
        }

        // shift the stored file descriptors where all active descriptors are placed left-most
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

    // close all stored file descriptors (server plus client(s))
    for (i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0)  closeSocket(fds[i].fd);
    }
}
/*
    receive and send messages from and to clients, concurrently
*/
void handleClient(int index) {
    char requestBuffer[REQUEST_BUFFER_SIZE];       
    int requestMsgSize;

    do {
        close_client = 0;
        // clear dank memory
        memset(requestBuffer, 0, REQUEST_BUFFER_SIZE);

        // retreive data from the file desriptor
        requestMsgSize = recv(fds[index].fd, requestBuffer, REQUEST_BUFFER_SIZE - 1, 0);

        if (requestMsgSize < 0){
            if (errno != EWOULDBLOCK) {
                perror("no msg found");
                close_client = 1;
            }
            break;
        } else if (requestMsgSize == 0){
            printf("  Connection closed by client\n");
            close_client = 1;
            break;
        }

        requestBuffer[requestMsgSize] = '\0';
        printf("Received: |%s|\nSize: %i\n", requestBuffer, requestMsgSize);
        
        printf("Enter Msg: ");

        fgets(requestBuffer, REQUEST_BUFFER_SIZE - 1, stdin);
        char* pch = strchr(requestBuffer, '\n');
        if (pch != NULL) *pch = '\0';

        // send message to client
        if (send(fds[index].fd, requestBuffer, strlen(requestBuffer), 0) < 0) {
            perror("msg failed to send");
            close_client = 1;
            break;
        }

    } while(1);  

    // close current client connection
    if (close_client) {
        close(fds[index].fd);
        fds[index].fd = -1;
        shift_pollfds = 1;
    }
}