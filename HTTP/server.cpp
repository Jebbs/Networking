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
#include <sstream>

enum
{
    ALLOWED_CONNECTIONS = 16
};

struct RequestInfo
{

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

    int port = 80;

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

    std::string request;
    int bufferSize = 512;
    char buffer[bufferSize];
    int bufferPos;

    while (request.find("\r\n\r\n") == std::string::npos)
    {
        bufferPos = read(sd, buffer, bufferSize);
        request += std::string(buffer, bufferPos);
    }

    pthread_mutex_lock(&mut);
    std::cout << request;
    pthread_mutex_unlock(&mut);

    std::string requestedFile;
    std::istringstream instream(request);

    bool requestOK = true;

    std::string next;
    instream >> next;
    if(next != "GET")
        requestOK = false;

    instream >> requestedFile;
    next = requestedFile;

    instream >> next;
    if(next != "HTTP/1.1")
        requestOK = false;

    if(!requestOK)
    {
        //bad request
    }

    //check file

    std::string body = "<html><body><h1>You did it!</h1></body></html>";

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "\r\n";
    response += body;

    write(sd, response.c_str(), response.length());



    close(sd);
    delete ((int*)args);
}