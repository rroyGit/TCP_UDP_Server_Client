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

#define DATA_SIZE 5

typedef struct packet {
    char name[30];
    char data[DATA_SIZE];
} Packet;

typedef struct frameReceive {
    Packet data;
    int seqNum;
} Frame;

typedef struct frameSend {
    int seqNum;
    int ack;
} ACKFrame;

// init descriptors to default
int serverSock = -1;                   
int clientSock = -1;    

// server address struct
struct sockaddr_in serverAddr;
// client address struct
struct sockaddr_in clientAddr;
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
int processFile (char* fileName, char* dataBuffer);

int main (int argc, char** argv) {

    if (argc != 2) {
       fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
       exit(1);
    }

    setSocket();

    fillServerAddr(argv);

    bindNetwork();

    listenRequest();

    return EXIT_SUCCESS;
}
/*
    create a TCP socket with guranteed packet delivery service via sock stream
*/
void setSocket () {

    serverSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (serverSock < 0) {
        perror("socket was not created");
        exit(EXIT_FAILURE);
    }
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
    listen for requests sent from clients
*/
void listenRequest() {
    int recvSize = -1, sendSize = -1, skipBytes = 0;
    Frame recvFrame;
    ACKFrame sendFrame;

    clientAddrLen = sizeof(clientAddr);

    while(1) {
       
        recvSize = recvfrom(serverSock, &recvFrame, sizeof(Frame), 0, (struct sockaddr *) &clientAddr, &clientAddrLen);

        Packet recvPacket;
        memcpy(&recvPacket, &recvFrame.data, sizeof(Packet) + 1);
 
        processFile(recvPacket.name, recvPacket.data);

        if (recvSize > 0) {
            sendFrame.ack = 1;
            sendFrame.seqNum = recvFrame.seqNum;
            
            sendSize = sendto(serverSock, &sendFrame, sizeof(ACKFrame), 0, (struct sockaddr *) &clientAddr, clientAddrLen);

            if (sendSize < 0) {
                printf("[Server] Sending error\n");
                exit(EXIT_FAILURE);
            }
        } else if (recvSize < 0) {
            printf("[Server] Receiving error\n");
            exit(EXIT_FAILURE);
        } else {
            printf("[Server] Received 0 bytes\n");
            exit(EXIT_FAILURE);
        }
        
    }
}

int processFile (char* fileName, char* dataBuffer) {
    char name[30] = "new_";
    strcat(name, fileName);
    
    FILE* file = fopen(name, "a");
    if (file == NULL) {
        perror("File not found");
        exit(EXIT_FAILURE);
    }

    
    int bytesRead = fwrite(dataBuffer, sizeof(char), strlen(dataBuffer), file);

    fclose(file);
    return bytesRead;
}