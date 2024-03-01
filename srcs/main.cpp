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
	read_sockets = copy.read_sockets;
	write_sockets = copy.write_sockets;
	// server_config = copy.server_config;

	return (*this);
}

ServerSocket::~ServerSocket()
{
	delete[] server;
	delete[] readBuffer;
	std::cout << "memory was freed" << std::endl;
}

void ServerSocket::Init(const std::string &configFile)
{
	std::ifstream file(configFile);
	if (!file.is_open())
	{
		std::cerr << "Error opening configuration file" << std::endl;
		exit(1);
	}
	if (file.peek() == EOF)
	{
		std::cout << "Error: configuration file empty" << std::endl;
		exit(1);
	}
	std::string line;
	servSize = 0;
	while (std::getline(file, line))
	{
		if (line.find("server") != std::string::npos && line.find("{") != std::string::npos)
			servSize++;
	}
	file.close();
	if (servSize < 1)
	{
		std::cerr << "Configuration file needs at least one Server" << std::endl;
		exit(1);
	}
	try{
		this->server = new servers[servSize];
	}
	catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
		exit(1);
    }

	readConfigFile(configFile);

	servPortsCount = 0;
	for (int j = 0; j < servSize; ++j)
	{
		if (server[j].getPorts().size() == 0)
		{
			std::cerr << "Error: no ports found" << std::endl;
			exit(1);
		}
	}

	FD_ZERO(&active_sockets);
	for (int j = 0; j < servSize; ++j)
	{
		for (size_t i = 0; i < server[j].getPorts().size(); ++i)
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
			server_addr.sin_port = htons(server[j].getPorts()[i]);

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
			servPortsCount++;
		}
	}

	std::cout << "----Awaiting connections on port(s): ";
	for (int j = 0; j < servSize; ++j)
	{
		for (size_t i = 0; i < server[j].getPorts().size(); ++i)
			std::cout << server[j].getPorts()[i] << " ";
	}
	std::cout << "----" << std::endl << std::endl;
	// readBuffer = (char **)malloc(sizeof(char *) * MAX_CLIENTS);
	// if (readBuffer = NULL)
	// {
	// 	std::cerr << "malloc failed" << std::endl;
	// 	exit(1);
	// }
	// for (int k = 0; k < MAX_CLIENTS; k++)
	// {
	// 	readBuffer[k] = (char *)malloc(sizeof(char) * 1024);
	// 	if (readBuffer[k] = NULL)
	// 	{
	// 		std::cerr << "malloc failed" << std::endl;
	// 		exit(1);
	// 	}
	// }
	if (!(MAX_CLIENTS > 0 && MAX_CLIENTS < 1025))
	{
		std::cerr << "Please enter between 1 and 1024 clients" << std::endl;
		exit(1);
	}
	try{
		readBuffer = new std::string[MAX_CLIENTS + 2];
	}
	catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
		exit(1);
    }
	bool end = false;
	while (1)
		Loop(end);
}

bool ServerSocket::_check(int socket_ID)
{
	std::vector<int>::iterator it;
	for (it = server_fds.begin(); it != server_fds.end(); ++it)
	{
		if (socket_ID == *it)
			return true;
	}
	return false;
}

int ServerSocket::_receive(int socket_ID)
{
	int socket_ID_tmp = socket_ID - 3 - servPortsCount; 
	//buffer.clear();
	if (socket_ID_tmp > MAX_CLIENTS + 1)
		return 0;
	std::cout << "SOCKET RECEIVE = " << socket_ID_tmp << std::endl;
	int bytesRead;
	int trigger = 0;
	char tmpBuffer[100];
	usleep(50);
	memset(tmpBuffer, 0, sizeof(tmpBuffer));
	bytesRead = recv(socket_ID, tmpBuffer, sizeof(tmpBuffer), MSG_PEEK);
	memset(tmpBuffer, 0, sizeof(tmpBuffer));
	if (bytesRead <= 99)
		trigger =  1;
	bytesRead = recv(socket_ID, tmpBuffer, sizeof(tmpBuffer) - 1, MSG_DONTWAIT);
	if (bytesRead > 0)
	{
		tmpBuffer[bytesRead] = '\0';
		readBuffer[socket_ID_tmp] += std::string(tmpBuffer, bytesRead);
		if (trigger == 1)
		 	return -1;
	}
	else if (bytesRead == 0)
	{
		std::cout << "Connection was closed" << std::endl;
		return 0;
	}
	else
		return -1;
	//std::cout << "buffer " << buffer << std::endl;
	return bytesRead;
}

