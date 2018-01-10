#include <sys/socket.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char *argv[])
{

    //should have 3 arguments here
    if (argc < 3)
    {
        std::cout << "You fucked up." << std::endl;
        return -1;
    }

    int port;
    int repetition;

    try
    {
        port = std::stoi(argv[1]);
        repetition = std::stoi(argv[2]);
    }
    catch (...)
    {
        std::cout << "You fucked up." << std::endl;
        return -1;
    }

    sockaddr_in acceptSockAddr;
    std::memset(&acceptSockAddr, 0, sizeof(acceptSockAddr));
    acceptSockAddr.sin_family = AF_INET; // Address Family Internet
    acceptSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSockAddr.sin_port = htons(port);

    int serverSd = socket(AF_INET, SOCK_STREAM, 0);

    const int on = 1;
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
               sizeof(on));

    bind( serverSd, ( sockaddr* )&acceptSockAddr, sizeof( acceptSockAddr ) );

    listen( serverSd, 16 );

    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof( newSockAddr );
    int newSd = accept( serverSd, ( sockaddr *)&newSockAddr, &newSockAddrSize );

    


    close(serverSd);

    return 0;
}