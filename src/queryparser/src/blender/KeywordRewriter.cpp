#include "KeywordRewriter.h"
#include "util/py_code.h"
#include "util/StringUtility.h"
#include "TokenizerPool.h"

namespace queryparser{

    KeywordRewriter::KeywordRewriter(MemPool *p_mem_pool,
                                     TokenizerPool *p_tokenizer_pool,
                                     StopwordDict *p_stopword_dict,
                                     const PackageConfigMap &package_conf_map)
        : _p_mem_pool(p_mem_pool),
        _p_tokenizer_pool(p_tokenizer_pool),
        _p_stopword_dict(p_stopword_dict),
        _package_conf_map(package_conf_map),
        _rew_keyword(NULL), _rew_keyword_len(0)
    {
    
    }

    KeywordRewriter::~KeywordRewriter()
    {
        _p_mem_pool = NULL;
        _p_tokenizer_pool = NULL;
        _p_stopword_dict = NULL; 
        
        _rew_keyword = NULL;
        _rew_keyword_len = 0; 
    }

    int32_t KeywordRewriter::doRewrite(const char *keyword, int32_t len)
    {
        char *keyword_copy = UTIL::replicate(keyword, _p_mem_pool, len);
        if(unlikely(keyword_copy == NULL))
        {
            TWARN("QUERYPARSER: alloc memory for keyword copy failed.");
            return -1;
        }

        SectionInfo section_infos[MAX_SECTION_NUM]; // keyword段落信息
        //memset((void*)section_infos, 0, sizeof(SectionInfo) * MAX_SECTION_NUM);
        int32_t     section_num = 0;                // keyword包含的段落数
        int32_t     ret         = -1;
        ret = doSectionSplit(keyword_copy, len, section_infos, section_num);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: split keyword to sections failed.");
            return -1;
        }

