/* sha.h */

#ifndef _SHA_H_
#define _SHA_H_ 1

/* #include "global.h" */

/* The structure for storing SHS info */

typedef struct 
{
	UINT4 digest[ 5 ];            /* Message digest */
	UINT4 countLo, countHi;       /* 64-bit bit count */
	UINT4 data[ 16 ];             /* SHS data buffer */
	int Endianness;
} SHA_CTX;

/* Message digest functions */

void SHAInit(SHA_CTX *);
void SHAUpdate(SHA_CTX *, BYTE *buffer, int count);
void SHAFinal(BYTE *output, SHA_CTX *);

#endif /* end _SHA_H_ */
