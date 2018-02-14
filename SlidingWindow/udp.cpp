/*
 * Author: Jeremy DeHaan
 * Date: 2/13/2018
 *
 * Description:
 * udp.cpp is a file that defines two methods of reliable transfer using UDP
 * sockets. One is a stop-and-wait and the other is sliding window transfer.
 *
 * It is intended to be part of a series on network programming.
 */

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

    // transfer message[] max times
    for ( int i = 0; i < max; i++ )
    {
        bool resend = false;

        message[0] = i;                            // message[0] has a sequence #
        sock.sendTo( ( char * )message, MSGSIZE ); // udp message send

        timer.start();

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

    return retransmits;
}


void serverReliable( UdpSocket &sock, const int max, int message[] )
{
    // receive message[] max times
    for ( int i = 0; i < max; i++ )
    {
        //wait until we have a packet
        //while(sock.pollRecvFrom()<1);

        sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive

        //if this wasn't the right message
        if(i != message[0])
        {
            //std::cerr < "got " << message[0] << " wanted " << seq
            //resend the ACK for the message we just got
            sock.ackTo((char*)message, sizeof(int));

            //then try again
            i--;
            continue;
        }

        //otherwise send an ACK
        sock.ackTo((char*)message, sizeof(int));

        cerr << message[0] << endl;                     // print out message
    }
}

int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
    int highestSequence = -1;
    int packetsInTransit = 0;
    int lowestUnAckedPacket;

    int retransmits;
    Timer timer;

    // transfer message[] max times
    for ( int i = 0; i < max; i++ )
    {
        bool resend = false;

        //std::cerr << "sending: " << i << std::endl;

        message[0] = i;                            // message[0] has a sequence #
        sock.sendTo( ( char * )message, MSGSIZE ); // udp message send


        //while we are less than the window size, do some stuff
        if(packetsInTransit < windowSize)
        {
            packetsInTransit++;
            highestSequence = i;

            continue;
        }

        //start the timer after we have gotten to our window size
        timer.start();

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

                //figures out the number of packets we didn't get ACK'd.
                int numUnackedPackets = highestSequence-ACK;

                //if we received the right ACK number, then we are good
                if(numUnackedPackets == 0)
                {
                    packetsInTransit = 0;

                     //reset the lowest unacked packet to the next sequence
                    lowestUnAckedPacket = i+1;
                    break;
                }
                else
                {
                    packetsInTransit = numUnackedPackets;
                    lowestUnAckedPacket = highestSequence - numUnackedPackets;
                    resend = true;
                    break;
                }

                //otherwise, we got a dup, and we will ignore it
            }

        }

        if(resend)
        {
            message[0] = lowestUnAckedPacket;
            sock.sendTo( (char*)message, MSGSIZE ); // udp message send
            retransmits++;
            continue;
        }

        //this needs to be fixed (it currently will only print when all packets in a window are ACK'D)
        cerr << "message = " << message[0] << endl;
    }

    return retransmits;
}

void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{
    //max+1 so that we don't go past the array when calculating cumulative ACK
    int messagesReceived[max+1];
    for(int i = 0; i < max+1; i++)
        messagesReceived[i] = -1;

    int base = 0;

    //loop until we have all the massages
    while(base < max)
    {
        //while we have packets queued to process
        while(sock.pollRecvFrom()>0)
        {
            sock.recvFrom( (char*)message, MSGSIZE );
            messagesReceived[message[0]] = message[0];

            //print out the message we received
            cerr << message[0] << endl;
        }

        /**
         * If we didn't receive the packet at the bottom of the window,
         * let the client time out instead of ACKing a packet outside the window
         */
        if(messagesReceived[base] == -1)
            continue;


        int cumulativeACK;
        for(cumulativeACK = base; cumulativeACK < base+windowSize; cumulativeACK++)
        {
            if(messagesReceived[cumulativeACK+1] == -1)
                break;
        }

        //send ack
        sock.ackTo((char*)&cumulativeACK, sizeof(int));
    }
}
