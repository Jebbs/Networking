/*
 * Author: Jeremy DeHaan
 * Date: 1/14/2017
 *
 * Description:
 * server.cpp is a server that is used to read a set amount of data from a
 * client, and then print how long it spent reading.
 *
 * It is intended to be part of an introduction in network programming.
 */

#include <sys/socket.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <cstdint>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>

enum
{
    BUFSIZE = 1500,
    ALLOWED_CONNECTIONS = 16
};

//forward declarations
void interruptHandler(int signal);
int createSocketListener(int port);
void *handleClient(void *args);

//global to allow cleanup if we receive SIGINT
int serverSd;

//because console output is not reentrant
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{

    int port = 8080;

    //start handling SIGINT (closes the server socket before termination)
    signal(SIGINT, interruptHandler);

    serverSd = createSocketListener(port);

    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);

    //allow the server to keep looking for incoming connections
    while (true)
    {
        int newSd = accept(serverSd, (sockaddr*)&newSockAddr, &newSockAddrSize);

        std::cout << "New client connected." << std::endl;

        pthread_t newThread;
        int* socketDescriptor = new int;
        *socketDescriptor = newSd;
        pthread_create(&newThread, nullptr, handleClient, socketDescriptor);
        pthread_detach(newThread);
    }

    return 0;
}

/*
 * Handle the SIGINT signal so that the socket can be closed.
 */
void interruptHandler(int signal)
{
    std::cout << "\nClosing server socket" << std::endl;
    close(serverSd);
    exit(0);
}

/*
 * Create a new socket that listens for incoming connections on a given port.
 *
 * This socket will accept connections from any IP address and will reuse local
 * addresses for new incoming connections.
 *
 * Returns a valid socket descriptor.
 */
int createSocketListener(int port)
{
    //Allow server to accept a connection from any IP address on the given port
    sockaddr_in acceptSockAddr;
    std::memset(&acceptSockAddr, 0, sizeof(acceptSockAddr));
    acceptSockAddr.sin_family = AF_INET;
    acceptSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSockAddr.sin_port = htons(port);

    //create the socket
    int sd = socket(AF_INET, SOCK_STREAM, 0);

    //create the the socket and bind it to our accepted address (which is any)
    const int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    bind(sd, (sockaddr *)&acceptSockAddr, sizeof(acceptSockAddr));

    listen(sd, ALLOWED_CONNECTIONS);

    return sd;
}

/*
 * Handles the transfer of information between the client and server.
 *
 * Once the data transfer is complete, it will close the connection to the
 * client.
 *
 * This function is intended to be run in a separate thread using pthreads.
 */
void* handleClient(void* args)
{
    int sd = *((int*)args);

    int bufferSize = 1024;
    char buffer[bufferSize];
    int bufferPos;

    bool running = true;
    
    while(running)
    {
        bufferPos = 0;
        while (bufferPos < bufferSize)
        {
            bufferPos += read(sd, &buffer[bufferPos], bufferSize);
            pthread_mutex_lock(&mut);
            std::cout << bufferPos << std::endl;
            pthread_mutex_unlock(&mut);

            if(bufferPos > 4 && buffer[bufferPos-1]=='\n'&& buffer[bufferPos-2]=='\r'
            && buffer[bufferPos-3]=='\n'&& buffer[bufferPos-4]=='\r')
            {
                running = false;
                break;
            }
        }

        pthread_mutex_lock(&mut);
        std::cout << buffer;
        pthread_mutex_unlock(&mut);

    }


    close(sd);
    delete ((int*)args);
}