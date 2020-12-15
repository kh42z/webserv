#pragma once 

#include <cstring>
#include <vector>
#include <string>

char ft_tolower(char i) { return std::tolower(i); }

int ft_atoi(std::string const &str)
{
	int res = 0;

	for (int i = 0; str[i] != '\0'; i++)
        res = res * 10 + str[i] - '0';
    return res;
}
size_t strHex_to_int(std::string const &str)
{ 
	size_t x;   
	std::stringstream ss;

	ss << std::hex << str;
	ss >> x;
	return x;
}

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

	pos = buf.find("\n");
	if (pos == std::string::npos)
		return (-1);
	line = std::string(buf, 0, pos++);
	buf = buf.substr(pos);
	if (pos != std::string::npos)
		return (1);
	return (0);
}

