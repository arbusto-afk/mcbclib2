#ifndef SIMPLESOCKET_H
#define SIMPLESOCKET_H

#include <winsock2.h>  // Include Winsock 2 for Windows socket programming
#include <ws2tcpip.h>  // Include additional Winsock definitions
#include <stdint.h>
#pragma comment(lib, "Ws2_32.lib")  // Link against the Winsock library (Ws2_32.lib)

// Function to initialize Winsock
int simpleSocket_initialize();

// Function to create and connect a socket to the specified hostname and port
// Returns a socket file descriptor on success, or -1 on failure
int simpleSocket_setSocket(const char* hostname, int port);

// Function to receive data from the specified socket into the provided buffer
// Returns the number of bytes received, or -1 on error
int simpleSocket_receiveSocket(int sockfd, unsigned char* buffer, int bufdim);

// Function to send data through the specified socket
// Returns the number of bytes sent, or -1 on error
int simpleSocket_send(int sockfd, const uint8_t* data, int datalen);

// Function to close the socket
void simpleSocket_close(int sockfd);

// Function to clean up Winsock
void simpleSocket_cleanup();

#endif // SIMPLESOCKET_H
#ifndef SIMPLESOCKET_H
#define SIMPLESOCKET_H

#include <winsock2.h>  // Include Winsock 2 for Windows socket programming
#include <ws2tcpip.h>  // Include additional Winsock definitions
#pragma comment(lib, "Ws2_32.lib")  // Link against the Winsock library (Ws2_32.lib)

// Function to initialize Winsock
int simpleSocket_initialize();

// Function to create and connect a socket to the specified hostname and port
// Returns a socket file descriptor on success, or -1 on failure
int simpleSocket_setSocket(const char* hostname, int port);

// Function to receive data from the specified socket into the provided buffer
// Returns the number of bytes received, or -1 on error
int simpleSocket_receiveSocket(int sockfd, char* buffer, int bufdim);

// Function to send data through the specified socket
// Returns the number of bytes sent, or -1 on error
int simpleSocket_send(int sockfd, const unsigned char* data, int datalen);

// Function to close the socket
void simpleSocket_close(int sockfd);

// Function to clean up Winsock
void simpleSocket_cleanup();

#endif // SIMPLESOCKET_H
