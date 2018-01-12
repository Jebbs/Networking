#include <sys/socket.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

#include <cstdint>

#include <pthread.h>

typedef uint8_t byte;

enum
{
    BUFSIZE = 1500
};

void *handleClient(void *args);

int main(int argc, char *argv[])
{

    //should have 3 arguments here
    if (argc < 3)
    {
        std::cerr << "Incorrect number of arguments." << std::endl;
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
        std::cerr << "Something is wrong with your arguments." << std::endl;
        return -1;
    }

    //Allow server to accept a connection from any IP address on the given port
    sockaddr_in acceptSockAddr;
    std::memset(&acceptSockAddr, 0, sizeof(acceptSockAddr));
    acceptSockAddr.sin_family = AF_INET;
    acceptSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSockAddr.sin_port = htons(port);

    //create the socket descriptor
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);

    //create the the socket and bind it to our accepted address (which is any)
    const int on = 1;
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    bind(serverSd, (sockaddr *)&acceptSockAddr, sizeof(acceptSockAddr));

    listen(serverSd, 16);

    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);

    //allow the server to keep looking for incoming connections
    while (true)
    {
        int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);

        std::cout << "New client connected." << std::endl;

        pthread_t newThread;
        int args[2];
        args[0]=newSd;
        args[1] = repetition;
        pthread_create(&newThread, nullptr, handleClient, &args);
        pthread_detach(newThread);


        //std::cout << "repeating " << repetition << " times" << std::endl;
    }

    close(serverSd);

    return 0;
}

void *handleClient(void *args)
{
    byte databuf[BUFSIZE];
    int newSd = ((int *)args)[0];
    int repetition = ((int *)args)[1];

    int nRead, count;
    for (int i = 0; i < repetition; i++)
    {
        for (nRead = 0, count = 1;
             (nRead += read(newSd, &databuf[nRead], BUFSIZE - nRead)) < BUFSIZE;
             ++count);
    }

    write(newSd, &count, sizeof(count));

}