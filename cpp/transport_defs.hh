//
//  transport_defs.hh
//  Gazebo transport definitions
//
#ifndef __TRANSPORT_DEFS_H_INCLUDED__
#define __TRANSPORT_DEFS_H_INCLUDED__

#include <cstddef>

//  This is the version of Gazebo transport we implement
#define TRNSP_VERSION       1

// Message types
#define ADV									1
#define SUB     						2
#define ADV_SVC							3
#define SUB_SVC  						4
#define PUB									5
#define REQ									6
#define REP_OK							7
#define REP_ERROR						8

static char *msgTypesStr [] = {
    NULL, (char*)"ADVERTISE", (char*)"SUBSCRIBE", (char*)"ADV_SRV",
    (char*)"SUB_SVC", (char*)"PUB", (char*)"REQ", (char*)"SRV_REP_OK",
    (char*)"SRV_REP_ERROR"
};

#endif