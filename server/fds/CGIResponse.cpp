#include "CGIResponse.hpp"

CGIResponse::CGIResponse(int fd, Client &client): client_(client)
{
	fd_ = fd;
	Log().Get(logDEBUG) << __FUNCTION__  << fd_;
}

CGIResponse::~CGIResponse()
{
	int status;

	pid_t result = waitpid(pid_, &status, WNOHANG);
	if (result == 0)
		kill(pid_, 9);
	close(fd_);
}

int CGIResponse::pipeToClient() {
	char 	buf[BUFFER_SIZE + 1];
	int		nbytes;

	if ((nbytes = read(fd_, buf, BUFFER_SIZE)) > 0)
	{
		Log().Get(logDEBUG) << "CGIResponse::read > FD " << fd_ << " READ " << nbytes;
		if (send(client_.getFd(), buf, nbytes, 0) == -1)
		{
			Log().Get(logERROR) << "CGIResponse::send > FD " << fd_ << " SENT ERROR " << strerror(errno);
			return 0;
		}
	}
	return nbytes;
}

void CGIResponse::onEvent() {
	int status;
	if ((status = pipeToClient()) <= 0)
	{
		if (status < 0)
			Log().Get(logDEBUG) << "CGIResponse > read error " << strerror(errno);
		Server::getInstance()->deleteFileDescriptor(client_.getFd());
		Server::getInstance()->deleteFileDescriptor(fd_);
	}
}

pid_t CGIResponse::getPid() const {
	return pid_;
}

void CGIResponse::setPid(pid_t pid) {
	pid_ = pid;
}
