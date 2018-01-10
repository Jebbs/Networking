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

typedef uint8_t byte;

int main(int argc, char *argv[])
{
    //should have 7 arguments here
    if (argc < 7)
    {
        std::cout << "You fucked up." << std::endl;
        return -1;
    }

    char* port;
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
        std::cout << "You fucked up." << std::endl;
        return -1;
    }

    std::vector<std::vector<byte>> buffers(nbufs, std::vector<byte>(bufsize));

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
        if((clientSd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))< 0)
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

    if(clientSd < 0)
    {
        std::cout << "You fucked up." << std::endl;
        perror("your mom uses perror");
        return -1;
    }

    /*
    if (result)
    {
        printf("%s\n", strerror(errno));
        std::cout << "You fucked up." << std::endl;
        freeaddrinfo(server);
        return -1;
    }
    else
    {
        std::cout << "You connected!" << std::endl;
    }
    

    while (true)
    {
    }*/

    
    close(clientSd);

    std::cout << "You didn't fuck up." << std::endl;
    return 0;
}
