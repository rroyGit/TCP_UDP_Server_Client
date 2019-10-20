# TCP_Server_Client
### Compile server.c  
    gcc server.c -o server
### Compile client.c  
    gcc client.c -o client

### Run server
    ./server <Port Number>
##### Example
    ./server 3000

### Run client
    ./client <Physical IP> <Port Number>
##### Example
    ./client 192.168.123.123 3000  

Polling and non-blocking mechanisms are used to provide concurrent communication between multiple clients and a single server.

The server has a timeout set to 1 minute. If a client is not connected to the server for more than 1 minute, the server will shutdown automatically. The client can be shutdown by entering "EXIT" when user is asked for input message.

Received messages are displayed between two pipe characters (e.g. |my_message|); this allows easy readability and error detectability.

### Sources/Credits:
    Polling:        https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm  
    Socket API:     https://linux.die.net/
    
# UDP_Server_Client
### Compile server.c  
    gcc server.c -o server
### Compile client.c  
    gcc client.c -o client

### Run server
    ./server <Port Number>
##### Example
    ./server 3000

### Run client
    ./client <Physical IP> <Port Number> <File Name>
##### Example
    ./client 192.168.123.123 3000  
