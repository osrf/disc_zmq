#ifndef __NET_UTILS_HH_INCLUDED__
#define __NET_UTILS_HH_INCLUDED__

#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <stdlib.h>
#include <string>

#include "config.hh"

#ifdef HAVE_IFADDRS_H
# include <ifaddrs.h>
#endif

//  ---------------------------------------------------------------------
// https://github.com/ros/ros_comm/blob/hydro-devel/clients/roscpp/src/
// libros/network.cpp
/// \brief Determine if an IP is private.
/// \param[in] Input IP address.
/// \return true if the IP address is private.
bool isPrivateIP(const char *ip)
{
  bool b = !strncmp("192.168", ip, 7) || !strncmp("10.", ip, 3) ||
           !strncmp("169.254", ip, 7);
  return b;
}

//  ---------------------------------------------------------------------
/// \brief Determine if an IP is private.
/// \param[in] _hostname Hostname
/// \param[out] _ip IP associated to the input hostname.
/// \return 0 when success.
int hostname_to_ip(char * hostname , char* ip)
{
  struct hostent *he;
  struct in_addr **addr_list;
  int i;

  if ( (he = gethostbyname( hostname ) ) == NULL)
  {
      // get the host info
      herror("gethostbyname");
      return 1;
  }

  addr_list = (struct in_addr **) he->h_addr_list;

  for(i = 0; addr_list[i] != NULL; i++)
  {
      //Return the first one;
      strcpy(ip , inet_ntoa(*addr_list[i]) );
      return 0;
  }

  return 1;
}

//  ---------------------------------------------------------------------
// https://github.com/ros/ros_comm/blob/hydro-devel/clients/roscpp/src/
// libros/network.cpp
/// \brief Determine IP or hostname.
/// \return The IP or hostname of this host.
std::string DetermineHost()
{
  char *ip_env;
  // First, did the user set DZMQ_IP?
  ip_env = getenv("DZMQ_IP");

  if (ip_env) {
    if (strlen(ip_env) != 0)
      return ip_env;
    else
      std::cerr << "invalid DZMQ_IP (an empty string)" << std::endl;
  }

  // Second, try the hostname
  char host[1024];
  memset(host, 0, sizeof(host));
  if (gethostname(host, sizeof(host) - 1) != 0)
    std::cerr << "determineIP: gethostname failed" << std::endl;

  // We don't want localhost to be our ip
  else if(strlen(host) && strcmp("localhost", host))
  {
    char hostIP[INET_ADDRSTRLEN];
    strcat(host, ".local");
    if (hostname_to_ip(host, hostIP) == 0)
    {
      return std::string(hostIP);
    }
  }

  // Third, fall back on interface search, which will yield an IP address
#ifdef HAVE_IFADDRS_H
  struct ifaddrs *ifa = NULL, *ifp = NULL;
  int rc;
  if ((rc = getifaddrs(&ifp)) < 0)
  {
   std::cerr << "error in getifaddrs: " << strerror(rc) << std::endl;
   exit(-1);
  }
  char preferred_ip[200] = {0};
  for (ifa = ifp; ifa; ifa = ifa->ifa_next)
  {
    char ip_[200];
    socklen_t salen;
    if (!ifa->ifa_addr)
      continue; // evidently this interface has no ip address
    if (ifa->ifa_addr->sa_family == AF_INET)
      salen = sizeof(struct sockaddr_in);
    else if (ifa->ifa_addr->sa_family == AF_INET6)
      salen = sizeof(struct sockaddr_in6);
    else
      continue;
    if (getnameinfo(ifa->ifa_addr, salen, ip_, sizeof(ip_), NULL, 0,
                    NI_NUMERICHOST) < 0)
    {
      std::cout << "getnameinfo couldn't get the ip of interface " <<
                   ifa->ifa_name << std::endl;
      continue;
    }
    // prefer non-private IPs over private IPs
    if (!strcmp("127.0.0.1", ip_) || strchr(ip_,':'))
      continue; // ignore loopback unless we have no other choice
    if (ifa->ifa_addr->sa_family == AF_INET6 && !preferred_ip[0])
      strcpy(preferred_ip, ip_);
    else if (isPrivateIP(ip_) && !preferred_ip[0])
      strcpy(preferred_ip, ip_);
    else if (!isPrivateIP(ip_) &&
             (isPrivateIP(preferred_ip) || !preferred_ip[0]))
      strcpy(preferred_ip, ip_);
  }
  freeifaddrs(ifp);
  if (!preferred_ip[0])
  {
    std::cerr <<
      "Couldn't find a preferred IP via the getifaddrs() call; "
      "I'm assuming that your IP "
      "address is 127.0.0.1.  This should work for local processes, "
      "but will almost certainly not work if you have remote processes."
      "Report to the disc-zmq development team to seek a fix." << std::endl;
    return std::string("127.0.0.1");
  }
  return std::string(preferred_ip);
#else
  // @todo Fix IP determination in the case where getifaddrs() isn't
  // available.
  std::cerr <<
    "You don't have the getifaddrs() call; I'm assuming that your IP "
    "address is 127.0.0.1.  This should work for local processes, "
    "but will almost certainly not work if you have remote processes."
    "Report to the disc-zmq development team to seek a fix." << std::endl;
  return std::string("127.0.0.1");
#endif
}

#endif
