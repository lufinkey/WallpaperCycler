
#pragma once

#include <exception>
#include <string>

namespace wallcycler
{
	class invalid_state : public std::exception
	{
	private:
		std::string message;
		
	public:
		invalid_state(const char* msg) : message(msg) {}
		invalid_state(const std::string& msg) : message(msg) {}
		
		virtual const char* what() const noexcept override
		{
			return message.c_str();
		}
	};
}
