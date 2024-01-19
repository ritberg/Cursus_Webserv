
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define PORT 8181

void handleClient(int clientSocket, const std::string& webRoot)
{
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    if (recv(clientSocket, buffer, sizeof(buffer), 0) <= 0)     // Receive data from client
    {
        perror("Error receiving data");
        return;
    }

    std::istringstream iss(buffer);                        // Extract requested webpage from the HTTP request
    std::string requestType, requestedPage, httpVersion;
    iss >> requestType >> requestedPage >> httpVersion;

    if (requestType != "GET")
    {
        perror("Invalid request");
        return;
    }

    std::string fullPath = webRoot + requestedPage;

    std::ifstream file(fullPath.c_str());
    if (!file.is_open())
    {
        perror("Error opening file");
        return;
    }

    std::ostringstream fileContent;    // Read the file content
    fileContent << file.rdbuf();
    file.close();

    std::ostringstream response;   // Prepare the HTTP response
    response << "GET /linux/man-pages/man2/recv.2.html HTTP/1.1\r\n";
    response << "Host: man7.org\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << fileContent.str();

    send(clientSocket, response.str().c_str(), response.str().size(), 0); // Send the response back to the client

    close(clientSocket);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config.txt>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string configFile = argv[1];

    std::ifstream configStream(configFile.c_str());
    if (!configStream.is_open())
    {
        std::cerr << "Error opening configuration file" << std::endl;
        return EXIT_FAILURE;
    }

    std::string webRoot;
    std::string line;
    while (std::getline(configStream, line))
    {
        if (line.find("web_root") != std::string::npos)
        {
            std::istringstream iss(line);
            iss >> webRoot >> webRoot;
            break;
        }
    }

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT); 

    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        perror("Error binding");
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 5) == -1)
    {
        perror("Error listening");
        return EXIT_FAILURE;
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    while (1)
    {
        if ((clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen)) == -1)
        {
            perror("Error accepting connection");
            continue;
        }
        std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        handleClient(clientSocket, webRoot); // Handle the client's request
    }
    close(serverSocket);

    return 0;
}
