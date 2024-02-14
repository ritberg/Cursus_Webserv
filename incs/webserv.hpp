#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 100000
#define MAX_CLIENTS 32

class ServerSocket
{
	private:
		// int server_fd;
		std::vector<int> server_fds;
		std::vector<int> ports;
    	int max_socket;
		struct sockaddr_in server_addr;
		// std::vector<int> client_sockets;
		fd_set active_sockets, ready_sockets;
    	/* char buffer[MAX_BUFFER_SIZE]; */
		std::map<std::string, std::string> server_config;
		//std::map< int, std::pair<std::string, std::string> > server_location;
		//int nb_locations;
		std::vector<std::map<std::string, std::string> > server_location;
		std::vector<std::string> uploaded_files;

		bool _check(int socket_ID);//
		int _receive(int socket_ID, std::string & buffer); //
		int _respond(int socket_ID, std::string & buffer); //

	public:
		ServerSocket();
		ServerSocket(const ServerSocket &copy);
		ServerSocket &operator=(const ServerSocket &copy);
		~ServerSocket();


		void Init(const std::string &configFile);
		void Loop(bool end); //
		void readConfigFile(const std::string &configFile);
		void parseLocation(const std::vector<std::string>& tmpLine, int index);
		std::string getFileInfo(std::string path, int type);
		std::string handleHttpRequest(std::string &buffer);
		std::string handleDeleteRequest(const std::string& path);
		std::string handlePostRequest(const std::string &path, const std::string &buffer);
		std::string handleGetRequest(const std::string &path, const std::string &buffer);
		std::string executeCGIScript(const std::string &shebang, const std::string &cgiScriptPath, const std::string &body, const std::string &filename);
};