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
#include <cstdlib>


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
        cerr << "message = " << message[0] << endl;

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
    }

    return retransmits;
}


void serverReliable( UdpSocket &sock, const int max, int message[] )
{
    // receive message[] max times
    for ( int i = 0; i < max; i++ )
    {
        sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive
        cerr << message[0] << endl;

        //if this wasn't the right message
        if(i != message[0])
        {
            //resend the ACK for the message we just got
            sock.ackTo((char*)message, sizeof(int));

            //then try again
            i--;
            continue;
        }

        //otherwise send an ACK
        sock.ackTo((char*)message, sizeof(int));
    }
}

int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
    int sequence = 0;
    int highestSequence = -1;
    int packetsInTransit = 0;
    int lowestUnAckedPacket = 0;

    int retransmits = 0;
    Timer timer;

    while(lowestUnAckedPacket < max)
    {
        bool resend = false;

        //only send more packets if we have packets to send
        if(sequence < max)
        {
            message[0] = sequence;                            // message[0] has a sequence #
            sock.sendTo( ( char * )message, MSGSIZE ); // udp message send
            cerr << message[0] << endl;

            packetsInTransit++;
            highestSequence = sequence;
            sequence++;

            //while the packets in transit is less than the window size, send more.
            if(packetsInTransit < windowSize)
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
                //cerr<<"Resending " << lowestUnAckedPacket << endl;
                message[0] = lowestUnAckedPacket;
                sock.sendTo( (char*)message, MSGSIZE ); // udp message send
                retransmits++;
                timer.start();
            }

            //check to see if we got anything form the server
            if(sock.pollRecvFrom()>0)
            {
                int ACK;
                sock.recvFrom((char*) &ACK, sizeof(ACK));

                //std::cerr << "ACK: " << ACK << std::endl;

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

    }

    return retransmits;
}

void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize, int dropRate )
{
    //max+1 so that we don't go past the array when calculating cumulative ACK
    int messagesReceived[max];
    for(int i = 0; i < max; i++)
        messagesReceived[i] = -1;

    //start at -1 incase we don't receive packet 0
    int cumulativeACK = -1;

    //loop until we have all the massages
    while(cumulativeACK < max-1)
    {
        //wait until we got a packet
        sock.recvFrom((char*)message, MSGSIZE);
        cerr << message[0] << endl;

        //only ack if the packet is within our window range
        if(message[0] > cumulativeACK && cumulativeACK+windowSize+1 > message[0])
        {
            //drop what ever percentage of ALL ACK's we receive based on dropRate
            if(rand()%100 < dropRate)
            {
                cerr << "Dropping Packet " << message[0] << endl;
                continue;
            }

            messagesReceived[message[0]] = message[0];

            /**
             * Starting from the last position of the cumulative ack, we check what
             * packets have been received until we get find one that we haven't.
             *
             * Each time we find a packet greater than the current cumulative ack,
             * it becomes the new cumulative ack.
             */
            for(; cumulativeACK < max-1; cumulativeACK++)
            {
                if(messagesReceived[cumulativeACK+1] == -1)
                    break;
            }
            //cerr << "Cumulative ACK: " << cumulativeACK << endl;
            sock.ackTo((char*)&cumulativeACK, sizeof(int));

        }
    }

}