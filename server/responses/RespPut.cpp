#include "RespPut.hpp"

RespPut::RespPut(const Request &r, char buf[], unsigned int bufSize) : RespFiles(r, buf, bufSize)
{ }

RespPut::~RespPut(){}

void RespPut::reachResource_()
{
	if (filePath_[filePath_.size() - 1] == '/')
		throw RespException(400);

	if (createDirectories_(filePath_) == -1)
		throw RespException(500);

	struct stat buffer = {};
	if (stat(filePath_.c_str(), &buffer) == -1)
		statusCode_ = 201;

	openFiles_(O_CREAT | O_TRUNC | O_RDWR, 500);
}

void RespPut::reachLangResources_()
{
    struct stat buffer = {};
    for(size_t i = 0; i < langFilePath_.size(); ++i)
    {
        if (stat(langFilePath_[i].c_str(), &buffer) == -1)
            statusCode_ = 201;
    }
    openFiles_(O_CREAT | O_TRUNC | O_RDWR, 500);
}

void RespPut::makeResponse_()
{
	initHeaders();
	writeContentType_(filePath_);
	writeContentLength_(0);
    if (contentLangNegotiated_)
    {
        for(size_t i = 0; i < langFilePath_.size(); ++i)
            writeThisHeader_ ("Content-location", langFilePath_[i]);
    }
    else if (statusCode_ == 201)
		writeThisHeader_("Location", filePath_);
	writeThisHeader_("Last-Modified", getStrDate());
	writeHeadersEnd_();
}

int RespPut::readResponse()
{
	nbytes_ = 0;

	writeFiles_();
	if (statusCode_ == 500)
		return -1;
	if (req_.getStatusCode() == 200 && !headersBuilt_)
		makeResponse_();
	return nbytes_;
}

void RespPut::build() {
	setFilePath_();
	negotiateContentLang_();
	if (contentLangNegotiated_)
	    reachLangResources_();
	else
    	reachResource_();
}