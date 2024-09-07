//
// Created by Ignacio on 8/30/2024.
//
#include "packetDefinitions.h"


void unhPacket(const uint8_t  * buf,int n, client * c){
}
#define CSTATE_STATUS 0
#define CSTATE_LOGIN 1
#define CSTATE_TRANSFER 2
#define CSTATE_CONFIGURATION 3
#define CSTATE_PLAY 4
#define CSTATE_TOTALCOUNT 5

PacketHandler stateStatusPackets[256] = {0};

PacketHandler stateLoginPackets[256] = {
        [0] = pHandlers_onDisconnect,
        [2] = pHandlers_onLoginSuccess,
        [3] = pHandlers_onSetCompression
};
PacketHandler stateConfigPacket[256] = {0};

PacketHandler stateConfigPackets[256] = {
        [2] = pHandlers_onDisconnect,
        [4] = pHandlers_KeepAlive
};
PacketHandler statePlayPackets[256] = {
        [38] = pHandlers_KeepAlive
};
PacketHandler *packetDefinitions_packetHandlers[CSTATE_TOTALCOUNT] = {
        //0 state status
        stateStatusPackets,
        //1 state login
        stateLoginPackets,
        //2: state transfer
         {},
         //state configuration id 0 is valid, 0x02 is index arr[2]
        stateConfigPackets,
        //state play
        statePlayPackets
};

