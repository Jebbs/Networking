#include <sys/socket.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

#include <cstdint>

#include <pthread.h>

#include <signal.h>

typedef uint8_t byte;

enum
{
    BUFSIZE = 1500
};

void interruptHandler(int signal);
void *handleClient(void *args);

//global to allow cleanup if we receive SIGINT
int serverSd;

int main(int argc, char *argv[])
{
    //handle SIGINT (closes the server socket then terminates the program)
    signal(SIGINT, interruptHandler);

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

    //create the socket
    serverSd = socket(AF_INET, SOCK_STREAM, 0);

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
        int newSd = accept(serverSd, (sockaddr*)&newSockAddr, &newSockAddrSize);

        std::cout << "New client connected." << std::endl;

        pthread_t newThread;
        int args[2];
        args[0] = newSd;
        args[1] = repetition;
        pthread_create(&newThread, nullptr, handleClient, &args);
        pthread_detach(newThread);
    }

    return 0;
}

void interruptHandler(int signal)
{
    std::cout << "Closing server socket" << std::endl;
    close(serverSd);
    exit(0);
}

void *handleClient(void *args)
{
    //socket descriptor
    int sd = ((int *)args)[0];

    int repetition = ((int *)args)[1];
    byte databuf[BUFSIZE];

    int nRead, count = 0;
    for (int i = 0; i < repetition; i++)
    {
        nRead = 0;
        while (nRead < BUFSIZE)
        {
            nRead += read(sd, &databuf[nRead], BUFSIZE - nRead);
            count++;
        }

    }

    write(sd, &count, sizeof(count));
    close(sd);
}