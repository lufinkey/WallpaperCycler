
#pragma once

#include <vector>
#include <string>
#include <sqlite3.h>
#include "parse/WallpaperSource.h"
#include "exception/invalid_state.h"

namespace wallcycler
{
	class WallpaperManager
	{
	public:
		WallpaperManager(const std::string& userdata_path);
		WallpaperManager(const WallpaperManager&) = delete;
		~WallpaperManager();
		
		WallpaperManager& operator=(const WallpaperManager&) = delete;
		
		bool load(std::string*error=nullptr);
		bool close(std::string*error=nullptr);
		
		void addSource(WallpaperSource* source);
		void removeSource(WallpaperSource* source);
		
		bool hasWallpaperBeenUsed(const std::string& source_id, const std::string& wall_id, unsigned long long how_far_back) const;
		std::vector<Wallpaper> filterUsedWallpapers(std::vector<Wallpaper> wallpapers, unsigned long long how_far_back) const;
		
		void loadNextWallpaper(std::function<void(Wallpaper, std::string)> onload);
		
	private:
		static std::string sqlite_escaped_string(const std::string& str);
		
		void loadNextWallpaper(std::function<void(Wallpaper, std::string)> onload, std::vector<std::pair<WallpaperSource*, std::string> > sources_and_tags);
		
		std::string getOption(const std::string& option_name, const std::string& default_return="") const;
		bool isOptionSet(const std::string& option_name) const;
		void setOption(const std::string& option_name, const std::string& option_value);
		
		void createtable_PreviousWallpaper();
		void createtable_Options();
		void createtable_SearchData();
		
		std::string userdata_path;
		std::vector<std::string> tags;
		std::vector<WallpaperSource*> sources;
		sqlite3* database;
		unsigned long long current_wallpaper_count;
		unsigned long long last_full_cycle;
	};
}
