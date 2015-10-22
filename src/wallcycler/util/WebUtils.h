
#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>

namespace webutils
{
	std::string urlencode(const std::string& value);
	std::string urldecode(const std::string& value);
		
	std::vector<std::string> string_split(const std::string& str, std::string::value_type deliminator);
	std::string string_replaceall(const std::string& str, const std::string& find, const std::string& replace);
	std::string string_trim(const std::string& str);
		
	std::string http_build_query(const std::map<std::string, std::string>& query_data);
	
	bool is_numeric(const std::string& str);
		
	template<typename T>
	bool vector_containsAll(const std::vector<T>&vect, const std::vector<T>&cmp)
	{
		size_t cmp_size = cmp.size();
		size_t vect_size = vect.size();
		for (size_t i = 0; i<cmp_size; i++)
		{
			bool doesHave = false;
			for (size_t j = 0; j<vect_size; j++)
			{
				if (cmp[i] == vect[j])
				{
					doesHave = true;
					j = vect_size;
				}
			}
			if (!doesHave)
			{
				return false;
			}
		}
		return true;
	}

	template<typename T>
	void vector_push_back(std::vector<T>&vect, const std::vector<T>&adding)
	{
		size_t adding_size = adding.size();
		size_t startIndex = vect.size();
		size_t total_size = startIndex + adding_size;
		vect.resize(total_size);
		size_t counter = 0;
		for (size_t i = startIndex; i<total_size; i++)
		{
			vect[i] = adding[counter];
			counter++;
		}
	}
}

