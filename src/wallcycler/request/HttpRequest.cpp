
#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "HttpRequest.h"

#include <regex>
#include <iostream>
#include <thread>

#ifdef _WIN32
	#include <WinSock2.h>
#else
	#include <sys/socket.h>
#endif

#ifdef _WIN32
	#pragma comment(lib,"ws2_32.lib")
#endif


HttpRequest::HttpRequest(const std::string& url, const std::string& method, const std::string& body)
	: callback_data(nullptr)
{
	setURL(url);
	setMethod(method);
	setBody(body);
}

void HttpRequest::setURL(const std::string& url_arg)
{
	if(url_arg.length()==0)
	{
		url_protocol = "";
		url_host = "";
		url_subpage = "";
		url = "";
		return;
	}
	std::string protocol;
	std::string host;
	std::string subpage;
	bool validURL = separateURL(url_arg, &protocol, &host, &subpage);
	if(!validURL)
	{
		throw std::invalid_argument("malformed url");
	}
	if(protocol.length() == 0)
	{
		protocol = "HTTP";
	}
	url_protocol = protocol;
	url_host = host;
	url_subpage = subpage;
	url = url_arg;
}

void HttpRequest::setMethod(const std::string& method_arg)
{
	method = method_arg;
}
	
void HttpRequest::setHeader(const std::string& name, const std::string& value)
{
	//TODO check if valid header name and value here
	header[name] = value;
}
	
void HttpRequest::removeHeader(const std::string& name)
{
	header.erase(name);
}
	
void HttpRequest::setBody(const std::string& body_arg)
{
	body = body_arg;
}
	
void HttpRequest::setCallbackData(void* data_arg)
{
	callback_data = data_arg;
}
	
const std::string& HttpRequest::getURL() const
{
	return url;
}
	
const std::string& HttpRequest::getMethod() const
{
	return method;
}
	
std::string HttpRequest::getHeader(const std::string& name) const
{
	std::string name_caps;
	size_t name_length = name.length();
	name_caps.resize(name_length);
	for(size_t i = 0; i<name_length; i++)
	{
		char c = name.at(i);
		if(c >= 'a' && c <= 'z')
		{
			c = c - ('a' - 'A');
		}
		name_caps.at(i) = c;
	}
	for(std::map<std::string, std::string>::const_iterator it = header.begin(); it != header.end(); it++)
	{
		std::string name_cmp = it->first;
		if(name_length == name_cmp.length())
		{
			bool match = true;
			for(size_t i = 0; i<name_length; i++)
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
	
const std::string& HttpRequest::getBody() const
{
	return body;
}
	
void* HttpRequest::getCallbackData() const
{
	return callback_data;
}

void HttpRequest::send(const HttpRequest & request, std::function<void(const HttpResponse&, void*)> onfinish)
{
	if(request.url.length() == 0)
	{
		throw std::invalid_argument("request has empty url");
	}
	else if(request.method.length() == 0)
	{
		throw std::invalid_argument("request has empty method");
	}

	std::string request_str;
	generateRequest(request, &request_str);
	std::thread([](std::string host, std::string request, std::function<void(const HttpResponse&, void*)> onfinish, void*data) {
		HttpResponse* response = nullptr;
		performHttpRequest(host, request, &response);
		if(onfinish)
		{
			onfinish(*response, data);
		}
		delete response;
	}, request.url_host, request_str, onfinish, request.callback_data).detach();
}

void HttpRequest::generateRequest(const HttpRequest& request, std::string* request_str)
{
	std::string& request_str_ref = *request_str;
	std::string subpage;
	if(request.url_subpage.length()==0)
	{
		subpage = "/";
	}
	else if(request.url_subpage.at(0) == '?')
	{
		subpage = "/"+request.url_subpage;
	}
	else
	{
		subpage = request.url_subpage;
	}
	std::string method;
	method.resize(request.method.length());
	for(size_t i=0; i<request.method.length(); i++)
	{
		char c = request.method.at(i);
		if(c >= 'a' && c <= 'z')
		{
			c = c - ('a' - 'A');
		}
		method.at(i) = c;
	}
		
	request_str_ref = method+" " + subpage + " HTTP/1.1\r\nHost: " + request.url_host;
	if(request.getHeader("Connection")=="")
	{
		request_str_ref += "\r\nConnection: close";
	}
	for(std::map<std::string, std::string>::const_iterator it = request.header.begin(); it != request.header.end(); it++)
	{
		request_str_ref += "\r\n"+it->first+": "+it->second;
	}
	if(request.body.length()>0 || method == "POST")
	{
		request_str_ref += "\r\nContent-Length :"+std::to_string(request.body.length());
		request_str_ref += "\r\n\r\n";
		request_str_ref += request.body;
	}
	else
	{
		request_str_ref += "\r\n\r\n";
	}
}

#ifdef _WIN32
#define close closesocket
#else
#define SOCKET int
#endif

void HttpRequest::performHttpRequest(const std::string& host, const std::string& request, HttpResponse** response)
{
	#ifdef _WIN32
		WSADATA wsaData;
		int init_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (init_result != 0)
		{
			*response = new HttpResponse();
			(*response)->error = "failed to initialize winsock";
			return;
		}
	#endif
		
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
	{
		*response = new HttpResponse();
		(*response)->error = "failed to open socket";
		#ifdef _WIN32
			WSACleanup();
		#endif
		return;
	}
	struct hostent*hostdata = nullptr;
	hostdata = gethostbyname(host.c_str());
	if(hostdata == nullptr)
	{
		*response = new HttpResponse();
		(*response)->error = "failed to resolve host";
		close(sock);
		#ifdef _WIN32
			WSACleanup();
		#endif
		return;
	}

	struct sockaddr_in sockaddr;
	sockaddr.sin_port = htons(80);
	sockaddr.sin_family = AF_INET;
	memcpy(&sockaddr.sin_addr.s_addr, hostdata->h_addr, sizeof(sockaddr.sin_addr.s_addr));
	if(::connect(sock, (struct sockaddr*)(&sockaddr), sizeof(sockaddr)) != 0)
	{
		*response = new HttpResponse();
		(*response)->error = "failed to connect to host";
		close(sock);
		#ifdef _WIN32
			WSACleanup();
		#endif
		return;
	}
		
	::send(sock, request.c_str(), request.length(), 0);
		
	std::string results;
	char buffer[10000];
	size_t nDataLength = 0;
	while((nDataLength = recv(sock, buffer, 9999, 0)) > 0)
	{
		results.append(buffer, nDataLength);
	}
	close(sock);
		
	#ifdef _WIN32
		WSACleanup();
	#endif
		
	*response = new HttpResponse(results);
}
	
#ifdef _WIN32
#undef close
#else
#undef SOCKET
#endif

bool HttpRequest::separateURL(const std::string& url, std::string* protocol, std::string* host, std::string* subpage)
{
	std::regex rgx("((\\w+)(\\:\\/\\/))?([\\w|\\.]+)(((\\/(((.*)(\\?))|(.*)))|\\?)(.*))?");
	std::smatch match;
	if(std::regex_match(url, match, rgx))
	{
		*protocol = match[2].str();
		*host = match[4].str();
		*subpage = match[5].str();
		return true;
	}
	return false;
}
