#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define RCVBUFSIZE 32
#define NAME_SIZE 32
#define MAX_FILENAME_LENGTH 256
#define MAX_LINE_LENGTH 1024

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

struct menu {
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
} men;

void DieWithError(char *errorMessage);
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
unsigned int sendMenuAndWaitForResponse(int);
void askForFileName(int sock, char *, unsigned int);
long findSize(FILE *fp);
void sendFileToClient(char *filename, int sock);
// void handleGetRequest(int);
// void handleDirectoryRequest(int);
void HandleTCPClient(int);
void ls_dir2(char *dname, char *buffer);

long findSize(FILE *fp) {
  fseek(fp, 0, SEEK_END);

  long res = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  return res;
}

void ls_dir2(char *dname, char *dst) {
  DIR *dp;
  struct dirent *dirp;
  dp = opendir(dname);
  while ((dirp = readdir(dp)) != NULL) {
    strcat(dst, dirp->d_name);
    strcat(dst, "\n");
  }
  closedir(dp);
}

void sendFileToClient(char *filename, int sock) {
  long fileSize;
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    DieWithError("File not found");
  }
  fileSize = findSize(fp);
  int size = htonl(fileSize);
  put(sock, &size, sizeof(fileSize));
  char *buffer = (char *)malloc(sizeof(char) * fileSize);
  fread(buffer, sizeof(char), fileSize, fp);
  printf("%s\n", buffer);
  put(sock, buffer, fileSize);
  fclose(fp);
}

unsigned int sendMenuAndWaitForResponse(int clntSocket) {
  struct menu mainMenu;
  unsigned int response = 0;
  memset(&mainMenu, 0, sizeof(struct menu)); /* Zero out structure */
  strcpy(mainMenu.line1, "1) Select a file\n");
  strcpy(mainMenu.line2, "2) list available files\n");
  strcpy(mainMenu.line3, "3) Quit\n");
  printf("Sending menu\n");
  put(clntSocket, &mainMenu, sizeof(struct menu));
  get(clntSocket, &response, sizeof(unsigned int));
  return ntohl(response);
}

void askForFileName(int socket, char *name, unsigned thunk) {
  unsigned char msg[80];
  memset(msg, 0, sizeof(msg));
  strcpy(msg, "Enter file name from list of directories:\n");
  put(socket, msg, sizeof(msg));
  memset(name, 0, NAME_SIZE);
  get(socket, name, NAME_SIZE);
  printf("Received file name from the client: %s\n", name);
}

void HandleTCPClient(int clntSocket) {
  char menu[] = "\nPlease choose an option:\n1. Get directory listing\n2. "
                "Select a file\n3. Quit\n";
  char recvBuffer[RCVBUFSIZE];
  int recvMsgSize;
  unsigned char name[NAME_SIZE]; // max length
  unsigned int option = 0;
  int quitFlag = 0;
  unsigned char errorMsg[] = "Invalid Choice!";
  unsigned char bye[] = "Bye!";

  option = sendMenuAndWaitForResponse(clntSocket);

  while (option != 3) {
    // Send menu to client
    option = sendMenuAndWaitForResponse(clntSocket);
    switch (option) {
    case 1:
      printf("Client chose 1\n");
      // handleDirectoryRequest(clntSocket);
      askForFileName(clntSocket, (char *)name, sizeof(name));
      sendFileToClient((char *)name, clntSocket);
      break;
    case 2:
      printf("Client chose 2\n");
      // handleGetRequest(clntSocket);
      char dirbuffer[5000];
      char cwd[1024];
      getcwd(cwd, sizeof(cwd));
      memset(dirbuffer, 0, sizeof(dirbuffer));
      break;
    default:
      printf("Client had wrong input");
      put(clntSocket, errorMsg, sizeof(errorMsg));
      break;
    }
    option = sendMenuAndWaitForResponse(clntSocket);
  } // ends the while loop

  put(clntSocket, bye, sizeof(bye));
  close(clntSocket);
  printf("Connection with client %d closed.\n", clntSocket);

  close(clntSocket); // Close client socket
}
#include <unistd.h>
// Function to receive data from a socket
void get(int sock, void *buffer, unsigned int bufferSize) {
  int totalBytesReceived = 0; // Total number of bytes received
  int bytesReceived = 0;      // Number of bytes received in each iteration

  // Loop until all expected bytes are received
  while (totalBytesReceived < bufferSize) {
    // Receive data from the socket
    bytesReceived = recv(sock, buffer + totalBytesReceived,
                         bufferSize - totalBytesReceived, 0);

    // Check for errors in the received data
    if (bytesReceived < 0)
      DieWithError("recv() failed");
    else if (bytesReceived == 0)
      DieWithError("Connection closed prematurely");

    // Update the total number of bytes received
    totalBytesReceived += bytesReceived;
  }
}

// Function to send data to a socket
void put(int sock, void *buffer, unsigned int bufferSize) {
  int totalBytesSent = 0; // Total number of bytes sent
  int bytesSent = 0;      // Number of bytes sent in each iteration

  // Loop until all bytes are sent
  while (totalBytesSent < bufferSize) {
    // Send data to the socket
    bytesSent =
        send(sock, buffer + totalBytesSent, bufferSize - totalBytesSent, 0);

    // Check for errors in sending data
    if (bytesSent < 0)
      DieWithError("send() failed");

    // Update the total number of bytes sent
    totalBytesSent += bytesSent;
  }
}
