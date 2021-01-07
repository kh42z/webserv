#include "RespPut.hpp"

RespPut::RespPut(const Request &r, char buf[], unsigned int bufSize) : RespFile(r, buf, bufSize)
{
	fd_ = 0;
	payloadCursor_ = 0;
}

RespPut::~RespPut()
{
	if (fd_ > 0)
		close(fd_);
}

void RespPut::reachResource_()
{
	if (filePath_[filePath_.size() - 1] == '/')
		throw RespException(400);

	if (createDirectories_() == -1)
		throw RespException(500);

	struct stat buffer = {};
	if (stat(filePath_.c_str(), &buffer) == -1)
		statusCode_ = 201;

	fd_ = open(filePath_.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0664);
	if (fd_ == -1)
	{
		Log::get(logERROR) << __FUNCTION__  << " unable to open: " << strerror(errno) << std::endl;
		throw RespException(500);
	}
}

void RespPut::putPayload_()
{
	int nbytes;

	std::string payload = req_.getBody();
	size_t len = payload.size() - payloadCursor_;
	nbytes = write(fd_, payload.c_str() + payloadCursor_, len);
	if (nbytes < 0)
		statusCode_ = 500;
	payloadCursor_ += len;
}

void RespPut::makeResponse_()
{
	writeStatusLine_(statusCode_);
	writeContentType_(filePath_);
	if (statusCode_ == 201)
		writeThisHeader_("Location", filePath_);
	writeThisHeader_("Last-Modified", getStrDate());
	writeHeadersEnd_();
	writeHeadersEnd_();
}

int RespPut::readResponse()
{
	nbytes_ = 0;

	putPayload_();
	if (statusCode_ == 500)
	{
		writeErrorPage(500);
		return nbytes_;
	}
	if (req_.getStatusCode() == 200 && !headersBuilt_)
		makeResponse_();
	return nbytes_;
}

void RespPut::build() {
	reachResource_();
}