/*
 * Author: Jeremy DeHaan
 * Date: 1/26/2017
 *
 * Description:
 * server.cpp is a simple HTTP 1.0 server. It only understands the GET command.
 *
 * It is intended to be part of a series on network programming.
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

//forward declarations
void interruptHandler(int signal);
int createSocketListener(int port);
void *handleClient(void *args);
bool checkRequest(std::string request, std::string& outfile);

//global to allow cleanup if we receive SIGINT
int serverSd;

//because console output is not reentrant
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
    int port = 80;
    if (argc == 2)
    {
        port = std::stoi(argv[1]);
    }
    else
    {
        std::cerr << "Error: Argument for port number required." << std::endl;
        return -1;
    }

    //start handling SIGINT (closes the server socket before termination)
    signal(SIGINT, interruptHandler);

    serverSd = createSocketListener(port);
     if(serverSd < 0)
        return -1;

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
    if(sd<1)
    {
        perror("socket error");
        return -1;
    }

    //create the the socket and bind it to our accepted address (which is any)
    const int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    if(bind(sd, (sockaddr *)&acceptSockAddr, sizeof(acceptSockAddr)) < 0)
    {
        perror("socket port error");
        return -1;
    }

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
    std::string response;

    if(!checkRequest(request, requestedFile))
    {
        response = "HTTP/1.0 400 Bad Request\r\n\r\n";
        response += "<html><body><center><h1>Bad Request</h1></center>";
        response += "<center><p>The server could not understand your request and is now sad.</p></center>";
        response +="</body></html>";
    }
    else if(requestedFile == "/" || requestedFile == "/index.html")
    {
        response = "HTTP/1.0 200 OK\r\n\r\n";
        response += "<html><body><center><h1>You did it!</h1></center>";
        response += "<center><p>Welcome to the website. It's a cool place to be.</p></center>";
        response +="</body></html>";
    }
    else if (requestedFile == "/admin.html")
    {
        response = "HTTP/1.0 401 UNAUTHORIZED\r\n\r\n";
        response += "<html><body><center><h1>Unauthorized</h1></center>";
        response += "<center><p>You don't have the proper authentications to access that.</p></center>";
        response +="</body></html>";
    }
    else if (requestedFile == "/passwords.txt")
    {
        response = "HTTP/1.0 403 FORBIDDEN\r\n\r\n";
        response += "<html><body><center><h1>Forbidden</h1></center>";
        response += "<center><p>I don't know how you got here, friend, but you aren't allowed.</p></center>";
        response +="</body></html>";
    }
    else
    {
        response = "HTTP/1.0 404 Not Found\r\n\r\n";
        response += "<html><body><center><h1>File Not Found</h1></center>";
        response += "<center><pre>";
        response +="                                    ``                                \n";
        response +="                                /ymMMMMms-                            \n";
        response +="                              .dMMNs//+hMM/                           \n";
        response +="                              mMMh`     sMd                           \n";
        response +="                             .ddd`      oMo                           \n";
        response +="                              ```     .sNs                            \n";
        response +="                                    `oNy-                             \n";
        response +="                                   `dN:                               \n";
        response +="                                   :do                                \n";
        response +="                                   :o:                                \n";
        response +="                                   o+.                                \n";
        response +="                                `/yhhho-`                             \n";
        response +="                                dMMMMMMNh.                            \n";
        response +="                               -MMMMMMMMM+                            \n";
        response +="                              `mMMMMMMMMM-                            \n";
        response +="                              `NMNNNMMMNN-                            \n";
        response +="                              `ohdmddNhh+                             \n";
        response +="                       `-:/soshdyNNdhmhdo/:..`                        \n";
        response +="                       smmNMNmmMmdmmNMmddmMdddys`                     \n";
        response +="      `  `+:...`      :MMMMMNNmMNNmmNNNNNNNNNNmN/                     \n";
        response +="      oy/-+ymNNmh/.   hMMMMMMMMMMMMMMMMMMMMMMMMMm`   `....:+-         \n";
        response +="       :ydmmNMMMMMm+`sMMMMMMMMMMMMMMMMMMMMMMMMMMM: .ohmNMms+..-       \n";
        response +="          ..-hMMMMMMdMMMMMMMMMMMMMMMMMMMMMMMMMMMMd+mMMMMMNmdho-       \n";
        response +="             :dMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMN/-..`         \n";
        response +="               +mMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMs              \n";
        response +="                .hNMMMMMmMMMMMMMMMMMMMMMMMMMMMNMMMMMMMy`              \n";
        response +="                  -sddy++MMMMMMMMMMMMMMMMMMMMMyNMMMMd:                \n";
        response +="                        oMMMMMMMMMMMMMMMMMMMMM:.+os/`                 \n";
        response +="                        mMMMMMMMMMMMMMMMMMMMMMo                       \n";
        response +="                       oMMMMMMMMMMMMMMMMMMMMMMm                       \n";
        response +="                      .MMMMMMMMMMMMMMMMMMMMMMMM/                      \n";
        response +="                      -MMMMMMMMMMMMMMMMMMMMMMMMy                      \n";
        response +="</pre></center></body></html>";
    }

    write(sd, response.c_str(), response.length());

    close(sd);
    delete ((int*)args);
}

bool checkRequest(std::string request, std::string& outfile)
{
    std::istringstream instream(request);

    bool requestOK = true;
    std::string next;
    instream >> next;
    if(next != "GET")
        return false;

    instream >> outfile;
    next = outfile;

    instream >> next;
    if(next.find("HTTP/1.") == std::string::npos)
        return false;

    return true;
}