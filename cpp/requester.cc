#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "discZmq.hh"

namespace po = boost::program_options;

//  ---------------------------------------------------------------------
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, int _rc, const std::string &_rep)
{
  assert(_topic != "");
  if (_rc == 0)
    std::cout << "\nCallback [" << _topic << "][" << _rep << "]" << std::endl;
  else
    std::cout << "\nCallback [" << _topic << "] Error" << std::endl;
}

//  ---------------------------------------------------------------------
/// \brief Print program usage.
void PrintUsage(const po::options_description &_options)
{
  std::cout << "Usage: requester [options] <topic> <data> <numSrvCalls>\n"
            << "Positional arguments:\n"
            << "  <topic>               Topic to publish\n"
            << "  <data>                Data\n"
            << "  <numSrvCalls>         Number of service calls to request\n"
            << _options << "\n";
}

//  ---------------------------------------------------------------------
/// \brief Read the command line arguments.
int ReadArgs(int argc, char *argv[], bool &_verbose, bool &_async,
  std::string &_master, std::string &_topic, std::string &_data,
  int &_numMessages)
{
  // Optional arguments
  po::options_description visibleDesc("Options");
  visibleDesc.add_options()
    ("help,h", "Produce help message")
    ("verbose,v", "Enable verbose mode")
    ("asynchronous,a", "Enable asynchronous requests")
    ("master,m", po::value<std::string>(&_master)->default_value(""),
       "Set the master endpoint");

  // Positional arguments
  po::options_description hiddenDesc("Hidden options");
  hiddenDesc.add_options()
    ("topic", po::value<std::string>(&_topic), "Topic to publish")
    ("data", po::value<std::string>(&_data), "Data")
    ("num", po::value<int>(&_numMessages), "Number of srv calls to request");

  // All the arguments
  po::options_description desc("Options");
  desc.add(visibleDesc).add(hiddenDesc);

  // One value per positional argument
  po::positional_options_description positionalDesc;
  positionalDesc.add("topic", 1).add("data", 1).add("num", 1);

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

  if (vm.count("help")  || !vm.count("topic") ||
      !vm.count("data") || !vm.count("num"))
  {
    PrintUsage(visibleDesc);
    return -1;
  }

  _verbose = false;
  if (vm.count("verbose"))
    _verbose = true;

  _async = false;
  if (vm.count("asynchronous"))
    _async = true;

  if (vm.count("master"))
    _master = vm["master"].as<std::string>();

  return 0;
}

//  ---------------------------------------------------------------------
int main(int argc, char *argv[])
{
  // Read the command line arguments
  std::string master, topic, data, response;
  int numSrvs, rc;
  bool verbose, async;
  if (ReadArgs(argc, argv, verbose, async, master, topic, data,
               numSrvs) != 0)
    return -1;

  // Transport node
  Node node(master, verbose);

  for (int i = 0; i < numSrvs; ++i)
  {
    if (async)
      rc = node.SrvRequestAsync(topic, data, cb);
    else
    {
      rc = node.SrvRequest(topic, data, response);
      if (rc == 0)
        std::cout << "Result: " << response << std::endl;
    }

    if (rc != 0)
      std::cout << "srv_request did not work" << std::endl;

    //node.SpinOnce();
  }

  node.Spin();

  // Zzzzzz Zzzzzz
  std::cout << "\nPress any key to exit" << std::endl;
  getchar();

  return 0;
}
