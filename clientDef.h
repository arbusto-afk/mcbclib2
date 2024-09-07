//
// Created by Ignacio on 9/5/2024.
//

#ifndef MCCLIENT2_CLIENTDEF_H
#define MCCLIENT2_CLIENTDEF_H


#define CSTATE_HANDSHAKING -1
#define CSTATE_STATUS 0
#define CSTATE_LOGIN 1
#define CSTATE_TRANSFER 2
#define CSTATE_CONFIGURATION 3
#define CSTATE_PLAY 4
#define CSTATE_TOTALCOUNT 5

typedef struct vec3{
    int x;
    int y;
    int z;
} vec3_t;

typedef struct rotation2D{
    int yaw;
    int pitch;
} rotation2D_t;

typedef void (*packetHandler_t)(const uint8_t *data, int dataLen, struct client *c);

typedef struct client {
    int sockfd;
    int compressionTreshold;
    int state; //0 status, 1 login, 2 for transfer?, 3 configuration, 4 play
    char * playerName;
    char * hostname;
    int port;
    unsigned char isOnGround;
    vec3_t position;
    rotation2D_t rotation;
    packetHandler_t ** customPacketHandlers;
} client;

#endif //MCCLIENT2_CLIENTDEF_H
