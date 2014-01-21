#include <arpa/inet.h>
#include <dns_sd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#define LONG_TIME 1

static volatile int stopNow = 0;
static volatile int timeOut = LONG_TIME;

//  ---------------------------------------------------------------------
void HandleEvents(DNSServiceRef serviceRef)
{
  int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
  int nfds = dns_sd_fd + 1;
  fd_set readfds;
  struct timeval tv;
  int result;

  while (!stopNow)
  {
    FD_ZERO(&readfds);
    FD_SET(dns_sd_fd, &readfds);
    tv.tv_sec = timeOut;
    tv.tv_usec = 0;
    result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
    if (result > 0)
    {
      std::cout << "Fuera del select()" << std::endl;
      DNSServiceErrorType err = kDNSServiceErr_NoError;
      if (FD_ISSET(dns_sd_fd, &readfds))
        err = DNSServiceProcessResult(serviceRef);
      if (err) stopNow = 1;
    }
    else
    {
      std::cout << "Select() returned " << result << " errno " << errno <<
                   " " << strerror(errno) << std::endl;
      if (errno == EINTR) stopNow = 1;
    }
  }
}

//  ---------------------------------------------------------------------
static void MyBrowseCallBack(DNSServiceRef service,
                             DNSServiceFlags flags,
                             uint32_t interfaceIndex,
                             DNSServiceErrorType errorCode,
                             const char *name,
                             const char *type,
                             const char *domain,
                             void *context)
{
  std::cout << "Callback()\n";
  #pragma unused(context)
  if (errorCode != kDNSServiceErr_NoError)
    std::cerr << "MyBrowseCallBack returned " << errorCode << std::endl;
  else
  {
    const char *addString = (flags & kDNSServiceFlagsAdd) ? "ADD" : "REMOVE";
    const char *moreString = (flags & kDNSServiceFlagsMoreComing) ? "MORE" : "   ";
    std::cout << addString << " " << moreString << " " << interfaceIndex
              << name << "." << type << domain << std::endl;
  }

  if (!(flags & kDNSServiceFlagsMoreComing)) fflush(stdout);
}

//  ---------------------------------------------------------------------
static DNSServiceErrorType MyDNSServiceBrowse()
{
  DNSServiceErrorType error;
  DNSServiceRef serviceRef;

  error = DNSServiceBrowse(&serviceRef, 0, 0, "_ros._tcp", "", MyBrowseCallBack,
    NULL);

  if (error == kDNSServiceErr_NoError)
  {
    HandleEvents(serviceRef);
    DNSServiceRefDeallocate(serviceRef);
  }
  else
    std::cerr << "MyRegisterCallBack() returned " << error << std::endl;

  return error;
}

//  ---------------------------------------------------------------------
int main (int argc, const char *argv[])
{
  DNSServiceErrorType error = MyDNSServiceBrowse();
  if (error) std::cerr << "DNSServiceDiscovery() returned " << error << "\n";

  getchar();
  return 0;
}