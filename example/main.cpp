/* Revision: 2.8.5 */

/******************************************************************************
* Copyright 1998-2017 NetBurner, Inc.  ALL RIGHTS RESERVED
*
*    Permission is hereby granted to purchasers of NetBurner Hardware to use or
*    modify this computer program for any use as long as the resultant program
*    is only executed on NetBurner provided hardware.
*
*    No other rights to use this program or its derivatives in part or in
*    whole are granted.
*
*    It may be possible to license this or other NetBurner software for use on
*    non-NetBurner Hardware. Contact sales@Netburner.com for more information.
*
*    NetBurner makes no representation or warranties with respect to the
*    performance of this computer program, and specifically disclaims any
*    responsibility for any damages, special or consequential, connected with
*    the use of this program.
*
* NetBurner
* 5405 Morehouse Dr.
* San Diego, CA 92121
* www.netburner.com
******************************************************************************/
/**

    @file       main.cpp
    @brief      NetBurner TCP/IP Server Multiple Socket Example

    This example creates a TCP server that listens on the specified TCP port
    number and can handle multiple TCP connections simultaneously (10 in this
    example).

    An easy way to test the example is to use multiple Telnet sessions to create
    simultaneous connections to the NetBurner device. Status messages are sent
    out stdio to the debug serial port, and to the client TCP connections.

**/

#include <predef.h>
#include <constants.h>
#include <utils.h>
#include <system.h>
#include <iosys.h>
#include <stdio.h>
#include <ctype.h>
#include <tcp.h>
#include <init.h>
#include "../lib/datalogger.h"


const char* AppName = "TCP Multiple Sockets Example";

#define PORT (1000) // TCP port number to listen on
#define NFDS (10)   // Max number of file descriptors/connections

int fd_array[NFDS]; // Array of TCP file descriptors



LOGFILEINFO;//Force the file info into the log 




//Now build the four objects that will be logged....

START_INTRO_OBJ(TCPConnectObj,"TCPConnect")
uint16_element hisport  {"hisport"};
uint16_element thefdnum {"fdn"};
uint16_element theaindx {"aindex"};
ipaddr_element hisaddr  {"hisaddr"};
END_INTRO_OBJ;


START_INTRO_OBJ(TCPCloseObj,"TCPClose")
uint16_element thefdnum {"fdn"};
uint16_element theaindx {"aindex"};
int32_element  errorwhy{"why"};
END_INTRO_OBJ;

START_INTRO_OBJ(TCPReadObj,"TCPRead")
uint16_element thefdnum {"fdn"};
uint16_element theaindx {"aindex"};
int32_element  nread{"nread"};
END_INTRO_OBJ;

START_INTRO_OBJ(TimeLogObj,"SecTick")
uint32_element sec {"sec"};
END_INTRO_OBJ;





TCPConnectObj TheTcpConnectObject;
TCPCloseObj   TheTcpCloseObject;
TCPReadObj   TheTcpCloseObject;
TimeLogObj   TimeLogObj;


