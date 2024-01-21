
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>

#define MAX_BUFFER_SIZE 1024

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
        else
            return "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
    }
    return "HTTP/1.1 200 Ok\r\n\r\n <html><body><h1>Hello, World!</h1></body></html>"; // default response
}

std::string handleHttpRequest(const std::string& method, const std::string& path, const std::string& requestBody)
{
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

    int serverSocket;
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(std::stoi(serverConfig["listen"]));
    serverAddr.sin_addr.s_addr = INADDR_ANY; 

    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        perror("Error binding");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 5) == -1)
    {
        perror("Error listening");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    std::cout << "Server listening on port " << serverConfig["listen"] << "..." << std::endl;

    while (1)
    {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1)
        {
            std::cerr << "Error accepting connection" << std::endl;
            close(serverSocket);
            return EXIT_FAILURE;
        }

        char buffer[MAX_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        if (read(clientSocket, buffer, sizeof(buffer) - 1) == -1)
        {
            std::cerr << "Error reading from socket" << std::endl;
            close(clientSocket);
            continue;
        }

        std::istringstream request(buffer);        // Parse the HTTP request
        std::string method, path, protocol, line;
        request >> method >> path >> protocol;

        std::string requestBody; // will be useful for POST
        
        std::string response = handleHttpRequest(method, path, requestBody);

        if (write(clientSocket, response.c_str(), response.size()) == -1) // Send the HTTP response to the client
            std::cerr << "Error writing to socket" << std::endl;
        
        close(clientSocket); 
    }
    close(serverSocket);

    return 0;
}
