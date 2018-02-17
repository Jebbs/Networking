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

        std::cerr << "sending: " << i << std::endl;

        message[0] = i;                            // message[0] has a sequence #
        sock.sendTo( ( char * )message, MSGSIZE ); // udp message send

        packetsInTransit++;
        highestSequence = i;

        //while the packets in transit is less than the window size, send more.
        if(packetsInTransit < windowSize)
            continue;

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
                sock.recvFrom( (char*) &ACK, sizeof(ACK) );

                std::cerr << "ACK: " << ACK << std::endl;

                //figures out the number of packets we didn't get ACK'd
                packetsInTransit = highestSequence-ACK;

                //The ACK is the highest number we have an acknowledgement for,
                //so ACK+1 is the lowest packet we haven't gotten one for
                lowestUnAckedPacket = ACK + 1;

                //if we have room to send more packets on the network
                if(packetsInTransit < windowSize)
                    break;
            }

        }

        if(resend)
        {
            cerr<<"Resending " << lowestUnAckedPacket;
            message[0] = lowestUnAckedPacket;
            sock.sendTo( (char*)message, MSGSIZE ); // udp message send
            retransmits++;
            continue;
        }
    }

    return retransmits;
}

void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{
    //max+1 so that we don't go past the array when calculating cumulative ACK
    int messagesReceived[max];
    for(int i = 0; i < max; i++)
        messagesReceived[i] = -1;

    //start at -1 incase we don't receive packet 0
    int cumulativeACK = -1;

    //loop until we have all the massages
    while(cumulativeACK < max)
    {
        //wait until we got a packet
        sock.recvFrom((char*)message, MSGSIZE);
        cerr << message[0] << endl;

        //only ack if the packet is within our window range
        if(message[0] > cumulativeACK && cumulativeACK+windowSize+1 > message[0])
        {
            messagesReceived[message[0]] = message[0];

            /**
             * Starting from the last position of the cumulative ack, we check what
             * packets have been received until we get find one that we haven't.
             *
             * Each time we find a packet greater than the current cumulative ack,
             * it becomes the new cumulative ack.
             */
            for(; cumulativeACK < max; cumulativeACK++)
            {
                if(messagesReceived[cumulativeACK+1] == -1)
                    break;
            }
            cerr << "Cumulative ACK: " << cumulativeACK << endl;
            sock.ackTo((char*)&cumulativeACK, sizeof(int));

        }
        else
        {
            cerr<<"Packet is outside the window!" << endl;
        }
    }

}

void serverEarlyRetrans1( UdpSocket &sock, const int max, int message[], int windowSize )
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

        //move the sliding window
        base = cumulativeACK;
    }
}