extern "C" void UserMain( void* pd )
{
    init();


	InitLogFtp(MAIN_PRIO-1); //Start the FTP server that will serve the log file 
	LogFileVersions(); //Log the version of the included files in the log

	bLog=true; //Start recording, you can turn log recording on/off to record special events.




    /*
      Listen for incoming TCP connections. You only need to call
      listen() one time.
      - Any IP address
      - Port number = PORT
      - Queue up to 5 incoming connection requests
     */
    int fdl = listen( INADDR_ANY, PORT, 5 );
    iprintf( "Listening for incoming connections on port %d\r\n", PORT );

    while ( 1 )
    {
        // Declare file descriptor sets for select()
        fd_set read_fds;
        fd_set error_fds;

        // Init the fd sets
        FD_ZERO( &read_fds );
        FD_ZERO( &error_fds );

        // Configure the fd sets so select() knows what to process. In
        // this case any fd data to be read, or an error.
        for ( int i = 0; i < NFDS; i++ )
        {
            if ( fd_array[i] )
            {
                FD_SET( fd_array[i], &read_fds );
                FD_SET( fd_array[i], &error_fds );
            }
        }

        // select() should also process the listen fd
        FD_SET( fdl, &read_fds );
        FD_SET( fdl, &error_fds );

        /*
          select() will block until any fd has data to be read, or
          has an error. When select() returns, read_fds and/or
          error_fds variables will have been modified to reflect
          the events.
         */
        select( FD_SETSIZE, &read_fds, ( fd_set * )0, &error_fds, 0 );

        // If the listen fd has a connection request, accept it.
        if ( FD_ISSET( fdl, &read_fds ) )
        {
            IPADDR client_ip;
            WORD   client_port;

            int fda = accept( fdl, &client_ip, &client_port, 0 );

            // If accept() succeeded, find an open fd array slot
            if ( fda > 0 )
            {
                for ( int i = 0; i < NFDS; i++ )
                {
                    if ( fd_array[i] == 0 )
                    {
                        fd_array[i] = fda;
                        writestring( fda, "Welcome! 'Q' to quit." );
                        iprintf( "Added connection on fd[%d] = %d, ", i, fda );
                        ShowIP(client_ip);
                        iprintf(":%d\n", client_port);
						
						
						//Record the value in the structure and then log them.
						TheTcpConnectObject.hisport=client_port;
						TheTcpConnectObject.hisaddr=client_ip; 
						TheTcpConnectObject.thefdnum=fda;
						TheTcpConnectObject.theaindx=i;
						TheTcpConnectObject.Log();





                        fda = 0;
                        break;
                    }
                }
            }

            // If no array positions are open, close the connection
            if ( fda )
            {
                writestring( fda, "Server Full" );
                iprintf("Server Full\r\n");
                close( fda );
            }
        }

        // If the listen fd has an error, close it and reopen
        if ( FD_ISSET( fdl, &error_fds ) )
        {
            close( fdl );
            fdl = listen( INADDR_ANY, PORT, 5 );
        }

        // Process each fd array element and check it against read_fds
        // and error_fds.
        for ( int i = 0; i < NFDS; i++ )
        {
            if ( fd_array[i] )
            {
                // Check for data to be read
                if ( FD_ISSET( fd_array[i], &read_fds ) )
                {
                    char buffer[21];
                    int rv = read( fd_array[i], buffer, 20 );
                    if ( rv > 0 )
                    {
                        buffer[rv] = 0;
                        if ( buffer[0] == 'Q' )
                        {
                            iprintf( "Closing connection fd[%d]\r\n", i );
                            writestring( fd_array[i], "Bye\r\n" );

							TheTcpCloseObject.thefdnum=	fd_array[i];
							TheTcpCloseObject.theaindx=i;
							TheTcpCloseObject.errorwhy=0;
							TheTcpCloseObject.Log();
                            close( fd_array[i] );
                            fd_array[i] = 0;
                        }
                        else
                        {
                            iprintf( "Read \"%s\" from fd[%d]\r\n", buffer, i );
                            sniprintf( buffer, 21, "Server read %d byte(s)\r\n", rv );
                            writestring( fd_array[i], buffer );
                        }
                    }
                    else
                    {
                        iprintf( "ReadError on fd[%d]\r\n", fd_array[i] );
                        FD_SET( fd_array[i], &error_fds );
                    }
                }

                // Check for errors
                if ( FD_ISSET( fd_array[i], &error_fds ) )
                {
                    iprintf( "Error on fd[%d], closing connection\r\n", i );
					TheTcpCloseObject.thefdnum=	fd_array[i];
					TheTcpCloseObject.theaindx=i;
					TheTcpCloseObject.errorwhy=getsocketerror(fd_array[i]);
					TheTcpCloseObject.Log();
                    close( fd_array[i] );
                    fd_array[i] = 0;
                }
            }
        }
    }
}
