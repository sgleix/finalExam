
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
