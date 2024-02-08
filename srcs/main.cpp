#include "webserv.hpp"

ServerSocket::ServerSocket() : max_socket(0)
{
}

ServerSocket::ServerSocket(const ServerSocket &copy)
{
	*this = copy;
}

ServerSocket &ServerSocket::operator=(const ServerSocket &copy)
{
	for (int i = 0; i < server_fds.size(); i++)
		server_fds[i] = copy.server_fds[i];
	max_socket = copy.max_socket;
	server_addr = copy.server_addr;
	client_sockets = copy.client_sockets;
	client_sockets = copy.client_sockets;
	active_sockets = copy.active_sockets;
	ready_sockets = copy.ready_sockets;
	server_config = copy.server_config;

	return (*this);
}

ServerSocket::~ServerSocket() {}

void ServerSocket::readConfigFile(const std::string &configFile)
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
		{
			if (key == "listen")
				ports.push_back(stoi(value));
			server_config[key] = value;
		}
	}
}

void ServerSocket::Init(const std::string &configFile)
{
	readConfigFile(configFile);

    FD_ZERO(&active_sockets);


    for (size_t i = 0; i < ports.size(); ++i)
    {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            perror("In socket");
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("In setsockopt");
            exit(EXIT_FAILURE);
        }

        fcntl(server_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);

		struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        server_addr.sin_port = htons(ports[i]);

        if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("In bind");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, MAX_CLIENTS) < 0)
        {
            perror("In listen");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        FD_SET(server_fd, &active_sockets);
        server_fds.push_back(server_fd);
        max_socket = std::max(max_socket, server_fd);
    }

    std::cout << "----Awaiting connections on port(s): ";
    for (size_t i = 0; i < ports.size(); ++i)
        std::cout << ports[i] << " ";
    std::cout << "----" << std::endl
              << std::endl;

	while (1)
		Loop();
}

std::string ServerSocket::executeCGIScript(const std::string &cgiScriptPath, const std::string &script_content)
{
	int stdin_pipe[2];
	int stdout_pipe[2];
	if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1)
	{
		perror("In pipe");
		exit(EXIT_FAILURE);
	}
	pid_t pid = fork();
	if (pid == -1)
	{
		perror("In fork");
		exit(EXIT_FAILURE);
	}
	if (pid == 0)
	{
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		dup2(stdin_pipe[0], STDIN_FILENO);
		dup2(stdout_pipe[1], STDOUT_FILENO);

		const char *const argv[] = {cgiScriptPath.c_str(), NULL};
		const char *const envp[] = {"REQUEST_METHOD=POST", "CONTENT_TYPE=application/x-www-form-urlencoded", NULL};

		execve(cgiScriptPath.c_str(), const_cast<char *const *>(argv), const_cast<char *const *>(envp));
		perror("In execve");
		exit(EXIT_FAILURE);
		return "";
	}
	else
	{
		close(stdin_pipe[0]);
		close(stdout_pipe[1]);
		write(stdin_pipe[1], script_content.c_str(), script_content.size());
		close(stdin_pipe[1]);

		char buffer[100000];
		std::string response_data;

		ssize_t bytes_read;
		while ((bytes_read = read(stdout_pipe[0], buffer, BUFSIZ)) > 0)
			response_data.append(buffer, bytes_read);

		int status;
		waitpid(pid, &status, 0);
		std::cout << "response_data " << response_data << std::endl;

		return response_data;
	}
}

void ServerSocket::Loop()
{
	int i = 0;

	std::string buffer;
	buffer.clear();
	ready_sockets = active_sockets;
	if (select(max_socket + 1, &ready_sockets, NULL, NULL, NULL) <= 0)
	{
		std::cerr << "Error in select(): " << strerror(errno) << std::endl;
		exit(1);
	}
	std::cout << "max_socket début " << max_socket << std::endl;

	for (int socket_ID = 0; socket_ID <= max_socket; socket_ID++)
	{
		if (FD_ISSET(socket_ID, &ready_sockets))
		{
			while (i < server_fds.size())
			{
				if (socket_ID == server_fds[i])
				{
					std::cout << "ERROR 2, socket_ID: " << socket_ID << std::endl;
					int client_socket = accept(server_fds[i], NULL, NULL);
					std::cout << "client_sockets accepted " << client_socket << std::endl;
					if (client_socket < 0)
					{
						perror("Error accepting client connection");
						exit(1);
					}
					fcntl(client_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);

					FD_SET(client_socket, &active_sockets);
					max_socket = (client_socket > max_socket) ? client_socket : max_socket;
					client_sockets.push_back(client_socket);
					break;
				}
				i++;
			}
			if (socket_ID != server_fds[i])
			{
				int bytesRead;
				char tmpBuffer[100];
				memset(tmpBuffer, 0 , sizeof(tmpBuffer));
				while ((bytesRead = recv(socket_ID, tmpBuffer, sizeof(tmpBuffer) - 1, 0)) > 0)
				{
					buffer.append(tmpBuffer, bytesRead);
					memset(tmpBuffer, 0, bytesRead);
				}
				if (bytesRead == 0)
				{
					close(socket_ID);
					FD_CLR(socket_ID, &active_sockets);
					// Remove the closed socket from clientSockets vector. Otherwise, the data is sent to a closed socket
					client_sockets.erase(std::remove(client_sockets.begin(), client_sockets.end(), socket_ID), client_sockets.end());
				}
				else if (bytesRead < 0 && errno != EWOULDBLOCK)
				{
					perror("error in read");
					exit(1);
				}
				else
				{
					std::cout << std::endl << "[BUFFER]" << std::endl << buffer << std::endl;
					for (int i = 0; i < client_sockets.size(); i++)
					{
						std::cout << "socket_ID = " << socket_ID << std::endl;
						std::cout << "client_sockets.size() = " << client_sockets.size() << std::endl;
						std::cout << "client_sockets[i] = " << client_sockets[i] << std::endl;
						std::string response = handleHttpRequest(buffer);
						send(client_sockets[i], response.c_str(), response.size(), 0);
						client_sockets.erase(std::remove(client_sockets.begin(), client_sockets.end(), socket_ID), client_sockets.end());
						close(socket_ID);
						FD_CLR(socket_ID, &active_sockets);
					}
				}
			}
		}
	}
}


int main(int argc, char **argv)
{
	if (argc == 2)
	{
		/* Create file existence checker */
		/* if (argv[1] exists) */
		/* 	do ss.Init() */
		/* else */
		/* 	error */
		ServerSocket ss;
		ss.Init(argv[1]);
	}
	else if (argc == 1)
	{
		ServerSocket ss;
		ss.Init("tools/config.txt");
	}
	else
		std::cout << "Wrong number of arguments" << std::endl;
	return (EXIT_FAILURE);
}