int ServerSocket::_respond(int socket_ID)
{
	int socket_ID_tmp = socket_ID - 3 - servPortsCount; 
    // std::cout << "buffer " << readBuffer[socket_ID_tmp] << std::endl;
	if (socket_ID_tmp > MAX_CLIENTS + 1)
		return -1;
	if (readBuffer[socket_ID_tmp].length() == 0)
		return -1;
	std::cout << "SOCKET RESPOND = " << socket_ID_tmp << std::endl;
    std::cout << "length " << readBuffer[socket_ID_tmp].length() << std::endl;
	serverName = getLastPart(readBuffer[socket_ID_tmp], "Host: ");
    int ret;
    std::string response;
    std::string host;
    size_t pos = readBuffer[socket_ID_tmp].find("Host: ");
    if (pos != std::string::npos)
    {
        pos += 6;
        std::cout << "host = " << readBuffer[socket_ID_tmp][pos] << std::endl;
        while (readBuffer[socket_ID_tmp][pos] != ':')
            pos++;
        pos++;
        int limit = pos;
        while (readBuffer[socket_ID_tmp][limit] != '\r')
            limit++;
        limit -= pos;
        std::cout << "limit = " << limit << std::endl;
        host = readBuffer[socket_ID_tmp].substr(pos, limit);
    }
    if (host.empty())
        return -1;
    std::cout << "host = " << host << std::endl;
    for (int j = 0; j < servSize; ++j)
    {
        for (size_t i = 0; i < server[j].getPorts().size(); ++i)
        {
            if (server[j].getPorts()[i] == stod(host))
            {
                currentServ = server[j];
                break;
            }
        }
    }
	currentSocket = socket_ID;
    if (readBuffer[socket_ID_tmp].length() == 0)
        response = callErrorFiles(400);
    else
        response = handleHttpRequest(readBuffer[socket_ID_tmp]);
    ret = send(socket_ID, response.c_str(), response.size(), 0);
	readBuffer[socket_ID_tmp].clear();
    return ret;
}

void ServerSocket::Loop(bool end)
{
	int ret, ready = 0, new_sd;
	bool close_connection;
	// int flag = 0;

	//std::string buffer;
	read_sockets = active_sockets;
	// FD_ZERO(&read_sockets);
	FD_ZERO(&active_write);
	while (!end)
	{
		//usleep(1000);
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_ZERO(&read_sockets);
		FD_ZERO(&write_sockets);
		// memcpy(&read_sockets, &active_sockets, sizeof(active_sockets));
		// memcpy(&write_sockets, &active_write, sizeof(active_write));
		read_sockets = active_sockets;
		write_sockets = active_write;
		ret = select(max_socket + 1, &read_sockets, &write_sockets, NULL, &tv);
		if (ret == -1)
		{
			std::cerr << "Error in select(): " << strerror(errno) << std::endl;
			return;
		}
		std::cout << "max socket 1 = " << max_socket << std::endl;
		if (ret == 0)
			continue;
		ready = ret;
		std::cout << "max_socket début " << max_socket << std::endl;
		std::cout << std::endl
				  << "==== WAITING ====" << std::endl;
		for (int socket_ID = 0; socket_ID <= max_socket; socket_ID++)
		{
			if (FD_ISSET(socket_ID, &read_sockets) || FD_ISSET(socket_ID, &write_sockets))
			{
				//ready = -1;
				if (_check(socket_ID))
				{
					// struct sockaddr_in clients;
					// socklen_t clientsize = sizeof(clients);
					// new_sd = accept(socket_ID, (struct sockaddr *)&clients, &clientsize);
					new_sd = accept(socket_ID, NULL, NULL);
					std::cout << "NEW_SD = " << new_sd << std::endl;
					if (new_sd < 0)
					{
						if (errno != EWOULDBLOCK)
						{
							std::cerr << "Error accepting client connection" << std::endl;
							end = true;
						}
						break;
					}
					FD_SET(new_sd, &active_sockets);
					max_socket = (new_sd > max_socket) ? new_sd : max_socket;
				}
				else
				{
					close_connection = false;
					if (FD_ISSET(socket_ID, &read_sockets))
					{
						std::cout << "in read" << std::endl;
						ret = _receive(socket_ID);
						std::cout << "ret " << ret << std::endl;
						// flag++;
						if (ret <= 0)
						{
							FD_CLR(socket_ID, &active_sockets);
							if (ret == -1)
								FD_SET(socket_ID, &active_write);
							else if (ret == 0)
								close(socket_ID);
							//close_connection = true;
						}
					}
					else if (FD_ISSET(socket_ID, &write_sockets))
					{
						std::cout << "in write" << std::endl;
						ret = _respond(socket_ID);
						if (ret == -1)
						{
							std::cerr << "Send() error" << std::endl;
							close_connection = true;
						}
						else if (ret == 0)
						{
							std::cerr << "No data was sent" << std::endl;
							close_connection = true;
						}
						FD_CLR(socket_ID, &active_write);
						close_connection = true;
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
	// for (int i = 0; i <= max_socket; ++i)
	// {
	// 	if (FD_ISSET(i, &active_sockets))
	// 		close(i);
	// }
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
