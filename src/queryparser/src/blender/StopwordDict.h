/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-11-23 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: StopwordDict.h 2577 2011-11-23 01:50:55Z yanhui.tk $
 *
 * $Brief: 停用词表结果，采用bitmap结构，
 *          每一个bit保存一个整数(字符对应的编码)的存在或不存在 $
 ********************************************************************/

#ifndef QUERYPARSER_STOPWORDDICT_H
#define QUERYPARSER_STOPWORDDICT_H

#include "util/common.h"
#include "util/errcode.h"
#include <stdint.h>
#include <locale.h>

namespace queryparser{

    class StopwordDict
    { 
        public:
            StopwordDict();

            ~StopwordDict();


            /** 
             * @brief     初始化bitmap
             *
             * @param[in] byte_count utf8编码单个中文字符字节数, 默认是单个汉字
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t init(int32_t byte_count = 3);
            
            /** 
             * @brief     初始化对外调用接口
             *
             * @param[in] filename 停用词表文件名
             * @param[in] byte_count utf8编码单个中文字符字节数, 默认是单个汉字
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t init(const char *filename, int32_t byte_count = 3);

            /** 
             * @brief     在bitmap中查找停用词是否存在
             *
             * @param[in] item 停用词字符串
             * @param[in] len  停用词字符串长度
             * 
             * @return    true 找到; false 未找到
             */
            bool find(const char *item, int32_t len);

            /** 
             * @brief     在bitmap中查找停用词是否存在
             *
             * @param[in] item  停用词字符
             * 
             * @return    true 找到; false 未找到
             */
            bool find(int64_t item);

            /** 
             * @brief     向bitmap中添加停用词
             *
             * @param[in] item 停用词字符串
             * @param[in] len  停用词字符串长度
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t add(char *item, int32_t len);
            
            /** 
             * @brief     向bitmap中添加停用词
             *
             * @param[in] item 停用词字符
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t add(int64_t item);

        private:
            char    *_bits;             // 停用词的位图
            int32_t  _byte_count;       // 停用词的最大字节数
    };

    inline StopwordDict::StopwordDict()
        : _bits(NULL), _byte_count(0)
    {

    }

    inline StopwordDict::~StopwordDict()
    {
        if(_bits)
        {
            delete [] _bits;
            _bits = NULL;
        }
        _byte_count = 0;
    }

    inline int32_t StopwordDict::init(int32_t byte_count)
    {
        if(byte_count <= 0)
        {
            return KS_EFAILED;
        }
        _byte_count = byte_count;
        int32_t item_count = (1<<(byte_count*8))/8;
        _bits = new char [item_count];
        if(unlikely(_bits == NULL))
        {
            TERR("QUERYPARSER: alloc memory for bitmap failed.");
            return KS_EFAILED;
        }
        bzero(_bits, item_count);

        return KS_SUCCESS;
    }

    inline int32_t StopwordDict::init(const char *filename, int32_t byte_count)
    {
        int32_t ret = init(byte_count);
        if(unlikely(ret != KS_SUCCESS))
        {
            TERR("QUERYPARSER: init bitmap failed.");
            return KS_EFAILED;
        }

        FILE *fp = fopen(filename, "rb");
        if(fp == NULL)
        {
            TERR("QUERYPARSER: fopen stopword list file failed, file does not exist.");
            return KS_EFAILED;
        }
        
        char line[128] = {0};
        while(fgets(line, sizeof(line), fp))
        {
            if(add(line, strlen(line)) != 0)
            {
                TWARN("QUERYPARSER: invalid stopword: %s, ignore it.", line);
                continue;
            }
        }
        fclose(fp);
        setlocale(LC_ALL, "zh_CN.UTF-8");
     
        return KS_SUCCESS;
    }

    inline bool StopwordDict::find(const char *item, int32_t len)
    {
        char tp_word[4] = {0, 0, 0, 0};
        wchar_t ws_word[4] = {0, 0, 0, 0};

        if(len > _byte_count)
        {
            return false;
        }
        memcpy(tp_word, item, len);
        mbstowcs(ws_word, tp_word, 3);
        if(ws_word[1]!=0)
        {
            return false;
        }
        return find(int32_t(ws_word[0]));
    }

    inline bool StopwordDict::find(int64_t item)
    { 	
        if(unlikely(item < 0 || item >= (1<<(_byte_count*8))))
        {
            return false;
        }
        int32_t pos = item / 8;
        int32_t offset = item % 8;

        return (_bits[pos] & (1<<offset));
    }
    
    inline int32_t StopwordDict::add(char *item, int32_t len)
    {
		return add(strtol(item, NULL, 10));
    }

    inline int32_t StopwordDict::add(int64_t item)
    {
        if(unlikely(item < 0 || item >= (1<<(_byte_count*8))))
        {
            return KS_EFAILED;
        }
        int32_t pos = item / 8;
        int32_t offset = item % 8;

        _bits[pos] |= (1 << offset);
        
        return KS_SUCCESS;
    }
}

#endif
