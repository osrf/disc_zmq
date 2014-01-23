#!/usr/bin/env python

"""
Testing multiple instances of mDNS clients.
"""
import argparse
import subprocess

def go(_prefix_name, _type, _domain, _port, _n, _verbose):
    '''
    '''
    if _verbose:
        print ('Registering %d services:' % _n)
        print ('\t Prefix_name: %s' % _prefix_name)
        print ('\t Type: %s' % _type)
        print ('\t Port: %s' % _port)

    for i in range(_n):
        try:
            # Updload the text file into the VRC Portal
            cmd = ('dns-sd -R ' + _prefix_name + str(i) + ' ' + _type + ' ' +
                   _domain + ' ' + _port)

            subprocess.check_call(cmd.split())

        except Exception, excep:
            print ('%sError in dns-sd operation : %s'
                   % repr(excep))
        _port = _port + 1


if __name__ == '__main__':

    # Specify command line arguments
    parser = argparse.ArgumentParser(
        description=('Run multiple instances of mDNS clients'))

    parser.add_argument('prefix_name', help='Prefix name of the instance')
    parser.add_argument('type', help='Service type')
    parser.add_argument('domain', help='Service domain')
    parser.add_argument('port', help='Starting port')
    parser.add_argument('n', help='Number of instances')

    parser.add_argument('-v', '--verbose',
                        help='Enable verbose mode', default=False)

    # Parse command line arguments
    args = parser.parse_args()
    arg_prefix_name = args.prefix_name
    arg_type = args.type
    arg_domain = args.domain
    arg_port = args.port
    arg_n = args.n
    arg_verbose = args.verbose


    # Attach the cloudsims!
    go(arg_prefix_name, arg_type, arg_domain, arg_port, arg_n, arg_verbose)
