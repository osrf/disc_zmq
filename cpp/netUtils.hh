/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef __NET_UTILS_HH_INCLUDED__
#define __NET_UTILS_HH_INCLUDED__

#include <string>

namespace transport
{
	/// \brief Determine if an IP is private.
	/// Reference: https://github.com/ros/ros_comm/blob/hydro-devel/clients/
	/// roscpp/src/libros/network.cpp
	/// \param[in] Input IP address.
	/// \return true if the IP address is private.
	bool isPrivateIP(const char *ip);

	/// \brief Determine if an IP is private.
	/// \param[in] _hostname Hostname
	/// \param[out] _ip IP associated to the input hostname.
	/// \return 0 when success.
	int hostname_to_ip(char * hostname , char* ip);

	/// \brief Determine IP or hostname.
	/// Reference: https://github.com/ros/ros_comm/blob/hydro-devel/clients/
	/// roscpp/src/libros/network.cpp
	/// \return The IP or hostname of this host.
	std::string DetermineHost();
}

#endif
