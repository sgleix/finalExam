void HandleTCPClient(int clntSocket)
{
    int recvMsgSize; /* Size of received message */
    unsigned int response = 0;
    unsigned char menuMsg[] = "Menu\n1) Get directory listing\n2) Select file\n3) Quit\n";
    unsigned char fileNotFoundMsg[] = "File not found\n";
    unsigned char goodbyeMsg[] = "Goodbye\n";
    unsigned char buffer[RCVBUFSIZE]; /* Buffer for file transfer */
    FILE *filePtr; /* Pointer to file */
    pid_t pid;
    int status;

    /* Spawn a new process to handle the client */
    if ((pid = fork()) < 0) {
        DieWithError("fork() failed");
    } else if (pid == 0) { /* Child process */
        close(servSock); /* Close the listening socket */
        response = sendAndWaitForResponse(clntSocket, menuMsg, sizeof(menuMsg));
        while (response != 3) {
            switch(response) {
                case 1: {
                    printf("Client selected 1: Get directory listing\n");
                    /* TODO: Implement code to get directory listing */
                    break;
                }
                case 2: {
                    printf("Client selected 2: Select file\n");
                    /* TODO: Implement code to select file */
                    break;
                }
                default: {
                    printf("Client selected an invalid option\n");
                    put(clntSocket, "Invalid selection\n", sizeof("Invalid selection\n"));
                    break;
                }
            }
            response = sendAndWaitForResponse(clntSocket, menuMsg, sizeof(menuMsg));
        }
        /* Client has chosen to quit */
        put(clntSocket, goodbyeMsg, sizeof(goodbyeMsg));
        close(clntSocket);
        exit(0);
    } else { /* Parent process */
        /* Wait for child process to complete before returning */
        while (waitpid(-1, &status, WNOHANG) != pid) {
            continue;
        }
        close(clntSocket);
        printf("Connection with client %d closed.\n", clntSocket);
    }
}
