#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#define RCVBUFSIZE 32
#define NAME_SIZE 21
#define MAX_FILENAME_LENGTH 256
#define MAX_LINE_LENGTH 1024

struct menu {
    unsigned char line1[20];
    unsigned char line2[20];
    unsigned char line3[20];
} men;

void DieWithError(char *errorMessage);
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
unsigned int sendMenuAndWaitForResponse(int);
char *askForFileName(int);
void handleGetRequest(int);
void handleDirectoryRequest(int);
void HandleTCPClient(int);

int main(int argc, char *argv[]) {
    int servSock;
    int clntSock;
    unsigned short servPort;
    pid_t processID;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    servPort = atoi(argv[1]);

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        DieWithError("socket() failed");
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        DieWithError("bind() failed");
    }

    if (listen(servSock, 5) < 0) {
        DieWithError("listen() failed");
    }

    for (;;) {
        struct sockaddr_in clntAddr;
        unsigned int clntLen = sizeof(clntAddr);

        if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntLen)) < 0) {
            DieWithError("accept() failed");
        }

        if ((processID = fork()) < 0) {
            DieWithError("fork() failed");
        } else if (processID == 0) {
            close(servSock);
            HandleTCPClient(clntSock);
            exit(0);
        }

        close(clntSock);
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    return 0;
}


void DieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(1);
}


void get(int sock, void *buffer, unsigned int length)
{
    unsigned char *buf = (unsigned char*) buffer;

    int totalBytesRcvd = 0; /* Total bytes received so far */
    int bytesRcvd = 0; /* Bytes received in last recv() call */
    while (totalBytesRcvd < length)
    {
        /* Receive up to the buffer size bytes from the sender */
        bytesRcvd = recv(sock, buf + totalBytesRcvd, length - totalBytesRcvd, 0);
        if (bytesRcvd <= 0)
        {
            DieWithError("recv() failed or connection closed prematurely");
        }
        totalBytesRcvd += bytesRcvd; /* Keep tally of total bytes */
    }
}


void put(int sock, void *buf, unsigned int bufSize) {
    unsigned int numBytes = send(sock, buf, bufSize, 0);
    if (numBytes < 0)
        DieWithError("send() failed");
    else if (numBytes != bufSize)
        DieWithError("sent unexpected number of bytes");
}


unsigned int sendMenuAndWaitForResponse(int clntSocket)
{
    struct menu mainMenu;
    unsigned int response = 0;
    memset(&mainMenu, 0, sizeof(struct menu)); /* Zero out structure */
    strcpy(mainMenu.line1,"1) Get directory listing\n");
    strcpy(mainMenu.line2, "2) Select a file\n");
    strcpy(mainMenu.line3, "3) Quit\n");
    printf("Sending menu\n");
    put(clntSocket, &mainMenu, sizeof(struct menu));
    get(clntSocket, &response, sizeof(unsigned int));
    return ntohl(response);
}

char *askForFileName(int socket) {
    char *filename = malloc(MAX_FILENAME_LENGTH);
    if (!filename) {
        DieWithError("Error allocating memory");
    }
    char buffer[MAX_LINE_LENGTH];
    int recvMsgSize;

    while (1) {
        printf("Enter the name of the file to retrieve: ");
        fflush(stdout);

        if (fgets(buffer, MAX_LINE_LENGTH, stdin) == NULL) {
            printf("\n");
            continue;
        }

        int len = strlen(buffer);
        if (buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }

        if (strcmp(buffer, "quit") == 0) {
            free(filename);
            return NULL;
        }

        if (strlen(buffer) == 0) {
            continue;
        }

        if (send(socket, buffer, strlen(buffer), 0) < 0) {
            DieWithError("send() failed");
        }

        if ((recvMsgSize = recv(socket, filename, MAX_FILENAME_LENGTH - 1, 0)) < 0) {
            DieWithError("recv() failed");
        }

        filename[recvMsgSize] = '\0';

        if (strcmp(filename, "not_found") == 0) {
            printf("File not found.\n");
            continue;
        }

        break;
    }

    return filename;
}


void handleGetRequest(int clntSocket) {
    // Ask for the file name
    char* fileName = askForFileName(clntSocket);

    // Open the file
    FILE* fp = fopen(fileName, "rb");
    if (fp == NULL) {
        // File not found
        DieWithError("File not found");
    }

    // Send the file contents
    int fileSize = sendFile(fp, clntSocket);

    // Log the successful transfer
    printf("File '%s' (%d bytes) sent successfully\n", fileName, fileSize);

    // Free memory and close file pointer
    free(fileName);
    fclose(fp);
}


void handleDirectoryRequest(int sock) {
    DIR *dir;
    struct dirent *entry;
    char *dir_name = ".";
    int num_entries = 0;
    char buffer[MAX_FILENAME_LENGTH];
    int buffer_len = 0;

    if ((dir = opendir(dir_name)) == NULL) {
        DieWithError("opendir() failed");
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            num_entries++;
            buffer_len += snprintf(buffer + buffer_len, MAX_FILENAME_LENGTH - buffer_len, "%s\n", entry->d_name);
        }
    }

    closedir(dir);

    if (num_entries == 0) {
        strcpy(buffer, "Directory is empty.\n");
    }

    put(sock, buffer, strlen(buffer));
}



void handleTCPClient(int clntSocket)
{
    char menu[] = "\nPlease choose an option:\n1. Get directory listing\n2. Select a file\n3. Quit\n";
    char recvBuffer[RCVBUFSIZE];
    int recvMsgSize;
    int option = 0;
    int quitFlag = 0;

    while (!quitFlag) {
        // Send menu to client
        sendMenuAndWaitForResponse(clntSocket, menu, &option);

        switch (option) {
        case 1:
            handleDirectoryRequest(clntSocket);
            break;
        case 2:
            handleGetRequest(clntSocket);
            break;
        case 3:
            quitFlag = 1;
            break;
        default:
            DieWithError("Invalid option selected.");
            break;
        }
    }

    close(clntSocket); // Close client socket
}