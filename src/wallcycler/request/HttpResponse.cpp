
#include "HttpResponse.h"
#include <regex>
#include <sstream>

HttpResponse::HttpResponse() : status(0)
{
	//
}
	
HttpResponse::HttpResponse(const std::string& fullResponse) : status(0)
{
	size_t pos = 0;
	std::string http_version = "1.1";
	{
		size_t firstEndLine = fullResponse.find("\r\n", pos);
		std::string firstLine = fullResponse.substr(0, firstEndLine);
		std::regex rgx("([A-z]+\\/([0-9]+(?:\\.[0-9]+)?)) ([0-9]+) (.*)");
		std::smatch match;
		if(std::regex_match(firstLine, match, rgx))
		{
			http_version = match[2].str();
			status = (unsigned short)stoi(match[3].str());
			statusText = match[4].str();
		}
		pos = firstLine.length()+2;
	}
	bool parsingHeader = true;
	while(parsingHeader)
	{
		size_t endLine = fullResponse.find("\r\n", pos);
		if(endLine == pos)
		{
			parsingHeader = false;
			pos+=2;
			break;
		}
		std::string line = fullResponse.substr(pos, endLine-pos);
		std::regex rgx("([\\w|\\-|_]+)\\:\\s+(.*)");
		std::smatch match;
		if(std::regex_match(line, match, rgx))
		{
			std::string name = match[1];
			std::string value = match[2];
			header[name]=value;
		}
		pos += (line.length()+2);
	}
	std::string transfer_encoding = getHeader("Transfer-Encoding");
	std::string content_length = getHeader("Content-Length");
	if(transfer_encoding=="chunked")
	{
		bool parsingChunks = true;
		size_t responseLength = fullResponse.length();
		while(parsingChunks && pos<responseLength)
		{
			size_t endLine = fullResponse.find("\n", pos);
			if(endLine == std::string::npos)
			{
				parsingChunks = false;
			}
			else
			{
				std::string line = fullResponse.substr(pos, endLine-pos);
				if(line.length()>0 && line.at(line.length() - 1) == '\r')
				{
					line = line.substr(0, line.length()-1);
				}
				std::stringstream ss;
				ss << std::hex << line;
				size_t length = 0;
				ss >> length;
				pos = endLine+1;
				body += fullResponse.substr(pos, length);
				pos += length;
				if(fullResponse.length() > pos)
				{
					if(fullResponse.at(pos) == '\n')
					{
						pos += 1;
					}
					else if(fullResponse.at(pos) == '\r')
					{
						pos += 1;
						if(fullResponse.length() > pos && fullResponse.at(pos) == '\n')
						{
							pos += 1;
						}
					}
				}
			}
		}
	}
	else if(content_length.length()>0)
	{
		size_t content_length_num = (size_t)std::stoull(content_length);
		body = fullResponse.substr(fullResponse.length()-content_length_num, content_length_num);
	}
	else
	{
		body = fullResponse.substr(pos, fullResponse.length()-pos);
	}
}
	
std::string HttpResponse::getHeader(const std::string& name) const
{
	std::string name_caps;
	size_t name_length = name.length();
	name_caps.resize(name_length);
	for(size_t i=0; i<name_length; i++)
	{
		char c = name.at(i);
		if(c >= 'a' && c <= 'z')
		{
			c = c - ('a' - 'A');
		}
		name_caps.at(i)=c;
	}
	for(std::map<std::string, std::string>::const_iterator it=header.begin(); it!=header.end(); it++)
	{
		std::string name_cmp = it->first;
		if(name_length==name_cmp.length())
		{
			bool match=true;
			for(size_t i=0; i<name_length; i++)
			{
				char c = name_caps.at(i);
				char c2 = name_cmp.at(i);
				if(c2 >= 'a' && c2 <= 'z')
				{
					c2 = c2 - ('a' - 'A');
				}
				if(c != c2)
				{
					match = false;
					i = name_length;
				}
			}
			if(match)
			{
				return it->second;
			}
		}
	}
	return "";
}
