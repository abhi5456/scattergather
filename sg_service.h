#ifndef SG_SERVICE_INCLUDED
#define SG_SERVICE_INCLUDED

// Includes
#include <sg_defs.h>

// Defines 

// Type definitions

// Global interface definitions

int sgServicePost( char *packet, size_t *len, char *rpacket, size_t *rlen );
    // Post a packet to the ScatterGather service

#endif
