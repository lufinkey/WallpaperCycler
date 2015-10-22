
#pragma once

#include "WallpaperSource.h"

namespace wallcycler
{
	namespace sources
	{
		class AlphaCoders : public WallpaperSource
		{
		public:
			virtual void search(const std::string& query, unsigned long resultsPerPage, unsigned long pagenum, SearchFinishCallback onfinish) const override;
			virtual std::string getSourceID() const override;
			
		private:
			void search_page(const std::string& url, unsigned long resultsPerPage, SearchFinishCallback onfinish) const;
			
			bool validUrl(const std::string& url) const;
		};
	}
}
