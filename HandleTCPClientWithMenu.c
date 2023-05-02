#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys.types.h>35
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

#define RCVBUFSIZE 32
#define NAME_SIZE 32
#define MAX_FILENAME_LENGTH 256
#define MAX_LINE_LENGTH 1024

struct stat mystat, *sp;
char * t1 ="xwrxwrxwr-------";
char * t2 ="----------------";

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
long findSize(FILE * fp);
void sendFileToClient(char * filename, int sock);
//void handleGetRequest(int);
//void handleDirectoryRequest(int);
void HandleTCPClient(int);
void ls_dir2(char * dname, char * buffer);


//NASTY MF!
/*int main(int argc, char *argv[]) {
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
} */


void DieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(1);
}


void get(int sock, void *buffer, unsigned int bufferSize)
{
    int totalBytesReceived = 0;
    int bytesReceived = 0;

    while (totalBytesReceived < bufferSize) {
	    bytesReceived = recv(sock, buffer + totalBytesReceived, bufferSize - totalBytesReceived, 0);
	    if (bytesReceived < 0) 
	    {
		DieWithError("recv() failed");
	    } else if (bytesReceived == 0) {
		    DieWithError("Connection closed prematurely");
	    }
	    totalBytesreceived += bytesReceived;
    }
}


void put(int sock, void *buffer, unsigned int bufferSize) {
    int toatlBytesSent = 0;
    int bytesSent = 0;

    while (totalBytesSent < bufferSize) {
	    bytesSent = send(sock, buffer + totalBytesSent, bufferSize - totalBytesSent, 0);
	    if (bytesSent < 0) {
		    DieWithError("send() failed\n");
	    }
	    toatlBytesSent += bytesSent;
    }
}

void ls_dir2(char * dname, char * dst) {
	DIR *dp;
	struct dirent *dirp;
	dp = opendir(dname);
	while((dirp = readdir(dp)) != null) {
		strcat(dst, dirp->dname);
		strcat(dst, "\n");
	}
	closedir(dp);
}

void sendFileToClient(char * filename, int sock) 
{
	long fileSize;
	FILE* fp = fopen(filename, "r");
	if (fp == NULL) {
		DieWithError("File not found");
	}
	fileSize = findSize(fp);
	int size = htonl(fileSize);
	put(sock, &size, sizeof(fileSize));
	char * buffer = (char *)malloc(sizeof(char) * fileSize);
	fread(buffer, sizeof(char), fileSize, fp);
	printf("%s\n", buffer);
	put(sock, buffer, fileSize);
	fclose(fp);
}


unsigned int sendMenuAndWaitForResponse(int clntSocket)
{
    struct menu mainMenu;
    unsigned int response = 0;
    memset(&mainMenu, 0, sizeof(struct menu)); /* Zero out structure */
    strcpy(mainMenu.line1,"1) Select a file\n");
    strcpy(mainMenu.line2, "2) list available files\n");
    strcpy(mainMenu.line3, "3) Quit\n");
    printf("Sending menu\n");
    put(clntSocket, &mainMenu, sizeof(struct menu));
    get(clntSocket, &response, sizeof(unsigned int));
    return ntohl(response);
}

void askForFileName(int socket)
{
   unsigned char msg[80];
   memset(msg, 0, sizeof(msg));
   strcpy(msg, "Enter file name from list of directories:\n");
   put(sock, msg, sizeof(msg));
   memset(name, 0, NAME_SIZE);
   get(sock, name, NAME_SIZE);
   printf("Received file name from the client: %s\n", name);
}


/*void handleGetRequest(int clntSocket) {
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
} */

//girl what
/*void handleDirectoryRequest(int sock) {
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
} */



void handleTCPClient(int clntSocket)
{
    char menu[] = "\nPlease choose an option:\n1. Get directory listing\n2. Select a file\n3. Quit\n";
    char recvBuffer[RCVBUFSIZE];
    int recvMsgSize;
    unsigned char name[NAME_SIZE]; // max length 
    unsigned int option = 0;
    int quitFlag = 0;
    unsigned char errorMsg[] = "Invalid Choice!";
    unsigned char bye{} = "Bye!";

    while (!quitFlag) {
        // Send menu to client
        sendMenuAndWaitForResponse(clntSocket, menu, &option);

        switch (option) {
        case 1:
	    printf("Client chose 1\n");
            //handleDirectoryRequest(clntSocket);
	    askForFileName(clntSocket,name, sizeof(name));
	    sendFiletoClient(name, clntSocket);
            break;
        case 2:
	    printf("Client chose 2\n");
            //handleGetRequest(clntSocket);
	    char dirbuffer[5000];
	    char cwd[1024];
	    getcwd(cwd, sizeof(cwd));
	    memset(dirbuffer, 0, sizeof(dirbuffer));
            break;
        case 3:
            quitFlag = 1;
            break;
        default:
            DieWithError("Invalid option selected.");
	    //NOT supposed to do this!!
            break;
        }
	option = sendMenuAndWaitForResponse(clntSocket);
    } //ends the while loop

    put(clntSocket, bye, sizeof(bye));
    close(clntSocket);
    printf("Connection with client %d closed.\n", clntSocket);

    close(clntSocket); // Close client socket
}

