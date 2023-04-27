#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>    
#include <unistd.h>    

// Maximum number of pending connections
#define MAXPENDING 5   

// Function prototypes
void DieWithError(char *errorMessage);  
void HandleTCPClient(int clntSocket);   

// Main function
int main(int argc, char *argv[])
{
    int servSock;                    // Server socket
    int clntSock;                    // Client socket
    struct sockaddr_in echoServAddr; // Server address
    struct sockaddr_in echoClntAddr; // Client address
    unsigned short echoServPort;     // Server port
    unsigned int clntLen;            // Length of client address data structure

    // Check if the number of arguments is correct
    if (argc != 2)     
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    // Get server port from command line argument
    echoServPort = atoi(argv[1]);  

    // Create a TCP socket
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    // Initialize server address structure
    memset(&echoServAddr, 0, sizeof(echoServAddr));   
    echoServAddr.sin_family = AF_INET;                
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    echoServAddr.sin_port = htons(echoServPort);     

    // Bind the socket to the local server address
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    // Start listening for incoming connections
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    // Run the server loop
    for (;;) /* Run forever */
    {
        // Set the size of the client address structure
        clntLen = sizeof(echoClntAddr);

        // Accept a new incoming client connection
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0)
            DieWithError("accept() failed");

        // Print the client's IP address
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        // Call the function to handle the client connection
        HandleTCPClient(clntSock);
    }
    /* NOT REACHED */
}
