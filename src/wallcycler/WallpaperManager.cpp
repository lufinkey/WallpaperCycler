
#include "WallpaperManager.h"
#include "util/WebUtils.h"
#include <thread>
#include <iostream>

namespace wallcycler
{
	std::string WallpaperManager::sqlite_escaped_string(const std::string& str)
	{
		return webutils::string_replaceall(str, "\"", "\"\"");
	}
	
	WallpaperManager::WallpaperManager(const std::string& userdata_path) : userdata_path(userdata_path), database(nullptr), current_wallpaper_count(0), last_full_cycle(0)
	{
		//TODO make sure userdata_path is valid
	}
	
	WallpaperManager::~WallpaperManager()
	{
		if(database!=nullptr)
		{
			close();
		}
	}
	
	void WallpaperManager::createtable_PreviousWallpaper()
	{
		std::string query =
			"CREATE TABLE IF NOT EXISTS PreviousWallpaper("
				"source_id TEXT,"
				"wall_id TEXT,"
				"url TEXT,"
				"display_order_num INTEGER"
			");";
		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	
	void WallpaperManager::createtable_Options()
	{
		std::string query =
			"CREATE TABLE IF NOT EXISTS Options("
				"option_name TEXT,"
				"option_value TEXT,"
				"primary key(option_name)"
			");";
		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	
	void WallpaperManager::createtable_SearchData()
	{
		std::string query =
			"CREATE TABLE IF NOT EXISTS SearchData("
				"search_query TEXT,"
				"source_id TEXT,"
				"result_count INTEGER"
			");";
		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	
	bool WallpaperManager::load(std::string*error)
	{
		if(database != nullptr)
		{
			if(error!=nullptr)
			{
				*error = "already loaded";
			}
			return false;
		}
		int opened = sqlite3_open((userdata_path+"/wallcycler.db").c_str(), &database);
		if(opened != SQLITE_OK)
		{
			if(error!=nullptr)
			{
				*error = std::to_string(opened) + ": " + sqlite3_errstr(opened);
			}
			return false;
		}
		
		createtable_Options();
		
		std::string current_wallpaper_count_str = getOption("current_wallpaper_count", "0");
		bool reset_prevwallpapers_table = false;
		if(!webutils::is_numeric(current_wallpaper_count_str))
		{
			current_wallpaper_count_str = "0";
			setOption("current_wallpaper_count", current_wallpaper_count_str);
			setOption("last_full_cycle", "0");
			reset_prevwallpapers_table = true;
		}
		else if(!isOptionSet("current_wallpaper_count"))
		{
			setOption("current_wallpaper_count", "0");
			setOption("last_full_cycle", "0");
			reset_prevwallpapers_table = true;
		}
		current_wallpaper_count = std::stoull(current_wallpaper_count_str);
		
		std::string last_full_cycle_str = getOption("last_full_cycle", "0");
		if(!webutils::is_numeric(last_full_cycle_str))
		{
			last_full_cycle_str = "0";
			setOption("last_full_cycle", last_full_cycle_str);
			reset_prevwallpapers_table = true;
		}
		last_full_cycle = std::stoull(last_full_cycle_str);
		
		if(reset_prevwallpapers_table)
		{
			std::string query = "DROP TABLE PreviousWallpaper";
			sqlite3_stmt* stmt = nullptr;
			sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
			setOption("last_full_cycle", "0");
		}
		
		createtable_PreviousWallpaper();
		createtable_SearchData();
		
		std::ifstream tags_file_in(userdata_path+"/tags.txt");
		if(tags_file_in.is_open())
		{
			std::cout << "opened tags file. reading..." << std::endl;
			std::string line;
			while(std::getline(tags_file_in, line))
			{
				std::cout << line << std::endl;
				std::string trimmed_line = webutils::string_trim(line);
				if(trimmed_line.length()>0)
				{
					std::cout << "loaded tag: " << trimmed_line << std::endl;
					tags.push_back(trimmed_line);
				}
			}
			tags_file_in.close();
			std::cout << "closed tags file" << std::endl;
		}
		else
		{
			std::cout << "tags file does not exist. creating one with a default entry" << std::endl;
		}
		if(tags.size()==0)
		{
			std::ofstream tags_file_out(userdata_path+"/tags.txt");
			if(tags_file_out.is_open())
			{
				tags_file_out << "abstract\n";
				tags_file_out.close();
			}
			tags.push_back("abstract");
		}
		
		return true;
	}
	
	bool WallpaperManager::close(std::string*error)
	{
		if(database==nullptr)
		{
			if(error!=nullptr)
			{
				*error = "not loaded";
			}
			return false;
		}
		int closed = sqlite3_close(database);
		if(closed != SQLITE_OK)
		{
			if(error!=nullptr)
			{
				*error = std::to_string(closed) + ": " + sqlite3_errstr(closed);
			}
			return false;
		}
		tags.clear();
		tags.shrink_to_fit();
		return true;
	}
	
	void WallpaperManager::addSource(WallpaperSource* source)
	{
		if(source==nullptr)
		{
			throw std::invalid_argument("source cannot be null");
		}
		size_t sources_size = sources.size();
		for(size_t i=0; i<sources_size; i++)
		{
			WallpaperSource* source_cmp = sources[i];
			if(source_cmp == source)
			{
				return;
			}
		}
		sources.push_back(source);
	}
	
	void WallpaperManager::removeSource(WallpaperSource* source)
	{
		size_t sources_size = sources.size();
		for(size_t i=0; i<sources_size; i++)
		{
			WallpaperSource* source_cmp = sources[i];
			if(source_cmp == source)
			{
				sources.erase(sources.begin()+i);
				return;
			}
		}
	}
	
	bool WallpaperManager::hasWallpaperBeenUsed(const std::string& source_id, const std::string& wall_id, unsigned long long how_far_back) const
	{
		if(database==nullptr)
		{
			throw wallcycler::invalid_state("not loaded");
		}
		unsigned long long wallcount_num = 0;
		if(how_far_back<current_wallpaper_count)
		{
			wallcount_num = current_wallpaper_count - how_far_back;
		}
		std::string query = "SELECT COUNT() as count FROM PreviousWallpaper WHERE source_id=\""+source_id+"\" AND wall_id=\""+wall_id+"\" AND display_order_num>="+std::to_string(wallcount_num);
		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
		int stepped = sqlite3_step(stmt);
		if(stepped==SQLITE_ROW)
		{
			int count = sqlite3_column_int(stmt, 0);
			if(count>0)
			{
				sqlite3_finalize(stmt);
				return true;
			}
		}
		sqlite3_finalize(stmt);
		return false;
	}
	
	std::vector<Wallpaper> WallpaperManager::filterUsedWallpapers(std::vector<Wallpaper> wallpapers, unsigned long long how_far_back) const
	{
		if(database==nullptr)
		{
			throw wallcycler::invalid_state("not loaded");
		}
		for(size_t i=0; i<wallpapers.size(); i++)
		{
			if(hasWallpaperBeenUsed(wallpapers[i].source_id, wallpapers[i].wall_id, how_far_back))
			{
				wallpapers.erase(wallpapers.begin()+i);
				i--;
			}
		}
		return wallpapers;
	}
	
	void WallpaperManager::loadNextWallpaper(std::function<void(Wallpaper, std::string)> onload)
	{
		if(database==nullptr)
		{
			throw wallcycler::invalid_state("not loaded");
		}
		std::vector<std::pair<WallpaperSource*, std::string> > sources_and_tags;
		size_t sources_size = sources.size();
		size_t tags_size = tags.size();
		for(size_t i=0; i<sources_size; i++)
		{
			for(size_t j=0; j<tags_size; j++)
			{
				sources_and_tags.push_back(std::pair<WallpaperSource*, std::string>(sources[i], tags[j]));
			}
		}
		loadNextWallpaper(onload, sources_and_tags);
	}
	
	void WallpaperManager::loadNextWallpaper(std::function<void(Wallpaper, std::string)> onload, std::vector<std::pair<WallpaperSource*, std::string> > sources_and_tags)
	{
		//TODO I actually want to make wallpapers have an even distribution, but I just don't really feel like doing that right now
		
		if(sources_and_tags.size()==0)
		{
			std::thread([](std::function<void(Wallpaper, std::string)> onload) {
				onload(Wallpaper(), "no sources or tags");
			}, onload).detach();
			return;
		}
		size_t random_source_tag_index = (size_t)(((double)std::rand()/(double)RAND_MAX)*(double)sources_and_tags.size());
		std::pair<WallpaperSource*, std::string> source_tag_pair = sources_and_tags[random_source_tag_index];
		std::cout << "selected random source: " << source_tag_pair.first->getSourceID() << std::endl;
		std::cout << "selected random tag: " << source_tag_pair.second << std::endl;
		source_tag_pair.first->search(source_tag_pair.second, 1000, 1, [this, sources_and_tags, source_tag_pair, random_source_tag_index, onload](const WallpaperSource::SearchData& searchData){
			if(searchData.results.size()==0)
			{
				std::vector<std::pair<WallpaperSource*, std::string> > new_sources_and_tags = sources_and_tags;
				new_sources_and_tags.erase(new_sources_and_tags.begin()+random_source_tag_index);
				loadNextWallpaper(onload, new_sources_and_tags);
				return;
			}
			size_t random_wallpaper_index = (size_t)(((double)std::rand()/(double)RAND_MAX)*(double)searchData.results.size());
			std::cout << "Selected random index: " << random_wallpaper_index << std::endl;
			Wallpaper wallpaper = searchData.results[random_wallpaper_index];
			onload(wallpaper, "");
		});
	}
	
	std::string WallpaperManager::getOption(const std::string& option_name, const std::string& default_return) const
	{
		std::string query = "SELECT option_value FROM Options WHERE option_name=\""+sqlite_escaped_string(option_name)+"\"";
		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
		int stepped = sqlite3_step(stmt);
		if(stepped==SQLITE_ROW)
		{
			std::string option_value = (const char*)sqlite3_column_text(stmt, 0);
			sqlite3_finalize(stmt);
			return option_value;
		}
		sqlite3_finalize(stmt);
		return default_return;
	}
	
	bool WallpaperManager::isOptionSet(const std::string& option_name) const
	{
		std::string query = "SELECT * FROM Options WHERE option_name=\""+sqlite_escaped_string(option_name)+"\"";
		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
		int stepped = sqlite3_step(stmt);
		if(stepped==SQLITE_ROW)
		{
			sqlite3_finalize(stmt);
			return true;
		}
		sqlite3_finalize(stmt);
		return false;
	}
	
	void WallpaperManager::setOption(const std::string& option_name, const std::string& option_value)
	{
		std::string query = "INSERT OR REPLACE INTO Options (option_name,option_value) VALUES(\""+sqlite_escaped_string(option_name)+"\",\""+sqlite_escaped_string(option_value)+"\")";
		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(database, query.c_str(), (int)query.length(), &stmt, nullptr);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}
