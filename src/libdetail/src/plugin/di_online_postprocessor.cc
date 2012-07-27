#include "di_postprocessor_header.h"
#include "util/Log.h"
#include "util/Postcode.h"
#include "util/strfunc.h"
#include "detail.h"
#include "util/py_code.h"
#include <math.h>


/* some define */
#define g_di_hl_start "<span class=H>"           // 语法高亮标签头
#define g_di_hl_end  "</span>"                   // 语法高亮标签尾
#define MAX_HL_TERMS 64                          // 飘红关键字最大个数
#define MAX_HL_TITLE_LEN 1024                    // 飘红后标题的最大长度
#define MULTIVAL_CNT_MAX 64                      // 单文档中多值信息最大数量


static const char* name_string = "default";
static const int name_length = 7;

static char g_di_model_string[MAX_MODEL_LEN] = {0};   // detail数据样式
static int g_di_model_length = 0;                     // detail数据样式长度
static const char** g_di_field_array = 0;             // detail字段名数组
static const int* g_di_flen_array = 0;                // detail字段名长度数组
static int g_di_field_count = 0;                      // detail中存储的字段数量
static int g_di_exfield_count = 0;                    // detail展示中添加的字段数量

static int g_di_title_idx = -1;                       // title在detail中的field序号
static int g_di_tmall_title_idx = -1;                 // tmall_title在detail中的field序号
static int g_di_search_prop_idx = -1;                 // search_prop在detail中的field序号
static int g_di_shop_title_idx = -1;                  // shop_title在detail中的field序号
static int g_di_vidname_idx = -1;                     // vidname在detail中的field序号
static int g_di_post_idx = -1;                        // real_post_fee在detail中的field序号
static int g_di_price_idx = -1;                       // price在detail中的field序号
static int g_di_sku_min_idx = -1;                     // sku_min在detail中的field序号
static int g_di_sku_max_idx = -1;                     // sku_max在detail中的field序号
static int g_di_ends_idx = -1;                        // ends在detail中的field序号
static int g_di_start_idx = -1;                       // start在detail中的field序号
static int g_di_oldstart_idx = -1;                    // old_start在detail中的field序号
static int g_di_latlng_idx = -1;                      // latlng在detail中的field序号
static int g_di_zk_time_idx = -1;                     // zk_time在detail中的field序号
static int g_di_zk_rate_idx = -1;                     // zk_rate在detail中的field序号
static int g_di_zk_rate2_idx = -1;                    // zk_rate2在detail中的field序号
static int g_di_zk_rate3_idx = -1;                    // zk_rate3在detail中的field序号
static int g_di_zk_money_idx = -1;                    // zk_money在detail中的field序号
static int g_di_zk_group_idx = -1;                    // zk_group在detail中的field序号


