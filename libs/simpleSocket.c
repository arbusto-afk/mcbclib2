#include "simpleSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to initialize Winsock
int simpleSocket_initialize() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
    return 0;
}

// Function to create and connect a socket to the specified hostname and port
int simpleSocket_setSocket(const char* hostname, int port) {
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // Use AF_UNSPEC to allow for both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP

    char portStr[6];
    snprintf(portStr, sizeof(portStr), "%d", port);

    // Improved error handling with gai_strerror
    int status = getaddrinfo(hostname, portStr, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    // Create a socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "socket creation failed: %d\n", WSAGetLastError());
        freeaddrinfo(res);
        return -1;
    }

    // Connect to the server
    if (connect(sockfd, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        fprintf(stderr, "connection failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

// Function to receive data from the specified socket into the provided buffer
int simpleSocket_receiveSocket(int sockfd, unsigned char* buffer, int bufdim) {
    int bytesReceived = recv(sockfd, buffer, bufdim, 0);
    if (bytesReceived == SOCKET_ERROR) {
        fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
        return -1;
    }
    return bytesReceived;
}

// Function to send data through the specified socket
int simpleSocket_send(int sockfd, const uint8_t * data, int datalen) {
    int bytesSent = send(sockfd, data, datalen, 0);
    if (bytesSent == SOCKET_ERROR) {
        fprintf(stderr, "send failed: %d\n", WSAGetLastError());
        return -1;
    }
    printf("Sent %d bytes <", bytesSent);
    for(int i = 0; i < datalen; i++)
        printf("%02x ", data[i]);
    printf(">\n");
    return bytesSent;
}

// Function to close the socket
void simpleSocket_close(int sockfd) {
    closesocket(sockfd);
}

// Function to clean up Winsock
void simpleSocket_cleanup() {
    WSACleanup();
}
