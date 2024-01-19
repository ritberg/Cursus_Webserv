
// Server.cpp
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT 8081
#define MAX_BUFFER_SIZE 1024

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Prepare sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) == -1)
    {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    // Accept connection
    if ((clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen)) == -1)
    {
        perror("Error accepting connection");
        exit(EXIT_FAILURE);
    }

    std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

    // Receive data from client and echo it back
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        if (recv(clientSocket, buffer, sizeof(buffer), 0) <= 0)
        {
            perror("Error receiving data");
            break;
        }

        if (strcmp(buffer, "exit") == 0)
        {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        std::cout << "Received message: " << buffer << std::endl;

        // Echo back to the client
        send(clientSocket, buffer, strlen(buffer), 0);
    }

    // Close sockets
    close(clientSocket);
    close(serverSocket);

    return 0;
}

