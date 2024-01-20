
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
#define PORT 8181

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
        if (iss >> key >> value)
            serverConfig[key] = value;
    }
}

// Function to read the content of a file
std::string readFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (file.is_open())
    {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }
    else
        return "File not found";
}

// Function to handle HTTP GET requests
std::string handleGetRequest(const std::string& path) {
    // Construct the full path to the requested file
    std::string fullPath = serverConfig["web_root"] + path;

    // Read the content of the file
    std::string content = readFile(fullPath);

    // Return the content as the response
    return content;
}

// Function to handle HTTP requests
std::string handleHttpRequest(const std::string& method, const std::string& path, const std::string& requestBody) {
    if (method == "GET")
        return handleGetRequest(path);
    // else if (method == "POST")
    //     return handlePostRequest(path, requestBody);
    // else 
    //     // Handle other HTTP methods if needed
    //     return "Unsupported HTTP method";

}

/*
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
*/



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

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

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

        // Parse the HTTP request
        std::istringstream request(buffer);
        std::string method, path, protocol, line;
        request >> method >> path >> protocol;

        std::string requestBody;
        //process the request body and header. example of a request:
        
        // GET /path/to/resource HTTP/1.1
        // Host: www.example.com
        // User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:97.0) Gecko/20100101 Firefox/97.0
        // Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
        // Accept-Language: en-US,en;q=0.5
        // Connection: keep-alive
        
        std::string response = handleHttpRequest(method, path, requestBody);

            // Send the HTTP response to the client
        if (write(clientSocket, response.c_str(), response.size()) == -1)
            std::cerr << "Error writing to socket" << std::endl;
        
        close(clientSocket);

    }
    close(serverSocket);

    return 0;
}
