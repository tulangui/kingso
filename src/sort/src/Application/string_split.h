/**
 * author: weilei
 */
#ifndef _STRINGSPLIT_H__
#define _STRINGSPLIT_H__

#include <string>
#include <vector>

using namespace std;

namespace sort_application{

class StringSplit
{
	public:
		StringSplit(const string & src, const char*pseparator)
		{
			c_src=src;
			c_seperator = pseparator;
			init();
		}
		StringSplit(const char* psrc, const char*pseparator)
		{
			c_src=psrc;
			c_seperator = pseparator;
			init();
		}
		StringSplit(){}
		~StringSplit(){	c_substr.clear();}

		void setsrc(const char* psrc, const char*pseparator)
		{
			c_src=psrc;
			c_seperator = pseparator;
			init();
		}
		void setsrc(const string &src, const char*pseparator)
		{
			c_src=src;
			c_seperator = pseparator;
			init();
		}

		int	size()
		{
			return c_substr.size();
		}
		const char*operator[](int index)
		{
			return getsubstr(index);
		}
		const char*getsubstr(int index)
		{
			if((size_t)index < c_substr.size())
			{
				return c_substr[index].c_str();
			}
			else
				return NULL;
		}
		bool empty()
		{
			return c_substr.empty();
		}
	protected:
		int init()
		{
			c_substr.clear();
			size_t firstpos = 0;
			while(1)
			{
				if((firstpos = c_src.find_first_not_of(c_seperator.c_str(), firstpos)) == string::npos)
					break;
				size_t lastpos = c_src.find_first_of(c_seperator.c_str(), firstpos);
				if(lastpos == string::npos)
					lastpos = c_src.size();
				string tmp = c_src.substr(firstpos, lastpos - firstpos);
				c_substr.push_back(tmp);
				if(lastpos == c_src.size())
					break;
				firstpos = lastpos;
			}
			return 0;
		}
		string c_src;
		string c_seperator;
		vector<string> c_substr;
};
}


#endif//_STRINGSPLIT_H__
