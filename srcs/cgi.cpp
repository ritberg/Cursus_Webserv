#include "webserv.hpp"

void mfree(char **f)
{
    int i = 0;

    while(f[i] != 0)
        free (f[i++]);
	free (f);
}

std::string ServerSocket::executeCGIScript(const std::string &shebang, const std::string &cgiScriptPath, const std::string &body, const std::string &filename)
{
	std::string response_data;
	std::cout << shebang << "|" << cgiScriptPath << "|" << body <<  "|" << filename << std::endl;
	int stdin_pipe[2];
	int stdout_pipe[2];
	char **argv = (char **)malloc(sizeof(char *) * 3);
	char **envp;

	std::string path;
	std::map<std::string, std::string>::iterator it = currentServ.getServConf("web_root");
	if (it != currentServ.getConfEnd())
		path = it->second + cgiScriptPath;
	else
		path = cgiScriptPath;
	argv[0] = strdup(shebang.c_str());
	argv[1] = strdup(path.c_str());
	argv[2] = 0;
	if (filename[0] == 0)
	{
		envp = (char **)malloc(sizeof(char *) * 7);
		envp[0] = strdup("REQUEST_METHOD=GET");
		envp[1] = strdup("CONTENT_LENGTH");
		envp[2] = strdup("PATH_INFO");
		envp[3] = strdup("PATH_TRANSLATED");
		envp[4] = strdup("QUERY_STRING)");
		envp[5] = strdup("SERVER_NAME");
		envp[6] = 0;
	}
	else
	{
		envp = (char **)malloc(sizeof(char *) * 8);
		std::string tmp = "FILENAME=" + filename;
		envp[0] = strdup("REQUEST_METHOD=POST");
		envp[1] = strdup("CONTENT_LENGTH");
		envp[2] = strdup("PATH_INFO");
		envp[3] = strdup("PATH_TRANSLATED");
		envp[4] = strdup("QUERY_STRING)");
		envp[5] = strdup("SERVER_NAME");
		envp[6] = strdup(tmp.c_str());
		envp[7] = 0;
	}
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

		execve(argv[0], argv, envp);
		perror("In execve");
		exit(EXIT_FAILURE);
		return "";
	}
	else
	{
		close(stdin_pipe[0]);
		close(stdout_pipe[1]);
		write(stdin_pipe[1], body.c_str(), body.size());
		close(stdin_pipe[1]);

		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));

		response_data.clear();
		response_data.append("HTTP/1.1 200 OK\r\n\r\n");

		int bytes_read;
		while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			response_data.append(buffer, bytes_read);
			memset(buffer, 0, bytes_read);
		}
		if (bytes_read < 0 && errno != EWOULDBLOCK)
		{
			perror("error in read");
			exit(EXIT_FAILURE);
		}

		int status;
		waitpid(pid, &status, 0);
		mfree(argv);
		mfree(envp);
		status = status / 256;
		std::cout << "status = " << status << std::endl;
		if (status == 1)
			return (callErrorFiles(404));
		if (status == 255)
			return (callErrorFiles(418));
	}
	return response_data;
}