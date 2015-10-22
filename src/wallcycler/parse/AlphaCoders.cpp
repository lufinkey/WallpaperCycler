
#include "AlphaCoders.h"
#include "../request/HttpRequest.h"
#include "../libxml_plus/libxml_plus.h"
#include "../util/WebUtils.h"
#include <iostream>
#include <regex>

namespace wallcycler
{
	namespace sources
	{
		std::string AlphaCoders_apikey = "a977835ce83c3ad285aebef5309a4ac6";

		void AlphaCoders::search(const std::string& query, unsigned long resultsPerPage, unsigned long pagenum, SearchFinishCallback onfinish) const
		{
			if(pagenum == 0)
			{
				throw std::invalid_argument("pagenum cannot be 0");
			}
			std::string escaped_query = webutils::string_replaceall(webutils::urlencode(query), "%20", "+");
			std::string url = "http://wall.alphacoders.com/search.php?search="+escaped_query+"&page="+std::to_string(pagenum);
			search_page(url, resultsPerPage, onfinish);
		}
		
		void AlphaCoders::search_page(const std::string& url, unsigned long resultsPerPage, SearchFinishCallback onfinish) const
		{
			HttpRequest request(url, "GET");
			request.setHeader("Cookie", "AlphaCodersElementPerPage=" + std::to_string(resultsPerPage) + "; AlphaCodersView=paged");
			HttpRequest::send(request, [this, resultsPerPage, onfinish](const HttpResponse& response, void* data) {
				if(response.status==302)
				{
					std::string location = response.getHeader("Location");
					if(location.length()==0)
					{
						SearchData searchData;
						searchData.totalpages = 0;
						searchData.error = response.statusText;
						onfinish(searchData);
						return;
					}
					if(location.at(0)=='/')
					{
						search_page("http://wall.alphacoders.com"+location, resultsPerPage, onfinish);
						return;
					}
					
					if(!validUrl(location))
					{
						search_page("http://wall.alphacoders.com/"+location, resultsPerPage, onfinish);
						return;
					}
					
					std::string host = "http://wall.alphacoders.com";
					if(location.length()>=host.length())
					{
						std::string beginning = location.substr(0, host.length());
						if(beginning!=host)
						{
							search_page(location, resultsPerPage, onfinish);
							return;
						}
					}
					SearchData searchData;
					searchData.totalpages = 0;
					searchData.error = "cannot parse redirect";
					onfinish(searchData);
					return;

				}
				else if(response.status!=200)
				{
					SearchData searchData;
					searchData.totalpages = 0;
					searchData.error = response.statusText;
					onfinish(searchData);
					return;
				}

				xmlpp::Document*doc = new xmlpp::Document(htmlReadDoc((xmlChar*)response.body.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING));

				std::vector<xmlpp::Element*> containers = xmlpp::Node_getElementsByClassName(doc->get_root_node(), "container_masonry");
				xmlpp::Element* results_container = nullptr;
				for(size_t i=0; i<containers.size(); i++)
				{
					xmlpp::Element* container = containers.at(i);
					xmlpp::Element* parent = container->get_parent();
					if(parent != nullptr)
					{
						xmlpp::Attribute* class_attr = parent->get_attribute("class");
						if(class_attr!=nullptr && class_attr->get_value()=="main")
						{
							results_container = container;
							i = containers.size();
						}
					}
				}

				if(results_container == nullptr)
				{
					SearchData searchData;
					searchData.totalpages = 0;
					onfinish(searchData);
					return;
				}

				SearchData searchData;

				xmlpp::Element* pagination = nullptr;
				std::vector<xmlpp::Element*> pagination_nodes = xmlpp::Node_getElementsByClassName(doc->get_root_node(), "pagination");
				size_t pagination_nodes_size = pagination_nodes.size();
				for(size_t i=0; i<pagination_nodes_size; i++)
				{
					xmlpp::Element* node = pagination_nodes.at(i);
					xmlpp::Element* parent = node->get_parent();
					if(parent!=nullptr)
					{
						xmlpp::Attribute* class_attr = parent->get_attribute("class");
						if(class_attr!=nullptr && class_attr->get_value()=="hidden-xs hidden-sm")
						{
							pagination = node;
							i = pagination_nodes_size;
						}
					}
				}

				if(pagination != nullptr)
				{
					std::vector<xmlpp::Element*> anchors = xmlpp::Node_getElementsByTagName(pagination, "a");
					unsigned int highestPageNum = 0;
					size_t anchors_size = anchors.size();
					for(size_t i=0; i<anchors_size; i++)
					{
						xmlpp::Element* anchor = anchors[i];
						xmlpp::Attribute* id = anchor->get_attribute("id");
						if(!(id!=nullptr && (id->get_value()=="prev_page" || id->get_value()=="next_page")))
						{
							xmlpp::TextNode* textNode = anchor->get_child_text();
							if(textNode!=nullptr)
							{
								std::string content = textNode->get_content();
								if(webutils::is_numeric(content))
								{
									unsigned long value = (unsigned long)std::stol(content);
									if(value>highestPageNum)
									{
										highestPageNum = value;
									}
								}
							}
						}
					}
					if(highestPageNum==0)
					{
						highestPageNum = 1;
					}
					searchData.totalpages = highestPageNum;
				}
				else
				{
					searchData.totalpages = 1;
				}

				std::vector<xmlpp::Element*> children = xmlpp::Node_getChildElements(results_container);
				size_t children_size = children.size();
				searchData.results.resize(children_size);
				size_t item_count = 0;
				for(size_t i=0; i<children_size; i++)
				{
					xmlpp::Element* item = children.at(i);
					xmlpp::Attribute* class_attr = item->get_attribute("class");
					if(class_attr != nullptr && class_attr->get_value() == "item")
					{
						bool valid_item = true;
						std::string wall_id;
						std::string wall_url;
						xmlpp::Attribute* id_attr = item->get_attribute("id");
						if(id_attr != nullptr)
						{
							std::string value = id_attr->get_value();
							if(value.length()>5)
							{
								std::string id_num = value.substr(5, value.length()-5);
								if(webutils::is_numeric(id_num))
								{
									wall_id = id_num;
								}
								else
								{
									valid_item = false;
								}
							}
							else
							{
								valid_item = false;
							}
						}
						else
						{
							valid_item = false;
						}
						
						std::vector<xmlpp::Element*> images = xmlpp::Node_getElementsByTagName(item, "img");
						xmlpp::Element* thumb_image = nullptr;
						for(size_t j=0; j<images.size(); j++)
						{
							xmlpp::Element* image = images[j];
							xmlpp::Attribute* parent_href = image->get_parent()->get_attribute("href");
							if(parent_href!=nullptr && parent_href->get_value()==("big.php?i="+wall_id))
							{
								thumb_image = image;
								j = images.size();
							}
						}
						if(thumb_image!=nullptr && valid_item)
						{
							xmlpp::Attribute* src_attr = thumb_image->get_attribute("src");
							if(src_attr!=nullptr)
							{
								std::regex rgx("(http\\:\\/\\/images\\d+\\.alphacoders\\.com\\/\\d+\\/).+(\\.\\w+)");
								std::smatch match;
								std::string src = src_attr->get_value();
								if(std::regex_match(src, match, rgx))
								{
									wall_url = match[1].str() + wall_id + match[2].str();
								}
								else
								{
									valid_item = false;
								}
							}
							else
							{
								valid_item = false;
							}
						}
						else
						{
							valid_item = false;
						}
						
						if(valid_item)
						{
							Wallpaper result(getSourceID(), wall_id, wall_url);
							searchData.results[item_count] = result;
							item_count++;
						}
					}
				}
				searchData.results.resize(item_count);

				onfinish(searchData);
			});
		}

		std::string AlphaCoders::getSourceID() const
		{
			return "wall.alphacoders.com";
		}
		
		bool AlphaCoders::validUrl(const std::string& url) const
		{
			std::regex rgx("((\\w+)(\\:\\/\\/))([\\w|\\.]+)(((\\/(((.*)(\\?))|(.*)))|\\?)(.*))?");
			std::smatch match;
			if(std::regex_match(url, match, rgx))
			{
				return true;
			}
			return false;
		}
	}
}
