
#include "libxml_plus.h"
#include "../util/WebUtils.h"

namespace xmlpp
{
	std::vector<std::string> string_split(const std::string&str, char delim)
	{
		std::vector<std::string> elements;
		size_t str_length = str.length();
		size_t next_element_start = 0;
		for(size_t i=0; i<str_length; i++)
		{
			char c = str.at(i);
			if(c == delim)
			{
				if(i!=next_element_start)
				{
					elements.push_back(str.substr(next_element_start, i));
				}
				next_element_start = i+1;
			}
		}
		if(next_element_start != str_length)
		{
			elements.push_back(str.substr(next_element_start, str_length));
		}
		return elements;
	}
	
	xmlpp::Element* Node_getFirstChildElement(xmlpp::Node*node)
	{
		xmlpp::Node::NodeList children(node->get_children());
		size_t children_size = children.size();
		for(size_t i=0; i<children_size; i++)
		{
			xmlpp::Node* node = *std::next(children.begin(), i);
			if(node->cobj()->type == XML_ELEMENT_NODE)
			{
				return static_cast<xmlpp::Element*>(node);
			}
		}
		return nullptr;
	}
	
	xmlpp::Element* Node_getLastChildElement(xmlpp::Node*node)
	{
		xmlpp::Node::NodeList children(node->get_children());
		size_t children_size = children.size();
		for(size_t i=(children_size-1); i!=SIZE_MAX; i--)
		{
			xmlpp::Node* node = *std::next(children.begin(), i);
			if(node->cobj()->type == XML_ELEMENT_NODE)
			{
				return static_cast<xmlpp::Element*>(node);
			}
		}
		return nullptr;
	}
	
	xmlpp::Element* Node_getNthChildElement(xmlpp::Node*node, size_t n)
	{
		if(n == 0)
		{
			return nullptr;
		}
		xmlpp::Node::NodeList children(node->get_children());
		size_t children_size = children.size();
		size_t counter = 1;
		for(size_t i=0; i<children_size; i++)
		{
			xmlpp::Node* node = *std::next(children.begin(), i);
			if(node->cobj()->type == XML_ELEMENT_NODE)
			{
				if(counter==n)
				{
					return static_cast<xmlpp::Element*>(node);
				}
				counter++;
			}
		}
		return nullptr;
	}
	
	std::vector<xmlpp::Element*> Node_getChildElements(xmlpp::Node*node)
	{
		std::vector<xmlpp::Element*> childElements;
		xmlpp::Node::NodeList children(node->get_children());
		size_t children_size = children.size();
		childElements.resize(children_size);
		size_t childElementCount = 0;
		for(size_t i=0; i<children_size; i++)
		{
			xmlpp::Node* node = *std::next(children.begin(), i);
			if(node->cobj()->type == XML_ELEMENT_NODE)
			{
				childElements[childElementCount] = static_cast<xmlpp::Element*>(node);
				childElementCount++;
			}
		}
		childElements.resize(childElementCount);
		return childElements;
	}

	xmlpp::Element* Node_getElementById(xmlpp::Node*node, const std::string&id)
	{
		std::string id_attr = "id";
		xmlpp::Node::NodeList children(node->get_children());
		size_t children_size = children.size();
		for(size_t i=0; i<children_size; i++)
		{
			xmlpp::Node* current_node = *std::next(children.begin(), i);
			if(node->cobj()->type == XML_ELEMENT_NODE)
			{
				xmlpp::Element*current_element = static_cast<xmlpp::Element*>(node);
				xmlpp::Attribute* attribute = current_element->get_attribute(id_attr);
				if(attribute!=nullptr && attribute->get_value()==id)
				{
					return current_element;
				}
				else
				{
					xmlpp::Element*element = Node_getElementById(current_node, id);
					if(element != nullptr)
					{
						return element;
					}
				}
			}
		}
		return nullptr;
	}
	
	std::vector<xmlpp::Element*> Node_getElementsByClassName(xmlpp::Node*node, const std::string&className)
	{
		std::vector<std::string> classNames(string_split(className, ' '));
		return Node_getElementsByClassNames(node, classNames);
	}
	
	std::vector<xmlpp::Element*> Node_getElementsByClassNames(xmlpp::Node*node, const std::vector<std::string>&classNames)
	{
		std::vector<xmlpp::Element*> elements;
		std::string class_attr = "class";
		
		xmlpp::Node::NodeList children(node->get_children());
		size_t children_size = children.size();
		for(size_t i=0; i<children_size; i++)
		{
			xmlpp::Node* current_node = *std::next(children.begin(), i);
			if(node->cobj()->type == XML_ELEMENT_NODE)
			{
				xmlpp::Element*current_element = static_cast<xmlpp::Element*>(node);
				xmlpp::Attribute* attribute = current_element->get_attribute(class_attr);
				if(attribute!=nullptr && webutils::vector_containsAll(string_split(attribute->get_value(),' '), classNames))
				{
					elements.push_back(current_element);
				}
				webutils::vector_push_back(elements, Node_getElementsByClassNames(current_node, classNames));
			}
		}
		return elements;
	}
	
	std::vector<xmlpp::Element*> Node_getElementsByTagName(xmlpp::Node*node, const std::string& tagName)
	{
		std::vector<xmlpp::Element*> elements;
		xmlpp::Node::NodeList children(node->get_children());
		size_t children_size = children.size();
		for(size_t i=0; i<children_size; i++)
		{
			xmlpp::Node* current_node = *std::next(children.begin(), i);
			if(current_node->cobj()->type == XML_ELEMENT_NODE)
			{
				xmlpp::Element*current_element = static_cast<xmlpp::Element*>(current_node);
				std::string tagName_cmp = current_element->get_name();
				if(tagName_cmp==tagName)
				{
					elements.push_back(current_element);
				}
				webutils::vector_push_back(elements, Node_getElementsByTagName(current_node, tagName));
			}
		}
		return elements;
	}
}
