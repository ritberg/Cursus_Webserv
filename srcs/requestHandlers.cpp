#include "webserv.hpp"

std::string ServerSocket::getFileInfo(std::string path, int type)
{
	FILE * fin;
	std::vector<char> bufferFile;
	std::string response;
	std::string path_cpy = path;
	int trigger = 0;

	std::map<int, std::string> tmp = parseFileInfo(path);
	std::map<int, std::string>::iterator it;
	it = tmp.begin();
	int return_value = it->first;
	if (return_value == 1)
		return (it->second);
	path = it->second;
	if (type == 0 || type == -1)
	{
		if (path_cpy.length() == 1 && return_value == 2)
		{
			for (int i = 0; i < currentServ.getLocationSize(); i++)
			{
				std::map<std::string, std::string>::iterator it = currentServ.getServLocation(i, "location");
				if (it != currentServ.getLocationEnd(i))
				{
					if (it->second == "/")
					{
						std::map<std::string, std::string>::iterator it = currentServ.getServLocation(i, "default_file");
						if (it != currentServ.getLocationEnd(i))
						{
							path = path + "/" + it->second;
							trigger = 1;
						}
					}
				}
			}
			if (trigger == 0)
				path = path + "/index.html";
			std::cout << "PATH = " << path << std::endl;
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
		std::map<std::string, std::string>::iterator it;
		it = currentServ.getServError(std::to_string(404));
		if (it != currentServ.getErrorEnd())
		{
			std::string tmp = it->second;
			std::map<std::string, std::string>::iterator it2;
			it2 = currentServ.getServConf("web_root");
			if (it2 != currentServ.getConfEnd())
				tmp = it2->second + it->second;
			fin = fopen(tmp.c_str(), "rb");
			if (fin == NULL)
				return (buildErrorFiles("404 Not Found"));
		}
		return (callErrorFiles(404));
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
	//assembly of the response
	response = "HTTP/1.1 200 OK\r\n\r\n" + content;
	return (response);
}

std::string ServerSocket::handleGetRequest(const std::string &path, const std::string &buffer)
{
	std::string response;

	if (path.find(".php") != std::string::npos)
		return executeCGIScript("/usr/bin/php", path, "", "");
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

	std::map<std::string, std::string>::iterator it;
	it = currentServ.getServConf("client_max_body_size");
	if (it != currentServ.getConfEnd())
	{
		int len = buffer.length();
		if (len > stoi(it->second))
			return (callErrorFiles(413));
	}
	int test;
	if (buffer.find("Content-Length: ") == std::string::npos)
		return(callErrorFiles(411));
	test = buffer.find("Content-Length: ");
	std::string stringtest = buffer.substr(test + 16, 1);
	if (stoi(stringtest) == 0)
		return (callErrorFiles(400));

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
		outfile << body; // Put the body of the uploaded file into the folder
		outfile.close();

		return handleGetRequest(path, buffer) + "\nSuccessfully uploaded!<br>";
	}
	if (path.find(".php") != std::string::npos)
		return(executeCGIScript("/usr/bin/php", path, body, filename));
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
	std::string method, path, line, protocol, path_cpy;
	request >> method >> path >> protocol;
	int trigger = 0;
	int trigger2 = 0;
	path_cpy = path;

	while (trigger2 == 0)
	{
		for (int i = 0; i < currentServ.getLocationSize(); i++)
		{
			std::map<std::string, std::string>::iterator it = currentServ.getServLocation(i, "location");
			if (!((it->second.substr(0, it->second.rfind("/")) + "/").compare(path_cpy.substr(0, path_cpy.rfind("/")) + "/")))
			{
				trigger2 = 1;
				for (it = currentServ.getLocationBegin(i); it != currentServ.getLocationEnd(i); it++)
				{
					if (it->second == "allow" && it->first == method)
						trigger = 1;
				}
			}
		}
		if (path_cpy.length() == 1)
			break;
		if (trigger == 0)
		{
			int pos = path_cpy.length();
			pos--;
			while (path_cpy[pos] != '/')
				pos--;
			path_cpy = path_cpy.substr(0, pos);
		}
	}

	if (trigger != 1)
		return (callErrorFiles(405));
	if (protocol != "HTTP/1.1")
		return (callErrorFiles(505));
	if (method == "GET")
		return handleGetRequest(path, buffer);
	else if (method == "POST")
		return handlePostRequest(path, buffer);
	else if (method == "DELETE")
		return handleDeleteRequest(path);
	return (callErrorFiles(501));
}
