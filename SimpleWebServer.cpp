
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <map>
#include <vector>

#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 10

std::map<std::string, std::string> serverConfig; //global variable. is it allowed??

void readConfigFile(const std::string& configFile)
{
    std::ifstream file(configFile);
    if (!file.is_open())
    {
        std::cerr << "Error opening configuration file" << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key, value;
        if (line.find("server") == 0)
            continue;
        if (iss >> key >> value)
            serverConfig[key] = value;
    }
}

std::string handleGetRequest(const std::string& path)
{
    if (path == "/image.html")
    {
        std::ifstream file("image.html");
        if (file.is_open())
        {
            std::ostringstream oss;
            oss << "HTTP/1.1 200 Ok\r\n\r\n"; // we need to add a header to each html file
            oss << file.rdbuf();
            return oss.str();
        }
    }
    else
    {
        std::ifstream file("index.html");
        if (file.is_open())
        {
            std::ostringstream oss;
            oss << "HTTP/1.1 200 Ok\r\n\r\n"; 
            oss << file.rdbuf();
            return oss.str();
        }
    }
    return "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
}

std::string handleHttpRequest(char* buffer)
{
    std::istringstream request(buffer);        // Parse the HTTP request
    std::string method, path, protocol, line;
    request >> method >> path >> protocol;

    std::string requestBody; // will be useful for POST

    if (method == "GET")
        return handleGetRequest(path);
    // else if (method == "POST")                // TO DO
    //     return handlePostRequest(path, requestBody);
    else 
        return "Unsupported HTTP method";

}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config.txt>" << std::endl;
        return EXIT_FAILURE;
    }

    readConfigFile(argv[1]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // reuse the same socket and avoid "Error binding: Address already in use"
    {
        perror("Setsockopt failed");
        exit(1);
    }

    struct sockaddr_in serverAddr = {0};                            // Structure to hold the server address
    serverAddr.sin_family = AF_INET;                               // set IP addresses to IPv4
    serverAddr.sin_port = htons(std::stoi(serverConfig["listen"])); // set the port from config.txt
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);           //set add to localhost

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error binding");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0)
    {
        perror("Error listening");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    std::cout << "Server listening on port " << serverConfig["listen"] << "..." << std::endl;
  
    std::vector<int> clientSockets;  //store client socket descriptors
    
    fd_set activeSockets, readySockets; // fd_set is a structure type that can represent a set of file descriptors. see select
    FD_ZERO(&activeSockets);             //removing all file descriptors the set of fds
    FD_SET(serverSocket, &activeSockets);  // add the server socket to the set of fds
    char buffer[MAX_BUFFER_SIZE];         // storing received messages TO DO: a buffer for each client!!
    int maxSocket = serverSocket;

    while (1)
    {
        readySockets = activeSockets;   // Copy the active sockets set for use with select()
        if (select(maxSocket + 1, &readySockets, NULL, NULL, NULL) < 0)
        {
            std::cerr << "Error in select(): " << strerror(errno) << std::endl;
            exit(1);
        }
    
        for (int socketId = 0; socketId <= maxSocket; socketId++)  // Check each socket for activity
        {
            if (FD_ISSET(socketId, &readySockets)) 
            {
                if (socketId == serverSocket) 
                {
                    int clientSocket = accept(serverSocket, NULL, NULL);
                    if (clientSocket < 0) 
                    {
                        perror("Error accepting client connection");
                        exit(1);
                    }

                    FD_SET(clientSocket, &activeSockets);
                    maxSocket = (clientSocket > maxSocket) ? clientSocket : maxSocket; // Update the max socket descriptor
                                                                    // it's mandatory to do it because select() uses bitsets to represent the fds to monitor
                                                                    // and the highest fd value is determined by the maximum fd in the sets 
                    clientSockets.push_back(clientSocket);
                } 
                else 
                {
                    int bytesRead = recv(socketId, buffer, sizeof(buffer) - 1, 0);

                    if (bytesRead <= 0) 
                    {
                        for (int i = 0; i < clientSockets.size(); i++)
                        {
                            if (clientSockets[i] != socketId)
                            {
                                std::string response = handleHttpRequest(buffer);
                                std::cout << "write in the first for loop " << clientSockets[i] << std::endl;
                                write(clientSockets[i], response.c_str(), response.size());
                            }
                        }
                        close(socketId);
                        FD_CLR(socketId, &activeSockets);
                    } 
                    else 
                    {
                        buffer[bytesRead] = '\0';
                        for (int i = 0; i < clientSockets.size(); i++)
                        {
                            if (clientSockets[i] != socketId) 
                            {
                                std::string response = handleHttpRequest(buffer);
                                std::cout << "write in the second for loop " << clientSockets[i] << std::endl;
                                write(clientSockets[i], response.c_str(), response.size());
                            }
                        }
                    }
                }
            }
        }
        // for (int i = 0; i < MAX_CLIENTS; ++i)
        // {
        //     if (clientSockets[i] > 0)
        //     close(clientSockets[i]);
        // }
    }
    close(serverSocket);
    return 0;
}
