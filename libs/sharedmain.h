//
// Created by Ignacio on 8/29/2024.
//

#ifndef SHAREDMAIN_H
#define SHAREDMAIN_H

#include <stdlib.h>

#define CSTATE_STATUS 0
#define CSTATE_LOGIN 1
#define CSTATE_TRANSFER 2
#define CSTATE_CONFIGURATION 3
#define CSTATE_PLAY 4
#define CSTATE_TOTALCOUNT 5

//in 1.12.2 client goes from handshake directly to play
//1.12.2 status 1 login 2
// 1.12.2 login seq
/*
 * C→S: Handshake with Next State set to 2 (login)
C→S: Login Start
S→C: Encryption Request
Client auth
C→S: Encryption Response
Server auth, both enable encryption
S→C: Set Compression (optional)
S→C: Login Success
 */
typedef struct varInt {
    int value;
    int byteSize;
    unsigned char *byteAfterEnd;
} TVarInt;

#define VARINTMAXLEN 5
#define VARINTMAXVAL 2147483647
#define VARINTMINVAL -2147483648

/*
 * Given a buffer interprets a varInt
 * Performs compulsory  bounds checking
 * Return a varint or nullVarInt on error
 */
TVarInt sharedMain_readVarInt(const unsigned char *buf, int bufDim);

/*
 * Writes a varInt into a mem address
 * Assumes there is enough space in the buffer
 * Returns byte after end
 * Returns NULLVARINT on incomplete, len excedeed varint, buf == NULL, dim < 0
 */
unsigned char *sharedMain_writeVarInt(unsigned char *address, int value);

/*
 * given a buffer write a string prefixed with its length as a varint
 * Assumes there is enough space in the buffer
 * Returns an unsigned char *  to the next byte after string
 */
unsigned char *sharedMain_writeString(unsigned char * buf, const char * str);

/*
 * Given an int returns its size as a varInt
 */
int sharedMain_getSizeAsVarInt(int n);


#endif //SHAREDMAIN_H
