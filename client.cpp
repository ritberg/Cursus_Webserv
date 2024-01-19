// Client.cpp
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8081
#define MAX_BUFFER_SIZE 1024

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];

    // Create socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Prepare sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        perror("Error connecting to the server");
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to server at " << SERVER_IP << ":" << PORT << std::endl;

    // Send data to the server
    while (true)
    {
        std::cout << "Enter message (type 'exit' to disconnect): ";
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove the newline character

        send(clientSocket, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "exit") == 0) {
            std::cout << "Disconnected from server." << std::endl;
            break;
        }

        // Receive and display the echoed message from the server
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        std::cout << "Server says: " << buffer << std::endl;
    }

    // Close socket
    close(clientSocket);

    return 0;
}
