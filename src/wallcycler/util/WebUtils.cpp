
#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "WebUtils.h"
#include <iomanip>
#include <sstream>

#ifdef _WIN32
	#include <WinSock2.h>
#else
	#include <sys/socket.h>
#endif

namespace webutils
{
	std::string urlencode(const std::string& value)
	{
		std::ostringstream escaped;
		escaped.fill('0');
		escaped << std::hex;

		for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
		{
			std::string::value_type c = (*i);
				
			// Keep alphanumeric and other accepted characters intact
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			{
				escaped << c;
			}
			// Any other characters are percent-encoded
			else
			{
				escaped << '%' << std::setw(2) << int((unsigned char)c);
			}
		}
			
		return escaped.str();
	}
		
	std::string urldecode(const std::string& value)
	{
		std::ostringstream unescaped;
		bool inQueryString = false;
			
		for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; i++)
		{
			std::string::value_type c = (*i);
				
			if(c == '+')
			{
				if(inQueryString)
				{
					unescaped << ' ';
				}
				else
				{
					unescaped << '+';
				}
			}
			else if(c == '%')
			{
				char hex_val[3] = {0,0,0};
				bool cancelled = false;
				for(size_t j = 0; j < 2; j++)
				{
					i++;
					if(i == n)
					{
						unescaped << '%' << hex_val;
						j = 2;
						cancelled = true;
					}
					else
					{
						std::string::value_type c2 = (*i);
						if((c2 >= '0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F'))
						{
							hex_val[j] = c2;
						}
						else
						{
							unescaped << '%' << hex_val << c2;
							j = 2;
							cancelled = true;
						}
					}
				}
				if(!cancelled)
				{
					std::stringstream ss;
					ss << std::hex << hex_val;
					int val = 0;
					ss >> val;
					unescaped << ((std::string::value_type)val);
				}
			}
			else if(c == '?')
			{
				inQueryString = true;
				unescaped << '?';
			}
			else
			{
				unescaped << c;
			}
		}
		return unescaped.str();
	}
		
	std::string querytoken_urlencode(const std::string& querytoken)
	{
		std::ostringstream escaped;
		escaped.fill('0');
		escaped << std::hex;

		for (std::string::const_iterator i=querytoken.begin(), n=querytoken.end(); i!=n; i++)
		{
			std::string::value_type c = (*i);
				
			// Keep alphanumeric and other accepted characters intact
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			{
				escaped << c;
			}
			// Any other characters are percent-encoded
			else
			{
				escaped << '%' << std::setw(2) << int((unsigned char)c);
			}
		}
			
		return escaped.str();
	}
		
	std::vector<std::string> string_split(const std::string&str, std::string::value_type deliminator)
	{
		std::vector<std::string> tokens;
		size_t token_start = 0;
		size_t length = str.length();
		for(size_t i=0; i<length; i++)
		{
			std::string::value_type c = str.at(i);
			if(c == deliminator)
			{
				if(i != 0)
				{
					tokens.push_back(str.substr(token_start, i-token_start));
				}
				token_start = i+1;
			}
		}
		if(token_start != length)
		{
			tokens.push_back(str.substr(token_start, length-token_start));
		}
		return tokens;
	}
		
	std::string string_replaceall(const std::string& str, const std::string& find, const std::string& replace)
	{
		std::string str_new = str;
		std::string::size_type n = 0;
		while ((n=str_new.find(find, n)) != std::string::npos)
		{
			str_new.replace(n, find.length(), replace);
			n += replace.size();
		}
		return str_new;
	}
		
	std::string string_trim(const std::string& str)
	{
		size_t size = str.length();
			
		if(size == 0)
		{
			return str;
		}
	
		size_t i=0;
		size_t startIndex = 0;
		bool hitLetter = false;
		while(!hitLetter && i<size)
		{
			std::string::value_type c = str.at(i);
			if(c>' ' || (std::numeric_limits<char>::is_signed && c<0))
			{
				startIndex = i;
				hitLetter = true;
			}
			i++;
		}
	
		if(!hitLetter)
		{
			return "";
		}
	
		hitLetter = false;
		i = size-1;
	
		size_t endIndex = 0;
	
		while(!hitLetter && i!=SIZE_MAX)
		{
			std::string::value_type c = str.at(i);
			if(c>' ' || (std::numeric_limits<char>::is_signed && c<0))
			{
				endIndex = i+1;
				hitLetter = true;
			}
			i--;
		}
		return str.substr(startIndex, endIndex-startIndex);
	}
		
	std::string http_build_query(const std::map<std::string, std::string>& query_data)
	{
		std::ostringstream query_string;
		std::map<std::string, std::string>::const_iterator begin = query_data.begin();
		std::map<std::string, std::string>::const_iterator end = query_data.end();
		for(std::map<std::string, std::string>::const_iterator it=begin; it != end; it++)
		{
			if(it != begin)
			{
				query_string << '&';
			}
			query_string << querytoken_urlencode(it->first) << '=' << querytoken_urlencode(it->second);
		}
		return query_string.str();
	}
	
	bool is_numeric(const std::string& str)
	{
		bool hitStuff = false;
		size_t dec_count = 0;
		size_t length = str.length();
		for(size_t i=0; i<length; i++)
		{
			char c = str.at(i);
			if(c == '.')
			{
				hitStuff = true;
				dec_count++;
				if(dec_count > 1)
				{
					return false;
				}
			}
			else if(c==' ' || c=='-' || c=='+')
			{
				if(hitStuff)
				{
					return false;
				}
			}
			else if(c>='0' && c<='9')
			{
				hitStuff = true;
			}
			else
			{
				return false;
			}
		}
		return true;
	}
}
