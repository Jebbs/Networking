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

#include <sstream>

//the headers we care about
struct HeaderInfo
{
    std::string responseCode;
    int contentLength;
    bool chunked;
};

//forward declaration
int connectToHost(std::string host, const char* port);

HeaderInfo parseHeaders(std::string headers);


int main(int argc, char *argv[])
{

    std::string serverName, file;

    if (argc == 2)
    {
        serverName = std::string(argv[1]);
        file = "/";
    }
    else if(argc == 3)
    {
        serverName = std::string(argv[1]);
        file = std::string(argv[2]);
    }
    else
    {
        std::cerr << "Error: Incorrect number of arguments." << std::endl;
        std::cerr << "Arguments: serverIp, file(optional)" << std::endl;
        return -1;
    }

    const char* port = "80";
    std::string request = "GET " + file + " HTTP/1.1\r\n";
    request += "Host: " + serverName + "\r\n";
    request += "\r\n";

    int clientSd = connectToHost(serverName, port);
    if(clientSd < 0)
        return -1;

    write(clientSd, request.c_str(), request.length());


    std::string response = "";
    std::string headers = "";
    std::string body = "";
    int bufferSize = 512;
    char buffer[bufferSize];
    int bufferPos;

    while (response.find("\r\n\r\n") == std::string::npos)
    {
        bufferPos = read(clientSd, buffer, bufferSize);
        response += std::string(buffer, bufferPos);
    }

    std::size_t headerEndPos = response.find("\r\n\r\n");

    headers = response.substr(0, headerEndPos);

    HeaderInfo headerInfo = parseHeaders(headers);

    std::cout << "Response code: " << headerInfo.responseCode << std::endl;
    std::cout << "Content length: " << headerInfo.contentLength << std::endl;

    std::size_t bodyStartPos = headerEndPos+4;

    body = response.substr(bodyStartPos, bodyStartPos+headerInfo.contentLength);

    while (body.length() < headerInfo.contentLength)
    {
        bufferPos = read(clientSd, buffer, bufferSize);
        body += std::string(buffer, bufferPos);
    }

    std::cout << response << std::endl;



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
int connectToHost(std::string host, const char* port)
{
    addrinfo* serverAddress;
    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //attempt to resolve the IP address
    int result = getaddrinfo(host.c_str(), port, &hints, &serverAddress);
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

HeaderInfo parseHeaders(std::string headers)
{
    HeaderInfo ret;
    ret.contentLength = -1;
    ret.chunked = false;
    std::istringstream instream(headers);

    for(std::string line; std::getline(instream, line);)
    {
        std::size_t pos = line.find("HTTP/1.1");
        if(pos != std::string::npos)
            ret.responseCode = line;

        pos = line.find("Content-Length:");
        if(pos != std::string::npos)
        {
            try
            {
                //Content-Length: " is 16 characters long
                ret.contentLength = std::stoi(line.substr(16, line.length()));
            }
            catch (...)
            {
                ret.contentLength = -1;
            }
        }
    }

    return ret;
}
