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
	std::cout << shebang << "|" << cgiScriptPath << "|" << body <<  "|" << filename << std::endl;
	int stdin_pipe[2];
	int stdout_pipe[2];
	char **argv = (char **)malloc(sizeof(char *) * 3);
	char **envp;

	std::string path = "/Users/joerober/code/webserv/rendu" + cgiScriptPath;
	argv[0] = strdup(shebang.c_str());
	argv[1] = strdup(path.c_str());
	argv[2] = 0;
	if (filename[0] == 0)
	{
		envp = (char **)malloc(sizeof(char *) * 2);
		envp[0] = strdup("REQUEST_METHOD=GET");
		envp[1] = 0;
	}
	else
	{
		envp = (char **)malloc(sizeof(char *) * 3);
		std::string tmp = "FILENAME=" + filename;
		envp[0] = strdup("REQUEST_METHOD=POST");
		envp[1] = strdup(tmp.c_str());
		envp[2] = 0;
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

		char buffer[100];
		memset(buffer, 0, sizeof(buffer));

		std::string response_data;
		response_data.clear();

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
		std::cout << "response_data " << response_data << std::endl;

		return response_data;
	}
}