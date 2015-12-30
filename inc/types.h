#ifndef _TYPES_H
#define _TYPES_H
#include "stdio.h"
#include "time.h"

typedef char  U8;
typedef unsigned short S16;
typedef unsigned int U32;

typedef struct{unsigned char octet[16];} U128;
typedef enum boolean{false,true}  bool;

#define TURNSUCCEED 0
#define TURNFAIL	-1

#endif // ifdef