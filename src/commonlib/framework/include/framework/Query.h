/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 221 $
 *
 * $LastChangedDate: 2011-03-29 18:29:00 +0800 (Tue, 29 Mar 2011) $
 *
 * $Id: Query.h 221 2011-03-29 10:29:00Z taiyi.zjh $
 *
 * $Brief: deal with query's parser $
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_QUERY_H_
#define _FRAMEWORK_QUERY_H_
#include "util/common.h"
#include "util/namespace.h"
#include "framework/Compressor.h"
#include "framework/namespace.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <map>
#include <string>

FRAMEWORK_BEGIN;

class Query {
    public:
        Query();
        ~Query();
    public:
        /**
         *@brief 设置原始查询串
         *@param data 原始查询串
         *@param size 原始查询串长度
         *@return 0,成功 非0,失败
         */
        int32_t setOrigQuery(const char *data, uint32_t size);
        int32_t resetOrigQuery(const char *data, uint32_t size);
        /**
         *@brief 从原始查询串中去除前缀
         *@param prefix 查询串的前缀
         *@return 0,成功 非0,失败
         */
        int32_t purge(const char *prefix = NULL);
    public:
        /**
         *@brief 设置处理后的查询串
         *@param data 查询串
         *@param size 查询串长度
         *@return 0,成功 非0,失败
         */
        int32_t setPureQuery(const char *data, uint32_t size) {
            char *pureData;
            if (!data) {
                size = 0;
            }
            pureData = (char *)malloc(size + 1);
            if (unlikely(!pureData)) {
                return ENOMEM;
            }
            if (size > 0) {
                memcpy(pureData, data, size);
            }
            pureData[size] = '\0';
            if (_pureData) {
                ::free(_pureData);
            }
            _pureData = pureData;
            _pureSize = size;
            return SUCCESS;
        }
        /**
         *@brief 设置查询串中的subcluste名称
         *@param clusters subcluster名称
         *@return 0,成功 非0,失败
         */
        int32_t setSubClusters(const char *clusters) {
            char *subClusters = NULL;
            if (clusters) {
                subClusters = strdup(clusters);
                if (unlikely(!subClusters)) {
                    return ENOMEM;
                }
            }
            if (_subClusters) {
                ::free(_subClusters);
            }
            _subClusters = subClusters;
            return SUCCESS;
        }
        /**
         *@brief 设置查询串中的压缩类型 
         *@param type 压缩类型
         */
        void setCompress(compress_type_t type) { 
            _compr = type; 
        }
        /**
         *@brief 设置查询串中的usecache参数
         *@param on 是否使用cache
         */
        void setUseCache(bool on = true) { 
            _useCache = on; 
        }
    public:
        /**
         *@brief 获取原始查询串
         *@return 原始查询串
         */
        const char *getOrigQueryData() const { 
            return _origData; 
        }
        /**
         *@brief 获取原始查询串长度
         *@return 原始查询串长度
         */
        uint32_t getOrigQuerySize() const { 
            return _origSize; 
        }
        bool getSeparate() const { return _separate; }
        bool getSearchLists() const { return _searchlists ; }
        void setSeparate(bool sep) { _separate = sep; }
        const char *getUniqInfo() const { return _uniqInfo ; }
        const char *getUniqField() const { return _uniqField; }
        void setUniqField(char *field) {
            if(_uniqField) ::free(_uniqField);
            _uniqField = field;
        }
        void setUniqInfo(char *buf) {
            if(_uniqInfo) ::free(_uniqInfo);
            _uniqInfo = buf;
        }
        /**
         *@brief 获取处理后的查询串
         *@return 处理后的查询串
         */
        const char *getPureQueryData() const {
            return _pureData; 
        }
        /**
         *@brief 获取处理后的查询串长度
         *@return 处理后的查询串长度
         */
        uint32_t getPureQuerySize() const {
            return _pureSize;
        }
        /**
         *@brief 获取subcluster参数值
         *@return subcluster参数值
         */
        const char *getSubClusters() const { 
            return _subClusters; 
        }
        /**
         *@brief 获取compress参数值
         *@return compress参数值
         */
        compress_type_t getCompress() const { 
            return _compr; 
        }
        /**
         *@brief 获取usecache参数值
         *@return usecache参数值
         */
        bool useCache() const { 
            return _useCache; 
        }
        /**
         *@brief 取得query语句中的返回结果数
         *@return 返回要返回的结果数
         **/
        int getCount();
        /**
         *@brief 设置qurey语句中的返回结果数
         *@param count 返回结果数
         **/
        void setCount(int count) { 
            _nCount = count; 
        } 
        /**
         *@brief 取得query语句中的返回结果的开始位置
         *@return 返回要返回结果的开始位置
         **/
        int getStart();
    public:
        /**
         *@brief 获取指定参数的值
         *@param q 查询串
         *@param size 查询串长度
         *@param name 参数名称
         *@param out 返回的参数值
         **/
        static int32_t getParam(char *q, uint32_t  &size, 
                const char *name,
                char *& out);

        static int32_t getStatisticParam(char *q, uint32_t &size, 
                const char *name,
                char * &out);
        /**
         *@brief 获取指定参数的值
         *@param q 查询串
         *@param size 查询串长度
         *@param name 参数名称
         *@return 返回的参数值
         **/
        static char *getParam(const char *q, uint32_t size, 
                const char *name);
        /**
         *@brief 取得query语句中的cluster
         *@return cluster query
         **/
        std::map<std::string, std::string> &getClusterQuery() {
            return _clusterQuery; 
        }
    private:
        void clear();
    private:
        char *_origData;
        uint32_t _origSize;
        char *_pureData;
        uint32_t _pureSize;
        char *_subClusters;
        compress_type_t _compr;
        bool _useCache;
        std::map<std::string, std::string> _clusterQuery;
        int _nCount; //返回结果条数
        int _nStart; //返回结果起始位置
        char *_uniqField; //uniqField字段名
        char *_uniqInfo;  //uniqInfo查询串
        bool _separate;   //是否分步查询
        bool _searchlists;
};

FRAMEWORK_END

#endif //_FRAMEWORK_QUERY_H_

