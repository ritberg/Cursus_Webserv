#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define PORT 12345

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // server's IP address
    serverAddr.sin_port = htons(PORT);  // the same port as the server

    if (connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        perror("Error connecting to the server");
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to server at 127.0.0.1:" << PORT << "\""<< std::endl;

    std::string request = "GET /recv.2.html HTTP/1.1\r\nHost: localhost\r\n\r\n"; // Send a simple GET request. POST and DELETE implementation TO DO
    send(clientSocket, request.c_str(), request.size(), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cout << "Server response:\n" << buffer << std::endl; // display /recv.2.html on localhost:8181 TO DO

    close(clientSocket);

    return 0;
}
