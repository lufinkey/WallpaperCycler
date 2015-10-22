
#pragma once

#include <map>
#include <string>

class HttpResponse
{
public:
	unsigned short status;
	std::string statusText;
	std::map<std::string, std::string> header;
	std::string body;
	std::string error;

	HttpResponse();
	HttpResponse(const std::string& fullResponse);
	
	std::string getHeader(const std::string& name) const;
};
