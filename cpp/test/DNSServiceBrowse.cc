/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <arpa/inet.h>
#include <dns_sd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <string>

//#define LONG_TIME 10
#define LONG_TIME 100000000


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
    result = select(nfds, &readfds, reinterpret_cast<fd_set*>(NULL),
      reinterpret_cast<fd_set*>(NULL), &tv);
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
    const char *addStr = (flags & kDNSServiceFlagsAdd) ? "ADD" : "REMOVE";
    const char *moreStr = (flags & kDNSServiceFlagsMoreComing) ? "MORE" : "   ";
    std::cout << addStr << " " << moreStr << " " << interfaceIndex
              << name << "." << type << domain << std::endl;
  }

  if (!(flags & kDNSServiceFlagsMoreComing)) fflush(stdout);
}

//  ---------------------------------------------------------------------
static DNSServiceErrorType MyDNSServiceBrowse()
{
  DNSServiceErrorType error;
  DNSServiceRef serviceRef;

  error = DNSServiceBrowse(&serviceRef, 0, 0, "_discZmq._tcp", "",
    MyBrowseCallBack, NULL);

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
static void MyRegisterCallBack(DNSServiceRef service,
                               DNSServiceFlags flags,
                               DNSServiceErrorType errorCode,
                               const char *name,
                               const char *type,
                               const char *domain,
                               void *contest)
{
  #pragma unused(flags)
  #pragma unused(context)

  if (errorCode != kDNSServiceErr_NoError)
    std::cerr << "MyRegisterCallBack() returned " << errorCode << std::endl;
  else
    std::cout << "REGISTER " << name << " " << type << domain << "\n";
}

//  ---------------------------------------------------------------------
static DNSServiceErrorType MyDNSRegisterService()
{
  DNSServiceErrorType error;
  DNSServiceRef serviceRef;

  // Create the DNS TXT register data
  char buffer[512];
  TXTRecordRef txtRecord;

  TXTRecordCreate(&txtRecord, sizeof(buffer), buffer);
  // Create a TXT record for the topic name
  std::string topic = "test_topic";
  TXTRecordSetValue(&txtRecord, "topic", topic.size(), topic.c_str());

  error = DNSServiceRegister(&serviceRef,
                             0,
                             0,
                             "caguero",
                             "_discZmq._tcp",
                             "",
                             NULL,
                             htons(9092),
                             TXTRecordGetLength(&txtRecord),
                             TXTRecordGetBytesPtr(&txtRecord),
                             MyRegisterCallBack,
                             NULL);

  TXTRecordDeallocate(&txtRecord);

  if (error == kDNSServiceErr_NoError)
  {
    HandleEvents(serviceRef);
    DNSServiceRefDeallocate(serviceRef);
  }
  else
    std::cerr << "MyRegisterCallBack() returned " << error << std::endl;

  /*if (error != kDNSServiceErr_NoError)
  {
    std::cerr << "MyRegisterCallBack() returned " << error << std::endl;
  }*/

  return error;
}

//  ---------------------------------------------------------------------
int main(int argc, const char *argv[])
{
  /*DNSServiceErrorType error = MyDNSRegisterService();
  if (error) std::cerr << "DNSRegisterService() returned "
                       << error << "\n";*/

  DNSServiceErrorType error = MyDNSServiceBrowse();
  if (error) std::cerr << "DNSServiceDiscovery() returned " << error << "\n";

  getchar();
  return 0;
}
