#include "SortQuery.h"
namespace sort_framework
{

void ToLowerString(std::string &str)
{
    transform(str.begin(), str.end(), str.begin(), (int (*)(int))tolower);
}

/** 
 *@解析queryString,存储到paramMap中
 *@param  query，querystring
 *@param  area, query字段
 *@param  paramMap,存储对应的key，value字段
*/
static void parseQueryString(const char * query, std::string & area,
                             std::map<std::string, std::string> & paramMap)
{
    if(!query) {
        return;
    }
    const char *start, * end, *equal;
    end = strchr(query, '?');
    if (end == NULL){
        start = query;
        area.clear();
    }
    else{
        area = std::string(query, end - query);
        start = end + 1;
    }
    while(true) {
        if(*start == '\0') {
            break;
        }
        end = strchr(start, '&');
        if(end == start) {
            start = end + 1;
            continue;
        }
        equal = strchr(start, '=');
        if(equal) {
            if(equal == start) {
                if(end) {
                    start = end + 1;
                    continue;
                }
                else {
                    break;
                }
            }
            else {
                if(end) {
                    if(end < equal) {
                        std::string str(start, end - start);
                        ToLowerString(str);
                        paramMap.insert(std::make_pair(str, std::string("")));
                    }
                    else {
                        std::string str(start, equal - start);
                        ToLowerString(str);
                        paramMap.insert(std::make_pair(str, std::string(equal + 1, 
                                                                        end - equal - 1)));
                    }
                }
                else {
                    std::string str(start, equal - start);
                    ToLowerString(str);
                    paramMap.insert(std::make_pair(str, std::string(equal + 1)));
                    break;
                }
            }
        }
        else {
            if(end) {
                std::string str(start, end - start);
                ToLowerString(str);
                paramMap.insert(std::make_pair(str, std::string("")));
            }
            else {
                std::string str(start);
                ToLowerString(str);
                paramMap.insert(std::make_pair(str, std::string("")));
                break;
            }
        }
        if(!end) {
            break;
        }
        start = end + 1;
    }
}

SortQuery::SortQuery(const queryparser::Param & param) : 
    _param(param),
    _min(0x7fffffffffffffff),
    _do_first_audi(false), 
    _is_full(true),
    _available_count(0)
{
    _pQuery = param.getParam(QUERY_FLAG);
    parseQueryString(_pQuery, _area, _paramMap);
}

SortQuery::SortQuery(const queryparser::Param & param, const std::vector<const char *> & desc) 
            : _descs(desc) , _param(param) 
{
    _pQuery = param.getParam(QUERY_FLAG);
    parseQueryString(_pQuery, _area, _paramMap);
}

/**
 *@查找query的参数
 *@param name, 查找的名字
 *@param val, 找到的值
 *@return ture, 找到; false,没找到
*/
bool SortQuery::findParam(const std::string & name, std::string & val) const
{
    std::map<std::string, std::string>::const_iterator itt = _paramMap.begin();
    for (; itt != _paramMap.end(); itt++){
    }
    std::map<std::string, std::string>::const_iterator it = _paramMap.find(name);
    if(it == _paramMap.end()) {
        return false;
    }
    val = it->second;
    return true;
}

}
