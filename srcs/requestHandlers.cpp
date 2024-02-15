#include "webserv.hpp"

std::string ServerSocket::getFileInfo(std::string path, int type)
{
	FILE * fin;
	struct stat s;
	std::vector<char> bufferFile;
	std::string response;
	std::string path_cpy;

	path_cpy = path;
	std::map<std::string, std::string>::iterator it = server_config.find("web_root");
	if (it != server_config.end())
		path = it->second + path;
	if (stat(path.c_str(), &s) == 0)
	{
		if(s.st_mode & S_IFDIR)
		{
			if (path_cpy[path_cpy.length() - 1] != '/')
				path_cpy.append("/");
			for (size_t i = 0; i < server_location.size(); i++)
			{
	 			std::map<std::string, std::string>::iterator it = server_location[i].find("location");
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
								path_cpy = "not / anymore";
							}
						}
					}
				}
			}
		}
	}
	std::cout << std::endl << path << std::endl << std::endl;
	//if the type is html, the file might be "", meaning it is the index
	if (type == 0 || type == -1)
	{
		if (path_cpy.length() == 1)
		{
			path = path + "/home.html";
			fin = fopen(path.c_str(), "rb");
		}
		else
			fin = fopen(path.c_str(), "rb");
	}
	else
		fin = fopen(path.c_str(), "rb");
	if (fin == NULL)
	{
		//if the file doesn't exist, a special file is made to display error 404
		if (type == -1)
			return (buildErrorFiles("404 Not Found"));
		if (type == 0)
			return (callErrorFiles(404));
		else
			return ("HTTP/1.1 404 Not Found\r\n\r\n");
	}
	//here we calculate the size of the file
	fseek(fin, 0, SEEK_END);
	long file_len = ftell(fin);
	rewind(fin);
	//the characters are placed in a vector because we need an array and character array interpret some characters
	bufferFile.clear();
	bufferFile.resize(file_len);
	fread(&bufferFile[0], 1, file_len, fin);
	fclose(fin);
	std::string content(bufferFile.begin(), bufferFile.end());
	//std::string content_len = std::to_string((content.length() + 20));
	//assembly of the response
	response = "HTTP/1.1 200 OK\r\n\r\n" + content;
	return (response);
}

std::string ServerSocket::handleGetRequest(const std::string &path, const std::string &buffer)
{
	std::string response;

	for (size_t i = 0; i < server_location.size(); i++)
	{
	 	std::map<std::string, std::string>::iterator it = server_location[i].find("location");
		if (it != server_location[i].end())
		{
			if (it->second == path)
			{
				it = server_location[i].find("redirect");
				if (it != server_location[i].end())
				{
					response = "HTTP/1.1 302 Found\r\nLocation: " + it->second + "\r\n\r\n";
					return (response);
				}
			}
		}
	}
	if (path.find(".php") != std::string::npos)
	{
		std::ostringstream oss;
		oss << "HTTP/1.1 200 OK\r\n\r\n";
		// Use CGI script for GET request, just to display the web page
		return oss.str() + executeCGIScript("/usr/bin/php", path, "", "");
	}
	else if (buffer.find("Accept: text/html") != std::string::npos)
		response = getFileInfo(path, 0);
	else
		response = getFileInfo(path, 1);
	return (response);
}

std::string extractFilename(const std::string& header) {
	std::string filename;
	size_t filenamePos = header.find("filename=");
	if (filenamePos != std::string::npos)
	{
		filename = header.substr(filenamePos + 10); // filename =""
		filename = filename.substr(0, filename.find("\""));
	}
	return filename;
}

std::string ServerSocket::handlePostRequest(const std::string& path, const std::string& buffer) {
	std::string body;
	std::string boundary;

	std::map<std::string, std::string>::iterator it = server_config.find("client_max_body_size");
	if (it != server_config.end())
	{
		int len = buffer.length();
		if (len > stoi(it->second))
			return (callErrorFiles(413));
	}

	size_t pos_marker = buffer.find("boundary=");
	if (pos_marker == std::string::npos)
	{
		size_t pos_empty_line = buffer.find("\r\n\r\n");
		std::string extracted_line;
    	if (pos_empty_line != std::string::npos)
		{
        	std::string body = buffer.substr(pos_empty_line + 4);
			std::istringstream body_stream(body);
			std::getline(body_stream, extracted_line);

			std::cout << "hello " << extracted_line << std::endl;
		}
		return ("HTTP/1.1 200 OK\r\n\r\n" + extracted_line);
	}

	size_t end_marker;
	size_t i = pos_marker + std::string("boundary=").length() - 1;

	while (++i < buffer.length() && buffer[i] != '\n')
		boundary.push_back(buffer[i]);
	pos_marker = buffer.find("Content-Type", i);
	i = pos_marker - 1;
	while (++i < buffer.length() && buffer[i] != '\n');
	i += 2;
	end_marker = buffer.find(boundary.substr(0, boundary.length() - 1), i);
	while (++i < buffer.length() && i < end_marker - 2)
		body.push_back(buffer[i]);
	size_t contentDispositPos = buffer.find("Content-Disposition");
	std::string contentDispositHeader = buffer.substr(contentDispositPos, end_marker - contentDispositPos);
	std::string filename = extractFilename(contentDispositHeader);

	uploaded_files.push_back(filename);

	if (path == "/upload.html")
	{
		std::ofstream outfile(("uploaded_files/" + filename).c_str(), std::ios::binary);    // Save the uploaded image with the extracted filename
		if (outfile.fail())
			return "No file was chosen";
		std::cout << "body " << body << std::endl;
		outfile << body; // Put the body of the uploaded file into the folder
		outfile.close();

		return handleGetRequest(path, buffer) + "\nSuccessfully uploaded!<br>";
	}
	if (path.find(".php") != std::string::npos)
	{
		std::string res = executeCGIScript("/usr/bin/php", path, body, filename);
		return ("HTTP/1.1 200 OK\r\n\r\n" + res);
	}
	return "Unsupported HTTP method\n";
}

std::string ServerSocket::handleDeleteRequest(const std::string& path)
{
	if ((path == "/upload.html" || path == "/cgi-bin/cgi.php") && !uploaded_files.empty())
	{
		std::string lastFilename = uploaded_files.back();
		uploaded_files.pop_back();

		if (remove(("uploaded_files/" + lastFilename).c_str()) == 0)
			return "HTTP/1.1 200 Ok\r\n\r\n\nResource deleted successfully!";
	}
	return "HTTP/1.1 404 Not Found\r\n\r\n\nResource not found";
}

std::string ServerSocket::handleHttpRequest(std::string &buffer)
{
	std::istringstream request(buffer);
	std::string method, path, line;
	request >> method >> path;

	if (method == "GET")
		return handleGetRequest(path, buffer);
	else if (method == "POST")
		return handlePostRequest(path, buffer);
	else if (method == "DELETE")
		return handleDeleteRequest(path);
	return "Unsupported HTTP method";
}