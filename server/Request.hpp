#pragma once

#include <vector>
#include <stdlib.h>
#include <algorithm>
#include "fds/Client.hpp"

#define LEN 100
#define CRLF "\r\n"

class Request
{
	private:
	//Client &client_;
	std::vector<std::string> requestLine_;
	std::vector<std::string> headersRaw_;
	std::string msgBody_;
	
	std::string headerDate_;
	std::string headerAuth_;
	std::string headerHost_;
	std::string headerReferer_;
	std::string headerContentLength_;
	std::string headerContentLocation_;
	
	std::vector<std::string> headerAcceptCharset_;
	std::vector<std::string> headerAcceptLanguage_;
	std::vector<std::string> headerAllow_;
	std::vector<std::string> headerContentLanguage_;
	std::vector<std::string> headerContentType_;

	public:
	//Request(Client &);
	Request(std::string &);
	~Request();
	
	std::string & request_;
	bool requestLine_parsed;
	bool headers_parsed;
	bool body_parsed;

	std::vector<std::string> methods;
	enum e_methods { GET, HEAD, POST, PUT, DELETE, OPTION, TRACE, PATCH };
	enum e_RequestLine { METHOD, REQTARGET, VERSION };

	std::vector<std::string> headersName;
	enum e_headers { ACCEPT_CHARSETS, ACCEPT_LANGUAGE, ALLOW, AUTHORIZATION, CONTENT_LANGUAGE, \
		CONTENT_LENGTH, CONTENT_LOCATION, CONTENT_TYPE, DATE, HOST, REFERER };
	enum e_headerLine { HEADERTITLE, HEADERCONTENT };

	int parse();
	int parseRequestLine();
	int parseHeaders();
	int checkMethod();
	int checkVersion();
	int checkHeadersEnd();
	int getBody();
	void parseHeadersContent();
	void reset();

	std::vector<std::string> workLine(std::string &, const char &);
	std::string decodeBase64(std::string &);
	std::string decode_authorization();
	
	//Client &getClient();
	std::vector<std::string> getRequestLine();
	std::string getHeaderDate();
	std::string getHeaderAuth();
	std::string getHeaderHost();
	std::string getHeaderReferer();
	std::string getHeaderContentLength();
	std::string getHeaderContentLocation();
	std::vector<std::string> getHeaderAcceptCharset();
	std::vector<std::string> getHeaderAcceptLanguage();
	std::vector<std::string> getHeaderAllow();
	std::vector<std::string> getHeaderContentLanguage();
	std::vector<std::string> getHeaderContentType();
};
