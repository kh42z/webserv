#pragma once 

#include <cstring>
#include <vector>
#include <string>

std::vector<std::string> explode(const std::string & str, const char & delim)
{
	size_t start, end = 0;
	std::vector<std::string> res;

	while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
	{
		end = str.find(delim, start);
		res.push_back(str.substr(start, end - start));
	}
	return (res);
}

int getNextLine(std::string & buf, std::string & line)
{
	size_t pos;

	if ((pos = buf.find("\n") == std::string::npos))
		return (-1);
	line = std::string(buf, 0, pos++);
	buf = buf.substr(pos);
	if (pos != std::string::npos)
		return (1);
	return (0);
}
