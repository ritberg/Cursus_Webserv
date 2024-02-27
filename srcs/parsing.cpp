#include "webserv.hpp"

bool checkValue(const std::string value)
{
	try{
		if (value.find_first_not_of("0123456789") != std::string::npos)
		{
			std::cerr << "Error: wrong characters in config detected" << std::endl;
			exit(1);
		}
		stod(value);
		return true;
	}
	catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
		exit(1);
    }
	return false;
}

std::map<int, std::string> ServerSocket::parseFileInfo(std::string path)
{
	struct stat s;
	std::string path_cpy;
	std::string tmp;
	std::map<int, std::string> response;
	std::map<std::string, std::string>::iterator it;

	for (int k = 0; k < currentServ.getLocationSize(); k++)
	{
	 	it = currentServ.getServLocation(k, "location");
		if (it != currentServ.getLocationEnd(k))
		{
			if (it->second == path)
			{
				it = currentServ.getServLocation(k, "redirect");
				if (it != currentServ.getLocationEnd(k))
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
	it = currentServ.getServConf("web_root");
	if (it != currentServ.getConfEnd())
		path = it->second + path;
	if (path_cpy.length() == 1)
		tmp = "/";
	else
		tmp = path_cpy.substr(0, path_cpy.find_first_of("/", 1) + 1);
	for (int j = 0; j < currentServ.getLocationSize(); j++)
	{
	 	it = currentServ.getServLocation(j, "location");
		if (it != currentServ.getLocationEnd(j))
		{
			if (tmp == it->second)
			{
				it = currentServ.getServLocation(j, "deny");
				if (it != currentServ.getLocationEnd(j))
				{
					if (it->second == "all")
					{
						it = currentServ.getServLocation(j, "return");
						if (it != currentServ.getLocationEnd(j))
						{
							response[1] = callErrorFiles(stoi(it->second));
							return (response);
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
			for (int i = 0; i < currentServ.getLocationSize(); i++)
			{
	 			it = currentServ.getServLocation(i, "location");
				if (it != currentServ.getLocationEnd(i))
				{
					if (it->second == path_cpy)
					{
						it = currentServ.getServLocation(i, "autoindex");
						if (it != currentServ.getLocationEnd(i))
						{
							if (it->second == "on")
							{
								std::map<std::string, std::string>::const_iterator it = currentServ.getServConf("index");
								if (it != currentServ.getConfEnd())
								{
									if (path_cpy.length() == 1)
										path = path + it->second;
									else
										path = path + "/" + it->second;		
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

void ServerSocket::parseLocation(const std::vector<std::string> &tmpLine, int index, int ind_serv)
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
				if (key == "return")
				{
					if (!checkValue(value))
					{
						std::cerr << "Error: wrong config format - code error" << std::endl;
						exit(1);
					}
				}
				if (key == "}")
					break;
				if (key == "allow")
					tmp[value] = key;
				else
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
	server[ind_serv].setServLocation(tmp);
	// nb_locations = index + 1; // how many locations in config file
}

void ServerSocket::readConfigFile(const std::string &configFile)
{
	int index = 0;
	int ind_serv = -1;
	servers tmp;
	int inside = 0;
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
			if (inside == 1)
			{
				std::cerr << "Error: wrong config format" << std::endl;
				exit(1);
			}
			inside = 1;
			ind_serv++;
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
			{
				inside = 0;
			}
			else if (key == "listen")
			{
				if (inside == 0)
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
				if (iss >> value)
				{
					if (checkValue(value))
					{
						if (stod(value) >= 1024 && stod(value) <= 65535)
							server[ind_serv].setPorts(stod(value));
						else
						{
							std::cerr << "Error in ports config" << std::endl;
							exit(1);
						}
					}
				}
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
				if (inside == 0)
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
				if (iss >> key)
				{
					if (iss >> value)
					{
						if (!checkValue(key))
						{
							std::cerr << "Error: wrong config format" << std::endl;
							exit(1);
						}
						server[ind_serv].setServError(key, value);
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
				if (inside == 0)
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
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
				parseLocation(tmpLine, index, ind_serv);
				index++;
			}
			else
			{
				if (inside == 0)
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
				if (iss >> value)
					server[ind_serv].setServConf(key, value);
				else
				{
					std::cerr << "Error: wrong config format" << std::endl;
					exit(1);
				}
				if (key == "client_max_body_size")
				{
					if (!checkValue(value))
					{
						std::cerr << "Error: wrong config format" << std::endl;
						exit(1);
					}
				}
				if (iss >> value)
				{
					std::cerr << "Error: too many arguments in one line in conf" << std::endl;
					exit(1);
				}
			}
		}
		if (line.find("{") != std::string::npos || line.find("}") != std::string::npos)
			bracket_counter++;
	}
	if (bracket_counter % 2 != 0 || bracket_counter == 0)
	{
		std::cerr << "Error: wrong config format" << std::endl;
		exit(EXIT_FAILURE);
	}
	file.close();
}