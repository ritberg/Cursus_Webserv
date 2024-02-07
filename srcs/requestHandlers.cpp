#include "webserv.hpp"

std::string ServerSocket::getFileInfo(const std::string path, int type)
{
	FILE * fin;
	struct stat s;
	std::vector<char> bufferFile;
	std::string response;
	std::string path_mod;

	path_mod = path;
	path_mod.erase(0, 1);
	if (stat(path_mod.c_str(), &s) == 0)
	{
		if( s.st_mode & S_IFDIR )
			path_mod = path_mod + "/index.html";
	}
	//if the type is html, the file might be "", meaning it is the index
	if (type == 0)
	{
		if (path_mod[0] == 0)
			fin = fopen("tools/home.html", "rb");
		else
			fin = fopen(path_mod.c_str(), "rb");
	}
	else
		fin = fopen(path_mod.c_str(), "rb");
	if (fin == NULL)
	{
		//if the file doesn't exist, a special file is made to display error 404
		if (type == 0)
			fin = fopen("tools/404.html", "rb");
		if (fin == NULL)
		{
			perror("file open error");
			return ("HTTP/1.1 404 Not Found\r\n\r\n");
		}
		type = -1;
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
	if (type >= 0)
		response = "HTTP/1.1 200 OK\r\n\r\n" + content;
	else if (type == -1)
		response = "HTTP/1.1 404 Not Found\r\n\r\n" + content;
	return (response);
}

std::string ServerSocket::handleGetRequest(const std::string &path, const std::string &buffer)
{
	std::string response;

	if (path == "/tools/cgi-bin/cgi.php")
	{
		std::ifstream file("tools/cgi-bin/cgi.php");
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
			return oss.str() + executeCGIScript("/usr/bin/php", scriptContent);
		}
	}
	if (buffer.find("Accept: text/html") != std::string::npos)
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
	size_t pos_marker = buffer.find("boundary=");
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

	std::ofstream outfile(("uploaded_files/" + filename).c_str(), std::ios::binary);    // Save the uploaded image with the extracted filename
	if (outfile.fail())
		return "No file was chosen";

	uploaded_files.push_back(filename);

	if (path == "/tools/upload.html")
	{
		outfile << body; // Put the body of the uploaded file into the folder
		outfile.close();

		return handleGetRequest(path, buffer) + "\nSuccessfully uploaded!<br>";
	}
	if (path == "/tools/cgi-bin/cgi.php")
	{
		std::string res = executeCGIScript("/usr/bin/php", body);
		outfile << res;
		outfile.close();

		return handleGetRequest(path, buffer) + "<label for=\"name\">Successfully uploaded!</label><br>";
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
