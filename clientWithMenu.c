// Required header files
// Our team received help from Micah Parilla
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Constants
#define RCVBUFSIZE 100
#define NAME_SIZE 1024

// Struct for transmitting data (x, y, operation)
typedef struct {
  unsigned int x;
  unsigned int y;
  unsigned char oper;
} TRANS_DATA_TYPE;

// Struct for storing data (x, y)
typedef struct {
  unsigned int x;
  unsigned int y;
} DATA_TYPE;

// Struct for the menu options
struct menu {
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
};

// Function prototypes
void DieWithError(char *errorMessage);
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
void getLsdir(int sock);
void talkToServer(int);
unsigned int displayMenuAndSendSelection(int);
void sendName(int);
void getFile(int sock);
// void sendNumber(int);

int main(int argc, char *argv[]) {
  int sock;                        // descriptor for the socket
  struct sockaddr_in echoServAddr; // echoes server address
  unsigned short echoServPort;     // echoes the server port
  char *servIP;                    // server's ip address
  char *echoString;                // string that's sent to server
  unsigned int echoStringLen;      // length of string to echo
  int bytesRcvd, totalBytesRcvd; // bytes read in single recv() and total bytes
  int answer;

  // Data structures for incoming and outgoing data
  DATA_TYPE data;
  TRANS_DATA_TYPE incoming;
  memset(&incoming, 0, sizeof(TRANS_DATA_TYPE));

  // Check if the correct number of arguments are provided
  if ((argc < 2) || (argc > 3)) {
    fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
    exit(1);
  }

  // Set server IP and port
  servIP = argv[1];

  if (argc == 3)
    echoServPort = atoi(argv[2]);
  else
    echoServPort = 7;

  // Create a socket for connecting to the server
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError("socket() failed");

  // Set the server address and port
  memset(&echoServAddr, 0, sizeof(echoServAddr));
  echoServAddr.sin_family = AF_INET;
  echoServAddr.sin_addr.s_addr = inet_addr(servIP);
  echoServAddr.sin_port = htons(echoServPort);

  // Connect to the server
  if (connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
    DieWithError("connect() failed");

  // Start communication with the server
  talkToServer(sock);

  // Close the socket and exit the program
  close(sock);
  exit(0);
}
// Function for interacting with the server
void talkToServer(int sock) {
  unsigned int selection = 0;
  unsigned char bye[5];

  // Loop until the user selects option 3 (Quit)
  while (1) {
    // Display menu and send the user's selection to the server
    selection = displayMenuAndSendSelection(sock);
    printf("Client selected: %d\n", selection);

    // Perform actions based on the user's selection
    switch (selection) {
    case 1:
      sendName(sock);
      getFile(sock);
      break;
    case 2:
      printf("ask for lsdir\n");
      // sendNumber(sock);
      getLsdir(sock);
      break;
    }
    // Break the loop if the user selected option 3 (Quit)
    if (selection == 3)
      break;
  }
  // Send the final selection to the server and receive the 'bye' message
  selection = htonl(selection);
  put(sock, &selection, sizeof(unsigned int));
  get(sock, bye, 5);
  printf("%s\n", bye);
}

// Function to display the menu and send the user's selection to the server
unsigned int displayMenuAndSendSelection(int sock) {
  struct menu menuBuffer;
  unsigned int response = 0;
  unsigned int output;
  char junk;

  // Receive the menu options from the server and display them
  get(sock, &menuBuffer, sizeof(struct menu));
  printf("%s\n", menuBuffer.line1);
  printf("%s\n", menuBuffer.line2);
  printf("%s\n", menuBuffer.line3);

  // Get the user's response and send it to the server
  scanf("%d", &response);
  getc(stdin);
  output = htonl(response);
  put(sock, &output, sizeof(unsigned int));

  return response;
}

void getLsdir(int sock) {
  printf("inside LSDIR\n");
  unsigned char buffer[5000];
  memset(buffer, 0, sizeof(buffer));
  get(sock, buffer, sizeof(buffer));
  printf("%s\n", buffer);
}

// Function to send the user's name to the server
void sendName(int sock) {
  unsigned char msg[21];
  unsigned char name[NAME_SIZE];

  // Receive the prompt from the server and display it
  memset(msg, 0, sizeof(msg));
  get(sock, msg, sizeof(msg));
  printf("%s\n", msg);

  // Get the user's name and send it to the server
  memset(name, 0, NAME_SIZE);
  fgets(name, NAME_SIZE, stdin);
  if (name[strlen(name) - 1] == '\n') {
    name[strlen(name) - 1] = '\0';
  }
  put(sock, name, NAME_SIZE);
}

// Function to send the user's number to the server
/*void sendNumber(int sock)
{
    unsigned char msg[21];
    int number;

    // Receive the prompt from the server and display it
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);

    // Get the user's number and send it to the server
    scanf("%d", &number);
    number = htonl(number);
    put(sock, &number, sizeof(int));
} */

void get(int sock, void *buffer, unsigned int bufferSize) {
  int totalBytesReceived = 0;
  int bytesReceived = 0;

  while (totalBytesReceived < bufferSize) {
    bytesReceived = recv(sock, buffer + totalBytesReceived,
                         bufferSize - totalBytesReceived, 0);
    if (bytesReceived < 0) {
      DieWithError("recv() failed");
    } else if (bytesReceived == 0) {
      DieWithError("Connection closed prematurely");
    }
    totalBytesReceived += bytesReceived;
  }
}

void put(int sock, void *buffer, unsigned int bufferSize) {
  int totalBytesSent = 0;
  int bytesSent = 0;

  while (totalBytesSent < bufferSize) {
    bytesSent =
        send(sock, buffer + totalBytesSent, bufferSize - totalBytesSent, 0);
    if (bytesSent < 0) {
      DieWithError("Send() failed");
    }
    totalBytesSent += bytesSent;
  }
}

void getFile(int sock) {
  long fileSize;
  get(sock, &fileSize, sizeof(long));
  // convert to host order
  fileSize = ntohl(fileSize);
  printf("Size of fileL %ld\n", fileSize);
  char *buffer = (char *)malloc(sizeof(char) * fileSize);
  get(sock, buffer, fileSize);
  printf("%s\n", buffer);
}
