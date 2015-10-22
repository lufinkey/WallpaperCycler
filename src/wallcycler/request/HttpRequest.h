
#pragma once

#include "HttpResponse.h"
#include <functional>
#include <exception>
#include <mutex>

class HttpRequest
{
public:
	HttpRequest(const std::string& url=std::string(), const std::string& method="POST", const std::string& body=std::string());

	void setURL(const std::string& url);
	void setMethod(const std::string& method);
	void setHeader(const std::string& name, const std::string& value);
	void removeHeader(const std::string& name);
	void setBody(const std::string& body);
	void setCallbackData(void* data);

	const std::string& getURL() const;
	const std::string& getMethod() const;
	std::string getHeader(const std::string& name) const;
	const std::string& getBody() const;
	void* getCallbackData() const;

	static void send(const HttpRequest& request, std::function<void(const HttpResponse&, void*)> onfinish);
		
private:
	std::string url;
	std::string url_protocol;
	std::string url_host;
	std::string url_subpage;

	std::string method;
	std::map<std::string, std::string> header;
	std::string body;
		
	void* callback_data;

	static void generateRequest(const HttpRequest& request, std::string* request_str);
	static void performHttpRequest(const std::string& host, const std::string& request, HttpResponse** response);
	static bool separateURL(const std::string& url, std::string* protocol, std::string* host, std::string* subpage);
};

