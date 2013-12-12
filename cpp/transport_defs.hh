//
//  transport_defs.hh
//  Gazebo transport definitions
//
#ifndef __TRANSPORT_DEFS_H_INCLUDED__
#define __TRANSPORT_DEFS_H_INCLUDED__

#include <cstddef>

//  This is the version of Gazebo transport we implement
#define TRNSP_VERSION       "0.1.0"

// Gazebo reserved services
#define SRV_HELLO						"#hello"
#define SRV_REGISTER				"#register"

//  Gazebo transport return values, as strings
#define OK              "\001"
#define WRONG_VERSION   "\002"
#define DUPLICATED_SRV  "\003"


static char *transport_rc [] = {
    NULL, (char*)"OK", (char*)"WRONG_VERSION", (char*)"DUPLICATED_SERVICE"
};

#endif