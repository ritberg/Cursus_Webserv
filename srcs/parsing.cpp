#include "webserv.hpp"

std::map<int, std::string> ServerSocket::parseFileInfo(std::string path)
{
	struct stat s;
	std::string path_cpy;
	std::string tmp;
	std::map<int, std::string> response;

	for (size_t k = 0; k < server_location.size(); k++)
	{
	 	std::map<std::string, std::string>::iterator it = server_location[k].find("location");
		if (it != server_location[k].end())
		{
			if (it->second == path)
			{
				it = server_location[k].find("redirect");
				if (it != server_location[k].end())
				{
					response[1] = "HTTP/1.1 302 Found\r\nLocation: " + it->second + "\r\n\r\n";
					return (response);
				}
			}
		}
	}
	path_cpy = path;
	if (path_cpy[path_cpy.length() - 1] != '/')
		path_cpy.append("/");
	std::map<std::string, std::string>::iterator it = server_config.find("web_root");
	if (it != server_config.end())
		path = it->second + path;
	if (path_cpy.length() == 1)
		tmp = "/";
	else
		tmp = path_cpy.substr(0, path_cpy.find_first_of("/", 1) + 1);
	for (size_t j = 0; j < server_location.size(); j++)
	{
	 	it = server_location[j].find("location");
		if (it != server_location[j].end())
		{
			if (tmp == it->second)
			{
				it = server_location[j].find("deny");
				if (it != server_location[j].end())
				{
					if (it->second == "all")
					{
						it = server_location[j].find("return");
						if (it != server_location[j].end())
						{
							if (it->second.find_first_not_of("0123456789") == std::string::npos)
							{
								response[1] = callErrorFiles(stoi(it->second));
								return (response);
							}
						}
					}
				}
			}
		}
	}
	if (stat(path.c_str(), &s) == 0)
	{
		if(s.st_mode & S_IFDIR)
		{
			for (size_t i = 0; i < server_location.size(); i++)
			{
	 			it = server_location[i].find("location");
				if (it != server_location[i].end())
				{
					if (it->second == path_cpy)
					{
						it = server_location[i].find("autoindex");
						if (it != server_location[i].end())
						{
							if (it->second == "on")
							{
								std::map<std::string, std::string>::iterator it2 = server_config.find("index");
								if (it2 != server_config.end())
								{
									if (path_cpy.length() == 1)
										path = path + it2->second;
									else
										path = path + "/" + it2->second;		
								}
								else
								{
									if (path_cpy.length() == 1)
										path = path + "index.html";
									else
										path = path + "/index.html";
								}
								response[0] = path;
								return (response);
							}
						}
					}
				}
			}
			if (path_cpy != "/")
				response[1] = callErrorFiles(403);
		}
	}
	response[2] = path;
	return(response);
}

void ServerSocket::parseLocation(const std::vector<std::string> &tmpLine, int index)
{
	std::map<std::string, std::string> tmp;
	for (size_t i = 0; i < tmpLine.size(); i++)
	{
		std::string toRead = tmpLine[i];
		std::istringstream iss(toRead);
		std::string key, value;
		if (iss >> key)
		{
			if (iss >> value)
			{
				if (key == "}")
					break;
				tmp[key] = value;
			}
			else
			{
				std::cerr << "Error in location conf" << std::endl;
				exit(1);
			}
			if (iss >> value && i != 0)
			{
				std::cerr << "Error: too many arguments in one line in conf" << std::endl;
				exit(1);
			}
		}
		else
		{
			std::cerr << "Error in location conf" << std::endl;
			exit(1);
		}
		std::cout << "index: " << index << " "
				  << "key: " << key << " "
				  << "value: " << value << std::endl;
	}
	server_location.push_back(tmp);
	// nb_locations = index + 1; // how many locations in config file
}

void ServerSocket::readConfigFile(const std::string &configFile)
{
	int index = 0;
	int trigger;
	int bracket_counter = 0;
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
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string key, value;
		if (line.find("server") == 0)
		{
			if (line.find("{") != std::string::npos)
				bracket_counter++;
			else
			{
				std::cerr << "Error: wrong config format" << std::endl;
				exit(1);
			}
			continue;
		}
		if (iss >> key)
		{
			if (key == "}")
				;
			else if (key == "listen")
			{
				if (iss >> value)
					ports.push_back(stoi(value));
				else
				{
					std::cerr << "Error in ports config" << std::endl;
					exit(1);
				}
				if (iss >> value)
				{
					std::cerr << "Error: too many arguments in one line in conf" << std::endl;
					exit(1);
				}
			}
			else if (key == "error_page")
			{
				if (iss >> key)
				{
					if (iss >> value)
					{
						server_error[key] = value;
						std::cout << "server_error key: " << value << "server_error value: " << key << std::endl;
					}
					else
					{
						std::cerr << "Error: wrong error_page format" << std::endl;
						exit(1);
					}
				}
				else
				{
					std::cerr << "Error: wrong error_page format" << std::endl;
					exit(1);
				}
				if (iss >> value)
				{
					std::cerr << "Error: too many arguments in one line in conf" << std::endl;
					exit(1);
				}
			}
			else if (key == "location")
			{
				trigger = 0;
				std::vector<std::string> tmpLine;
				if (line.find("{") == std::string::npos)
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
				tmpLine.push_back(line);
				while (std::getline(file, line))
				{
					if (line.find("{") != std::string::npos)
					{
						std::cerr << "Error: wrong config format" << std::endl;
						exit(1);
					}
					if (line.find("}") != std::string::npos)
					{
						bracket_counter++;
						trigger = 1;
						break;
					}
					tmpLine.push_back(line);
				}
				if (trigger == 0)
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
				parseLocation(tmpLine, index);
				index++;
			}
			else
			{
				if (iss >> value)
					server_config[key] = value;
				else
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
				if (iss >> value)
				{
					std::cerr << "Error: too many arguments in one line in conf" << std::endl;
					exit(1);
				}
			}
		}
		std::cout << line << std::endl;
		if (line.find("{") != std::string::npos || line.find("}") != std::string::npos)
			bracket_counter++;
		// std::cout << "key: " << key << " " << "value: " << value << std::endl;
	}
	std::cout << bracket_counter << std::endl;
	if (bracket_counter % 2 != 0 || bracket_counter == 0)
	{
		std::cerr << "Error: wrong config format" << std::endl;
		exit(EXIT_FAILURE);
	}
}