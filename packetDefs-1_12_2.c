//
// Created by Ignacio on 9/5/2024.
//


#define TYPELONG_BYTESIZE 8
#define CPID_SERVERBOUND_KEEPALIVE 0x0B //client packet id
#define CPID_CLIENTBOUND_KEEPALIVE 0x1F
#define CPID_CLIENTBOUND_LOGINSUCCESS 0x02
#define CPID_CLIENTBOUND_DISCONNECT_PLAY 0x1A
#define CPID_CLIENTBOUND_DISCONNECT_LOGIN 0x00
#define CPID_CLIENTBOUND_SETCOMPRESSION 0x03

#include <stdint.h>
#include "libs/sharedmain.h"
#include "packetDefs-1_12_2.h"

//default packet handler
static void dph_onSetCompression(const uint8_t *buf, int dataLen, client *c) {
    TVarInt cTreshold = sharedMain_readVarInt(buf, VARINTMAXLEN);
    c->compressionTreshold = cTreshold.value;
    return;
}
static void dph_onDisconnect(const uint8_t *buf, int dataLen, client *c) {
    char *str = malloc(dataLen + 1);
    memcpy(str, buf, dataLen + 1);
    str[dataLen] = 0;
    printf("Disconnected w/ reason: %s", str);
    free(str);
}
static void dph_onLoginSuccess(const uint8_t *buf, int dataLen, client *c) {
    c->state = CSTATE_PLAY;
    return;
}
static void dph_KeepAlive(const uint8_t *data, int dataLen, client *c) {
    uint8_t keepAliveResp[TYPELONG_BYTESIZE + 3] = {0x0A, 0x00, CPID_SERVERBOUND_KEEPALIVE};
    memcpy(keepAliveResp + 3, data, TYPELONG_BYTESIZE);
    simpleSocket_send(c->sockfd, keepAliveResp, TYPELONG_BYTESIZE + 3);
}

packetHandler_t defaultPackets_stateStatus[MAXPACKETID] = {0};
packetHandler_t defaultPackets_stateLogin[MAXPACKETID] = {
        [CPID_CLIENTBOUND_DISCONNECT_LOGIN] = dph_onDisconnect,
        [CPID_CLIENTBOUND_LOGINSUCCESS] = dph_onLoginSuccess,
        [CPID_CLIENTBOUND_SETCOMPRESSION] = dph_onSetCompression
};
packetHandler_t defaultPackets_stateConfig[MAXPACKETID] = {0};
packetHandler_t defaultPacket_statePlay[MAXPACKETID] = {
        [CPID_CLIENTBOUND_KEEPALIVE] = dph_KeepAlive,
        [CPID_CLIENTBOUND_DISCONNECT_PLAY] = dph_onDisconnect
};
packetHandler_t *packetDefinitions_defaultPacketHandlers[CSTATE_TOTALCOUNT] = {
        defaultPackets_stateStatus,
        defaultPackets_stateLogin,
        {},
        defaultPackets_stateConfig,
        defaultPacket_statePlay
};


/*
 * Given a buffer attempts to interpret all packets until invalid or incomplete packet
 * returns the amount of consumed bytes from reading valid packets
 * VERSION DEPENDANT
*/
int packetDefinitions_interpretBufAsMcPackets(unsigned char *buf, int bufDim, client *c) {
    int procBytes = 0;
    int remainingBytes = bufDim;

    while (remainingBytes > 0) {
        TVarInt packetLen = sharedMain_readVarInt(buf, remainingBytes);
        int totalPacketLen = packetLen.value + packetLen.byteSize;

        // Error: could not interpret a valid VarInt, incomplete or invalid packet
        if (packetLen.byteSize == -1) {
            printf("Error reading varint at %p with seq: ", buf);
            for (int i = 0; i < remainingBytes && i < 10; i++)
                printf("%02x ", buf[i]);
            printf("]\n");
            return procBytes;
        }

        // Incomplete packet, exiting
        if (totalPacketLen > remainingBytes) {
            return procBytes;
        }

        // Handle complete packet (last or middle)
        int handlerExitCode = packetDefinitions_handleSpecificPacket(buf, c);
        if (handlerExitCode) {
            printf("Error reading packet\n");
            return procBytes;
        }

        // Move the buffer forward and update counters
        buf += totalPacketLen;
        procBytes += totalPacketLen;
        remainingBytes -= totalPacketLen;
    }
    return procBytes;
}

// Function to uncompress the zlib compressed data
static int uncompressPacket(uint8_t *compressedData, int compressedSize, uint8_t *uncompressedData,
                            int uncompressedSize) {
    z_stream stream;
    int status;

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    stream.avail_in = compressedSize;
    stream.next_in = compressedData;
    stream.avail_out = uncompressedSize;
    stream.next_out = uncompressedData;

    status = inflateInit(&stream);
    if (status != Z_OK) {
        return status;
    }

    status = inflate(&stream, Z_FINISH);
    if (status != Z_STREAM_END) {
        inflateEnd(&stream);
        return status == Z_OK ? Z_BUF_ERROR : status;
    }

    status = inflateEnd(&stream);
    return status;
}

/*
 *  Given a byte seq containing a single WHOLE packet interpretsPacket   return 0 OK
    return 1 on error
    VERSION DEPENDANT
 */
