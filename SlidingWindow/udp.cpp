#include "UdpSocket.h"
#include "Timer.h"
#include <iostream>

/**
 * The client stops and waits for an ACK before sending the next packet.
 *
 * Returns the number of times it had to resend a packet.
 */
int clientStopWait( UdpSocket &sock, const int max, int message[] )
{
    int retransmits;
    Timer timer;
    bool resend = false;

    // transfer message[] max times
    for ( int i = 0; i < max; i++ )
    {

        //std::cerr << "sending: " << i << std::endl;

        message[0] = i;                            // message[0] has a sequence #
        sock.sendTo( ( char * )message, MSGSIZE ); // udp message send

        //timer.start();

        bool waitingForACK = true;
        //wait for an ACK
        while(waitingForACK)
        {

            if(timer.lap() > 1500)
            {
                resend = true;
                break;
            }

            //check to see if we got anything form the server
            if(sock.pollRecvFrom()>0)
            {
                int ACK;
                sock.recvFrom( ( char * ) &ACK, sizeof(ACK) );

                std::cerr << "ACK: " << ACK << std::endl;

                //if we received the right ACK number, then we are good
                if(ACK == i)
                    break;

                //otherwise, we got a dup, and we will ignore it
            }

        }

        if(resend)
        {
            retransmits++;
            i--;
            continue;
        }

        cerr << "message = " << message[0] << endl;
    }

    return retransmits;;
}

void serverReliable( UdpSocket &sock, const int max, int message[] )
{

    int sequence = 0;

    // receive message[] max times
    for ( int i = 0; i < max; i++ )
    {
        //wait until we have a packet
        //while(sock.pollRecvFrom()<1);

        sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive

        //if this wasn't the right message
        if(sequence != message[0])
        {
            //resend the ACK for the message we just got
            sock.ackTo((char*)message, sizeof(int));

            //then try again
            i--;
            continue;
        }

        sock.setDestAddress("localhost");

        //otherwise send an ACK
        sock.ackTo((char*)message, sizeof(int));

        cerr << message[0] << endl;                     // print out message
    }
}

int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
    return 0;
}
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{

}
