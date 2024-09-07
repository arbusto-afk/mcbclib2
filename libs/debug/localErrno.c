//
// Created by Ignacio on 8/31/2024.
//
#include "localErrno.h"

static char lerrno[255];

void * setLerrno(char * msg){
    strcpy(lerrno, msg);
    return NULL;
}