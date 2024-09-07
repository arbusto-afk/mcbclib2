#include <stdio.h>
#include <stdlib.h>

#include "libs/sharedmain.h"
#include "libs/simpleSocket.h"
//#include "packetDefinitions.h"
#include "packetDefs-1_12_2.h"
#include "libs/debug/localErrno.h"
#include "libs/debug/file_io.h"
#include "clientDef.h"
#define MAXBUFDIM 1024 * 1024 //1mb buffer
#define BUFPATH "C:\\Users\\Ignacio\\CLionProjects\\mcClient2\\buf.txt"
#include <stdio.h>
#include <stdint.h>


/*
 * works only for 1.12.2
 */
int sendHandshakeAndLogin(client *c){
    packet_t handshake = packetDefinitions_createHandShakePacket(340, c->hostname, c->port, 2);
    packet_t login = packetDefinitions_createLoginPacket(c->playerName);
    simpleSocket_send(c->sockfd, handshake.seq, handshake.dim);
    simpleSocket_send(c->sockfd, login.seq, login.dim);
    c->state = CSTATE_LOGIN;
    free(handshake.seq);
    free(login.seq);
    return 0;
}
client * initilizeAndConnectClient(char * playerName, char * hostname, int port, packetHandler_t **customHandler){
    client *c = malloc(sizeof(client));
    int sockfd = simpleSocket_setSocket(hostname, port);
    if (sockfd < 0)
    {
        printf("Failed to connect to server %s from client object\n", hostname);
        return NULL;
    }

    c->compressionTreshold = -1;
    c->sockfd = sockfd;
    c->state = -1;
    c->playerName = playerName;
    c->hostname = hostname;
    c->port = port;
    c->customPacketHandlers = customHandler;
    sendHandshakeAndLogin(c);
    return c;
}



void test(uint8_t const buf, int dataLen, client * c){
    printf("Received chunk");
    return;
}
int main(void) {
//    const char * playerName = "Arbusto1298";
    const char * playerName = "Bot1";
    const char * hostname = "ignacioFerrero.aternos.me";
    int port = 57188;

    simpleSocket_initialize();


    packetHandler_t customHandler[CSTATE_TOTALCOUNT][MAXPACKETID] = {
            [CSTATE_PLAY]= { [0x20] = (packetHandler_t)test
            }
    };
    client *c = initilizeAndConnectClient(playerName, hostname, port, customHandler);
    if(c == NULL){
        fprintf(stderr, "Failed to initialize client");
        exit(-1);
    }

    uint8_t buf[MAXBUFDIM] = {0};
    int dynBufDim = 0;
    uint8_t  * p = buf;
    while(1)
    {
        //definition of simpleSocket_receive socket
        // Function to receive data from the specified socket into the provided buffer
// Returns the number of bytes received, or -1 on error
        int recvB = simpleSocket_receiveSocket(c->sockfd, p, MAXBUFDIM - dynBufDim);
        dynBufDim += recvB;
        if(recvB > 0)
        {
            /*
 * Given a buffer attempts to interpret all packets until invalid or incomplete packet
 * returns the amount of consumed bytes from reading valid packets
     */
            int procBytes = packetDefinitions_interpretBufAsMcPackets(buf, dynBufDim, c);
            if(procBytes > 0)
            {
                dynBufDim -= procBytes; //dynBufDim is the remaining of unproccessed packets in case of partial packet
                memmove(buf, buf + procBytes, dynBufDim);
                p = buf + dynBufDim;
            }

        }
        else
        {
            int k= WSAGetLastError();
            switch(k){
                case WSAECONNRESET:
                    printf("Connection closed by remote host\n");
                    break;
                case 0:
                    if(recvB == 0){
                        printf("Conection closed by remote host\n");
                        break;
                    }
                default:
                    printf("Unhandled error %d, breaking w/ recvB: %d\n", k, recvB);

            }
            break;
        }
    }

    return 0;
}
