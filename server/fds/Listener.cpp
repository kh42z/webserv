#include "Listener.hpp"

Listener::Listener(): ip_(0), port_(0) {
	fd_ = 0;
	lastEventTimer_ = 0;
}

Listener::~Listener(){
	if (fd_ > 0)
		close(fd_);
}

void Listener::onEvent()
{
	onNewClient();
}

void Listener::onNewClient()
{
	socklen_t 				addrlen;
	struct sockaddr_storage remoteaddr;
	int						newfd;

	addrlen = sizeof(remoteaddr);
	if ((newfd = accept(fd_, (struct sockaddr *)(&remoteaddr), &addrlen)) == -1)
	{
		Log::get(logERROR) << "server::onClientConnect " << strerror(errno) << std::endl;
		exit(8);
	}
	Client *client = new Client(newfd, servers_);
	Server::getInstance()->addFileDescriptor(client);
}

void Listener::ListenAndServe() {
	struct sockaddr_in server;
	fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_ == -1)
	{
		Log::get(logERROR) << "server:start -> error in socket()\n" << std::endl;
		delete Server::getInstance();
		exit(EXIT_FAILURE);
	}
	int enable = 1;
	if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		Log::get(logERROR) << __FUNCTION__  << " setsockopt failed" << strerror(errno) << std::endl;
	memset(&server, 0, sizeof(sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = ip_;
	server.sin_port = htons_(port_);
	if ((bind(fd_, (struct sockaddr *)&server, sizeof(struct sockaddr))) == -1)
	{
		Log::get(logERROR) << "server:start -> error in bind() " << strerror(errno) << std::endl;
		delete Server::getInstance();
		exit(EXIT_FAILURE) ;
	}
	if (listen(fd_, FD_SETSIZE) == -1)
	{
		Log::get(logERROR) << "server:start -> error in listen() " << strerror(errno) << std::endl;
		delete Server::getInstance();
		exit(EXIT_FAILURE) ;
	}
	Log::get(logINFO) << " listening on port " << port_ << " (maxconn: " << FD_SETSIZE << ")" << std::endl;
}

uint16_t Listener::htons_(uint16_t hostshort)
{
	long ui = 0;

	ui |= (hostshort & 0xFF) << 8;
	ui |= (hostshort & 0xFF00) >> 8;
	return (ui);
}


unsigned int Listener::getPort() const {
	return port_;
}

int Listener::addServer(const Parsing::server &s) {
	in_addr_t  host = inet_addr(s.host.c_str());
	if (host == INADDR_NONE)
	{
		Log::get(logERROR) << __FUNCTION__  << " Unable to add: " << s.host << std::endl;
		return (0);
	}
	if (ip_ == 0 && port_ == 0)
	{
		ip_ = host;
		port_ = s.port;
	}
	else if (host != ip_ || port_ != s.port)
	{
		Log::get(logERROR) << __FUNCTION__  << " this host:port doesnt belong to this listener: " << s.host << ":" << s.port << std::endl;
		return (1);
	}
	Log::get(logINFO) << __FUNCTION__  << " > " << s.host << ":" << s.port << " add virtualhost: " << s.names[0] << " (maxconn: " << FD_SETSIZE << ")" << std::endl;
	servers_.push_back(s);
	return (0);
}

