
#include <stdio.h>
#include "sharedmain.h"
#include "debug/localErrno.h"
#include <string.h>

const TVarInt NULLVARINT = {0,-1,NULL};

static TVarInt sharedMain_returnVarInt(int value)
{
    unsigned char *buf = malloc(VARINTMAXLEN);
    sharedMain_writeVarInt(buf, value);
    TVarInt res = sharedMain_readVarInt(buf, VARINTMAXLEN);
    res.byteAfterEnd = NULL;
    free(buf);
    return res;
}

TVarInt sharedMain_readVarInt(const unsigned char *buf, int bufDim) {
    if (buf == NULL) {
        setLerrno("sharedMain_readVarInt pointer is NULL");
        return NULLVARINT;
    }
    if(bufDim < 0){
        setLerrno("invalid bufDim");
        return NULLVARINT;
    }

    int value = 0;
    unsigned char position = 0;
    int shift = 0;

    while (1) {
        if (position >= VARINTMAXLEN) {
            setLerrno(("sharedMain_readVarInt varint exceeds byte length"));
            return NULLVARINT;
        }
        if(position >= bufDim){
            setLerrno("sharedMain_readVaInt incomplete varint");
            return NULLVARINT;
        }

        unsigned char byte = buf[position];
        value |= (byte & 0x7F) << shift;
        shift += 7;

        if (!(byte & 0x80)) break; // If the continuation bit is not set, stop reading

        position++;
    }
    TVarInt res = {value , position + 1 , buf + (position + 1)};
    return res;
}

unsigned char *sharedMain_writeVarInt(unsigned char *buf , int value) {
    if (value > VARINTMAXVAL || value < VARINTMINVAL) {
        setLerrno("sharedMain_writeVarInt varint out of bounds");
        return NULL;
    }
    if (buf == NULL) {
        setLerrno("sharedMain_writeVarInt pointer is NULL");
        return NULL;
    }
    long long rawVal;
    if (value < 0)
        rawVal = VARINTMAXVAL - (long long)VARINTMINVAL+ value + 1;
    else
        rawVal = value;

    while (1) {
        *(buf) = rawVal & 0x7f;
        rawVal >>= 7;
        if (rawVal == 0)
            break;
        *(buf) |= 0x80;//turn on signed bit
        buf += 1;
    }
    //skipping one byte since here buf is the last byte used by the varint
    return buf + 1;
}

unsigned char *sharedMain_writeString(unsigned char * buf, const char * str){
    int len = strlen(str);
    buf = sharedMain_writeVarInt(buf, len);
    memcpy(buf, str, len);
    return buf + len;
}

int sharedMain_getSizeAsVarInt(int n)
{
    return sharedMain_returnVarInt(n).byteSize;
}