        ret = doSectionCheck(section_infos, section_num);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: sections check failed.");
            return -1;
        }

        ret = doKeywordTokenize(section_infos, section_num);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: tokenize keyword failed.");
            return -1;
        }
        
        ret = doSectionRemove(section_infos, section_num);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: bracket add failed.");
            return -1;
        }

        ret = doSectionCombine(section_infos, section_num);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: keyword section combine failed.");
            return -1;
        }

        return 0;
    }
    

    int32_t KeywordRewriter::doSectionCheck(SectionInfo *section_infos, int32_t section_num)
    {
        doBracketCheck(section_infos, section_num);
        doPackageNameCheck(section_infos, section_num);
        doOperatorCheck(section_infos, section_num);
        
        // 识别用户输入的括号并删除
        doBracketErase(section_infos, section_num);

        return 0;
    }

    int32_t doBracketMatch(SectionInfo *section_infos, 
                                            int32_t      section_num,
                                            char         left,
                                            char         right)
    {
        if(section_num <= 0)
        {
            return 0;
        }

        int32_t stack[section_num];
        int32_t top = -1;
        for(int32_t i=0; i<section_num; i++)
        {
            if(section_infos[i].type == QRST_BRACKET && section_infos[i].valid)
            {
                if(section_infos[i].str[0] == left)
                {
                    top++;
                    stack[top] = i;
                }
                else if(section_infos[i].str[0] == right)
                {
                    if(top >= 0)
                    {
                        section_infos[stack[top]].range = i - stack[top];
                        top--;
                    }
                    else
                    {
                        section_infos[i].valid = false;
                    }
                }
            }
        }

        while(top >= 0)
        {
            section_infos[stack[top]].valid = false;
            top--;
        }

        return 0;
    }

    int32_t KeywordRewriter::doBracketCheck(SectionInfo *section_infos, int32_t section_num)
    {
        // 先对[]进行匹配检查
        doBracketMatch(section_infos, section_num, PACKAGE_LEFT_BOUND, PACKAGE_RIGHT_BOUND);
        
        // 先检查[]内部的()
        for(int32_t i=0; i<section_num; i++)
        {
            if(section_infos[i].str[0] == PACKAGE_LEFT_BOUND
                    && section_infos[i].valid 
                    && section_infos[i].range != 0)
            {
                doBracketMatch(section_infos+i+1, section_infos[i].range-1, LEFT_BRACKET, RIGHT_BRACKET);
            }
        }
        
        // 然后对()进行匹配检查
        doBracketMatch(section_infos, section_num, LEFT_BRACKET, RIGHT_BRACKET);


        return 0;
    }

    int32_t KeywordRewriter::doBracketErase(SectionInfo *section_infos, int32_t section_num)
    {
        SectionType pre_type = QRST_OPERATOR;  // 保证最开始的左括号不被识别为用户输入
        for(int32_t i=0; i<section_num; i++)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }

            if(section_infos[i].type == QRST_BRACKET 
                    && section_infos[i].range != 0)  // 左括号
            {
                bool inner_operator = false;
                for(int j=1; j < section_infos[i].range; j++)
                {
                    if(section_infos[i+j].valid 
                            && section_infos[i+j].type == QRST_OPERATOR)
                    {
                        inner_operator = true;
                        break;
                    }
                }

                if(pre_type == QRST_TOKEN && inner_operator == false)
                {   
                    // 内部没有操作符, 括号前面是token，很可能是用户输入的括号
                    section_infos[i].valid                      = false;
                    section_infos[i+section_infos[i].range].valid = false;
                }
            }

            pre_type = section_infos[i].type;
        }

        return 0;
    }

    int32_t KeywordRewriter::doPackageNameCheck(SectionInfo *section_infos, int32_t section_num)
    {
        if(section_num <= 0)
        {
            return 0;
        }

        for(int32_t i=0; i<section_num-1; i++)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }
            if(section_infos[i].type == QRST_TOKEN)
            {
                if(section_infos[i].str && section_infos[i].str[section_infos[i].len-1] == FIELD_KEYVALUE_SEPARATOR
                        && strstr(section_infos[i].str, PACKAGE_FIELD_SEPARATOR) != NULL )
                {
                    int j = i+1;
                    while(section_infos[j].valid == false) {
                        j++;
                    }
                    if( section_infos[j].type == QRST_BRACKET && section_infos[j].range != 0) {
                        section_infos[i].type = QRST_PACKAGENAME;
                        section_infos[i].range = section_infos[j].range + j - i;
                    }
                }
            }
        }

        return 0;
    }

    /** 
     * @brief     将切分得到的段落信息赋值
     *
     * @param[in|out] section_infos keyword切分后的段落信息
     * @param[in|out] section_num   keyword切分后的段落个数，即当前待赋值的数组编号
     * @param[in]     str           段落信息内容
     * @param[in]     len           段落信息长度
     * @param[in]     type          段落类型
     * 
     * @return    0 成功; 非0 错误码 
     */
    int32_t setSectionInfo(SectionInfo *section_infos, int32_t &section_num,
                           char *str, int32_t len, SectionType type)
    {
        if(unlikely(section_num >= MAX_SECTION_NUM))
        {
            return -1;
        }
        section_infos[section_num].str            = str;
        section_infos[section_num].len            = len;
        section_infos[section_num].seg_begin      = NULL;
        section_infos[section_num].seg_len        = 0;
        section_infos[section_num].seg_result     = NULL;
        section_infos[section_num].seg_result_len = 0;
        section_infos[section_num].type           = type;
        section_infos[section_num].valid          = true;
        section_infos[section_num].range          = 0;
        section_num++; 

        return 0;    
    }

    int32_t KeywordRewriter::doSectionSplit(char *keyword, int32_t len,
                                            SectionInfo *section_infos, int32_t &section_num)
    {
        int32_t ret = 0;
 
        char *cur = keyword;
        char *end = keyword + len;
        char *section_begin = keyword;
        char *section_end = NULL;

        for(; cur < end; ++cur)
        {
           // 处理取非符号-
           if(*cur == SIGN_MINUS 
                   && (cur==keyword || *(cur-1)==LEFT_BRACKET || *(cur-1)==PACKAGE_LEFT_BOUND || *(cur-1)==SIGN_SPACES)
                   && cur+1!= end && *(cur+1)!=RIGHT_BRACKET && *(cur+1)!=PACKAGE_RIGHT_BOUND )
           { // 减号，且后面不是空格, 且在最开始或前面是空格或左括号，且不是最尾。
               ret = setSectionInfo(section_infos, section_num,
                                    cur, 1, QRST_OPERATOR);
               if(unlikely(ret != 0))
               {
                   break;
               }
               section_begin = cur + 1;
           }
           // 处理或运算符OR
           else if(end-cur>=OPERATOR_OR_LEN && memcmp(cur, OPERATOR_OR, OPERATOR_OR_LEN) == 0)
           {
                if(cur == keyword || *(cur-1) != SIGN_SPACES || cur+OPERATOR_OR_LEN == end
                        || *(cur+OPERATOR_OR_LEN) != SIGN_SPACES)  // 无效或运算符
                {
                    cur += OPERATOR_OR_LEN;
                }
                else
                { // OR，且不在最开始不在最尾，前后必须是空格，
                    ret = setSectionInfo(section_infos, section_num,
                                         OPERATOR_OR, OPERATOR_OR_LEN, QRST_OPERATOR);
                    if(unlikely(ret != 0))
                    {
                        break;
                    }
                    cur += OPERATOR_OR_LEN;
                    section_begin = cur + 1;
                }
           } 
           // 处理与运算符AND
           else if(end-cur>=OPERATOR_AND_LEN && memcmp(cur, OPERATOR_AND, OPERATOR_AND_LEN) == 0)
           {
                if(cur == keyword || *(cur-1) != SIGN_SPACES || cur+OPERATOR_AND_LEN == end
                        || *(cur+OPERATOR_AND_LEN) != SIGN_SPACES)  // 无效与运算符
                {
                    cur += OPERATOR_AND_LEN;
                }
                else
                { // AND, 且不在开始不在最尾，前后必须是空格 
                    ret = setSectionInfo(section_infos, section_num,
                                         OPERATOR_AND, OPERATOR_AND_LEN, QRST_OPERATOR);
                    if(unlikely(ret != 0))
                    {
                        break;
                    }
                    cur += OPERATOR_AND_LEN;
                    section_begin = cur + 1;
                }
           }
           // 处理取非运算符NOT
           else if(end-cur>=OPERATOR_NOT_LEN && memcmp(cur, OPERATOR_NOT, OPERATOR_NOT_LEN) == 0)
           {
                if(cur == keyword || *(cur-1) != SIGN_SPACES || cur+OPERATOR_NOT_LEN == end
                        || *(cur+OPERATOR_NOT_LEN) != SIGN_SPACES)  // 无效或运算符
                {
                    cur += OPERATOR_NOT_LEN;
                }
                else
                { // NOT, 且不在开始不在最尾，前后必须是空格
                    ret = setSectionInfo(section_infos, section_num,
                                         OPERATOR_NOT, OPERATOR_NOT_LEN, QRST_OPERATOR);
                    if(unlikely(ret != 0))
                    {
                        break;
                    }
                    cur += OPERATOR_NOT_LEN;
                    section_begin = cur + 1;
                }
           }
           else if(*cur == LEFT_BRACKET || *cur == PACKAGE_LEFT_BOUND)
           { // ( 或 [， 将它前面的作为一个token section，将它本身作为一个bracket section
                section_end = cur - 1;
                if(section_end >= section_begin)
                {
                    ret = setSectionInfo(section_infos, section_num,
                                         section_begin, section_end-section_begin+1, QRST_TOKEN);
                    if(unlikely(ret != 0))
                    {
                        break;
                    }
                }
                section_begin = cur + 1;

                ret = setSectionInfo(section_infos, section_num,
                                     cur, TOKEN_LEFT_BOUND_LEN, QRST_BRACKET);
                if(unlikely(ret != 0))
                {
                    break;
                }
           }
           else if(*cur == RIGHT_BRACKET || *cur == PACKAGE_RIGHT_BOUND)
           { // ) 或 ]，将它前面的作为一个token section，将它本身作为一个bracket section
                section_end = cur - 1;
                if(section_end >= section_begin)
                {
                    ret = setSectionInfo(section_infos, section_num,
                                         section_begin, section_end-section_begin+1, QRST_TOKEN);
                    if(unlikely(ret != 0))
                    {
                        break;
                    }
                }
                section_begin = cur + 1;
               
                ret = setSectionInfo(section_infos, section_num,
                                     cur, TOKEN_RIGHT_BOUND_LEN, QRST_BRACKET);
                if(unlikely(ret != 0))
                {
                    break;
                }
           }
           else if(*cur == SIGN_SPACES)
           { // 空格，将它前面的作为一个token section，将它本身忽略
               section_end = cur - 1;
               if(section_end >= section_begin)
               {
                   ret = setSectionInfo(section_infos, section_num,
                                        section_begin, section_end-section_begin+1, QRST_TOKEN);
                   if(unlikely(ret != 0))
                   {
                       break;
                   }
               }
               section_begin = cur + 1;
           }
        }
        
        if(unlikely(ret != 0 || section_num >= MAX_SECTION_NUM))
        {
            TWARN("QUERYPARSER: no enough space, section is too much.");
            return -1;
        }

        // 将剩余字符加入        
        section_end = cur - 1;
        if(section_end >= section_begin)
        {
            ret = setSectionInfo(section_infos, section_num,
                                 section_begin, section_end-section_begin+1, QRST_TOKEN);
        }

        return 0;
    }

    int32_t KeywordRewriter::doTokenizeDecision(SectionInfo *section_infos, int32_t section_num)
    { 
        int32_t ret = -1;
        char * flag = NULL;
        char *name = NULL;
        char *value = NULL;
        int32_t name_len = 0;
        int32_t value_len = 0;

        for(int32_t i = 0; i < section_num; ++i)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }

            if(section_infos[i].type == QRST_TOKEN)
            {
                char *str = section_infos[i].str;
                if((flag = strstr(str, PACKAGE_FIELD_SEPARATOR)) != NULL
                        && flag != str
                        && (flag-str+PACKAGE_FIELD_SEPARATOR_LEN) < section_infos[i].len)
                {
                    name      = str;
                    value     = flag + PACKAGE_FIELD_SEPARATOR_LEN;
                    name_len  = flag-name;
                    value_len = section_infos[i].len - name_len - PACKAGE_FIELD_SEPARATOR_LEN;

                    char *package_name = UTIL::replicate(name, _p_mem_pool, name_len);
                    PackageConfigConstItor const_itor = _package_conf_map.find(package_name);
                    if(const_itor != _package_conf_map.end() && const_itor->value->need_tokenize)
                    {
                        char *delim = strchr(value, FIELD_KEYVALUE_SEPARATOR);
                        if(delim != NULL && delim != value && (delim-value+1 < value_len))
                        {
                            section_infos[i].seg_begin = delim+1;
                            section_infos[i].seg_len   = value+value_len-delim-1;
                        }
                        else
                        {
                            section_infos[i].seg_begin = value;
                            section_infos[i].seg_len   = value_len;
                        }
                    }
                    else    // 不需要分词，如：userid::123
                    {
                        section_infos[i].seg_begin = NULL;
                        section_infos[i].seg_len = 0;
                    }
                }
                else if(section_infos[i].seg_result != NULL)
                {
                    // 在某个不需要分词的前缀的影响范围内
                    section_infos[i].seg_begin = NULL;
                    section_infos[i].seg_len   = 0;
                }
                else
                {
                    section_infos[i].seg_begin = section_infos[i].str;
                    section_infos[i].seg_len = section_infos[i].len;
                }

            }
            else if(section_infos[i].type == QRST_PACKAGENAME)
            {
                char * str = section_infos[i].str;
                flag = strstr(str, PACKAGE_FIELD_SEPARATOR);
                if(flag != NULL && flag != str 
                        && (flag-str+PACKAGE_FIELD_SEPARATOR_LEN) < section_infos[i].len)
                {
                    name     = str;
                    name_len = flag - str;
                    
                    char *package_name = UTIL::replicate(name, _p_mem_pool, name_len);
                    PackageConfigConstItor const_itor = _package_conf_map.find(package_name);
                    if(const_itor != _package_conf_map.end() && const_itor->value->need_tokenize)
                    {
                        // 需要分词，跟默认一致，让处理token的分支去处理即可
                    }
                    else
                    {
                        // 不需要分词，设置seg_result跟str一致，标志一下
                        for(int j=2; j<section_infos[i].range; j++)
                        {
                            if(section_infos[i+j].type == QRST_TOKEN && section_infos[i+j].valid)
                            {
                                section_infos[i+j].seg_result = section_infos[i+j].str;
                                section_infos[i+j].seg_result_len = section_infos[i+j].len;
                            }
                        }
                    }
                }
            }
        }
        return 0; 
    }

    int32_t KeywordRewriter::doKeywordTokenize(SectionInfo *section_infos, int32_t section_num)
    {
        int32_t ret = 0;
        
        ret = doTokenizeDecision(section_infos, section_num);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: check keyword tokenize failed.");
            return -1;
        }

        char *seg_begin = NULL;
        int32_t seg_len = 0;
        int32_t max_seg_result_len = 0;
        char *cur = NULL;
        
        ITokenizer *p_tokenizer = _p_tokenizer_pool->pop();
        if(unlikely(p_tokenizer == NULL))
        {
            TWARN("QUERYPARSER: get Tockenizer from pool failed.");
            return -1;
        }

        for(int32_t i = 0; i < section_num; ++i)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }
            if(section_infos[i].type == QRST_TOKEN && section_infos[i].seg_begin != NULL)
            {
                seg_begin = section_infos[i].seg_begin;
                seg_len = section_infos[i].seg_len;
                ret = py_utf8_normalize(seg_begin, seg_len, seg_begin, seg_len+1);
                if(unlikely(ret < 0))
                {
                    TWARN("QUERYPARSER: do keyword normalize failed.");
                    break;
                }

                max_seg_result_len = (seg_begin-section_infos[i].str) + seg_len * 2 + 3;
                section_infos[i].seg_result = NEW_VEC(_p_mem_pool, char, max_seg_result_len);
                if(unlikely(section_infos[i].seg_result == NULL))
                {
                    TWARN("QUERYPARSER: alloc memory for segment result failed.");
                    ret = KS_ENOMEM;
                    break;
                }
                section_infos[i].seg_result_len = 0;
                cur = section_infos[i].seg_result;
                // package类型查询，需要把title::xxx:先写入分词结果中，并写入左括号
                if(unlikely(seg_begin != section_infos[i].str))
                {
                    memcpy(cur, section_infos[i].str, seg_begin-section_infos[i].str);
                    section_infos[i].seg_result_len +=  seg_begin-section_infos[i].str; 
                    cur += seg_begin-section_infos[i].str;
                    *cur++ = LEFT_BRACKET;
                    section_infos[i].seg_result_len++;
                }
                
                ret = p_tokenizer->load(seg_begin);
                if(unlikely(ret < 0))
                {
                    TWARN("QUERYPARSER: tokenize keyword failed.");
                    break;
                }
                int32_t word_len = 0;

                const char * ws_str = NULL;
                int ws_lengh = 0;
                int ws_Serial = 0;

                while(p_tokenizer->next(ws_str, ws_lengh, ws_Serial) >= 0)
                {
                    if(unlikely(ws_lengh <= 0 || ws_str[0] == 0))
                    {
                        continue;
                    }
                    if(unlikely(ws_lengh+1 > max_seg_result_len-section_infos[i].seg_result_len))
                    {
                        TWARN("QUERYPARSER: no enough space for segment result.");
                        ret = -1;
                        break;
                    }
                    if(!_p_stopword_dict->find(ws_str, ws_lengh))
                    {
                        memcpy(cur, ws_str, ws_lengh);
                        cur += ws_lengh;
                        *cur = SIGN_SPACES;
                        cur++;
                        //section_infos[i].seg_result_len += itor->length+1;
                        word_len += ws_lengh+1;
                    }                        
                }
                if(unlikely(word_len == 0))
                {
                    section_infos[i].valid = false;
                }
                else
                {
                    // 去掉尾部空格
                    section_infos[i].seg_result_len += word_len;
                    section_infos[i].seg_result_len--;
                    // phrase类型查询，写入右括号
                    if(unlikely(seg_begin != section_infos[i].str))
                    {
                        *(cur-1) = RIGHT_BRACKET;
                        section_infos[i].seg_result_len++;
                    }
                } 
            }
            else if(section_infos[i].type == QRST_OPERATOR && section_infos[i].str[0] == SIGN_MINUS)
            {
                section_infos[i].seg_result     = OPERATOR_NOT;
                section_infos[i].seg_result_len = OPERATOR_NOT_LEN;
            }
            else if(section_infos[i].type == QRST_BRACKET)
            {
                if(section_infos[i].range != 0)
                {
                    section_infos[i].seg_result     = TOKEN_LEFT_BOUND;
                    section_infos[i].seg_result_len = TOKEN_LEFT_BOUND_LEN;
                }
                else
                {
                    section_infos[i].seg_result     = TOKEN_RIGHT_BOUND;
                    section_infos[i].seg_result_len = TOKEN_RIGHT_BOUND_LEN;
                }
            }
            else  // 无需分词的token或其他类型的section
            {
                section_infos[i].seg_result = section_infos[i].str;
                section_infos[i].seg_result_len = section_infos[i].len;
            }
        }
        _p_tokenizer_pool->push(p_tokenizer);

        return 0;
    }

    int32_t KeywordRewriter::doOperatorCheck(SectionInfo *section_infos, int32_t section_num)
    {
        if(section_num <= 0)
        {
            return 0;
        }

        // 末尾的操作符无效
        for(int i = section_num-1; i >= 0; --i)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }
            else if(section_infos[i].type == QRST_OPERATOR)
            {
                section_infos[i].valid = false;
            }
            else
            {
                break;
            }
        }

        //开头的非NOT操作符无效
        SectionType pre_type = QRST_BRACKET;
        int i=0;
        for(i=0; i<section_num; i++)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }
            else if(section_infos[i].type == QRST_OPERATOR
                    && (memcmp(section_infos[i].str, OPERATOR_NOT, OPERATOR_NOT_LEN) != 0
                        && memcmp(section_infos[i].str, "-", 1) != 0))

            {
                section_infos[i].valid = false;
            }
            else
            {
                pre_type = section_infos[i].type;
                break;
            }

        }

        // 连续的两个操作符去掉后面那个
        for(i++; i<section_num; i++) 
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }

            if(section_infos[i].type == QRST_OPERATOR && pre_type == QRST_OPERATOR)
            {
                // 前一个也是操作符的话，将这个操作符置为无效，不更新 pre_type
                section_infos[i].valid == false;
            }
            else if(section_infos[i].type == QRST_BRACKET && section_infos[i].range != 0)
            {
                // 递归处理括号内部的sections
                doOperatorCheck(section_infos + i + 1, section_infos[i].range - 1);
                // 内部处理完了，跳到对应的右括号
                i = i + section_infos[i].range;
                pre_type = QRST_BRACKET;
            }
            else
            {
                pre_type = section_infos[i].type;
            }
        }

        return 0;
    }

    int32_t KeywordRewriter::doOperatorRange(SectionInfo *section_infos, int32_t section_num)
    {
        // 确定每个后面跟着token或者packagename的有效操作符的影响范围(range)
        // 后面跟着左括号的操作符range仍保持为0
        for(int i=0; i<section_num; i++)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }
            
            if(section_infos[i].type == QRST_OPERATOR)
            {
                int j = i + 1;
                if(section_infos[i].str[0] == SIGN_MINUS)
                {
                    while(j < section_num && section_infos[j].valid == false)
                    {
                        j++;
                    }
                    section_infos[i].range = section_infos[j].range + j - i;
                }
                else
                {
                    while(j < section_num)
                    {
                        if(section_infos[j].valid == false
                                || section_infos[j].type == QRST_TOKEN)
                        {
                            j++;
                        }
                        else if(section_infos[j].type == QRST_PACKAGENAME)
                        {
                            j = j + section_infos[j].range + 1;
                        }
                        else
                        {
                            break;
                        }
                    }
                    section_infos[i].range = j - i - 1;  
                }
            }
        }

        return 0;
    }

    int32_t KeywordRewriter::doBracketRemove(SectionInfo *section_infos, int32_t section_num)
    {
        if(section_num <= 0)
        {
            return 0;
        }

        for(int i=0; i<section_num; i++)
        {
            if(section_infos[i].valid 
                    && section_infos[i].type == QRST_BRACKET 
                    && section_infos[i].range != 0)
            {
                bool all_invalid = true;
                for(int j = 1; j < section_infos[i].range; j++)
                {
                    if(section_infos[i+j].valid && section_infos[i+j].type == QRST_TOKEN)
                    {
                        all_invalid = false;
                        break;
                    }
                }
                if(all_invalid)
                {
                    for(int j = 0; j <= section_infos[i].range; j++)
                    {
                        section_infos[i+j].valid = false;
                    }
                }
            }
        }

        return 0;
    }

    int32_t KeywordRewriter::doPackageNameRemove(SectionInfo *section_infos, int32_t section_num)
    {
        for(int32_t i=0; i<section_num-1; i++)
        {
            if(section_infos[i].type == QRST_PACKAGENAME && section_infos[i].valid)
            {
                section_infos[i].valid = section_infos[i+1].valid;
            }
        }
        return 0;
    }

    int32_t KeywordRewriter::doOperatorRemove(SectionInfo *section_infos, int32_t section_num)
    {
        if(section_num <= 0)
        {
            return 0;
        }

        // 末尾的操作符无效
        for(int i = section_num-1; i >= 0; i++)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }
            else if(section_infos[i].type == QRST_OPERATOR)
            {
                section_infos[i].valid = false;
            }
            else
            {
                break;
            }
        }

        //开头的非NOT操作符无效
        for(int i=0; i<section_num; i++)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }
            else if(section_infos[i].type == QRST_OPERATOR
                    && ( memcmp(section_infos[i].str, OPERATOR_NOT, OPERATOR_NOT_LEN) != 0
                         && memcmp(section_infos[i].str, "-", 1) != 0)) 
            {
                section_infos[i].valid = false;
            }
            else
            {
                break;
            }

        }

        // 检查操作符右边的影响范围判断该操作符的有效性
        for(int i=0; i<section_num-1; i++) 
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }

            if(section_infos[i].type == QRST_OPERATOR)
            {
                if(section_infos[i+1].type == QRST_BRACKET && section_infos[i+1].range != 0)
                {
                    // 后面是左括号的话，操作符的有效性等于括号对的有效性
                    section_infos[i].valid == section_infos[i+1].valid;
                    // 递归处理括号对里面的操作符有效性
                    i++;
                    doOperatorRemove(section_infos + i + 1, section_infos[i].range - 1);
                    i = i + section_infos[i].range;
                }
                else
                {
                    int  j = i+1;
                    while(j<section_num && section_infos[j].valid == false)
                    {
                        j++;
                    }

                    if(section_infos[j].type != QRST_TOKEN 
                            && section_infos[j].type != QRST_PACKAGENAME)
                    {
                        // 后面没有一个有效的token或前缀，此操作符无效
                        section_infos[i].valid = false;
                    }
                    i = j-1;
                }
            }
            else if(section_infos[i].type == QRST_BRACKET && section_infos[i].range != 0)
            {
                doOperatorRemove(section_infos + i + 1, section_infos[i].range - 1);
                i = i + section_infos[i].range;
            }
        }

        return 0;
 
    }

    int32_t KeywordRewriter::doSectionRemove(SectionInfo *section_infos, int32_t section_num)
    {
        doBracketRemove(section_infos, section_num);
        doPackageNameRemove(section_infos, section_num);
        doOperatorRemove(section_infos, section_num);

        return 0;
    }

    int32_t KeywordRewriter::doSectionCombine(SectionInfo *section_infos, int32_t section_num)
    {
        doOperatorRange(section_infos, section_num);

        _rew_keyword = NEW_VEC(_p_mem_pool, char, MAX_KEYWORD_LEN);
        if(unlikely(_rew_keyword == NULL))
        {
            TWARN("QUERYPARSER: alloc memory for rewrite keyword failed.");
            return -1;
        }

        int   stack[section_num];
        int   top = -1;
        char *cur = _rew_keyword;
        for(int32_t i = 0; i < section_num; ++i)
        {
            if(section_infos[i].valid == false)
            {
                continue;
            }

            // 写入section
            if(unlikely(_rew_keyword_len+section_infos[i].seg_result_len > MAX_KEYWORD_LEN))
            {
                TWARN("QUERYPARSER: no enough space for rewrited keyword.");
                return -1;
            }
            memcpy(cur, section_infos[i].seg_result, section_infos[i].seg_result_len);
            cur += section_infos[i].seg_result_len;
            _rew_keyword_len += section_infos[i].seg_result_len;

            // section之间加上空格
            if(section_infos[i].type != QRST_PACKAGENAME)
            {
                *cur = SIGN_SPACES;
                cur++;
                _rew_keyword_len++;
            }

            // 对操作符加上适当的括号
            if(section_infos[i].type == QRST_OPERATOR && section_infos[i].range != 0)
            {
                cur[0] = LEFT_BRACKET;
                cur[1] = SIGN_SPACES;
                cur   += 2;
                _rew_keyword_len += 2;

                stack[++top] = i + section_infos[i].range;
            }
            else if(top >= 0)
            {
                while(i == stack[top])
                {
                    cur[0] = RIGHT_BRACKET;
                    cur[1] = SIGN_SPACES;
                    cur   += 2;
                    _rew_keyword_len += 2;
                    top--;
                }
            }
        }
        // 补全应添加的右括号
        while(top >= 0)
        {
            cur[0] = RIGHT_BRACKET;
            cur[1] = SIGN_SPACES;
            cur += 2;
            _rew_keyword_len += 2;
            top--;
        }

        // 只要写入了东西，就会有结尾的空格
        if(_rew_keyword_len > 0)
        {
            _rew_keyword_len--;
        }
        _rew_keyword[_rew_keyword_len] = '\0';

        return 0;
    }

    char *KeywordRewriter::getRewriteKeyword()
    {
        return _rew_keyword;
    }

    int32_t KeywordRewriter::getRewriteKeywordLen()
    {
        return _rew_keyword_len;
    }

}
