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

typedef uint8_t byte;

int main(int argc, char *argv[])
{
    //should have 7 arguments here
    if (argc < 7)
    {
        std::cerr << "Incorrect number of arguments." << std::endl;
        return -1;
    }

    char *port;
    int repetition;
    int nbufs;
    int bufsize;
    char *serverIp;
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
        std::cerr << "Error: something is wrong with your arguments." << std::endl;
        return -1;
    }

    if (nbufs * bufsize != 1500)
    {
        std::cerr << "Error: nfufs*bufsize must be 1500 bytes." << std::endl;
        return -1;
    }

    if (type < 1 || type > 3)
    {
        std::cerr << "Error: The 'type' parameter must have a value of 1,2, or 3." << std::endl;
        return -1;
    }

    std::vector<std::vector<byte>> databuf(nbufs, std::vector<byte>(bufsize));

    struct addrinfo *server;
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
    for (addrinfo *p = server; p != NULL; p = p->ai_next)
    {
        if ((clientSd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
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
                write(clientSd, &(databuf[0][j]), bufsize);
            }
            break;
        }
        case 2:
        {
            struct iovec vect[nbufs];
            for (int j = 0; j < nbufs; j++)
            {
                vect[j].iov_base = &databuf[j][0];
                vect[j].iov_len = bufsize;
            }
            writev(clientSd, vect, nbufs);
            break;
        }
        case 3:
        {
            write(clientSd, &databuf[0][0], nbufs * bufsize);
            break;
        }
        }
    }

    gettimeofday(&lap1, 0);

    read(clientSd, &nReads, sizeof(nReads));

    gettimeofday(&lap2, 0);

    std::cout << "Test " << type << ":";
    std::cout << "data-sending time = ";
    std::cout << (lap1.tv_usec - start.tv_usec) << "usec, ";
    std::cout << "round-trip time = ";
    std::cout << (lap2.tv_usec - start.tv_usec) << "usec, ";
    std::cout << "#reads = " << nReads << std::endl;

    gettimeofday(&start, 0);

    close(clientSd);

    std::cout << "You didn't fuck up." << std::endl;
    return 0;
}
