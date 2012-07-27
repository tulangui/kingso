#ifndef __SORT_QUERY_H_
#define __SORT_QUERY_H_
#include <vector>
#include <string>
#include <map>
#include <vector>
#include "queryparser/Param.h"

using namespace std;
namespace sort_framework
{

class SortQuery
{
public:
    SortQuery(const queryparser::Param & param);
    SortQuery(const queryparser::Param & param, const std::vector<const char *> & desc);
    ~SortQuery() { }
public:
    const std::string & getAreaName() const { return _area; }
    const char * getParam(const char * key) const
    {
        if(!key) {
            return NULL;
        }
        return _param.getParam(key);
    }

    bool findParam(const std::string & name, std::string & val) const;
    bool findParam(const std::string & name) const
    {
        return _paramMap.find(name) != _paramMap.end();
    }
    const std::vector<const char*> & getSubDesc() const { return _descs; }

    void set_min_score(int64_t min)
    {
        _min = min;
    }
    int64_t get_min_score()
    {
        return _min;
    }
    void set_do_first_audi(bool do_first_audi)
    {
        _do_first_audi = do_first_audi;
    }
    bool get_do_first_audi()
    {
        return _do_first_audi;
    }
    void set_is_full(bool is_full)
    {
        _is_full = is_full;
    }
    bool get_is_full()
    {
        return _is_full;
    }
    void set_available_count(int32_t available_count)
    {
        _available_count = available_count;
    }
    int32_t get_available_count()
    {
        return _available_count;
    }
public:
    /* 将字符串str以空格切分，并保存到vec中 */
    //static void splitString(const char * str, std::vector<std::string> & vec, bool flag = true);
private:
    std::string _area;
    std::map<std::string, std::string> _paramMap;   //存储query的key、value对
    std::vector<const char *> _descs;
    const char* _pQuery;
    const queryparser::Param & _param;
    int64_t _min;
    bool _is_full;
    bool _do_first_audi;
    int32_t _available_count;
};

}
#endif
