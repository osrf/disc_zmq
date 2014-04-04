#ifndef __NET_UTILS_HH_INCLUDED__
#define __NET_UTILS_HH_INCLUDED__

#include <string>

// https://github.com/ros/ros_comm/blob/hydro-devel/clients/roscpp/src/
// libros/network.cpp
/// \brief Determine if an IP is private.
/// \param[in] Input IP address.
/// \return true if the IP address is private.
bool isPrivateIP(const char *ip);

/// \brief Determine if an IP is private.
/// \param[in] _hostname Hostname
/// \param[out] _ip IP associated to the input hostname.
/// \return 0 when success.
int hostname_to_ip(char * hostname , char* ip);

// https://github.com/ros/ros_comm/blob/hydro-devel/clients/roscpp/src/
// libros/network.cpp
/// \brief Determine IP or hostname.
/// \return The IP or hostname of this host.
std::string DetermineHost();

#endif
