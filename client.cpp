/*
 * Author: Jeremy DeHaan
 * Date: 1/14/2017
 *
 * Description:
 * client.cpp is a application that connects to a server and sends it data. It 
 * then prints how long it spent sending the data, how long it took to get a
 * reply from the server after sending the data, and how many times the server
 * called read().
 * 
 * It is intended to be part of an introduction in network programming.
 */
#include <sys/socket.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include <netdb.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/uio.h>

#include <sys/time.h>

enum
{
    BUFSIZE = 1500
};

//forward declaration
int connectToHost(char* host, char* port);

int main(int argc, char *argv[])
{
    //should have 7 arguments here
    if (argc < 7)
    {
        std::cerr << "Error: Incorrect number of arguments." << std::endl;
        std::cerr << "Correct usage: serverIp port repetition nbufs bufsize type" << std::endl;
        return -1;
    }

    char* serverIp;
    char* port;
    int repetition;
    int nbufs;
    int bufsize;
    int type;

    try
    {
        port = argv[1];
        repetition = std::stoi(argv[2]);
        nbufs = std::stoi(argv[3]);
        bufsize = std::stoi(argv[4]);
        serverIp = argv[5];
        type = std::stoi(argv[6]);
    }
    catch (...)
    {
        //this should be changed later for a better error message based on where
        //we actually failed
        std::cerr << "Error: Something is wrong with your arguments." << std::endl;
        return -1;
    }

    if (nbufs * bufsize != BUFSIZE)
    {
        std::cerr << "Error: nfufs*bufsize must be " << BUFSIZE << " bytes." << std::endl;
        return -1;
    }

    if (type < 1 || type > 3)
    {
        std::cerr << "Error: The 'type' must be a value of 1, 2, or 3." << std::endl;
        return -1;
    }

    uint8_t databuf[nbufs][bufsize];
    
    int clientSd = createConnection(serverIp, port);
    if(clientSd < 0)
        return -1;

    timeval start, lap1, lap2;
    int nReads;


    gettimeofday(&start, 0);
    for (int i = 0, current = 0; i < repetition; i++, current++)
    {
        switch (type)
        {
        case 1:
        {
            for (int j = 0; j < nbufs; j++)
            {
                write(clientSd, databuf[j], bufsize);
            }
            break;
        }
        case 2:
        {
            struct iovec vect[nbufs];
            for (int j = 0; j < nbufs; j++)
            {
                vect[j].iov_base = databuf[j];
                vect[j].iov_len = bufsize;
            }
            writev(clientSd, vect, nbufs);
            break;
        }
        case 3:
        {
            write(clientSd, databuf, nbufs * bufsize);
            break;
        }
        }
    }
    gettimeofday(&lap1, 0);

    read(clientSd, &nReads, sizeof(nReads));

    gettimeofday(&lap2, 0);

    long dataTime = (lap1.tv_sec - start.tv_sec)*1000000;
    dataTime += (lap1.tv_usec - start.tv_usec);

    long roundTime = (lap2.tv_sec - start.tv_sec)*1000000;
    roundTime += (lap2.tv_usec - start.tv_usec);

    std::cout << "Test " << type << ":";
    std::cout << " data-sending time = " << dataTime << "usec, ";
    std::cout << "round-trip time = "<< roundTime << "usec, ";
    std::cout << "#reads = " << nReads << std::endl;

    close(clientSd);
    return 0;
}

/*
 * Attempts to create connection to a given host using the specified port.
 * 
 * Any encountered errors will be printed to stderr.
 * 
 * Returns a socket descriptor if successful, or -1 on failure.
 */
int connectToHost(char* host, char* port)
{
    addrinfo* serverAddress;
    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //attempt to resolve the IP address
    int result = getaddrinfo(serverIp, port, &hints, &serverAddress);
    if (result != 0)
    {
        std::err << "getaddrinfo: " << gai_strerror(result)) << std::endl;
        return -1;
    }

    //check to see if we got anything that allows us to make a connection
    int sd;
    for (addrinfo* addr = serverAddress; addr != NULL; addr = addr->ai_next)
    {
        sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sd < 0)
            continue;

        if (connect(sd, addr->ai_addr, addr->ai_addrlen) < 0)
        {
            close(sd);
            sd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(serverAddress);

    if (sd < 0)
    {
        perror("socket error:");
        return -1;
    }

    return sd;
}