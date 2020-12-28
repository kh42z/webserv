#include "Client.hpp"

char Client::buf_[CLIENT_BUFFER_SIZE];

Client::Client(int fd, std::vector<Parsing::server> &s): request_(s) {
	CGIResponse_ = 0;
	fd_ = fd;
	setLastEventTimer();
	Log().Get(logDEBUG) << "Creating Client: " << fd_;
}

Client::~Client() {
	Log().Get(logDEBUG) << "Client deleted: " << fd_;
	close(fd_);
	if (CGIResponse_ != 0)
		Server::getInstance()->deleteFileDescriptor(CGIResponse_->getFd());
}

void Client::onEvent()
{
	setLastEventTimer();
	int nbytes = recv(fd_, buf_, CLIENT_BUFFER_SIZE - 1, 0);
	buf_[nbytes] = '\0';
	Log().Get(logDEBUG) << __FUNCTION__  << " Client" << fd_ << " -> RECV " << nbytes;
	if (nbytes <= 0)
	{
		if (nbytes < 0)		
			Log().Get(logERROR) << __FUNCTION__ << "Client " << fd_ << " recv error" << strerror(errno);
		else
			Log().Get(logINFO) << __FUNCTION__ << "Client " << fd_ << " client closed connection";
		Server::getInstance()->deleteFileDescriptor(fd_);
		return ;
	}
	constructRequest(buf_, nbytes);
}

inline bool ends_with(std::string const & value, std::string const & ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void Client::constructRequest(char buf[], int nbytes) {
	int statusCode;

	statusCode = request_.doRequest(buf, nbytes);
	Log().Get(logDEBUG) << __FUNCTION__ << " Client: " << fd_ << " parsing status: " << statusCode;
	if (statusCode == 100)
		return ;
	else if (statusCode == 200)
		doResponse_();
	else
	{
		Log().Get(logERROR) << __FUNCTION__  << "Client: " << fd_ << " Parse Error code: " << statusCode;
		RespError resp(statusCode, request_, buf_, CLIENT_BUFFER_SIZE);
		sendResponse_(&resp);
	}
}

void Client::doResponse_() {
	if (request_.getLocation()->cgi_extension.empty() || !ends_with(request_.getReqTarget(), request_.getLocation()->cgi_extension[0]))
		doStaticFile_();
	else
		doCGI_();
}

void Client::sendResponse_(Response *resp) {
	int nbytes;
	while ((nbytes = resp->readResponse()) > 0)
	{
		if (send(fd_, buf_, nbytes, 0) < 0)
		{
			Log().Get(logERROR) << " unable to send to client " << strerror(errno) << " nbytes: " << nbytes;
			break ;
		}
	}
	Server::getInstance()->deleteFileDescriptor(fd_);
}

std::string &Client::getResponse()
{
	return response_;
}

Request &Client::getRequest() {
	return request_;
}

void Client::doStaticFile_() {
	// TODO: factory just like CPPdays to avoid if else branching?
	Response *resp;
	if (request_.getMethod() == "GET")
		resp = new RespGet(request_, buf_, CLIENT_BUFFER_SIZE);
	else if (request_.getMethod() == "POST")
		resp = new RespError(405, request_, buf_, CLIENT_BUFFER_SIZE);
	else
		resp = new RespHead(request_, buf_, CLIENT_BUFFER_SIZE);
	sendResponse_(resp);
	delete(resp);
}

void Client::doCGI_() {
	if (CGISocket::instances > MAX_CGI_FORKS) {
		Log().Get(logERROR) << __FUNCTION__ << "Too many CGIRunning, bounce this client: " << fd_;
		RespError resp(500, request_, buf_, CLIENT_BUFFER_SIZE);
		sendResponse_(&resp);
		Server::getInstance()->deleteFileDescriptor(fd_);
		return ;
	}
	CGIExec exec = CGIExec();
	CGIResponse_ = exec.run(*this);
	Server::getInstance()->addFileDescriptor(CGIResponse_);
	if (CGIResponse_ == 0) {
		RespError resp(500, request_, buf_, CLIENT_BUFFER_SIZE);
		sendResponse_(&resp);
	}
}
