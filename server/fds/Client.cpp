#include "Client.hpp"

Client::Client(int fd, std::vector<Parsing::server> &s): servers_(s), request_(s) {
	CGIResponse_ = 0;
	location_ = 0;
	fd_ = fd;
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
	static char buf[1024];
	int nbytes = recv(fd_, buf, sizeof(buf), 0);
	Log().Get(logDEBUG) << __FUNCTION__  << " Client" << fd_ << " -> RECV " << nbytes;
	if (nbytes <= 0)
	{
		if (nbytes < 0)
		{
			Log().Get(logERROR) << __FUNCTION__ << "Client " << fd_ << " recv error" << strerror(errno);
			Server::getInstance()->deleteFileDescriptor(fd_);
			return ;
		}
	}
	constructRequest(buf, nbytes);
}

void Client::sendResponse() const
{
}

void Client::constructRequest(char buf[], int nbytes) {
	int status;

	if (CGIResponse::instances > MAX_CGI_FORKS)
	{
		Log().Get(logERROR) << __FUNCTION__  << "Too many CGIRunning, bounce this client: " << fd_;
		send(fd_, "HTTP/1.1 500 Internal Server Error\r\n", 36, 0);
		Server::getInstance()->deleteFileDescriptor(fd_);
		return ;
	}
	status = request_.doRequest(buf, nbytes);
	Log().Get(logDEBUG) << __FUNCTION__ << fd_ << "parsing status: " << status;
	if (status == 100)
		return ;
	else if (status == 200)
	{
		if (CGIResponse_ != 0) {
			Log().Get(logERROR) << " parse returned 200 but CGIResponse was already set: "
								<< request_.request_;
			return ;
		}
		CGIExec exec = CGIExec();
		CGIResponse_ = exec.run("/usr/bin/php-cgi", servers_[0].root, "/index.php", *this);
		if (CGIResponse_ == 0)
			send(fd_, "HTTP/1.1 500 Internal Server Error\r\n", 36, 0);
		Server::getInstance()->addFileDescriptor(CGIResponse_);
	}
	else
	{
		Log().Get(logERROR) << __FUNCTION__  << " Parse Error code: " << status << " REQ BODY: " << request_.request_;
		send(fd_, "HTTP/1.1 400 Bad Request\r\n", 26, 0);
		Server::getInstance()->deleteFileDescriptor(fd_);
	}
}

std::string &Client::getResponse()
{
	return response_;
}

void Client::appendResponse(char buf[], int nbytes) { 
	response_.append(buf, nbytes);					 
}

Request &Client::getRequest(){
	return request_;
}

Parsing::server 	&Client::matchServer_()
{
	for (unsigned long i = 0; i < servers_.size(); i++)
	{
		for (unsigned long k = 0; k < servers_[i].names.size(); k++)
		{
			if (servers_[i].names[k].compare(request_.getHeaderHost()) == 0)
			{
				return (servers_[i]);
			}
		}
	}
	return (servers_[0]);
}

void	Client::matchLocation_()
{
	Parsing::server &server = matchServer_();
	for (unsigned long j = 0; j < server.locations.size(); j++)
	{
		if (server.locations[j].name.compare(request_.getReqTarget()))
		{
			location_ = &server.locations[j];
			return ;
		}
	}
}

bool 	Client::isAuthorized_()
{
	for (unsigned long i = 0; i < location_->methods.size(); i++)
	{
		if (location_->methods[i].compare(request_.getMethod()))
		{
			return true;
		}
	}
	return false;
}