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
	for (size_t i = 0; i < server_fds.size(); i++)
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

void ServerSocket::parseLocation(const std::vector<std::string>& tmpLine, int index)
{
	std::map<std::string, std::string> tmp;
	for (size_t i = 0; i < tmpLine.size(); i++)
	{
		std::string toRead = tmpLine[i];
		std::istringstream iss(toRead);
		std::string key, value;
		if (iss >> key >> value)
		{
			if (key == "}")
				break;
			tmp[key] = value;
		}
		std::cout << "index: " << index << " " <<  "key: " << key << " " << "value: " << value << std::endl;
	}
	server_location.push_back(tmp);
	//nb_locations = index + 1; // how many locations in config file
}

void ServerSocket::readConfigFile(const std::string &configFile)
{
	int index = 0;
	int trigger;
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
			if (key == "location")
			{
				trigger = 0;
				std::vector<std::string> tmpLine;
				if (line.find("{") == std::string::npos)
				{
					std::cerr << "wrong config format" << std::endl;
					exit(1);
				}
				tmpLine.push_back(line);
				while (std::getline(file, line))
				{
					if (line.find("{") != std::string::npos)
					{
						std::cerr << "wrong config format" << std::endl;
						exit(1);
					}
					if (line.find("}") != std::string::npos)
					{
						trigger = 1;
						break;
					}
					tmpLine.push_back(line);
				}
				if (trigger == 0)
				{
					std::cerr << "wrong config format" << std::endl;
					exit(1);
				}
				parseLocation(tmpLine, index);
				index++;
			}
			server_config[key] = value;
		}
		// std::cout << "key: " << key << " " << "value: " << value << std::endl;
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

void ServerSocket::Loop()
{
	size_t i = 0;

	std::string buffer;
	buffer.clear();
	ready_sockets = active_sockets;
	if (select(max_socket + 1, &ready_sockets, NULL, NULL, NULL) <= 0)
	{
		std::cerr << "Error in select(): " << strerror(errno) << std::endl;
		exit(1);
	}
	std::cout << "max_socket dÃ©but " << max_socket << std::endl;

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
			if (i == server_fds.size())
				i--;
			if (socket_ID != server_fds[i])
			{
				std::cout << socket_ID << std::endl;
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
					std::cout << errno << std::endl;
					perror("error in read");
					exit(1);
				}
				else
				{
					std::cout << std::endl << "[BUFFER]" << std::endl << buffer << std::endl;
					for (size_t i = 0; i < client_sockets.size(); i++)
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
		std::ifstream file(argv[1]);
		if (file.is_open())
		{
			file.close();
			ServerSocket ss;
			ss.Init(argv[1]);
		}
		else
			std::cerr << "Error: Unexistant configuration file" << std::endl;
		file.close();
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