#ifdef __cplusplus
extern "C"{
#endif


/*
 * 初始化detail后处理器
 *
 * @param field_count 字段总数
 *        field_array 字段名字数组
 *        flen_array 字段名字长度数组
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
extern int di_postprocessor_init(const int field_count, const char** field_array, const int* flen_array)
{
	char* ptr = g_di_model_string;
	int left = MAX_MODEL_LEN;
	int i = 0;
    int cpylen = 0;
	
	if(field_count<=0 || !field_array || !flen_array){
		TERR("di_postprocessor_init argument error, return!");
		return -1;
	}
	
	for(i=0; i<field_count; i++){
		if(left<=flen_array[i]+1){
			TERR("g_di_model_string no enough space, return!");
			return -1;
		}
	
		// 查找特殊处理字段
        if(flen_array[i]==5 && strncasecmp(field_array[i], "title", 5)==0){
			g_di_title_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==11 && strncasecmp(field_array[i], "tmall_title", 11)==0){
			g_di_tmall_title_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==11 && strncasecmp(field_array[i], "search_prop", 11)==0){
			g_di_search_prop_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==10 && strncasecmp(field_array[i], "shop_title", 10)==0){
			g_di_shop_title_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==7 && strncasecmp(field_array[i], "vidname", 7)==0){
			g_di_vidname_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
	    else if(flen_array[i]==13 && strncasecmp(field_array[i], "real_post_fee", 13)==0){
			g_di_post_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==13 && strncasecmp(field_array[i], "reserve_price", 13)==0){
			g_di_price_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==7 && strncasecmp(field_array[i], "sku_min", 7)==0){
			g_di_sku_min_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==7 && strncasecmp(field_array[i], "sku_max", 7)==0){
			g_di_sku_max_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==4 && strncasecmp(field_array[i], "ends", 4)==0){
			g_di_ends_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==6 && strncasecmp(field_array[i], "starts", 6)==0){
			g_di_start_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
		else if(flen_array[i]==10 && strncasecmp(field_array[i], "old_starts", 10)==0){
			g_di_oldstart_idx = i;
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
		}
        else if(flen_array[i]==6 && strncmp(field_array[i], "latlng", 6)==0){
            g_di_latlng_idx = i;
            g_di_exfield_count ++;
            memcpy(ptr, "latlng\001latlngdistance", 21);
            cpylen = 21;
        }
        else if(flen_array[i]==7 && strncmp(field_array[i], "zk_time", 7)==0){
            g_di_zk_time_idx = i;
            memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
        }
        else if(flen_array[i]==7 && strncmp(field_array[i], "zk_rate", 7)==0){
            g_di_zk_rate_idx = i;
            memcpy(ptr, "promotions", 10);
            cpylen = 10;
        }
        else if(flen_array[i]==8 && strncmp(field_array[i], "zk_rate2", 8)==0){
            g_di_zk_rate2_idx = i;
            memcpy(ptr, "sku_min_promotions", 18);
            cpylen = 18;
        }
        else if(flen_array[i]==8 && strncmp(field_array[i], "zk_rate3", 8)==0){
            g_di_zk_rate3_idx = i;
            memcpy(ptr, "sku_max_promotions", 18);
            cpylen = 18;
        }
        else if(flen_array[i]==8 && strncmp(field_array[i], "zk_money", 8)==0){
            g_di_zk_money_idx = i;
            memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
        }
        else if(flen_array[i]==8 && strncmp(field_array[i], "zk_group", 8)==0){
            g_di_zk_group_idx = i;
            memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
        }
		else{
		    memcpy(ptr, field_array[i], flen_array[i]);
            cpylen = flen_array[i];
        }
		ptr[cpylen] = '\001';
		ptr += cpylen+1;
		left -= cpylen+1;
	}
	
	if(left<1){
		TERR("g_di_model_string no enough space, return!");
		return -1;
	}
	*ptr = 0;
	g_di_model_length = ptr-g_di_model_string;
	g_di_field_count = field_count;
	g_di_field_array = field_array;
	g_di_flen_array = flen_array;
	
	return 0;
}

/*
 * 清理detail后处理器
 */
extern void di_postprocessor_dest()
{
}

static int di_default_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static double di_real_post_fee_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_price_postprocessor(char* input, int inlen, char* output, int outmax, int discount, double post_fee);
static int di_ends_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_start_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_oldstart_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_highlight_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_vidname_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_latlng_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_zk_time_postprocessor(char* input, int inlen, char* output, int outmax, unsigned long long* zk_time, char* arg);
static int di_multi_value_postprocessor(char* input, int inlen, char* output, int outmax, int idx);
static int di_multi_rate_postprocessor(char* input, int inlen, char* output, int outmax, int idx);

/*
 * 执行detail后处理器
 *
 * @param data 一条detail数据
 *        size 数据长度
 *        args 处理每个字段需要的参数数组
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
extern char* di_postprocessor_work(char* input, int inlen, char** args)
{
	char* input_tmp = 0;
	
	int multi_idx = -1;
    unsigned long long zk_time = 0;
	double post_fee = 0.0;
	
	char* output = 0;
	char* outcur = 0;
	int outlen = 0;
	char* incur = 0;
	char* inend = 0;
	char* inarg = 0;
	int field_no = 0;
	int ret = 0;
	
    input_tmp = strdup(input);
    if(!input_tmp){
        return 0;
    }
    incur = input_tmp;
    inend = input_tmp+inlen; 
	
	output = (char*)malloc(MAX_DETAIL_LEN);
	if(!output){
		TWARN("Out of memory!");
        free(input_tmp);
		return 0;
	}
	outcur = output;
	outlen = MAX_DETAIL_LEN;
	
	for(inarg=incur; inarg<inend; inarg++){
		if(*inarg=='\001'){
            if(field_no==g_di_zk_time_idx){
				multi_idx= di_zk_time_postprocessor(incur, inend-incur, 0, 0, &zk_time, args[field_no]);
			}
			else if(field_no==g_di_post_idx){
				post_fee = di_real_post_fee_postprocessor(incur, inend-incur, 0, 0, args[field_no]);
			}
			incur = inarg+1;
			field_no ++;
		}
	}
	
	field_no = 0;
	memcpy(input_tmp, input, inlen);
	input_tmp[inlen] = 0;
	incur = input_tmp;
	for(inarg=incur; inarg<inend; inarg++){
		if(*inarg=='\001'){
			if(field_no==g_di_title_idx){
				ret = di_highlight_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
            else if(field_no==g_di_tmall_title_idx){
				ret = di_highlight_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
            else if(field_no==g_di_search_prop_idx){
				ret = di_highlight_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
            else if(field_no==g_di_shop_title_idx){
				ret = di_highlight_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
            else if(field_no==g_di_vidname_idx){
				ret = di_vidname_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
		    else if(field_no==g_di_post_idx){
				ret = snprintf(outcur, outlen, "%.2f\001", post_fee);
				if(ret>outlen){
					ret = -1;
				}
			}
			else if(field_no==g_di_price_idx){
				ret = di_price_postprocessor(incur, inend-incur, outcur, outlen, 10000, post_fee);
			}
			else if(field_no==g_di_sku_min_idx){
				ret = di_price_postprocessor(incur, inend-incur, outcur, outlen, 10000, post_fee);
			}
			else if(field_no==g_di_sku_max_idx){
				ret = di_price_postprocessor(incur, inend-incur, outcur, outlen, 10000, post_fee);
			}
			else if(field_no==g_di_ends_idx){
				ret = di_ends_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
			else if(field_no==g_di_start_idx){
				ret = di_start_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
			else if(field_no==g_di_oldstart_idx){
				ret = di_oldstart_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
            else if(field_no==g_di_latlng_idx){
                ret = di_latlng_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
            }
            else if(field_no==g_di_zk_time_idx){
                ret = snprintf(outcur, outlen, "%llu\001", zk_time);
                if(ret>outlen){
                    ret = -1;
                }
            }
            else if(field_no==g_di_zk_rate_idx){
                ret = di_multi_rate_postprocessor(incur, inend-incur, outcur, outlen, multi_idx);
            }
            else if(field_no==g_di_zk_rate2_idx){
                ret = di_multi_rate_postprocessor(incur, inend-incur, outcur, outlen, multi_idx);
            }
            else if(field_no==g_di_zk_rate3_idx){
                ret = di_multi_rate_postprocessor(incur, inend-incur, outcur, outlen, multi_idx);
            }
            else if(field_no==g_di_zk_money_idx){
                ret = di_multi_value_postprocessor(incur, inend-incur, outcur, outlen, multi_idx);
            }
            else if(field_no==g_di_zk_group_idx){
                ret = di_multi_value_postprocessor(incur, inend-incur, outcur, outlen, multi_idx);
            }
			else{
				ret = di_default_postprocessor(incur, inend-incur, outcur, outlen, args[field_no]);
			}
			if(ret<0 || ret>outlen){
                free(input_tmp);
				free(output);
				return 0;
			}
			incur = inarg+1;
			outcur += ret;
			outlen -= ret;
			field_no ++;
		}
	}
	*outcur = 0;
    free(input_tmp);
	
	return output;
}

/*
 * 向字段参数数组中添加参数
 *
 * @param args_p 字段参数数组
 * @param argcnt 字段参数数组长度
 * @param field 待添加字段
 * @param flen 待添加字段名字长度
 * @param arg 参数
 */
extern void di_argument_set(char** args, const int argcnt, const char* field, const int flen, char* arg)
{
    int i=0;

    if(!args || argcnt<g_di_field_count || !field || flen<=0){
        return;
    }

    if(flen==5 && strncmp(field, "title", 5)==0){
        if(g_di_title_idx>=0){
            args[g_di_title_idx] = arg;
        }
        if(g_di_tmall_title_idx>=0){
            args[g_di_tmall_title_idx] = arg;
        }
        if(g_di_search_prop_idx>=0){
            args[g_di_search_prop_idx] = arg;
        }
        if(g_di_shop_title_idx>=0){
            args[g_di_shop_title_idx] = arg;
        }
        if(g_di_vidname_idx>=0){
            args[g_di_vidname_idx] = arg;
        }
    }
    else if(flen==13 && strncmp(field, "real_post_fee", 13)==0){
        if(g_di_post_idx>=0){
            args[g_di_post_idx] = arg;
        }
    }
    else if(flen==4 && strncmp(field, "ends", 4)==0){
        if(g_di_ends_idx>=0){
            args[g_di_ends_idx] = arg;
        }
    }
    else if(flen==7 && strncmp(field, "zk_time", 7)==0){
        if(g_di_zk_time_idx>=0){
            args[g_di_zk_time_idx] = arg;
        }
    }
    else if(flen==6 && strncmp(field, "latlng", 6)==0){
        if(g_di_latlng_idx>=0){
            args[g_di_latlng_idx] = arg;
        }
    }
}

/*
 * 获取detail后处理器名字
 *
 * @return detail后处理器名字
 */
extern const char* di_get_name_string()
{
	return name_string;
}

/*
 * 获取detail后处理器名字长度
 *
 * @return detail后处理器名字长度
 */
extern const int di_get_name_length()
{
	return name_length;
}

/*
 * 获取处理后展示的字段数量
 *
 * @return 字段数量
 */
extern const int di_get_field_count(const char* dl)
{
	return g_di_field_count+g_di_exfield_count;
}

/*
 * 获取处理后展示的字段样式
 *
 * @return 字段样式
 */
extern const char* di_get_model_string(const char* dl)
{
	return g_di_model_string;
}

/*
 * 获取处理后展示的字段样式长度
 *
 * @return 字段样式长度
 */
extern const int di_get_model_length(const char* dl)
{
	return g_di_model_length;
}


/*
 * 分割detail
 *
 * @param str 待分割字符串
 * @param items 用于保存分割结果的指针数组
 * @param item_size 用于保存分割结果的指针数组长度
 * @param delim 分割依据，出现在该字符串中的字符都是分割依据
 *
 * @return < 0 分割出错
 *         >=0 分割结果个数
 */
static int split_with_blank(char* str, char** items, int item_size, const char* delim)
{
	char* begin = 0;
	char* end = 0;
	int cnt = 0;
	int finish = 0;
	
	if(!str || !items || item_size<=0 || !delim){
		return -1;
	}

	begin = 0;
	while(!finish){
		// find word begin
		if(!begin){
			begin = str;
		}

		// find the word end.
		end = begin;
		while(*end && strchr(delim, *end)==0){
			end++;
		}
		if(*end==0){
			finish = 1;
		}

		if(end>begin){
			if(cnt==item_size){
		  	return -1;
			}
			items[cnt++] = begin;
		}
		
		if(*end){
			*end = 0;
			begin = end+1;
		}
		else{
			break;
		}
	}
	
	return cnt;
}

static int di_default_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
	char* inend = input+inlen;
	char* incur = input;
	char* outend = output+outmax;
	char* outcur = output;
	int len = 0;
	
	for(; (incur<inend)&&(outcur<outend); incur++,outcur++){
		*outcur = *incur;
		len ++;
		if(*incur=='\001'){
			break;
		}
	}
    if(outcur==outend){
        return (len+1);
    }
	
	return len;
}