int packetDefinitions_handleSpecificPacket(uint8_t *buf, client *c) {
    TVarInt packetLen = sharedMain_readVarInt(buf, VARINTMAXLEN);
    int totalPacketLen = packetLen.value + packetLen.byteSize;

    TVarInt packetId;
    uint8_t *data; //buf or uncompresed buf
    int dataLen;

    void *memToFree = NULL;
    if (c->compressionTreshold < 0) {
        packetId = sharedMain_readVarInt(packetLen.byteAfterEnd, VARINTMAXLEN);
        data = packetId.byteAfterEnd;
        dataLen = packetLen.value - packetId.byteSize;
    } else {
        TVarInt uncompressedPacketLen = sharedMain_readVarInt(packetLen.byteAfterEnd, VARINTMAXLEN);
        //compression set, but packet is smaller than treshold, interpret directly
        if (uncompressedPacketLen.value == 0) {
            packetId = sharedMain_readVarInt(uncompressedPacketLen.byteAfterEnd, VARINTMAXLEN);
            data = packetId.byteAfterEnd;
            dataLen = packetLen.value - packetId.byteSize;
        }
            //compression set, packet larger or equal than treshold, decompress, then interpret
        else {
            uint8_t *compressedData = uncompressedPacketLen.byteAfterEnd;
            int compressedSize = packetLen.value - uncompressedPacketLen.byteSize;
            uint8_t *uncompressedData = malloc(uncompressedPacketLen.value);
            if (!uncompressedData || errno == ENOMEM) {
                printf("Memory allocation failed\n");
                return 1;
            }
            memToFree = uncompressedData;

            int result = uncompressPacket(compressedData, compressedSize, uncompressedData,
                                          uncompressedPacketLen.value);
            if (result != Z_OK) {
                printf("Decompression failed: %d , packetLen: %d, no:(   flushing packet\n", result, packetLen.value);
                free(uncompressedData);
                free(memToFree);
              //  return totalPacketLen;
                return 1;
            } else {
          //      printf("Succesfull uncompression of packetId: %d\n", packetId.value);
            }

            // After decompression, handle the packet
            packetId = sharedMain_readVarInt(uncompressedData, VARINTMAXLEN);
            data = packetId.byteAfterEnd;
            dataLen = uncompressedPacketLen.value - packetId.byteSize;
            //  printf("Decompressed and found packet with id %d [0x%02x]\n", packetId.value, (uint8_t)packetId.value);
            // Process the uncompressed packet based on its ID
        }

    }
    if ((packetDefinitions_defaultPacketHandlers[c->state])[packetId.value] == NULL &&
            (c->customPacketHandlers == NULL || c->customPacketHandlers[c->state] == NULL || c->customPacketHandlers[c->state][packetId.value] == NULL))
        ;//printf("Unhandled packet [%d][%d][0x%02x]\n", c->state, packetId.value,(uint8_t)packetId.value);
    else {
        printf("Handling packet [%d][%d][0x%02x]\n", c->state, packetId.value, (uint8_t) packetId.value);
        if (packetDefinitions_defaultPacketHandlers[c->state][packetId.value] != NULL) {
            packetDefinitions_defaultPacketHandlers[c->state][packetId.value](data, dataLen, c);
        }
        if (c->customPacketHandlers != NULL && c->customPacketHandlers[c->state] != NULL &&
        c->customPacketHandlers[c->state][packetId.value] != NULL) {
            c->customPacketHandlers[c->state][packetId.value](data, dataLen, c);
        }
    }
    free(memToFree);

    return 0;
}

/*
 *
 */


packet_t packetDefinitions_createHandShakePacket(int protocolVer, const unsigned char *serverAdress, unsigned short port, int nextState) {
    unsigned char *handshakePacket = malloc(1024); // 0.5kb
    if (!handshakePacket) {
        perror("Failed to allocate memory for handshake packet");
        exit(EXIT_FAILURE);
    }

    unsigned char *startAdress = handshakePacket;
    (*handshakePacket) = 0x00; // handshake packet id
    handshakePacket++;
    handshakePacket = sharedMain_writeVarInt(handshakePacket, protocolVer);
    handshakePacket = sharedMain_writeString(handshakePacket, serverAdress);
    *handshakePacket = (port >> 8) & 0xFF;
    handshakePacket++;
    *handshakePacket = port & 0xFF;
    handshakePacket++;
    handshakePacket = sharedMain_writeVarInt(handshakePacket, nextState);
    int prePacketByteSize = handshakePacket - startAdress;

    int varIntSize = sharedMain_getSizeAsVarInt(prePacketByteSize);
    printf("varint size %d pre packet:%d\n", varIntSize, prePacketByteSize);

    packet_t res;
    res.dim = prePacketByteSize + varIntSize;
    res.seq = malloc(res.dim);
    unsigned char *aux = sharedMain_writeVarInt(res.seq, prePacketByteSize);
    memcpy(aux, startAdress, prePacketByteSize);
    free(startAdress);

    return res;
}

packet_t packetDefinitions_createLoginPacket(const unsigned char *playerName) {
    unsigned char *loginPacket = malloc(256);
    unsigned char *startPtr = loginPacket;
    (*loginPacket) = 0x00; // login packet id
    loginPacket++;
    loginPacket = sharedMain_writeString(loginPacket, playerName);
    // loginPacket = writeUUID(loginPacket);
    int loginPacketByteSize = loginPacket - startPtr;
    int dataLen = sharedMain_getSizeAsVarInt(loginPacketByteSize);

    unsigned char *finalLoginPacket = malloc(dataLen + loginPacketByteSize);
    unsigned char *finalStartAdress = finalLoginPacket;
    finalLoginPacket = sharedMain_writeVarInt(finalLoginPacket, loginPacketByteSize);
    finalLoginPacket = memcpy(finalLoginPacket, startPtr, loginPacketByteSize);
    free(startPtr);
    int byteLength = finalLoginPacket - finalStartAdress + loginPacketByteSize;

    packet_t res;
    res.seq = finalStartAdress;
    res.dim = byteLength;
    return res;
}