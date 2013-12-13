//
//  transport_defs.hh
//  Gazebo transport definitions
//
#ifndef __TRANSPORT_DEFS_H_INCLUDED__
#define __TRANSPORT_DEFS_H_INCLUDED__

#include <cstddef>

//  This is the version of Gazebo transport we implement
#define TRNSP_VERSION       "0.1.0"

// Message types
#define ADVERTISE						1
#define SUBSCRIBE						2

static char *msgTypesStr [] = {
    NULL, (char*)"ADVERTISE", (char*)"SUBSCRIBE"
};

#endif