static int parse_userloc(char* loc)
{
	bool alldigit = true;
	char* cur = loc;
	const char* postcode = 0;
	
	if(!loc || strlen(loc)==0){
		return -1;
	}
	
	for(; *cur; cur++) {
		if(!isdigit(*cur)) {
			alldigit = false;
			break;
		}
	}
	
	if(alldigit){
		return atoi(loc);
	}
	else{
		postcode = Postcode::convert(loc);
		return (postcode?atoi(postcode):1);
	}
}

static double di_real_post_fee_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
	char* postcodes[MULTIVAL_CNT_MAX];
	int postcode_cnt = 0;
	int userloc = parse_userloc(arg);
	int postcode = 0;
	
	char* substnend = 0;
	char* substncur = 0;
	char* semicolon = 0;
	char* realcolon = 0;
	char* underline = 0;
	char* nationwide = 0;
	int nationwidelen = 0;
	char* postfee = 0;
	int postfeelen = 0;
	
	int i = 0;
	
	if(userloc==-1){
		return 0.00;
	}
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
		return 0.00;
	}
	else{
		substnend[0] = 0;
	}
	
	do{
		semicolon = strchr(substncur, ';');
		if(!semicolon){
			semicolon = substnend;
		}
		else{
			semicolon[0] = 0;
		}
		
		realcolon = strchr(substncur, ':');
		if(!realcolon){
			return 0.00;
		}
		else{
			realcolon[0] = 0;
		}
		
		postfee = realcolon+1;
		postfee = str_trim(postfee);
		postfeelen = strlen(postfee);
		
		substncur = str_trim(substncur);
		if(strcmp(substncur, "1")==0){
			nationwide = postfee;
			nationwidelen = postfeelen;
		}
		
		postcode_cnt = str_split_ex(substncur, '_', postcodes, MULTIVAL_CNT_MAX);
		if(postcode_cnt<=0){
			return 0.00;
		}
		for(i=0; i<postcode_cnt; i++){
			postcodes[i] = str_trim(postcodes[i]);
			if(postcodes[i][0]==0){
				continue;
			}
			postcode = atoi(postcodes[i]);
			if(postcode==userloc){
				break;
			}
		}
		if(i<postcode_cnt){
			return atof(postfee);
		}
		
		if(semicolon==substnend){
			break;
		}
		else{
			substncur = semicolon+1;
		}
	}while(1);
	
	if(nationwide){
		return atof(nationwide);
	}
	else{
		return 0.00;
	}
}

