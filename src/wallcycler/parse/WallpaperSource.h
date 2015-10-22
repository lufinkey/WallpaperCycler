
#pragma once

#include <string>
#include <vector>
#include <functional>

namespace wallcycler
{
	class Wallpaper
	{
	public:
		Wallpaper() {}
		Wallpaper(const std::string& source_id, const std::string& wall_id, const std::string& url) : source_id(source_id), wall_id(source_id), url(url) {}
		
		std::string source_id;
		std::string wall_id;
		std::string url;
	};
	
	class WallpaperSource
	{
	public:
		typedef struct SearchData
		{
			std::vector<Wallpaper> results;
			unsigned int totalpages;
			std::string error;
		} SearchData;
		
		typedef std::function<void(const SearchData& searchData)> SearchFinishCallback;
		
		virtual void search(const std::string& query, unsigned long resultsPerPage, unsigned long pagenum, SearchFinishCallback onfinish) const = 0;
		virtual std::string getSourceID() const = 0;
	};
}
