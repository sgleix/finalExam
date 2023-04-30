#include <stdio.h>      
#include <sys/socket.h> 
#include <unistd.h>     
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#define RCVBUFSIZE 32   
#define NAME_SIZE 21 

// Define a struct for the menu
struct menu{
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
} men;

void DieWithError(char *errorMessage); 
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
unsigned int sendMenuAndWaitForResponse(int);
void askForName(int sock, char *, unsigned int);
void doSomethingWithName(char *);
void askForNumber(int sock, int *, unsigned int);
void doSomethingWithNumber(int);

// Main function to handle the client's requests
void HandleTCPClient(int clntSocket)
{
    int recvMsgSize;                   
    unsigned int response = 0;
    unsigned char name[NAME_SIZE]; 
    int number = 0;
    unsigned char errorMsg[] = "Invalid Choice";
    unsigned char bye[] = "Bye!";

    // Send the menu to the client and wait for their response
    response = sendMenuAndWaitForResponse(clntSocket);

    // Continue handling requests until the client chooses to exit (option 3)
    while(response != 3)
    {
        switch(response)
        {
            case 1: // Client selected option 1 (name)
                printf("Client selected 1.\n");
                // Ask the client for their name and store it in the 'name' variable
                askForName(clntSocket, name, NAME_SIZE);
                // Process the received name
                doSomethingWithName(name);
                break;
            case 2: // Client selected option 2 (number)
                printf("Client selected 2.\n");
                // Ask the client for a number and store it in the 'number' variable
                askForNumber(clntSocket, &number, sizeof(int));
                // Process the received number
                doSomethingWithNumber(number);
                break;
            default: // Client selected an invalid option
                printf("Client selected junk.\n");
                // Send an error message to the client
                put(clntSocket, errorMsg, sizeof(errorMsg));
                break;
        }
        // Send the menu to the client again and wait for their response
        response = sendMenuAndWaitForResponse(clntSocket);
    }//end while

    // Send a goodbye message to the client and close the connection
    put(clntSocket, bye, sizeof(bye));
    close(clntSocket);    
    printf("Connection with client %d closed.\n", clntSocket);
}

// Function to send a menu to the client and wait for their response
unsigned int sendMenuAndWaitForResponse(int clntSocket)
{
    struct menu mainMenu;
    unsigned int response = 0;
    // Initialize the mainMenu struct with the menu options
    memset(&mainMenu, 0, sizeof(struct menu));  
    strcpy(mainMenu.line1,"1) Enter name\n");
    strcpy(mainMenu.line2, "2) Enter number\n");
    strcpy(mainMenu.line3, "3) Quit\n");
    // Send the menu to the client
    printf("Sending menu\n");
    put(clntSocket, &mainMenu, sizeof(struct menu));
    // Receive the client's response
    get(clntSocket, &response, sizeof(unsigned int));
    // Convert the response from network byte order to host byte order and return it
    return ntohl(response);
}

// Function to ask the client for their name and store it in the provided buffer
void askForName(int sock, char * name, unsigned int size)
{
    unsigned char msg[21];
    // Initialize the message buffer and set the message content
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter name:\n");
    // Send the message to the client
    put(sock, msg, sizeof(msg));
    // Receive the client's name and store it in the provided buffer
    memset(name, 0, NAME_SIZE);
    get(sock, name, NAME_SIZE);
}

// Function to process the received name
void doSomethingWithName(char * name)
{
    printf("Received name from the client: %s\n", name);
}

// Function to ask the client for a number and store it in the provided integer pointer
void askForNumber(int sock, int * numPtr, unsigned int size)
{
    unsigned char msg[21];
    int numIn = 0;

    // Initialize the message buffer and set the message content
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter number:\n");
    // Send the message to the client
    put(sock, msg, sizeof(msg));
    // Receive the client's number
    get(sock, &numIn, sizeof(int));
    // Convert the received number from network byte order to host byte order and store it in the provided integer pointer
    *numPtr = ntohl(numIn);
}

// Function to process the received number
void doSomethingWithNumber(int number)
{
    printf("Received number from the client: %d\n", number);
}