packet_t packetDefinitions_createHandShakePacket(int protocolVer, const unsigned char *serverAdress, unsigned short port, int nextState)
{
    unsigned char *handshakePacket = malloc(1024); // 0.5kb
    if (!handshakePacket)
    {
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
    unsigned char * aux = sharedMain_writeVarInt(res.seq, prePacketByteSize);
    memcpy(aux, startAdress, prePacketByteSize);
    free(startAdress);

    return res;
}
packet_t packetDefinitions_createLoginPacket(const unsigned char *playerName)
{
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

/*
 * Given a buffer attempts to interpret all packets until invalid or incomplete packet
 * returns the amount of consumed bytes from reading valid packets
*/
int packetDefinitions_interpretBufAsMcPackets(unsigned char * buf, int remaningBytes, client * c)
    {
        int procBytes = 0;
        while(remaningBytes > 0){
            TVarInt packetLen = sharedMain_readVarInt(buf, remaningBytes);
            int totalPacketLen = packetLen.value + packetLen.byteSize;
            //error reading buffer cannot interpret valid varint, prob incomplete varint
            if(packetLen.byteAfterEnd == NULL)
                 {
                printf("error reading varint at %p wih seq: ", buf);
                for(int i = 0; i < 10; i++)
                    printf("%02x ", buf[i]);
                printf("]\n");
                return procBytes;
            }

            //incomplete packet, exiting
            if(totalPacketLen > remaningBytes){
                return procBytes;
            }

            //Case: complete last packet
            else if(totalPacketLen == remaningBytes){
                packetDefinitions_handleSpecificPacket(buf, c);
                return procBytes + totalPacketLen;
            }
            //case complete packet, continuing to next
            int handlerExitCode = packetDefinitions_handleSpecificPacket(buf, c);
            if(handlerExitCode){
                return procBytes;
            }
            buf += totalPacketLen;
            procBytes += totalPacketLen;
            remaningBytes -= totalPacketLen;
        }
        return procBytes;
    }

// Assuming sharedMain_readVarInt, unhPacket, and other relevant structures and functions are defined elsewhere
// Function to uncompress the zlib compressed data
static int uncompressPacket(uint8_t *compressedData, int compressedSize, uint8_t *uncompressedData, int uncompressedSize) {
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
 */
int packetDefinitions_handleSpecificPacket(uint8_t * buf, client * c)
{
    TVarInt packetLen = sharedMain_readVarInt(buf, VARINTMAXLEN);
    int totalPacketLen = packetLen.value + packetLen.byteSize;

    TVarInt packetId;
    uint8_t * data; //buf or uncompresed buf
    int dataLen;

    void * memToFree = NULL;
    if(c->compressionTreshold < 0){
        packetId = sharedMain_readVarInt(packetLen.byteAfterEnd, VARINTMAXLEN);
        data = packetId.byteAfterEnd;
        dataLen = packetLen.value - packetId.byteSize;
    }
    else
    {
        TVarInt uncompressedPacketLen = sharedMain_readVarInt(packetLen.byteAfterEnd, VARINTMAXLEN);

        //compression set, but packet is smaller than treshold, interpret directly
        if(uncompressedPacketLen.value == 0)
        {
            packetId = sharedMain_readVarInt(uncompressedPacketLen.byteAfterEnd, VARINTMAXLEN);
            data = packetId.byteAfterEnd;
            dataLen = packetLen.value - packetId.byteSize;
        }
        //compression set, packet larger or equal than treshold, decompress, then interpret
        else
        {
            uint8_t * compressedData = uncompressedPacketLen.byteAfterEnd;
            int compressedSize = packetLen.value - uncompressedPacketLen.byteSize;
            uint8_t *uncompressedData = malloc(uncompressedPacketLen.value);
            if (!uncompressedData || errno == ENOMEM) {
                printf("Memory allocation failed\n");
                return 1;
            }
            memToFree = uncompressedData;

            int result = uncompressPacket(compressedData, compressedSize, uncompressedData, uncompressedPacketLen.value);
            if (result != Z_OK) {
                printf("Decompression failed: %d , packetLen: %d\n", result, packetLen.value);
                free(uncompressedData);
                return 1;
            } else {
                printf("Sucecsfull uncompression of packetId: %d\n", packetId.value);
            }

            // After decompression, handle the packet
             packetId = sharedMain_readVarInt(uncompressedData, VARINTMAXLEN);
            data = packetId.byteAfterEnd;
            dataLen = uncompressedPacketLen.value - packetId.byteSize;
          //  printf("Decompressed and found packet with id %d [0x%02x]\n", packetId.value, (uint8_t)packetId.value);
            // Process the uncompressed packet based on its ID
        }

    }
    if((packetDefinitions_packetHandlers[c->state])[packetId.value] == NULL)
        ;//printf("Unhandled packet [%d][%d][0x%02x]\n", c->state, packetId.value,(uint8_t)packetId.value);
    else {
        printf("Handling packet [%d][%d][0x%02x]\n", c->state, packetId.value, (uint8_t)packetId.value);
        packetDefinitions_packetHandlers[c->state][packetId.value](data, dataLen, c);
    }
        free(memToFree);

    return 0;
}

/*
 * Separator for handlers
 * standarized first byte of data
 */

// Converts a 64-bit big-endian value to host byte order
void pHandlers_onSetCompression(const uint8_t * buf,int dataLen, client *c){
    TVarInt cTreshold = sharedMain_readVarInt(buf, VARINTMAXLEN);
    c->compressionTreshold = cTreshold.value;
    return;
}

void pHandlers_onLoginSuccess(const uint8_t * buf, int dataLen, client * c) {
    c->state = CSTATE_CONFIGURATION;
    if (c->compressionTreshold < 0) {
        uint8_t loginAcknowledgePacket[4] = {0x02, 0x03, 0x00};
        simpleSocket_send(c->sockfd, loginAcknowledgePacket, 3);
    }
    else
    {
        uint8_t loginAcknowledgePacket[4] = {0x03, 0x00, 0x03, 0x00};
        simpleSocket_send(c->sockfd, loginAcknowledgePacket, 4);
    }
}
void pHandlers_onDisconnect(const uint8_t * buf,int dataLen, client * c){
    char * str = malloc(dataLen + 1);
    memcpy(str, buf, dataLen + 1);
    str[dataLen] = 0;
    printf("Disconnected w/ reason: %s", str);
    free(str);
}
void pHandlers_KeepAlive(const uint8_t *data, int dataLen, client *c)
{
       uint8_t keepAliveResp[13] = { 0x0A, 0x00, 0x18};

       if(c->state == CSTATE_CONFIGURATION)
            keepAliveResp[2] = 0x04;

        for(int i = 0; i < 8; i++)
            keepAliveResp[i + 3] = data[i];;

      //      uint8_t test = 0x00;
    //simpleSocket_send(c->sockfd, &test, 1);
   //     simpleSocket_send(c->sockfd, keepAliveResp, 11 );
}
