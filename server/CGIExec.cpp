#include "CGIExec.hpp"
#include "fds/CGIResponse.hpp"

CGIExec* CGIExec::instance_ = 0;

const std::string CGIExec::vars_[] = {
								  "AUTH_TYPE=",
								  "CONTENT_LENGTH=",
								  "GATEWAY_INTERFACE=",
								  "PATH_INFO=",
								  "PATH_TRANSLATED=",
								  "QUERY_STRING=",
								  "REMOTE_ADDR=",
								  "REMOTE_IDENT=",
								  "REMOTE_USER=",
								  "REQUEST_METHOD=",
								  "REQUEST_URI=",
								  "SCRIPT_NAME=",
								  "SERVER_NAME=",
								  "SERVER_PORT=",
								  "SERVER_PROTOCOL=",
								  "SERVER_SOFTWARE="
};

CGIExec::~CGIExec() {}

CGIExec::CGIExec() {
	envs_.reserve(17);
	envs_[16] = 0;
}

void CGIExec::build_(const RequestMock &request) {
	setEnv_(AUTH_TYPE, "");
	setEnv_(CONTENT_LENGTH, request.getHeaderContentLength());
	setEnv_(GATEWAY_INTERFACE, "");
	setEnv_(PATH_INFO, "");
	setEnv_(PATH_TRANSLATED, "");
	setEnv_(QUERY_STRING, "");
	setEnv_(REMOTE_ADDR, "");
	setEnv_(REMOTE_IDENT, "");
	setEnv_(REMOTE_USER, "");
	setEnv_(REQUEST_METHOD, request.getRequestMethod());
	setEnv_(REQUEST_URI, "");
	setEnv_(SCRIPT_NAME, "");
	setEnv_(SERVER_NAME, "");
	setEnv_(SERVER_PORT, "");
	setEnv_(SERVER_PROTOCOL, "");
	setEnv_(SERVER_SOFTWARE, "webserver/0.0.0");
}

void CGIExec::run(const std::string &script, RequestMock &request)
{
	build_(request);
	int pfd[2];
	pid_t cpid = fork();

	if (pipe(pfd))
	{
		Log().Get(logERROR) << "Unable to pipe: " << strerror(errno);
		throw ;
	}
	if (cpid < 0)
	{
		Log().Get(logERROR) << "Unable to fork: " << strerror(errno);
		throw ;
	}
	if (cpid == 0)
	{
		pipeStdout(pfd);
		CGIResponse *response = new CGIResponse(stdoutFD_, request.getClient());
		Server::getInstance()->addFileDescriptor(response);
		exec_(script);
		close(STDOUT_FILENO);
	}
	else
	{
		close(pfd[0]);
		close(pfd[1]);
	}
}

void CGIExec::exec_(const std::string &script)
{
	std::vector<char *> cmd;
	cmd.push_back(const_cast<char *>(script.c_str()));
	cmd.push_back(0);
	int ret = execve(cmd[0], &cmd.data()[0], &envs_.data()[0]);
	Log().Get(logDEBUG) << "ENVS :" << envs_[15] << std::endl;
	Log().Get(logDEBUG) << "RET: " << cmd[0] << " >> " << strerror(errno);
	freeEnvs_();
	if (ret == -1)
	{
		Log().Get(logERROR) << "Unable to execve " << strerror(errno);
	}
}


void	CGIExec::pipeStdout(int pfd[2])
{
	if (close(pfd[0]) == -1)
	{
		Log().Get(logERROR) << "Unable to close " << strerror(errno);
		throw ;
	}
	if (dup2(pfd[1], STDOUT_FILENO) == -1)
	{
		Log().Get(logERROR) << "Unable to dup2 " << strerror(errno);
		throw ;
	}
	if (close(pfd[1]) == -1)
	{
		Log().Get(logERROR) << "Unable to close " << strerror(errno);
		throw ;
	}
	stdoutFD_ = STDOUT_FILENO;
}

void CGIExec::setEnv_(int name, std::string c)
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
	Log().Get(logDEBUG) << "ENV: " << name << " : " << envs_[name];
}

void CGIExec::freeEnvs_()
{
	for (int i = 0; i < 16; i++)
		free(envs_[i]);
}

CGIExec *CGIExec::getInstance() {
	if (instance_ == 0)
		instance_ = new CGIExec();
	return instance_;
}

