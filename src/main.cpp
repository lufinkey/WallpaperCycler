
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include "wallcycler/WallpaperManager.h"
#include "wallcycler/parse/AlphaCoders.h"
#include "wallcycler/request/HttpRequest.h"
#include <ShlObj.h>
#include <iostream>
#include <direct.h>
#include <thread>
#include <fstream>
#include "wallcycler/util/posixtime.h"

using namespace wallcycler;

std::string image_extension_for_mimetype(const std::string& mime_type)
{
	if(mime_type=="image/jpeg" || mime_type=="image/pjpeg" || mime_type=="image/jpg")
	{
		return "jpeg";
	}
	else if(mime_type=="image/png")
	{
		return "png";
	}
	else if(mime_type=="image/bmp" || mime_type=="image/x-windows-bmp")
	{
		return "bmp";
	}
	return "jpg";
}

bool file_exists(const std::string& name)
{
	std::ifstream f(name.c_str());
	if(f.good())
	{
		f.close();
		return true;
	}
	else
	{
		f.close();
		return false;
	}
}

std::string prev_wallpaper_filename;

int main(int argc, char*argv[])
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	
	timeval srand_time;
	gettimeofday(&srand_time, nullptr);
	std::srand((unsigned int)srand_time.tv_usec);
	
	char path_buffer[1024];
	if(!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path_buffer)))
	{
		std::cerr << "unable to resolve appdata path" << std::endl;
		return -1;
	}
	std::string userdata_path = path_buffer;
	userdata_path += "/lufinkey";
	mkdir(userdata_path.c_str());
	userdata_path += "/wallcycler";
	mkdir(userdata_path.c_str());
	
	WallpaperManager* wallpaperManager = new WallpaperManager(userdata_path);
	sources::AlphaCoders* alpha_coders_source = new sources::AlphaCoders();
	wallpaperManager->addSource(alpha_coders_source);
	std::string wallmanager_load_error;
	if(!wallpaperManager->load(&wallmanager_load_error))
	{
		std::cerr << "error loading wallpaper manager: " << wallmanager_load_error << std::endl;
		return -2;
	}
	
	mkdir((userdata_path+"/rotation").c_str());
	bool running = true;
	while(running)
	{
		std::cout << "loading next wallpaper" << std::endl;
		wallpaperManager->loadNextWallpaper([userdata_path](Wallpaper wallpaper, std::string error) {
			if(error.length()>0)
			{
				std::cerr << "error loading new wallpaper: " << error << std::endl;
				return;
			}
			std::cout << "found next wallpaper" << std::endl;
			std::cout << "wall_id: " << wallpaper.wall_id << std::endl;
			std::cout << "source_id: " << wallpaper.source_id << std::endl;
			std::cout << "url: " << wallpaper.url << std::endl;
			std::cout << "downloading..." << std::endl;
			HttpRequest request(wallpaper.url, "GET");
			HttpRequest::send(request, [userdata_path](const HttpResponse& response, void*){
				if(response.status!=200)
				{
					std::cerr << "error downloading wallpaper: " << response.statusText << std::endl;
					return;
				}
				else
				{
					std::cout << "downloaded wallpaper" << std::endl;
				}
				std::string random_file_name;
				do
				{
					random_file_name = std::to_string(std::rand())+std::to_string(std::rand())+std::to_string(std::rand())+"."+image_extension_for_mimetype(response.getHeader("Content-Type"));
				}
				while(file_exists(userdata_path+"/rotation/"+random_file_name));
				
				std::string wallpaper_path = userdata_path+"/rotation/"+random_file_name;
				
				FILE* file = std::fopen(wallpaper_path.c_str(), "wb");
				if(file==nullptr)
				{
					return;
				}
				size_t written = std::fwrite(response.body.c_str(), 1, response.body.length(), file);
				if(written!=response.body.length())
				{
					std::cerr << "error writing wallpaper to file" << std::endl;
				}
				std::fclose(file);
				
				/*if(prev_wallpaper_filename.length()>0)
				{
					std::string old_wallpaper_path = userdata_path+"/rotation/"+prev_wallpaper_filename;
					std::remove(old_wallpaper_path.c_str());
				}*/
				
				prev_wallpaper_filename = random_file_name;
			});
		});
		std::this_thread::sleep_for(std::chrono::hours(6));
	}
	return 0;
}
