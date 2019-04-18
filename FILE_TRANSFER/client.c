#include <stdio.h>
#include <sys/socket.h> /* socket(), connect(), send(), recv() */
#include <arpa/inet.h>  /* sockaddr_in and inet_addr() */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#define DATA_SIZE 5

int processFile (char* fileName, int skipBytes, char* dataBuffer);

typedef struct packet {
    char name[30];
    char data[DATA_SIZE];
} Packet;

typedef struct frameSend {
    Packet data;
    int seqNum;
} Frame;

typedef struct frameReceive {
    int seqNum;
    int ack;
} ACKFrame;


int main(int argc, char *argv[]) {
    // client file descriptor
    int clientSock;
    // server address struct
    struct sockaddr_in serverAddr;
    // client address struct
    struct sockaddr_in clientAddr;
    // server port number
    unsigned short serverPort;
    // physical network IP address
    char *serverPhyIP;
    //file name to be processed
    char *fileName;
    // buffer to store messages
    char *dataBuffer;
    // bytes received from recv(...) calls
    int bytesReceived;

    if (argc != 4) {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port> <File Name>\n", argv[0]);
       exit(1);
    }
    
    // get physical network IP and port number from command line arguments
    serverPhyIP = argv[1];
    serverPort = atoi(argv[2]);
    fileName = argv[3];

    dataBuffer = (char*) calloc(DATA_SIZE, sizeof(char));

    if (dataBuffer == NULL) {
        perror("Unable to allocated requested heap");
        exit(EXIT_FAILURE);
    }

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
    
    // create a client socket
    if ((clientSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket was not created");
        exit(EXIT_FAILURE);
    }

    // get a Packet struct filled by processing the .txt file
    int countBytes = 0, sequenceNumber = 0, readBytes, sendSize, recvSize;
    unsigned int serverAddrLen = sizeof(serverAddr);
    unsigned int clientAddrLen = sizeof(clientAddr);
    unsigned int currentAck = 1;
    Packet packet;
    Frame sendFrame;
    ACKFrame recvFrame;

    do {

        if (currentAck == 1) {
            currentAck = 0;
            readBytes = processFile(fileName, countBytes, dataBuffer);
            strcpy(packet.name, fileName);
            strcpy(packet.data, dataBuffer);
                
            
            memcpy(&sendFrame.data, &packet, sizeof(Packet) + 1);
            sendFrame.seqNum = sequenceNumber % 2;

            // send frame to server
            sendSize = sendto(clientSock, &sendFrame, sizeof(Frame), 0, (struct sockaddr *) &serverAddr, serverAddrLen);
        
            if (sendSize > 0) {
                recvSize = recvfrom(clientSock, &recvFrame, sizeof(ACKFrame), 0, (struct sockaddr *) &clientAddr, &clientAddrLen);
                if (recvSize < 0) {
                    printf("[Client] Receiving error\n");
                    exit(EXIT_FAILURE);
                }
            } else if (sendSize < 0) {
                printf("[Client] Sending error\n");
                exit(EXIT_FAILURE);
            } else {
                printf("[Client] Sent 0 bytes\n");
                exit(EXIT_FAILURE);
            }
            
            currentAck = recvFrame.ack;
            countBytes += readBytes;
            sequenceNumber = 1 + recvFrame.seqNum;
        }
    } while (readBytes == DATA_SIZE);

    printf("All data sent! Check for new file.\n");
    // close client socket
    close(clientSock);

   return EXIT_SUCCESS;
}

int processFile (char* fileName, int skipBytes, char* dataBuffer) {
    memset(dataBuffer, 0, DATA_SIZE);

    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        perror("File not found");
        exit(EXIT_FAILURE);
    }

    fseek(file, skipBytes , SEEK_SET);
    int bytesRead = fread(dataBuffer, sizeof(char), DATA_SIZE, file);

    //printf("%s", dataBuffer);

    dataBuffer[bytesRead] = '\0';
    fclose(file);
    return bytesRead;
}


