/*
 * Author: Jeremy DeHaan
 * Date: 2/18/2018
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
        message[0] = i;
        sock.sendTo( ( char * )message, MSGSIZE );
        cerr << "message = " << message[0] << endl;

        timer.start();

        //Loop until we get the correct ACK
        while(true)
        {
            if(timer.lap() > 1500)
            {
                sock.sendTo( ( char * )message, MSGSIZE );
                retransmits++;
                timer.start();
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
    }

    return retransmits;
}

/**
 * The server will acknowledge all packets that it recieves.
 */
void serverReliable( UdpSocket &sock, const int max, int message[] )
{
    // receive message[] max times
    for ( int i = 0; i < max; i++ )
    {
        sock.recvFrom( ( char * ) message, MSGSIZE );
        cerr << message[0] << endl;

        //ACK the message no matter what (could be a duplicate due to timeout lost ACK, etc.)
        sock.ackTo((char*)message, sizeof(int));

        //if this wasn't the right message
        if(i != message[0])
        {
            i--;
            continue;
        }
    }
}

/**
 * The client will send a set of packets to the server and will send more
 * packets as it receives acknowledgements.
 *
 * Returns the number of times that it retransmitted a packet.
 */
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
        //only send more packets if we have packets to send
        if(sequence < max)
        {
            message[0] = sequence;
            sock.sendTo( ( char * )message, MSGSIZE );
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

        //Loop until we get an ACK
        while(true)
        {
            if(timer.lap() > 1500)
            {
                message[0] = lowestUnAckedPacket;
                sock.sendTo( (char*)message, MSGSIZE );
                retransmits++;
                timer.start();
            }

            //check to see if we got anything form the server
            if(sock.pollRecvFrom()>0)
            {
                int ACK;
                sock.recvFrom((char*) &ACK, sizeof(ACK));

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

/**
 * The server willacknowledge all packets received with a cumulative ACK to
 * allow the client to send a range of packets and not worry about missing ACKs.
 */
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{
    //max so that we don't go past the array when calculating cumulative ACK
    int messagesReceived[max];
    for(int i = 0; i < max; i++)
        messagesReceived[i] = -1;

    //start at -1 in case we don't receive packet 0
    int cumulativeACK = -1;

    //loop until we have all the massages
    while(cumulativeACK < max-1)
    {
        sock.recvFrom((char*)message, MSGSIZE);
        cerr << message[0] << endl;

        //mark this message as being recieved
        messagesReceived[message[0]] = message[0];

        //Calculate the new cumulative ACK
        for(int i = cumulativeACK+1; i < max; i++)
        {
            if(messagesReceived[i] == -1)
                break;

            cumulativeACK=i;
        }

        //send ACK
        sock.ackTo((char*)&cumulativeACK, sizeof(int));
    }
}