static int di_zk_time_postprocessor(char* input, int inlen, char* output, int outmax, unsigned long long* zk_time, char* arg)
{
	int idx = 0;
	
	char* times[MULTIVAL_CNT_MAX];
	int time_cnt = 0;
	
	char* substnend = 0;
	char* substncur = 0;
	
	time_t et = (arg)?(atoi(arg)):(time(0));
	time_t begin = 0;
	time_t end = 0;
	unsigned long long number = 0;
	
	int i = 0;
	char* endptr = 0;

    *zk_time = 0;
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
		return -1;
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);
	
	time_cnt = str_split_ex(substncur, ' ', times, MULTIVAL_CNT_MAX);
	if(time_cnt<=0){
		return -1;
	}
	
	for(i=0; i<time_cnt; i++){
		times[i] = str_trim(times[i]);
		if(times[i][0]==0){
			continue;
		}
		number = strtoull(times[i], &endptr, 0);
		begin = ((number>>32)&0xFFFFFFFF);
		end = (number&0xFFFFFFFF);
		if(et>=begin && et<end){
            *zk_time = number;
			break;
		}
        else{
            idx ++;
        }
	}
    if(idx==time_cnt){
        idx = -1;
    }

	return idx;
}

static int di_multi_value_postprocessor(char* input, int inlen, char* output, int outmax, int idx)
{
	char* values[MULTIVAL_CNT_MAX];
	int value_cnt = 0;
	
	char* substnend = 0;
	char* substncur = 0;

    int ret = 0;
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
        if((ret=snprintf(output, outmax, "\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);
	
    if(idx<0){
        if((ret=snprintf(output, outmax, "\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
    }
	
	value_cnt = str_split_ex(substncur, ' ', values, MULTIVAL_CNT_MAX);
	if(value_cnt<=idx){
        if((ret=snprintf(output, outmax, "\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
	}
	
	values[idx] = str_trim(values[idx]);
	if(values[idx][0]==0){
        if((ret=snprintf(output, outmax, "\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
	}
    if((ret=snprintf(output, outmax, "%s\001", values[idx]))>outmax){
        return -1;
	}
    else{
	    return ret;
    }
}

static int di_multi_rate_postprocessor(char* input, int inlen, char* output, int outmax, int idx)
{
	char* values[MULTIVAL_CNT_MAX];
	int value_cnt = 0;
	
	char* substnend = 0;
	char* substncur = 0;

    int ret = 0;
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
        if((ret=snprintf(output, outmax, "10000\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);

    if(idx<0){
        if((ret=snprintf(output, outmax, "10000\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
    }
	
	value_cnt = str_split_ex(substncur, ' ', values, MULTIVAL_CNT_MAX);
	if(value_cnt<=idx){
        if((ret=snprintf(output, outmax, "10000\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
	}
	
	values[idx] = str_trim(values[idx]);
	if(values[idx][0]==0){
        if((ret=snprintf(output, outmax, "10000\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
	}
    if((ret=snprintf(output, outmax, "%s\001", values[idx]))>outmax){
        return -1;
	}
    else{
	    return ret;
    }
}

/**
 * 根据目标经纬度信息计算距离(单位km)
 * @param   srcLat   源位置经度
 * @param   srcLng   源位置纬度
 * @param   dstLat   目标位置经度
 * @param   dstLng   目标位置纬度
 *
 * @return   经纬度距离(km)
 */
#define PI 3.1415926535898
#define rad(d) (d*PI/180.0)
#define EARTH_RADIUS  6378.137
static float getDistance(float srcLat, float srcLng, float dstLat, float dstLng)
{  
    double latDis = rad(srcLat) - rad(dstLat);
    double lonDis = rad(srcLng) - rad(dstLng);
    double s = 2 * asin(sqrt(pow(sin(latDis/2), 2) + cos(rad(srcLat)) * cos(rad(dstLat)) * pow(sin(lonDis/2), 2))) * EARTH_RADIUS;
    s = round(s * 10000) / 10000;
    return s;
} 

static int parselatlng(char* str, float* lat, float* lng)
{
    char* end = str+strlen(str);
    char* colon = NULL;
    *lat = *lng = 0.0;
    colon = strchr(str, ':');
    if(!colon || colon <= str || (colon >= end - 1)){
        return -1;
    }

    char * inValidEnd = NULL;
    float  val = 0.0f;
    val = strtof(str, &inValidEnd);
    if (unlikely(inValidEnd != colon || val > 90 || val < -90)) {
        return -1;
    }
    *lat = val;

    val = strtof(colon+1, &inValidEnd);
    if (unlikely(inValidEnd != end || val > 180 || val < -180)) {
        return -1;
    }
    *lng = val;
    return 0;
}

static int di_latlng_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
    char* substnend = 0;
    char* substncur = 0;

    double distance = 0.0;
    float srclat = 0.0;
    float srclng = 0.0;
    float dstlat = 0.0;
    float dstlng = 0.0;

    char* latlngs[MULTIVAL_CNT_MAX];
    int latlng_cnt = 0;
    float retval = 0.0; 
    int ret = 0;
    int i = 0;
    char* latlng_bak = 0;

	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
		if((ret=snprintf(output, outmax, "\001\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);

    if(!arg){
        if((ret=snprintf(output, outmax, "%s\001\001", substncur))>outmax){
            return -1;
        }
        else{
            return ret;
        }
    }

    if (unlikely(parselatlng(arg, &srclat, &srclng) < 0)) {
        if((ret=snprintf(output, outmax, "%s\001\001", substncur))>outmax){
            return -1;
        }
        else{
            return ret;
        }
    }

    latlng_bak = strdup(substncur);
    latlng_cnt = str_split_ex(substncur, ' ', latlngs, MULTIVAL_CNT_MAX);
	if(latlng_cnt<=0){
        if(latlng_bak){
            free(latlng_bak);
        }
		if((ret=snprintf(output, outmax, "\001\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	
	for(i=0; i<latlng_cnt; i++){
		latlngs[i] = str_trim(latlngs[i]);
		if(latlngs[i][0]==0){
			continue;
		}
        parselatlng(latlngs[i], &dstlat, &dstlng);
        retval = getDistance(srclat, srclng, dstlat, dstlng);
        if(i==0 || retval<distance){
            distance = retval;
        }
	}

    if((ret=snprintf(output, outmax, "%s\001%.2f\001", latlng_bak?latlng_bak:"", distance))>outmax){
        if(latlng_bak){
            free(latlng_bak);
        }
        return -1;
    }
    else{
        if(latlng_bak){
            free(latlng_bak);
        }
        return ret;
    }
}

static int di_price_postprocessor(char* input, int inlen, char* output, int outmax, int discount, double post_fee)
{
	char* substnend = 0;
	char* substncur = 0;
	
	double price = 0.0;
	long long exprice = 0;
	long long realprice = 0;
	
	int ret = 0;
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
		if((ret=snprintf(output, outmax, "0.00\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);
	
	price = atof(substncur);
	//exprice = (long long)(price*100);
	//realprice = exprice*discount;
	//price = realprice;
	//price /= 1000000.0;
	//price += post_fee;
	
	if((ret=snprintf(output, outmax, "%.2f\001", price))>outmax){
		return -1;
	}
	else{
		return ret;
	}
}

static int di_ends_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
	char* substnend = 0;
	char* substncur = 0;
	
	char* ends[MULTIVAL_CNT_MAX];
	int end_cnt = 0;
	
	time_t et = (arg)?(atoi(arg)):(time(0));
	time_t real_end = 0xFFFFFFFF;
	time_t max_end = 0;
	time_t end = 0;
	struct tm real_end_tm;
	
	char* endptr = 0;
	int ret = 0;
	int i = 0;
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
		if((ret=snprintf(output, outmax, "\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);
	
	end_cnt = str_split_ex(substncur, ' ', ends, MULTIVAL_CNT_MAX);
	if(end_cnt<=0){
		if((ret=snprintf(output, outmax, "\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	
	for(i=0; i<end_cnt; i++){
		ends[i] = str_trim(ends[i]);
		if(ends[i][0]==0){
			continue;
		}
		end = strtoul(ends[i], &endptr, 0);
		if(end>=et && end<real_end){
			real_end = end;
		}
		if(end>max_end){
			max_end = end;
		}
	}
	
	if(real_end==0xFFFFFFFF){
		real_end = max_end;
	}
	
	if(!localtime_r(&real_end, &real_end_tm)){
		if((ret=snprintf(output, outmax, "\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	
	if((ret=strftime(output, outmax, "%Y%m%d%H%M%S\001", &real_end_tm))>outmax){
		return -1;
	}
	else{
		return ret;
	}
}

static int di_start_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
	char* substnend = 0;
	char* substncur = 0;
	
	time_t real_start = 0;
	struct tm real_start_tm;
	
	char* endptr = 0;
	int ret = 0;
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
		if((ret=snprintf(output, outmax, "\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);
	
	real_start = strtoul(substncur, &endptr, 0);
	
	if(!localtime_r(&real_start, &real_start_tm)){
		if((ret=snprintf(output, outmax, "\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	
	if((ret=strftime(output, outmax, "%Y%m%d%H%M%S\001", &real_start_tm))>outmax){
		return -1;
	}
	else{
		return ret;
	}
}

static int di_oldstart_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
	char* substnend = 0;
	char* substncur = 0;
	
	time_t real_start = 0;
	struct tm real_start_tm;
	
	char* endptr = 0;
	int ret = 0;
	
	substncur = input;
	substnend = strchr(input, '\001');
	if(!substnend){
		substnend = input+inlen;
	}
	if(substnend==input){
		if((ret=snprintf(output, outmax, "\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	else{
		substnend[0] = 0;
	}
	substncur = str_trim(substncur);
	
	real_start = strtoul(substncur, &endptr, 0);
	
	if(!localtime_r(&real_start, &real_start_tm)){
		if((ret=snprintf(output, outmax, "\001"))>outmax){
			return -1;
		}
		else{
			return ret;
		}
	}
	
	if((ret=strftime(output, outmax, "%Y%m%d%H%M%S\001", &real_start_tm))>outmax){
		return -1;
	}
	else{
		return ret;
	}
}

static int term_compar(const void* v1, const void* v2)
{
	const char* str1 = *((const char**)v1);
	const char* str2 = *((const char**)v2);

	return ((int)strlen(str2)-(int)strlen(str1));  // 长度大的字符串优先
}

extern unsigned short  g_uni_normal_map[];

inline int unicode_char_to_utf8(unsigned short value, char* buf, int bufsize)
{
	char word[6];
	int tail = sizeof(word)-2;

	word[sizeof(word)-1] = '\0';

	if(bufsize<1){
		return -1;
	}

	// deal with one byte ascii 
	if(value < 0x80){
		buf[0] = value;
		return 1;
	}
	// deal with multi byte word
	int j = tail;
	int len = 1;
	while(j>0){
		unsigned char ch = value & 0x3f;
		word[j--] = ch | 0x80;
		value = value>>6;
		++len;
		if(value<0x20){  // less than 6 bytes
			break;
		}
	}

	word[j] = value|s_utf8_head_mask[len];

	if(len>bufsize){
		return -1;
	}
	memcpy(buf, word+j, len);
	return len;
}

static int light_normalize(const char* src, int srclen, char* dest, int* match_lens, int* word_lens, int dsize)
{
	int dcnt = 0;
	int pos  = 0;
	int len  = 0;
	int cnt  = 0;
	unsigned short word = 0;
	
	bzero(match_lens, sizeof(int)*dsize);

	for( int i = 0; i < srclen; ){
		word = get_utf8_word(src, srclen, &pos);
		if(word==0){
			break;
		}
		word = g_uni_normal_map[word];
		len = unicode_char_to_utf8(word, dest+dcnt, dsize-dcnt);
		if(len<0){
			return -1;
		}
		dcnt += len;
		match_lens[dcnt] = pos;
		word_lens[cnt++] = len;
	}

	dest[dcnt] = '\0';
	return dcnt;
}

static int di_highlight_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
	char hl_title[MAX_HL_TITLE_LEN];
	char* tlist[MAX_HL_TERMS];
	int nterms = 0;
	int ret = 0;
	const char* p = 0;
	char* pdst = hl_title;
	int i = 0;
	int term_len = 0;
	int marked = 0;
	char* title_start = 0;
	char* title_end = 0;
	char* arg_tmp = 0;
	int title_len = 0;
	int normal_len = 0;
	char* normal_buf = 0;
	int* match_lens = 0;
	int* word_lens = 0;
	char* title_cur = 0;
	int word_no = 0;
	int len = 0;
    char title_end_tmp = 0;
	
	if(!arg){
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
	
	arg_tmp = strdup(arg);
	if(!arg_tmp){
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
  
  // 分析飘红关键词  
	ret = split_with_blank(arg_tmp, tlist, MAX_HL_TERMS, ", ");
	if(ret<0){
		free(arg_tmp);
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
	nterms = ret;
	qsort(tlist, nterms, sizeof(char*), term_compar);

	// 找到title的头尾
	title_start = input;
	title_end = strchr(input, '\001');
	if(!title_end){
		title_end = input+inlen;
	}
	if(title_end==input){
		output[0] = '\001';
		free(arg_tmp);
		return 1;
	}
	else{
        title_end_tmp = title_end[0];
		title_end[0] = 0;
	}
	//title_start = str_trim(title_start);
	
	// 对title进行normalize，并获取对应关系：
	// 1. normal字符与原字符起始位置对应关系
	// 2. 单个normal字符的长度
	title_len = strlen(title_start);
	if(title_len==0){
		output[0] = '\001';
		free(arg_tmp);
		return 1;
	}
	normal_buf = (char*)malloc((sizeof(char)+sizeof(int)*2)*(title_len+1));
	if(!normal_buf){
		free(arg_tmp);
        title_end[0] = title_end_tmp;
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
	match_lens = (int*)(normal_buf+title_len+1);
	word_lens = (match_lens+title_len+1);
	normal_len = light_normalize(title_start, title_len, normal_buf, match_lens, word_lens, title_len+1);
	if(normal_len<0){
		free(arg_tmp);
		free(normal_buf);
        title_end[0] = title_end_tmp;
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}

	// 生成detail，并为title飘红
	//p = title_start;
	p = normal_buf;
	title_cur = title_start;
	while(*p!=0){
		marked = 0;
		for(i=0; i<nterms; i++){
			term_len = strlen(tlist[i]);
			if(strncasecmp(p, tlist[i], term_len)==0){ //matched
				int no = word_no;
				len = 0;
				if(match_lens[p+term_len-normal_buf]==0){
					continue;
				}
				while(len<term_len){
					len += word_lens[no++];
				}
				if(len>term_len){
					continue;
				}
				word_no = no;
				p += term_len;
				len = title_start+match_lens[p-normal_buf]-title_cur;
				if(pdst-hl_title+(sizeof(g_di_hl_start)-1)+len+(sizeof(g_di_hl_end)-1)>=MAX_HL_TITLE_LEN){
					free(arg_tmp);
					free(normal_buf);
                    title_end[0] = title_end_tmp;
					return di_default_postprocessor(input, inlen, output, outmax, arg);
				}
				memcpy(pdst, g_di_hl_start, sizeof(g_di_hl_start)-1);
				pdst += sizeof(g_di_hl_start)-1;
				memcpy(pdst, title_cur, len);
				pdst += len;
				title_cur += len;
				memcpy(pdst, g_di_hl_end, sizeof(g_di_hl_end)-1);
				pdst += sizeof(g_di_hl_end)-1;
				marked = 1;
				break;
			}
		}
		if(!marked){
			p += word_lens[word_no++];
			len = title_start+match_lens[p-normal_buf]-title_cur;
            if(pdst-hl_title+len>=MAX_HL_TITLE_LEN){
                free(arg_tmp);
                free(normal_buf);
                title_end[0] = title_end_tmp;
                return di_default_postprocessor(input, inlen, output, outmax, arg);
            }
			memcpy(pdst, title_cur, len);
			pdst += len;
			title_cur += len;
		}
	}
	
	if(pdst-hl_title+1>=MAX_HL_TITLE_LEN){
		free(arg_tmp);
		free(normal_buf);
        title_end[0] = title_end_tmp;
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
	*pdst = 0;
	
	if(outmax<pdst-hl_title+1){
		free(arg_tmp);
		free(normal_buf);
        title_end[0] = title_end_tmp;
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
	memcpy(output, hl_title, pdst-hl_title);
	output[pdst-hl_title] = '\001';
	free(arg_tmp);
	free(normal_buf);
	return (pdst-hl_title+1);
}

static int whole_sub_compare(char* str, int len, char* terms[], int term_lens[], int term_cnt, int base)
{
    int i = base;
    int j = 0;
    for(; j<term_cnt; i=(i+1)%term_cnt, j++){
        if(term_lens[i]==0){
            continue;
        }
        if(term_lens[i]<=len && strncmp(str, terms[i], term_lens[i])==0){
            if(term_lens[i]==len){
                return 1;
            }
            else{
                if(whole_sub_compare(str+term_lens[i], len-term_lens[i], terms, term_lens, term_cnt, (i+1)%term_cnt)){
                    return 1;
                }
            }
        }
    }
    return 0;
}

#define whole_compare(str, len, terms, term_lens, term_cnt) whole_sub_compare(str, len, terms, term_lens, term_cnt, 0)

static int di_vidname_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
	char* arg_tmp = 0;
	char* tlist[MAX_HL_TERMS];
    int tlens[MAX_HL_TERMS];
	int nterms = 0;
	int ret = 0;
	char* vidname_start = 0;
	char* vidname_end = 0;
	char* vidname_str = 0;
	char* vidname_tmp = 0;
	int vidname_len = 0;
    char vidname_end_tmp = 0;
    char* vidname_normal = 0;
	int vidname_normal_len = 0;
    char** vidname_term = 0;
    char** vidname_normal_term = 0;
    int vidname_term_cnt = 0;
    int vidname_normal_term_cnt = 0;
    int vidname_term_lighted_cnt = 0;
    int lighted = 0;
    char* outptr = output;
    int outlen = 0;
    int len = 0;
    int i = 0;
	
	if(!arg){
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
	
	arg_tmp = strdup(arg);
	if(!arg_tmp){
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
  
    // 分析飘红关键词  
	ret = split_with_blank(arg_tmp, tlist, MAX_HL_TERMS, ", ");
	if(ret<0){
		free(arg_tmp);
		return di_default_postprocessor(input, inlen, output, outmax, arg);
	}
	nterms = ret;
    for(i=0; i<nterms; i++){
        tlens[i] = strlen(tlist[i]);
    }

	// 找到vidname的头尾
	vidname_start = input;
	vidname_end = strchr(input, '\001');
	if(!vidname_end){
		vidname_end = input+inlen;
	}
	if(vidname_end==input){
		output[0] = '\001';
		free(arg_tmp);
		return 1;
	}
	else{
        vidname_end_tmp = vidname_end[0];
		vidname_end[0] = 0;
        vidname_tmp = strdup(vidname_start);
        vidname_end[0] = vidname_end_tmp;
        if(!vidname_tmp){
            free(arg_tmp);
            return di_default_postprocessor(input, inlen, output, outmax, arg);
        }
	}
	vidname_str = str_trim(vidname_tmp);
    vidname_len = strlen(vidname_str);

    vidname_normal = strdup(vidname_str);
    if(!vidname_normal){
        free(arg_tmp);
        free(vidname_tmp);
        return di_default_postprocessor(input, inlen, output, outmax, arg);
    }
    ret = py_utf8_normalize(vidname_str, vidname_len, vidname_normal, vidname_len+1);
    if(ret<0){
        free(arg_tmp);
        free(vidname_tmp);
        free(vidname_normal);
        return di_default_postprocessor(input, inlen, output, outmax, arg);
    }
    vidname_normal_len = ret;
    
    vidname_term = (char**)malloc(sizeof(char*)*vidname_len*2);
    if(!vidname_term){
        free(arg_tmp);
        free(vidname_tmp);
        free(vidname_normal);
        return di_default_postprocessor(input, inlen, output, outmax, arg);
    }
    vidname_normal_term = vidname_term+vidname_len;
    
    ret = split_with_blank(vidname_str, vidname_term, vidname_len, " ");
    if(ret<0){
        free(arg_tmp);
        free(vidname_tmp);
        free(vidname_normal);
        free(vidname_term);
        return di_default_postprocessor(input, inlen, output, outmax, arg);
    }
    vidname_term_cnt = ret;
    ret = split_with_blank(vidname_normal, vidname_normal_term, vidname_len, " ");
    if(ret<0){
        free(arg_tmp);
        free(vidname_tmp);
        free(vidname_normal);
        free(vidname_term);
        return di_default_postprocessor(input, inlen, output, outmax, arg);
    }
    vidname_normal_term_cnt = ret;
    if(vidname_term_cnt!=vidname_normal_term_cnt){
        free(arg_tmp);
        free(vidname_tmp);
        free(vidname_normal);
        free(vidname_term);
        return di_default_postprocessor(input, inlen, output, outmax, arg);
    }

    for(i=0; i<vidname_term_cnt; i++){
        len = strlen(vidname_normal_term[i]);
        if(len==0){
            continue;
        }
        if(whole_compare(vidname_normal_term[i], len, tlist, tlens, nterms)){
            lighted = 1;
            len = strlen(vidname_term[i]);
            if((sizeof(g_di_hl_start)-1)+len+(sizeof(g_di_hl_end)-1)+1+outlen>=outmax){
                free(arg_tmp);
                free(vidname_tmp);
                free(vidname_normal);
                free(vidname_term);
                return di_default_postprocessor(input, inlen, output, outmax, arg);
            }
            else{
                memcpy(outptr, g_di_hl_start, sizeof(g_di_hl_start)-1);
                outptr += sizeof(g_di_hl_start)-1;
                memcpy(outptr, vidname_term[i], len);
                outptr += len;
                memcpy(outptr, g_di_hl_end, sizeof(g_di_hl_end)-1);
                outptr += sizeof(g_di_hl_end)-1;
                *outptr = ' ';
                outptr ++;
                outlen += (sizeof(g_di_hl_start)-1)+len+(sizeof(g_di_hl_end)-1)+1;
            }
        }
        else{
            len = strlen(vidname_term[i]);
            if(len+1+outlen>=outmax){
                free(arg_tmp);
                free(vidname_tmp);
                free(vidname_normal);
                free(vidname_term);
                return di_default_postprocessor(input, inlen, output, outmax, arg);
            }
            else{
                memcpy(outptr, vidname_term[i], len);
                outptr += len;
                *outptr = ' ';
                outptr ++;
                outlen += len+1;
            }
        }
    }

    free(arg_tmp);
    free(vidname_tmp);
    free(vidname_normal);
    free(vidname_term);
    if(lighted){
        if(outlen>=outmax){
            return di_default_postprocessor(input, inlen, output, outmax, arg);
        }
        output[outlen-1] = '\001';
        vidname_end[0] = 0;
        return outlen;
    }
    else{
        return di_highlight_postprocessor(input, inlen, output, outmax, arg);
    }
}

#ifdef __cplusplus
}
#endif
