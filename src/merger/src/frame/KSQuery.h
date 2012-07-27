#ifndef _MS_KS_QUERY_H_
#define _MS_KS_QUERY_H_

#include "util/Log.h"
#include "util/MemPool.h"
#include "commdef/commdef.h"
#include <stdint.h>
#include <map>
#include <string>


typedef struct ks_value_s
{
    struct ks_value_s* next;
    char value[0];
}
ks_value_t;

class KSQuery
{
    public:
        KSQuery(const char *query, MemPool* heap)
        {
            _prefix = 0;
            _heap = 0;
            _params.clear();

            if(!query || !heap){
                return;
            }
            _heap = heap;

            char *p_last_mark = NULL;
            p_last_mark = rindex(query, '?');
            int prefix_len = 0;
            if(p_last_mark){
                _prefix = ks_strndup(query, p_last_mark-query);
            }

            char *p = p_last_mark?(p_last_mark+1):(char*)query;
            char *pend = NULL;
            char *p_eq = NULL;
            int finish = 0;
            char *name = NULL;
            char *value = NULL;
            char *ptmp = NULL;

            while(!finish){
                pend = index(p, '&');
                if(!pend){
                    finish = 1;
                    pend = p+strlen(p);
                }
                p_eq = index(p, '=');
                if(!p_eq || p_eq>pend || p_eq==p || p_eq==pend-1){
                    p = pend+1;
                    continue;
                }
                name = ks_strndup(p, p_eq-p);
                value = ks_strndup(p_eq+1, pend-p_eq-1);
                if(!name || !value){
                    p = pend+1;
                    continue;
                }
                setParam(name, value, 0);
                p = pend+1;
            }

            return;
        }
        ~KSQuery(){
            _params.clear();
        }
        char* getParam(const char *name)
        {
            KS_PARAMS_MAP::iterator it;
            char lower_name[MAX_FIELD_NAME_LEN+1];
            snprintf(lower_name, MAX_FIELD_NAME_LEN+1, "%s", name);
            int i=0;
            for(i=0; lower_name[i]; i++){
                lower_name[i] = tolower(lower_name[i]);
            }
            it = _params.find(lower_name);
            if(it!=_params.end()){
                return it->second->value;
            }
            return NULL;
        }
        ks_value_t *getParamInternal(const char *name)
        {
            KS_PARAMS_MAP::iterator it;
            char lower_name[MAX_FIELD_NAME_LEN+1];
            snprintf(lower_name, MAX_FIELD_NAME_LEN+1, "%s", name);
            int i=0;
            for(i=0; lower_name[i]; i++){
                lower_name[i] = tolower(lower_name[i]);
            }
            it = _params.find(lower_name);
            if(it != _params.end()){
                return it->second;
            }
            return NULL;
        }
        int setParam(const char *name, const char *value, int replace=1)
        {
            char lower_name[MAX_FIELD_NAME_LEN+1];
            snprintf(lower_name, MAX_FIELD_NAME_LEN+1, "%s", name);
            int i=0;
            for(i=0; lower_name[i]; i++){
                lower_name[i] = tolower(lower_name[i]);
            }
            ks_value_t *new_value = ks_valuedup(value);
            if(!new_value){
                TERR("Out of memory!");
                return -1;
            }
            ks_value_t *old_v = getParamInternal(lower_name);
            if(old_v){
                if(replace){
                    _params[lower_name] = new_value;
                }
                else{
                    new_value->next = old_v->next;
                    old_v->next = new_value;
                }
            }
            else{
                _params.insert(std::pair<std::string, ks_value_t*>(lower_name, new_value));
            }
            return 0;
        }
        char* delParam(const char *name)
        {
            char* retval = 0;
            KS_PARAMS_MAP::iterator it;
            char lower_name[MAX_FIELD_NAME_LEN+1];
            snprintf(lower_name, MAX_FIELD_NAME_LEN+1, "%s", name);
            int i=0;
            for(i=0; lower_name[i]; i++){
                lower_name[i] = tolower(lower_name[i]);
            }
            it = _params.find(lower_name);
            if(it != _params.end()){
                retval = it->second->value;
            }
            _params.erase(lower_name);
            return retval;
        }
        char* toString()
        {
            static const char* query_header = "/bin/search";
            static const int query_header_len = strlen(query_header);
            KS_PARAMS_MAP::iterator it;
            uint32_t total = 0;
            total = (_prefix?strlen(_prefix):query_header_len)+1; //prefix+'?'
            ks_value_t *ptmp = NULL;
            for(it=_params.begin(); it!=_params.end(); it++){
                for(ptmp=it->second; ptmp; ptmp=ptmp->next){
                    total += it->first.length();
                    total += strlen(ptmp->value);
                    total += 1; //for '='
                    total += 1; //for '&'
                }
            }

            char *query = NULL;
            query = (char*)ks_malloc(total);
            if(!query){
                return NULL;
            }
            int ret = 0;
            uint32_t written = 0;
            ret = snprintf(query+written, total-written, "%s?",  _prefix?_prefix:query_header);
            written += ret;
            for(it=_params.begin(); it!=_params.end(); it++){
                for(ptmp=it->second; ptmp; ptmp=ptmp->next){
                    //if(strcasecmp(it->first.c_str(),"debuglevel")==0){ //patch for debuglevel
                    //    ret = snprintf(query+written,total-written,"%s=%s&","debugLevel",ptmp->value);
                    //}
                    //else{
                    //    ret = snprintf(query+written,total-written,"%s=%s&",it->first.c_str(),ptmp->value);
                    //}
                    ret = snprintf(query+written, total-written, "%s=%s&", it->first.c_str(), ptmp->value);
                    written += ret;
                }
            }
            //不需要移除最后一个&,snprintf done.
            return query;
        }
        void *ks_malloc(uint32_t size)
        {
            if(!_heap){
                return NULL;
            }
            return NEW_VEC(_heap, char, size);
        }
        char *ks_strndup(const char *str, uint32_t len)
        {
            if(!str || len==0){
                return NULL;
            }
            if(!_heap){
                return NULL;
            }
            uint32_t str_len = len;
            char *ks_dup = NEW_VEC(_heap, char, str_len+1);
            if(!ks_dup){
                return NULL;
            }
            memcpy(ks_dup, str, str_len);
            ks_dup[str_len] = 0;
            return ks_dup;
        }
        char *ks_strdup(const char *str)
        {
            if(!str){
                return NULL;
            }
            uint32_t str_len = strlen(str);
            return ks_strndup(str, str_len);
        }
        ks_value_t *ks_valuedup(const char *value)
        {
            uint32_t str_len = 0;
            str_len = strlen(value);
            ks_value_t *retval = NULL;
            if(!_heap){
                return NULL;
            }
            retval = (ks_value_t*)NEW_VEC(_heap, char, str_len+sizeof(ks_value_t) +1);
            retval->next = NULL;
            snprintf(retval->value, str_len+1, "%s", value);
            return retval;
        }
    private:
        typedef std::map<std::string, ks_value_t*> KS_PARAMS_MAP;

        char* _prefix;
        MemPool* _heap;
        KS_PARAMS_MAP _params;
};

#endif //_MS_KS_QUERY_H_
