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
	// client_sockets = copy.client_sockets;
	active_sockets = copy.active_sockets;
	ready_sockets = copy.ready_sockets;
	server_config = copy.server_config;

	return (*this);
}

ServerSocket::~ServerSocket() {}

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

	bool end = false;
	while (1)
		Loop(end);
}

bool ServerSocket::_check(int socket_ID)
{
	for (std::vector<int>::iterator it = server_fds.begin(); it != server_fds.end(); ++it)
	{
		if (socket_ID == *it)
			return true;
	}
	return false;
}

int ServerSocket::_receive(int socket_ID, std::string & buffer)
{
	buffer.clear();
	int bytesRead;
	char tmpBuffer[100];
	memset(tmpBuffer, 0, sizeof(tmpBuffer));
	do {
		bytesRead = recv(socket_ID, tmpBuffer, sizeof(tmpBuffer) - 1, MSG_DONTWAIT);
		if (bytesRead > 0)
		{
			tmpBuffer[bytesRead] = '\0';
			buffer += std::string(tmpBuffer, bytesRead);
		}
		else if (bytesRead == 0)
		{
			std::cout << "Connection was closed" << std::endl;
			return 0;
		}
		// else if (bytesRead < 0 && errno != EWOULDBLOCK) // pas droit a l'errno !
		// {
		// 	perror("error in read");
		// 	exit(10);
		// }
	} while (bytesRead > 0);
	std::cout << "buffer " << buffer << std::endl;
	return bytesRead;
}

int ServerSocket::_respond(int socket_ID, std::string & buffer)
{
	int ret;
	std::string response;
	if (buffer.length() == 0)
		response = callErrorFiles(400);
	else
		response = handleHttpRequest(buffer);
	ret = send(socket_ID, response.c_str(), response.size(), 0);
	return ret;
}

void ServerSocket::Loop(bool end)
{
	//std::cout << "server fds= " << server_fds[0] << ", " << server_fds[1] << std::endl;
	int ret = 0, ready = 0, new_sd;
	bool close_connection;
	int flag = 0;
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	std::string buffer;
	ready_sockets = active_sockets;
	while(!end)
	{
		memcpy(&ready_sockets, &active_sockets, sizeof(active_sockets));
		ret = select(max_socket + 1, &ready_sockets, NULL, NULL, &tv);
		if (ret == -1)
		{
			std::cerr << "Error in select(): " << strerror(errno) << std::endl;
			exit(1);
		}
		ready = ret;
		std::cout << "max_socket début " << max_socket << std::endl;
		std::cout << std::endl << "==== WAITING ====" << std::endl;
		for (int socket_ID = 0; socket_ID <= max_socket && ready > 0; socket_ID++)
		{
			if (FD_ISSET(socket_ID, &ready_sockets))
			{
				ready = -1;
				if (_check(socket_ID))
				{
					while (1)
					{
						new_sd = accept(socket_ID, NULL, NULL);
						if (new_sd < 0)
						{
							if (errno != EWOULDBLOCK)
							{
									std::cerr << "Error accepting client connection" << std::endl;
									end = true;
							}
							break;
						}
						std::cout << "server socket: " << new_sd << std::endl;
						FD_SET(new_sd, &active_sockets);
						max_socket = (new_sd > max_socket) ? new_sd : max_socket;
						if (new_sd == -1)
							break;
					}
				}
				else
				{
					close_connection = false;
					while (1)
					{
						ret = _receive(socket_ID, buffer);
						std::cout << "ret " << ret << std::endl;
						flag++;
						if (ret <= 0 && flag != 1)
						{
							close_connection = true;
							break;
						}
						ret = _respond(socket_ID, buffer);
						if (ret == -1)
						{
							std::cerr << "Send() error" << std::endl;
							close_connection = true;
							break;
						}
					}
					if (close_connection)
					{
						close(socket_ID);
						FD_CLR(socket_ID, &active_sockets);
						if (socket_ID == max_socket)
						{
							while (!FD_ISSET(max_socket, &active_sockets))
								max_socket -= 1;
						}
						return;
					}
				}
			}
		}
	}
	for (int i = 0; i <= max_socket; ++i)
	{
		if (FD_ISSET(i, &active_sockets))
			close(i);
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
		ss.Init("tools/conf/config.txt");
	}
	else
		std::cout << "Wrong number of arguments" << std::endl;
	return (EXIT_FAILURE);
}