#include "StatisticParser.h"

namespace statistic {

    StatisticParser::StatisticParser(index_lib::ProfileDocAccessor *pProfileAccessor)
        : _pProfileAccessor(pProfileAccessor)
    { 
    }

    StatisticParser::~StatisticParser()
    {
    }

    int32_t StatisticParser::doParse(MemPool *pMemPool , char *szStatClause)
    {
        if(unlikely(szStatClause == NULL || szStatClause[0] == '\0'))
        {
            return 0;
        }
        char *ptr = NULL;
        char *szKey = NULL;
        char *szValue = NULL;
        StatisticParameter *pStatParam = NULL;
        bool bFinished = false;
        bool bValidParam = true;
        bool bIsColon = false;
        ptr = szStatClause;
        szKey = ptr;

        do {
            switch(*ptr)
            {
                case '=':
                    *ptr = '\0';
                    szValue = ptr+1;
                    ptr++;
                    break;

                case ',':
                case ';':
                case '\0':
                    if(*ptr == ';')
                    {
                        bIsColon = true;
                        if(*(ptr+1) == '\0')    // 尾部分号作为结束符处理
                        {
                            bFinished = true;
                        }
                    }
                    else if(*ptr == '\0')
                    {
                        bFinished = true;
                    }
                    *ptr = '\0';

                    if(unlikely(szKey == NULL || szKey[0] == '\0'
                            || szValue == NULL || szValue[0] == '\0'))
                    {
                        TWARN("统计参数中指定的key和value中存在空值, 请检查.");
                        ptr++;
                        szKey = ptr;
                        szValue = NULL;
                        if(bIsColon)    // 如果是统计子句的最后一个kv对, 需要标识归零
                        {
                            bIsColon = false;
                            bValidParam = true;
                        }
                        else
                        {
                            bValidParam = false;
                        }
                        continue;
                    }
                    if(pStatParam == NULL)
                    {
                        pStatParam = (StatisticParameter*)NEW(pMemPool, StatisticParameter);
                        if(unlikely(pStatParam== NULL))
                        {
                            TWARN("构建统计参数子句对象失败");
                            return KS_ENOMEM; 
                        }

                    }
                    if(strcmp(szKey, "field") == 0)
                    {
                        if(isValidField(szValue))
                        {
                            pStatParam->szFieldName = szValue;
                        }
                        else
                        {
                            bValidParam = false;
                        }
                    }
                    else if(strcmp(szKey, "field2") == 0)
                    {
                        if(isValidField(szValue))
                        {
                            pStatParam->szFirstFieldName = szValue;
                        }
                        else
                        {
                            bValidParam = false;
                        }
                    }
                    else if(strcmp(szKey, "field3") == 0)
                    {
                        if(isValidField(szValue))
                        {
                            pStatParam->szSecondFieldName = szValue;
                        }
                        else
                        {
                            bValidParam = false;
                        }
                    }
                    else if(strcmp(szKey, "cattype") == 0)
                    {
                        if(strcmp(szValue, "count") == 0)
                        {
                            pStatParam->eStatType = ECountStatType;
                        }
                        else if(strcmp(szValue, "custom") == 0)
                        {
                            pStatParam->eStatType = ECustomStatType;
                        }
                        else if(strcmp(szValue, "multicount") == 0)
                        {
                            pStatParam->eStatType = EMultiCountStatType;
                        }
                        else if(strcmp(szValue, "sum") == 0)
                        {
                            pStatParam->eStatType = ESumStatType;
                            bValidParam = false;    // 不支持sum类型统计
                        }
                        else 
                        {
                            TWARN("统计子句中指定了不支持的统计类型: %s", szValue);
                            bValidParam = false;
                        }
                    }
                    else if(strcmp(szKey, "count") == 0)
                    {
                        pStatParam->nCount = atoi(szValue);
                        if(unlikely(pStatParam->nCount <= 0))
                        {
                            TWARN("统计子句中指定的count值为非正整数 %s", szValue);
                            bValidParam = false;
                        }
                    }
                    else if(strcmp(szKey, "percent") == 0)
                    {
                        pStatParam->nPercent = atoi(szValue);
                        if(unlikely(pStatParam->nPercent < 0 || pStatParam->nPercent > 100))
                        {
                      //    TWARN("统计子句中指定的percent: %d 无效, 按默认值0处理", pStatParam->nPercent);
                            pStatParam->nPercent = 0;
                        }
                    }
                    else if(strcmp(szKey, "cusvalue") == 0)
                    {
                        if(unlikely(szValue == NULL || szValue[0] == '\0'))
                        {
                            TWARN("统计子句指定的值cusvalue为空");
                            bValidParam = false;
                        }
                        else if(unlikely(strlen(szValue) >= MAX_CUSVALUE_LENGTH))
                        {
                            TWARN("统计子句指定的值cusvalue=%s 长度过长", szValue);
                            bValidParam = false;
                        }
                        else
                        {
                            pStatParam->szCusValue = szValue;
                        }
                    }
                    else if(strcmp(szKey, "field2value") == 0)
                    {
                        if(unlikely(szValue == NULL || szValue[0] == '\0'))
                        {
                            TWARN("统计子句指定的值field2value为空");
                            bValidParam = false;
                        }
                        else if(unlikely(strlen(szValue) >= MAX_MULTIVALUE_LENGTH))
                        {
                            TWARN("统计子句指定的值field2value=%s 长度过长", szValue);
                            bValidParam = false;
                        }
                        else
                        {
                            pStatParam->szFirstValue = szValue;
                        }
                    }
                    else if(strcmp(szKey, "field3value") == 0)
                    {
                        if(unlikely(szValue == NULL || szValue[0] == '\0'))
                        {
                            TWARN("统计子句指定的值field3value为空");
                            bValidParam = false;
                        }
                        else if(unlikely(strlen(szValue) >= MAX_MULTIVALUE_LENGTH))
                        {
                            TWARN("统计子句指定的值field3value=%s 长度过长", szValue);
                            bValidParam = false;
                        }
                        else
                        {
                            pStatParam->szSecondValue = szValue;
                        }
                    }
                    else if(strcmp(szKey, "sumfield") == 0) 
                    {
                        if(isValidField(szValue))
                        {
                            pStatParam->szSumFieldName = szValue;
                        }
                        else
                        {
                            bValidParam = false;
                        }
                    }
                    else if(strcmp(szKey, "exact") == 0)
                    {
                        pStatParam->nExact = atoi(szValue);
                        if(unlikely(pStatParam->nExact != 0 && pStatParam->nExact != 1))
                        {
                            TWARN("统计子句指定了无效的exact值: %s", szValue);
                            bValidParam = false;
                        }
                    }
                    else
                    {
                        TWARN("统计子句指定了无效的值: key=%s, value=%s ", szKey, szValue);
                        bValidParam = false;
                    }

                    ptr++;
                    szKey = ptr;
                    szValue = NULL;

                    if(bIsColon || bFinished)   // 分号和'\0'是统计子句结束的标示
                    {
                        if(bValidParam) // 只保留有效的统计子句·
                        {
                            if(unlikely(pStatParam == NULL || pStatParam->szFieldName == NULL))
                            {
                                TWARN("统计子句中未指定统计字段名, 请检查.");
                            }
                            else if(unlikely(pStatParam->eStatType == ECustomStatType && pStatParam->szCusValue == NULL))
                            {
                                TWARN("Custom类型统计子句中未指定cusvalue, 请检查.");
                            }
                            else if(unlikely(pStatParam->eStatType != ECustomStatType && pStatParam->szCusValue != NULL))
                            {
                                TWARN("Custom类型统计子句中未指定cattype, 请检查.");
                            }
                            else if(unlikely(pStatParam->eStatType == EMultiCountStatType
                                    && (pStatParam->szFirstFieldName == NULL || pStatParam->szFirstValue == NULL)))
                            {
                                TWARN("多维统计中未指定限定字段field2或取值");
                            }
                            else if(unlikely(pStatParam->eStatType == EMultiCountStatType
                                    && ((pStatParam->szSecondFieldName != NULL && pStatParam->szSecondValue == NULL)
                                        || (pStatParam->szSecondFieldName == NULL && pStatParam->szSecondValue != NULL))))
                            {
                                TWARN("多维统计中第二限定字段field3及取值需要同时指定");
                            }
                            else
                            {
                                _statParams.pushBack(pStatParam);
                            }
                        }
                        if(!bFinished)
                        {
                            pStatParam = (StatisticParameter*)NEW(pMemPool, StatisticParameter);
                            if(unlikely(pStatParam== NULL))
                            {
                                TWARN("构建统计参数子句对象失败");
                                return KS_ENOMEM; 
                            }
                            bValidParam = true;
                            bIsColon = false;
                        }
                    }
                    break;
                default:
                    ptr++;
                    break;
            }       

        } while(!bFinished);

        return 0;
    }

    StatisticParameter *StatisticParser::getStatParam(int32_t idx)
    {
        if(idx >= 0 && idx < _statParams.size())
        {
            return _statParams[idx];
        }
        return NULL;
    }

    int32_t StatisticParser::getStatNum()
    {
        return _statParams.size();
    }

    bool StatisticParser::isValidField(char *szFieldName)
    {
        if(unlikely(_pProfileAccessor == NULL))
        {
            TWARN("Profile读取接口为空, 无法进行字段有效性检查");
            return false;
        }
        if(unlikely(szFieldName == NULL || szFieldName[0] == '\0'))
        {
            TWARN("统计子句中指定的字段名为空值");
            return false;
        }
        const index_lib::ProfileField *pProfileField = _pProfileAccessor->getProfileField(szFieldName);
        if(unlikely(pProfileField == NULL))
        {
            TWARN("统计参数中指定了无效的字段名: %s", szFieldName);
            return false;
        }

        return true;
    }

}
