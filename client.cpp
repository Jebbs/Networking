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

//because characters shoudl be for text only
typedef uint8_t byte;

int main(int argc, char *argv[])
{
    //should have 7 arguments here
    if (argc < 7)
    {
        //should make this more meaningful and shorter.
        std::cerr << "Error: Incorrect number of arguments.";
        std::cerr << "Expected 6 and was given " << argc-1 << "."<< std::endl;
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
        //this could be expanded later for a better error message
        std::cerr << "Error: Something is wrong with your arguments.";
        std::cerr << std::endl;
        return -1;
    }

    if (nbufs * bufsize != 1500)
    {
        std::cerr << "Error: nfufs*bufsize must be 1500 bytes." << std::endl;
        return -1;
    }

    if (type < 1 || type > 3)
    {
        std::cerr << "Error: The 'type' must be a value of 1,2, or 3.";
        std::cerr << std::endl;
        return -1;
    }

    //C99 apparently let's us do static arrays at runtime
    int databuf[nbufs][bufsize];

    struct addrinfo* server;
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int result = getaddrinfo(serverIp, port, &hints, &server);
    if (result)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    int clientSd;
    for (addrinfo* p = server; p != NULL; p = p->ai_next)
    {
        clientSd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientSd < 0)
            continue;

        if (connect(clientSd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(clientSd);
            clientSd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(server);

    if (clientSd < 0)
    {
        perror("socket error:");
        return -1;
    }

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

    gettimeofday(&start, 0);

    close(clientSd);
    return 0;
}
