//
// Created by Ignacio on 8/30/2024.
//

#ifndef MCCLIENT2_PACKETDEFINITIONS_H
#define MCCLIENT2_PACKETDEFINITIONS_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libs/debug/localErrno.h"
#include "libs/sharedmain.h"
#include "packetDefinitions.h"
#include <stdint.h>

//#include "uncompr.c"
#include "libs/zlib/zlib.h"
#include "libs/simpleSocket.h"


#include <winsock2.h>  // Include Winsock 2 for Windows socket programming
#include <ws2tcpip.h>  // Include additional Winsock definitions
#include "clientDef.h"

typedef struct packet_t {
    unsigned char * seq;
    int dim;
} packet_t;


packet_t packetDefinitions_createHandShakePacket(int protocolVer, const unsigned char *serverAdress, unsigned short port, int nextState);
packet_t packetDefinitions_createLoginPacket(const unsigned char *playerName);

/*
 * Given a buffer attempts to interpret all packets until invalid or incomplete packet
 * calls corresponding packet handlers for each packet found
 * returns the amount of consumed bytes from reading valid packets
 */
int packetDefinitions_interpretBufAsMcPackets(unsigned char * buf, int remaningBytes, client * c);

int packetDefinitions_handleSpecificPacket(uint8_t * buf, client * c);

/*
 * Separator for handlers
 */
void dph_onSetCompression(const uint8_t * buf, int dataLen, client *c);
void dph_onDisconnect(const uint8_t * buf, int dataLen, client * c);
void dph_KeepAlive(const uint8_t * data, int dataLen, client * c);
void dph_onLoginSuccess(const uint8_t * buf, int dataLen, client * c);


typedef void (*PacketHandler)(const uint8_t *data, int dataLen, client *c);



#endif //MCCLIENT2_PACKETDEFINITIONS_H
