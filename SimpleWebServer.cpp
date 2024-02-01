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

#define MAX_BUFFER_SIZE 100000
#define MAX_CLIENTS 32

std::map<std::string, std::string> serverConfig; // global variable

void readConfigFile(const std::string &configFile)
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

std::string executeCgiScript(const std::string &cgiScriptPath, const std::string &scriptContent)
{
    int stdinPipe[2];
    int stdoutPipe[2];

    if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1)
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

    if (pid == 0) // Child process
    {
        close(stdinPipe[1]); // Close unused ends of the pipes
        close(stdoutPipe[0]);

        dup2(stdinPipe[0], STDIN_FILENO); // Redirect stdin and stdout
        dup2(stdoutPipe[1], STDOUT_FILENO);

        const char *const argv[] = {cgiScriptPath.c_str(), NULL};
        const char *const envp[] = {
            "REQUEST_METHOD=POST", // Add PATH_INFO later
            NULL};

        // execute the CGI script
        execve(cgiScriptPath.c_str(), const_cast<char *const *>(argv), const_cast<char *const *>(envp));
        perror("execve");
        exit(1);
        return "";
    }
    else // Parent process
    {
        close(stdinPipe[0]); // Close unused ends of the pipes
        close(stdoutPipe[1]);

        // Write the request body to the child process
        write(stdinPipe[1], scriptContent.c_str(), scriptContent.size());
        close(stdinPipe[1]);

        char buffer[100000]; // Read the response from the child process
        std::string responseData;

        ssize_t bytesRead;
        while ((bytesRead = read(stdoutPipe[0], buffer, BUFSIZ)) > 0)
            responseData.append(buffer, bytesRead);

        int status;
        waitpid(pid, &status, 0);
        std::cout << "responseData " << responseData << std::endl;

        return responseData;
    }
}

std::string handleGetRequest(const std::string &path)
{
    if (path == "/upload.html")
    {
        std::ifstream file("upload.html");
        if (file.is_open())
        {
            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n\r\n";
            oss << file.rdbuf();
            return oss.str();
        }
    }
    else if (path == "/cgi-bin/cgi.php")
    {
        std::ifstream file("cgi-bin/cgi.php");
        if (file.is_open())
        {
            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n\r\n";
            std::string scriptContent;
            while (!file.eof())
            {
                char ch;
                file.get(ch);
                scriptContent += ch;
            }
            // Use CGI script for GET request, just to display the web page
            return oss.str() + executeCgiScript("/usr/bin/php", scriptContent);
        }
    }
    else if (path == "/image.html")
    {
        std::ifstream file("image.html");
        if (file.is_open())
        {
            std::ostringstream oss;
            oss << "HTTP/1.1 200 Ok\r\n\r\n"; // We need to add a header to each html file
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

std::string handlePostRequest(const std::string &path, const std::string &buffer)
{
    std::string body; // Parsing by Diogo
    std::string boundary;
    size_t pos_marker = buffer.find("boundary=");
    size_t end_marker;
    size_t i = pos_marker + std::string("boundary=").length() - 1;

    while (++i < buffer.length() && buffer[i] != '\n')
        boundary.push_back(buffer[i]);
    pos_marker = buffer.find("Content-Type", i);
    i = pos_marker - 1;
    while (++i < buffer.length() && buffer[i] != '\n')
        i += 2;
    end_marker = buffer.find(boundary.substr(0, boundary.length() - 1), i);
    while (++i < buffer.length() && i < end_marker)
        body.push_back(buffer[i]);
    // std::cout << std::endl << "Binary Data:\n" << body << std::endl;
    std::ofstream outfile;
    outfile.open("upload_folder/test.jpg"); // The image uploaded by a client
    if (outfile.fail())
        return "Unsupported HTTP method";
    if (path == "/upload.html")
    {
        std::ifstream htmlFile("upload.html"); // Read the content of the original HTML page
        std::string originalHtml;

        if (htmlFile.is_open()) // Check if the file is open before reading
        {
            char c;
            while (htmlFile.get(c))
                originalHtml += c;
            htmlFile.close();
        }

        std::string updatedHtml = originalHtml + "\nSuccessfully uploaded!"; // Add a success msg to the intial html page

        outfile << body; // Put the body of the uploaded file into the folder
        outfile.close();

        return "HTTP/1.1 200 Ok\r\n\r\n" + updatedHtml; // Return the HTTP response with the updated HTML content
    }
    if (path == "/cgi-bin/cgi.php")
    {
        std::string res = executeCgiScript("/usr/bin/php", body);
        outfile << res;
        outfile.close();
        return "HTTP/1.1 200 Ok\r\n\r\n" + res; // display success msg TO DO
    }
    return "Unsupported HTTP method";
}

std::string handleHttpRequest(std::string &buffer)
{
    std::istringstream request(buffer); // Parse the HTTP request
    std::string method, path, line;
    request >> method >> path;

    std::string requestBody;

    if (method == "GET")
        return handleGetRequest(path);
    else if (method == "POST")
        return handlePostRequest(path, buffer);
    else if (method == "DELETE")
    {
    }
    return "Unsupported HTTP method";
}

int main(int argc, char **argv)
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

    // Setsockopt allows reusing the same socket and avoiding "Error binding: Address already in use"
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
    serverAddr.sin_family = AF_INET;                                // set IP addresses to IPv4
    serverAddr.sin_port = htons(std::stoi(serverConfig["listen"])); // set the port from config.txt
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);            // set the addr to localhost

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

    std::vector<int> clientSockets; // Store client socket descriptors

    fd_set activeSockets, readySockets;   // fd_set is a structure type that can represent a set of fds. see select
    FD_ZERO(&activeSockets);              // removing all fds from the set of fds
    FD_SET(serverSocket, &activeSockets); // add the server socket to the set of fds
    // char buffer[MAX_BUFFER_SIZE];         // storing received messages
    std::vector<char> VectBuff;
    int maxSocket = serverSocket;

    while (1)
    {
        // memset(buffer, '\0', sizeof(buffer));
        VectBuff.clear(); // Use a vectorized buffer to read all the characters, including \0
        VectBuff.reserve(100000);
        VectBuff.resize(100000);
        readySockets = activeSockets; // Copy the active sockets set to use them with select()
        if (select(maxSocket + 1, &readySockets, NULL, NULL, NULL) <= 0)
        {
            std::cerr << "Error in select(): " << strerror(errno) << std::endl;
            exit(1);
        }
        std::cout << "maxSocket début " << maxSocket << std::endl;

        for (int socketId = 0; socketId <= maxSocket; socketId++) // Check each socket for activity
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
                    int bytesRead = recv(socketId, &VectBuff[0], VectBuff.size() - 1, 0);

                    if (bytesRead <= 0)
                    {
                        close(socketId);
                        FD_CLR(socketId, &activeSockets);
                        // Remove the closed socket from clientSockets vector. Otherwise, the data is sent to a closed socket
                        clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), socketId), clientSockets.end());
                    }
                    else
                    {
                        std::string buffer(VectBuff.begin(), VectBuff.end()); // Converting vectorized buffer to a std::string
                        std::cout << std::endl
                                  << "BUFFER " << std::endl
                                  << buffer << std::endl;

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