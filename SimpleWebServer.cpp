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
#include <fcntl.h>
#include <map>
#include <vector>

#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 32

std::map<std::string, std::string> serverConfig; //global variable

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


void executeCgiScript(const std::string& cgiScriptPath, const std::string& requestBody)
{
    // Create a pipe for communication between parent and child processes
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }

    if (pid == 0)
    {
        // Child process

        // Close the write end of the pipe
        close(pipefd[1]);

        // Redirect standard input to read from the pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        // Execute the CGI script
        char* argv[] = {const_cast<char*>(cgiScriptPath.c_str()), NULL};
        if (execve(cgiScriptPath.c_str(), argv, NULL) == -1)
        {
            perror("execve");
            exit(1);
        }
    }
    else
    {
        // Parent process

        // Close the read end of the pipe
        close(pipefd[0]);

        // Write the request body to the pipe (it will be read by the CGI script)
        write(pipefd[1], requestBody.c_str(), requestBody.size());
        close(pipefd[1]);

        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);
    }
}

std::string handleHttpRequest(char* buffer)
{
    std::istringstream request(buffer);        // Parse the HTTP request
    std::string method, path, protocol, line;
    request >> method >> path >> protocol;

    std::string requestBody; // will be useful for POST

    if (method == "GET")
        return handleGetRequest(path);
    else if (method == "POST")    // TO DO
    {
        while (getline(std::cin, line)) {
            if (line.empty()) {
                break;  // End of request body
            }
            requestBody += line + "\n";
        }

        // Execute the CGI script with the request body
        std::string cgiScriptPath = "/Users/mmakarov/Documents/Cursus_Webserv_github/cgi-bin/cgi.py";
        executeCgiScript(cgiScriptPath, requestBody);
        return "";
    }            
        // return handlePostRequest(path, requestBody);
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

    // setsockopt allows reusing the same socket and avoiding "Error binding: Address already in use"
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        perror("Setsockopt failed");
        exit(1);
    }

    // fcntl sets the non-blocking mode for both server and client sockets.
    // Non-blocking sockets are useful to handle multiple connections concurrently
    // without blocking the execution of the program.
    fcntl(serverSocket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);

    struct sockaddr_in serverAddr = {0};                            // Structure to hold the server address
    serverAddr.sin_family = AF_INET;                               // set IP addresses to IPv4
    serverAddr.sin_port = htons(std::stoi(serverConfig["listen"])); // set the port from config.txt
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);           //set the addr to localhost

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
    
    fd_set activeSockets, readySockets; // fd_set is a structure type that can represent a set of fds. see select
    FD_ZERO(&activeSockets);             //removing all fds from the set of fds
    FD_SET(serverSocket, &activeSockets);  // add the server socket to the set of fds
    char buffer[MAX_BUFFER_SIZE];         // storing received messages 
    int maxSocket = serverSocket;


    while (1)
    {
        memset(buffer, '\0', sizeof(buffer));
        readySockets = activeSockets;   // Copy the active sockets set to use them with select()
        if (select(maxSocket + 1, &readySockets, NULL, NULL, NULL) <= 0)
        {
            std::cerr << "Error in select(): " << strerror(errno) << std::endl;
            exit(1);
        }
        std::cout << "maxSocket début " << maxSocket << std::endl;
    
        for (int socketId = 0; socketId <= maxSocket; socketId++)  // Check each socket for activity
        {
            if (FD_ISSET(socketId, &readySockets)) 
            {
                if (socketId == serverSocket) 
                {
                    std::cout << "ERROR 2, socketID: " << socketId << std::endl;
                    int clientSocket = accept(serverSocket, NULL, NULL);
                    std::cout << "clienSocket accepted " << clientSocket << std::endl;
                    if (clientSocket < 0) 
                    {
                        perror("Error accepting client connection");
                        exit(1);
                    }
                    fcntl(clientSocket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);

                    FD_SET(clientSocket, &activeSockets);
                    maxSocket = (clientSocket > maxSocket) ? clientSocket : maxSocket; // Update the max socket descriptor
                                                                    // it's mandatory to do it because select() uses bitsets to represent the fds to monitor
                                                                    // and the highest fd value is determined by the maximum fd in the sets 
                    clientSockets.push_back(clientSocket);
                    break; // Break after accepting a connection. Otherwise, I try to accept all the connections in the same loop
                } 
                else 
                {
                    int bytesRead = recv(socketId, buffer, sizeof(buffer) - 1, 0);

                    if (bytesRead <= 0) 
                    {
                        close(socketId);
                        FD_CLR(socketId, &activeSockets);
                        // Remove the closed socket from clientSockets vector. Otherwise, the data is sent to a closed socket
                        clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), socketId), clientSockets.end());
                    } 
                    else 
                    {
                        std::cout << std::endl << "BUFFER " << std::endl<< buffer << std::endl;
                        for (int i = 0; i < clientSockets.size(); i++)
                        {
                            std::cout << "socketId = " << socketId << std::endl;
                            std::cout << "clientSockets.size() = " << clientSockets.size() << std::endl;
                            std::cout << "clientSockets[i] = " << clientSockets[i] << std::endl;
                            std::string response = handleHttpRequest(buffer);
                            send(clientSockets[i], response.c_str(), response.size(), 0);
                            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), socketId), clientSockets.end());
                            close(socketId);
                            FD_CLR(socketId, &activeSockets);
                        }
                    }
                }
            }
        }
    }
    close(serverSocket);
    return (0);
}