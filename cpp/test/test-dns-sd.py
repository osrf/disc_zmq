#!/usr/bin/env python

"""
Testing multiple instances of mDNS clients.
"""
import argparse
import subprocess

def go(_prefix_name, _service, _domain, _port, _n):
    '''
    '''
    print ('Registering %s service/s:' % _n)
    print ('\t Prefix_name: %s' % _prefix_name)
    print ('\t Type: %s' % _service)
    print ('\t Port: %d' % _port)

    for i in range(int(_n)):
        try:
            # Updload the text file into the VRC Portal
            cmd = ('dns-sd -R ' + _prefix_name + str(i) + ' ' + _service + ' ' +
                   _domain + ' ' + str(_port))

            subprocess.check_call(cmd.split())

        except Exception, excep:
            print ('Error in dns-sd operation : %s'
                   % repr(excep))
        _port = _port + 1


if __name__ == '__main__':

    # Specify command line arguments
    parser = argparse.ArgumentParser(
        description=('Run multiple instances of mDNS clients'))

    parser.add_argument('prefix_name', help='Prefix name of the instance')
    parser.add_argument('service', help='Service type')
    parser.add_argument('domain', help='Service domain')
    parser.add_argument('port', help='Starting port')
    parser.add_argument('n', help='Number of instances')

    # Parse command line arguments
    args = parser.parse_args()
    arg_prefix_name = args.prefix_name
    arg_service = args.service
    arg_domain = args.domain
    arg_port = args.port
    arg_n = args.n


    # Attach the cloudsims!
    go(arg_prefix_name, arg_service, arg_domain, int(arg_port), arg_n)
