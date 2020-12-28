#include "CGIExec.hpp"

const std::string CGIExec::vars_[] = {
								  "AUTH_TYPE=",
								  "CONTENT_LENGTH=",
								  "CONTENT_TYPE=",
								  "GATEWAY_INTERFACE=",
								  "PATH_INFO=",
								  "PATH_TRANSLATED=",
								  "QUERY_STRING=",
								  "REMOTE_ADDR=",
								  "REMOTE_IDENT=",
								  "REMOTE_USER=",
								  "REQUEST_URI=",
								  "REQUEST_METHOD=",
								  "SCRIPT_FILENAME=",
								  "SCRIPT_NAME=",
								  "SERVER_NAME=",
								  "SERVER_PORT=",
								  "SERVER_PROTOCOL=",
								  "SERVER_SOFTWARE="
};

CGIExec::~CGIExec() {
	free(envs_[18]);
}

CGIExec::CGIExec(const Request &r): request_(r) {
	setEnv_(18, "REDIRECT_STATUS=200");
	envs_[19] = 0;
}

void CGIExec::build_(const std::string &workDir, const std::string &filename) {
	setEnv_(CGIExec::AUTH_TYPE, request_.getHeaderAuth());
	setEnv_(CGIExec::CONTENT_LENGTH, ft_itoa(request_.getHeaderContentLength()));
	setEnv_(CGIExec::CONTENT_TYPE, request_.getHeaderContentType());
	setEnv_(CGIExec::GATEWAY_INTERFACE, "CGI/1.1");
	setEnv_(CGIExec::QUERY_STRING, request_.getQueryStr());
	setEnv_(CGIExec::PATH_INFO, request_.getReqTarget());
	setEnv_(CGIExec::PATH_TRANSLATED, workDir + request_.getReqTarget());
	setEnv_(CGIExec::REMOTE_ADDR, "");
	setEnv_(CGIExec::REMOTE_IDENT, "");
	setEnv_(CGIExec::REMOTE_USER, "");
	setEnv_(CGIExec::REQUEST_METHOD, request_.getMethod());
	setEnv_(CGIExec::REQUEST_URI, request_.getReqTarget());
	setEnv_(CGIExec::SCRIPT_FILENAME, workDir + filename);
	setEnv_(CGIExec::SCRIPT_NAME, filename);
	setEnv_(CGIExec::SERVER_NAME, request_.getHeaderHost());
	setEnv_(CGIExec::SERVER_PORT, ft_itoa(request_.getServer()->port));
	setEnv_(CGIExec::SERVER_PROTOCOL, "HTTP/1.1");
	setEnv_(CGIExec::SERVER_SOFTWARE, "webserv/0.0.0");
}

FileDescriptor *CGIExec::run(Client &client)
{
	int pipeOUT[2];
	int pipeIN[2];
	Parsing::location *location = client.getRequest().getLocation();

	Log().Get(logDEBUG) << "CGI: " << location->cgi_path << " " << location->root << client.getRequest().getReqTarget() ;
	if (pipe(pipeOUT) == -1 || pipe(pipeIN) == -1)
	{
		Log().Get(logERROR) << __FUNCTION__  << "Unable to pipe: " << strerror(errno);
		return (0);
	}
	CGISocket *response = new CGISocket(pipeOUT[0], client);
	build_(location->root, client.getRequest().getReqTarget()); //TODO rewrite since we got request in scope
	pid_t cpid = fork();
	if (cpid < 0)
	{
		delete response;
		Log().Get(logERROR) << __FUNCTION__  << "Unable to fork: " << strerror(errno);
		return (0);
	}
	if (cpid == 0)
	{
		if (chdir(location->root.c_str()) == -1)
		{
			Log().Get(logERROR) << __FUNCTION__  << " Unable to chdir: " << strerror(errno) << " DIR: " << location->root;
			exit(EXIT_FAILURE);
		}
		pipeSTDOUT_(pipeOUT);
		pipeSTDIN_(pipeIN);
		dupSTDERR_();
		exec_(location->cgi_path, location->root + client.getRequest().getReqTarget());
		close(STDOUT_FILENO);
		close(STDIN_FILENO);
		close(STDERR_FILENO);
		freeEnvs_();
		exit(EXIT_FAILURE);
	}
	else
	{
		close(pipeIN[0]);
		if (client.getRequest().getHeaderContentLength() > 0)
		{
			std::string body = client.getRequest().getBody();
			write(pipeIN[1], body.c_str(), body.size());
		}
		response->setPid(cpid);
		close(pipeOUT[1]);
		close(pipeIN[1]);
		freeEnvs_();
	}
	return (response);
}

void CGIExec::exec_(const std::string &bin, const std::string &filename)
{
	static char * cmd[3];
	cmd[0] = const_cast<char *>(bin.c_str());
	cmd[1] = const_cast<char *>(filename.c_str());
	cmd[2] = 0;
	if (execve(cmd[0], cmd, envs_) == -1)
	{
		Log().Get(logERROR) << __FUNCTION__  << " Unable to execve " << strerror(errno);
		write500();
	}
}


void 	CGIExec::pipeSTDIN_(int pfd[2])
{
	if (close(pfd[1]) == -1)
		{
			Log().Get(logERROR) << __FUNCTION__  << " Unable to close " << strerror(errno);
			write500();
		}
	if (dup2(pfd[0], STDIN_FILENO) == -1)
	{
		Log().Get(logERROR) << __FUNCTION__  << "Unable to dup2 " << strerror(errno);
		write500();
	}
	if (close(pfd[0]) == -1)
	{
		Log().Get(logERROR) << __FUNCTION__  << "Unable to close " << strerror(errno);
		write500();
	}
	stdinFD_ = STDIN_FILENO;
}

void	CGIExec::pipeSTDOUT_(int pfd[2])
{
	if (close(pfd[0]) == -1)
	{
		Log().Get(logERROR) << __FUNCTION__  << " Unable to close " << strerror(errno);
		write500();
	}
	if (dup2(pfd[1], STDOUT_FILENO) == -1)
	{
		Log().Get(logERROR) << __FUNCTION__  << "Unable to dup2 " << strerror(errno);
		write500();
	}
	if (close(pfd[1]) == -1)
	{
		Log().Get(logERROR) << __FUNCTION__  << "Unable to close " << strerror(errno);
		write500();
	}
	stdoutFD_ = STDOUT_FILENO;
}

void	CGIExec::dupSTDERR_()
{
	int fd;
	if ((fd = open("/dev/null", O_RDONLY)) == -1)
		return ;
	dup2(fd, STDERR_FILENO);
}

void CGIExec::setEnv_(int name, const std::string &c)
{
	//TODO: concat instead allocating tmp buf
	std::string buf = vars_[name] + c;
	envs_[name] = (char*)malloc((buf.length() + 1) * sizeof(char));
	if (envs_[name] == 0)
		return ;
	unsigned long i;
	for (i = 0; i < buf.length(); i++)
	{
		envs_[name][i] = buf[i];
	}
	envs_[name][i] = '\0';
	Log().Get(logDEBUG) << envs_[name];
}

void CGIExec::freeEnvs_()
{
	for (int i = 0; i < 18; i++)
		free(envs_[i]);
}

void CGIExec::write500() {
	char buf[1024];
	int nbytes;

	RespError err(500, request_, buf, 1024);
	nbytes = err.readResponse();
	write(STDOUT_FILENO, buf, nbytes);
	exit(EXIT_FAILURE);
}

