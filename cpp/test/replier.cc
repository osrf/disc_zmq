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

#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "../discZmq.hh"

namespace po = boost::program_options;

//  ---------------------------------------------------------------------
/// \brief Function is called everytime a service call is requested.
int echo(const std::string &_topic, const std::string &_data, std::string &_rep)
{
  assert(_topic != "");
  std::cout << "\nCallback [" << _topic << "][" << _data << "]" << std::endl;
  _rep = _data;
  return 0;
}

//  ---------------------------------------------------------------------
/// \brief Print program usage.
void PrintUsage(const po::options_description &_options)
{
  std::cout << "Usage: replier [options] <topic>\n"
            << "Positional arguments:\n"
            << "  <topic>               Topic to advertise\n"
            << _options << "\n";
}

//  ---------------------------------------------------------------------
/// \brief Read the command line arguments.
int ReadArgs(int argc, char *argv[], bool &_verbose, bool &_selfCall,
  std::string &_master, std::string &_topic)
{
  // Optional arguments
  po::options_description visibleDesc("Options");
  visibleDesc.add_options()
    ("help,h", "Produce help message")
    ("verbose,v", "Enable verbose mode")
    ("self-call,s", "Self-execute the advertised service call")
    ("master,m", po::value<std::string>(&_master)->default_value(""),
       "Set the master endpoint");

  // Positional arguments
  po::options_description hiddenDesc("Hidden options");
  hiddenDesc.add_options()
    ("topic", po::value<std::string>(&_topic), "Topic to publish");

  // All the arguments
  po::options_description desc("Options");
  desc.add(visibleDesc).add(hiddenDesc);

  // One value per positional argument
  po::positional_options_description positionalDesc;
  positionalDesc.add("topic", 1);

  po::variables_map vm;

  try
  {
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(positionalDesc).run(), vm);
    po::notify(vm);
  }
  catch(boost::exception &_e)
  {
    PrintUsage(visibleDesc);
    return -1;
  }

  if (vm.count("help")  || !vm.count("topic"))
  {
    PrintUsage(visibleDesc);
    return -1;
  }

  _verbose = false;
  if (vm.count("verbose"))
    _verbose = true;

  if (vm.count("master"))
    _master = vm["master"].as<std::string>();

  _selfCall = false;
  if (vm.count("self-call"))
    _selfCall = true;

  return 0;
}

//  ---------------------------------------------------------------------
int main(int argc, char *argv[])
{
  // Read the command line arguments
  std::string master, topic, data, response;
  bool verbose, selfCall;
  if (ReadArgs(argc, argv, verbose, selfCall, master, topic) != 0)
    return -1;

  // Transport node
  transport::Node node(master, verbose);

  // Advertise a service call
  if (node.SrvAdvertise(topic, echo) != 0)
    std::cout << "SrvAdvertise did not work" << std::endl;

  if (selfCall)
  {
    // Request my own service call
    data = "";
    int rc = node.SrvRequest(topic, data, response);
    if (rc == 0)
      std::cout << "Response: " << response << std::endl;
    else
      std::cout << "SrvRequest did not work" << std::endl;
  }

  // Unadvertise a service call
  //if (node.SrvUnAdvertise(topic) != 0)
  //  std::cout << "SrvUnAdvertise did not work" << std::endl;

  // Zzzzzz Zzzzzz
  node.Spin();

  return 0;
}
