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


//forward declaration
int connectToHost(const char* host, const char* port);

std::string getResponse(int sd);

int main(int argc, char *argv[])
{
    const char* serverIP = "google.com";
    const char* port = "80";

    const char* request = "GET / HTTP/1.1\r\n\r\n";


    int clientSd = connectToHost(serverIP, port);
    if(clientSd < 0)
        return -1;

 

    write(clientSd, request, strlen(request));

    std::cout << "getting response" << std::endl;
    std::string response = getResponse(clientSd);
    std::cout << "got response" << std::endl;

    std::cout << response;

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
int connectToHost(const char* host, const char* port)
{
    addrinfo* serverAddress;
    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //attempt to resolve the IP address
    int result = getaddrinfo(host, port, &hints, &serverAddress);
    if (result != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(result) << std::endl;
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
        perror("socket error");
        return -1;
    }

    return sd;
}

std::string getResponse(int sd)
{
    std::string response = "";
    int bufferSize = 1024;
    char buffer[bufferSize];

    int bufferPos;
    
    while (true)
    {
        bufferPos = 0;
        if(response.find("\r\n\r\n") != std::string::npos)
        {
            std::cout << "found it!" << std::endl;
            break;
        }

        std::cout << "getting line" << std::endl;
        
        while(bufferPos < bufferPos)
        {
            bufferPos += read(sd, &buffer[bufferPos], sizeof(char));
        }
        std::cout << "got line: " << std::string(&buffer[bufferPos], bufferPos) << std::endl;
        response += std::string(&buffer[bufferPos], bufferPos);
    }

    return response;
}