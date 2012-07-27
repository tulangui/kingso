#include "CfgManager.h"
#include "SearchDef.h"
#include "commdef/commdef.h"
#include "util/Log.h"
#include "util/strfunc.h"
#include "util/StringUtility.h"
#include "util/Vector.h"

using namespace util;

namespace search{

CfgManager::CfgManager()
{
    _abnormal_len = MAX_INT32;
}

CfgManager::~CfgManager()
{
    HashMap<const char*, FieldConfNode*>::iterator it;
    HashMap<const char*, bool>::iterator           stopit;
    it = _field_map.begin();
    while(it != _field_map.end()) {
        stopit = it->value->stop_words.begin();
        while(stopit != it->value->stop_words.end()) {
            delete [] stopit->key;
            stopit++;
        }
        it->value->stop_words.clear();
        delete it->value;
        it++;
    }
    _field_map.clear();
}

int CfgManager::parse(mxml_node_t *config)
{
    char sub_field [MAX_FIELD_VALUE_LEN];
    char occ_str   [MAX_FIELD_VALUE_LEN];
    char weight_str[MAX_FIELD_VALUE_LEN];
    char *field_name = NULL;
    const char *p_attr = NULL;
    mxml_node_t *p_field_node = NULL;
    mxml_node_t *p_fields_node = NULL;
    mxml_node_t *p_sub_node = NULL;
    mxml_node_t *p_node = NULL;
    p_node = mxmlFindElement(config, config, "abnormal", 
                             NULL,   NULL,   MXML_DESCEND);
    if(p_node != NULL && p_node->parent == config) {
        p_attr = mxmlElementGetAttr(p_node, "length");
        if(p_attr && p_attr[0] != 0) {
            _abnormal_len = strtoul(p_attr, NULL, 10);
        }
    }

    p_fields_node = mxmlFindElement(config, config, "fields", 
                                    NULL,   NULL,   MXML_DESCEND);
    if(p_fields_node == NULL || p_fields_node->parent != config) {
        return KS_SUCCESS;
    }
    p_field_node = p_fields_node;
    p_field_node = mxmlFindElement(p_field_node, p_fields_node, "field", 
                                   NULL,         NULL,          MXML_DESCEND);
    while(p_field_node != NULL && p_field_node->parent == p_fields_node) {
        p_attr = mxmlElementGetAttr(p_field_node, "name");
        if(!p_attr || p_attr[0] == 0) {
            continue;
        }
        field_name = new char[strlen(p_attr)+1];
        strcpy(field_name, p_attr);
        
        p_sub_node = p_field_node;
        p_sub_node = mxmlFindElement(p_sub_node, p_field_node, "sub_field", 
                                   NULL,         NULL,          MXML_DESCEND);
        while(p_sub_node != NULL && p_sub_node->parent == p_field_node) {
            p_attr = mxmlElementGetAttr(p_sub_node, "name");
            if(!p_attr || p_attr[0] ==0) {
                break; 
            }
            strncpy(sub_field, p_attr, MAX_FIELD_VALUE_LEN);
            p_attr = mxmlElementGetAttr(p_sub_node, "occ");
            if(!p_attr || p_attr[0] ==0) {
                break; 
            }
            strncpy(occ_str, p_attr, MAX_FIELD_VALUE_LEN);
            p_attr = mxmlElementGetAttr(p_sub_node, "weight");
            if(!p_attr || p_attr[0] ==0) {
                break; 
            }
            strncpy(weight_str, p_attr, MAX_FIELD_VALUE_LEN);

            if(addPackageElement(field_name, sub_field, occ_str, weight_str) < 0) {
                return KS_ENOMEM;
            }
            p_sub_node = mxmlFindElement(p_sub_node, p_field_node, "sub_field", 
                                              NULL,         NULL,          MXML_DESCEND);
        }

        FieldConfNode *p_field = new FieldConfNode();
        if(p_field == NULL) {
            TERR("new memory failed");
            delete [] field_name;
            return KS_ENOMEM;
        }

        p_attr = mxmlElementGetAttr(p_field_node, "analyzer");
        if(p_attr && strcmp(p_attr, "MetaCharAnalyzer") == 0) {
            strcpy(p_field->analyzer, p_attr);
        }
        p_attr = mxmlElementGetAttr(p_field_node, "stopwords");
        if(p_attr && p_attr[0] != 0) { 
            char *p_ptr     = new char[strlen(p_attr)+1];
            char *stop_word = p_ptr;
            strcpy(p_ptr, p_attr);
            char* p_token = strchr(p_ptr, ',');
            do {
                if (p_token != NULL) {
                    *p_token ++ = '\0';
                }
                if (stop_word[0] != '\0') {    //空字符串忽略
                    char *p_word = str_trim(stop_word);
                    char *p_key  = new char[strlen(p_word)+1];
                    strcpy(p_key, p_word);
                    p_field->stop_words.insert(p_key, true);
                    TLOG("insert stop word. name:%s, field:%s", field_name, p_key);
                }
                if (p_token == NULL) {
                    break;
                }
                stop_word = p_token;
                p_token = strchr(p_token, ',');
            } while(true);
            delete[] p_ptr;
        }
        p_attr = mxmlElementGetAttr(p_field_node, "stopword_ignore");
        if(p_attr && p_attr[0] != 0 
                && (strcmp(p_attr, "Y") == 0 || strcmp(p_attr, "y") == 0)) 
        {
            p_field->ignore_stop_word = true;
        } else {
            p_field->ignore_stop_word = false;
        }
    
        _field_map.insert(field_name, p_field);
        p_field_node = mxmlFindElement(p_field_node, p_fields_node, "field", 
                                       NULL,         NULL,           MXML_DESCEND);
    }
    return KS_SUCCESS;

}
 
Package* CfgManager::getPackage(const char *package_name)
{
    HashMap<const char*, Package*>::iterator it;
    it = _packages.find(package_name);
    if(it != _packages.end()) {
        return it->value; 
    } 
    return NULL;
}

int CfgManager::addPackageElement(char *package_name, 
                                  char *sub_field, 
                                  char *p_occ, 
                                  char *p_weight)
{
    int32_t occ_min = 0;
    int32_t occ_max = 0;
    int32_t weight  = 0;
    char *p = strchr(p_occ, '-');
    if(p) {
        *p = '\0';
        p  += 1;
        occ_min = atoi(p_occ);
        occ_max = atoi(p);
    } else {
        occ_min = atoi(p_occ);
        occ_max = occ_min;
    }
    weight = atoi(p_weight);

    PackageElement *element = (PackageElement *) malloc (sizeof(PackageElement));
    if(!element) {
        TERR("malloc mem for element failed");
        return -1; 
    }
    strncpy(element->field_name, sub_field, MAX_FIELD_VALUE_LEN);
    element->occ_min    = occ_min;
    element->occ_max    = occ_max;
    element->weight     = weight;

    Package *p_package = NULL;
    HashMap<const char*, Package*>::iterator it = _packages.find(package_name);
    if(it != _packages.end()) {
        p_package = it->value;
        p_package->elements[p_package->size++] = element; 
    } else {
        p_package = (Package *) malloc (sizeof(Package));
        if(!p_package) {
            free(element); 
            TERR("malloc mem for package failed");
            return -1;
        }
        p_package->name = package_name;
        p_package->elements[0] = element;
        p_package->size = 1;
        _packages.insert(package_name, p_package);
    }
    return 0;
}

char* CfgManager::getAnalyzerName(const char* field_name)
{
    char *ret = NULL;
    HashMap<const char*, FieldConfNode*>::iterator it = _field_map.find(field_name);
    if(it != _field_map.end()) {
        ret = it->value->analyzer;
    }
    return ret;
}
 
bool CfgManager::isStopWord(const char* field_name, const char* field_value)
{
    bool ret = false;
    HashMap<const char*, FieldConfNode*>::iterator it = _field_map.find(field_name);
    if(it != _field_map.end() 
            && it->value->stop_words.find(field_value) != it->value->stop_words.end()) 
    {
        ret = true;
    }
    return ret;
}

bool CfgManager::stopWordIgnore(const char* field_name)
{
    bool ret = false;
    HashMap<const char*, FieldConfNode*>::iterator it = _field_map.find(field_name);
    if(it != _field_map.end()) {
        ret = it->value->ignore_stop_word;
    }
    return ret;
}

uint32_t CfgManager::getAbnormalLen()
{
    return _abnormal_len;
}

}

