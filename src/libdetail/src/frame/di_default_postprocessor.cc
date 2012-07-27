#include "di_default_postprocessor.h"
#include "util/Log.h"
#include "detail.h"
#include "util/Postcode.h"
#include "util/strfunc.h"

/* some define */
#define POSTCODE_CNT_MAX 64                      // 单文档中邮政编码最大数量
#define PROMOTION_CNT_MAX 64                     // 单文档中折扣信息最大数量
#define ENDTIME_CNT_MAX 64                       // 单文档中下架时间最大数量

static const char* name_string = "other";
static const int name_length = 5;

static char g_di_model_string[MAX_MODEL_LEN] = {0};     // detail数据样式
static int g_di_model_length = 0;                       // detail数据样式长度
static const char** g_di_field_array = 0;               // detail字段名数组
static const int* g_di_flen_array = 0;                  // detail字段名长度数组
static int g_di_field_count = 0;                        // detail中存储的字段数量

static int g_di_post_idx = -1;                        // real_post_fee在detail中的field序号
static int g_di_promotions_idx = -1;                  // promotions在detail中的field序号
static int g_di_price_idx = -1;                       // price在detail中的field序号
static int g_di_ends_idx = -1;                        // ends在detail中的field序号
static int g_di_start_idx = -1;                       // start在detail中的field序号
static int g_di_oldstart_idx = -1;                    // old_start在detail中的field序号


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
	
	if(field_count<=0 || !field_array || !flen_array){
		TERR("di_postprocessor_init argument error, return!");
		return -1;
	}
	
	for(i=0; i<field_count; i++){
		if(left<=flen_array[i]+1){
			TERR("g_di_model_string no enough space, return!");
			return -1;
		}
		memcpy(ptr, field_array[i], flen_array[i]);
		ptr[flen_array[i]] = '\001';
		ptr += flen_array[i]+1;
		left -= flen_array[i]+1;
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

    for(i=0; i<field_count; i++){
        // 查找特殊处理字段
        if(flen_array[i]==10 && strncmp(field_array[i], "promotions", 10)==0){
            g_di_promotions_idx = i;
        }   
        else if(flen_array[i]==13 && strncasecmp(field_array[i], "real_post_fee", 13)==0){
            g_di_post_idx = i;
        }   
        else if(flen_array[i]==13 && strncasecmp(field_array[i], "reserve_price", 13)==0){
            g_di_price_idx = i;
        }   
        else if(flen_array[i]==4 && strncasecmp(field_array[i], "ends", 4)==0){
            g_di_ends_idx = i;
        }   
        else if(flen_array[i]==6 && strncasecmp(field_array[i], "starts", 6)==0){
            g_di_start_idx = i;
        }   
        else if(flen_array[i]==10 && strncasecmp(field_array[i], "old_starts", 10)==0){
            g_di_oldstart_idx = i;
        }   
    }

	return 0;
}

/*
 * 清理detail后处理器
 */
extern void di_postprocessor_dest()
{
}

static int di_default_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_real_post_fee_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_promotions_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_price_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_ends_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_start_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);
static int di_oldstart_postprocessor(char* input, int inlen, char* output, int outmax, char* arg);

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
    char* field_idx = 0;
    int i = 0;
    int dllen = 0;
    int len = 0;
    char* ptr = 0;
    char* start = 0;
    char* end = 0;
    char* ret = 0;
    char* cur = 0;
    int left = MAX_DETAIL_LEN;
    int retval = 0;

    if(args[g_di_field_count]){
        dllen = strlen(args[g_di_field_count]);
    }
    else{
        return strdup(input);
    }

    ret = (char*)malloc(MAX_DETAIL_LEN+g_di_field_count);
    if(!ret){
        return strdup(input);
    }
    field_idx = ret+MAX_DETAIL_LEN;
    bzero(field_idx, g_di_field_count);

    start = args[g_di_field_count];
    end = args[g_di_field_count]+dllen;
    if(end[-1]==','){
        end --;
    }
    for(ptr=start; ptr<=end; ptr++){
         if(*ptr==',' || *ptr==0){
            if(start==ptr){
                free(ret);
                return strdup(input);
            }
            for(i=0; i<g_di_field_count; i++){
                if(g_di_flen_array[i]==ptr-start && strncmp(g_di_field_array[i], start, g_di_flen_array[i])==0){
                    break;
                }   
            }   
            if(i==g_di_field_count){
                free(ret);
                return strdup(input);
            }
            field_idx[i] = 1;
            start = ptr+1;
        }       
    }
    
    i = 0;
    cur = ret;
    start = input;
    end = input+inlen;
    if(end[-1]=='\001'){
        end --;
    }
    for(ptr=start; ptr<=end; ptr++){
        if(*ptr=='\001' || *ptr==0){
            if(field_idx[i]==0){
                start = ptr+1;
                i ++;
                continue;
            }
            if(i==g_di_post_idx){
                if((retval=di_real_post_fee_postprocessor(start, ptr-start+1, cur, left, args[i]))<0){
                    free(ret);
                    return strdup(input);
                }
                cur += retval;
                left -= retval;
            }
            else if(i==g_di_promotions_idx){
                if((retval=di_promotions_postprocessor(start, ptr-start+1, cur, left, args[i]))<0){
                    free(ret);
                    return strdup(input);
                }
                cur += retval;
                left -= retval;
            }
            else if(i==g_di_price_idx){
                if((retval=di_price_postprocessor(start, ptr-start+1, cur, left, args[i]))<0){
                    free(ret);
                    return strdup(input);
                }
                cur += retval;
                left -= retval;
            }
            else if(i==g_di_ends_idx){
                if((retval=di_ends_postprocessor(start, ptr-start+1, cur, left, args[i]))<0){
                    free(ret);
                    return strdup(input);
                }
                cur += retval;
                left -= retval;
            }
            else if(i==g_di_start_idx){
                if((retval=di_start_postprocessor(start, ptr-start+1, cur, left, args[i]))<0){
                    free(ret);
                    return strdup(input);
                }
                cur += retval;
                left -= retval;
            }
            else if(i==g_di_oldstart_idx){
                if((retval=di_oldstart_postprocessor(start, ptr-start+1, cur, left, args[i]))<0){
                    free(ret);
                    return strdup(input);
                }
                cur += retval;
                left -= retval;
            }
            else{
                if((retval=di_default_postprocessor(start, ptr-start+1, cur, left, args[i]))<0){
                    free(ret);
                    return strdup(input);
                }
                cur += retval;
                left -= retval;
            }           
            start = ptr+1;
            i ++;
        }
    }

    return ret;
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
    if(!args || argcnt!=(g_di_field_count+1) || !field || flen<=0){
        return;
    }
    if(flen==2 && strncmp(field, "dl", 2)==0){
        args[g_di_field_count] = arg;
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
    int i = 0;
    int dllen = 0;
    char* start = 0;
    char* end = 0;
    char* ptr = 0;
    int all_field = 1;
    int cnt_field = 0;

    if(dl){
        dllen = strlen(dl);
    }   
    else{
        return g_di_field_count;
    }   

    start = (char*)dl;
    end = (char*)dl+dllen;
    if(end[-1]=='\001'){
        end --;
    }
    for(ptr=start; ptr<=end; ptr++){
        if(*ptr=='\001' || *ptr==0){
            if(start==ptr){
                return g_di_field_count;
            }
            cnt_field ++;
            for(i=0; i<g_di_field_count; i++){
                if(g_di_flen_array[i]==ptr-start && strncmp(g_di_field_array[i], start, g_di_flen_array[i])==0){
                    break;
                }   
            }   
            if(i==g_di_field_count){
                all_field = 0;
                break;
            }
            start = ptr+1;
        }
    }

    if(all_field){
        return cnt_field;
    }   
    else{
        return g_di_field_count;
    }
}

/*
 * 获取处理后展示的字段样式
 *
 * @return 字段样式
 */
extern const char* di_get_model_string(const char* dl)
{
    int i = 0;
    int dllen = 0;
    char* start = 0;
    char* end = 0;
    char* ptr = 0;
    int all_field = 1;
    char* dl_ptr = (char*)dl;
    char* field_idx = 0;

    if(dl){
        dllen = strlen(dl);
    }   
    else{
        return g_di_model_string;
    }   

    field_idx = (char*)malloc(g_di_field_count);
    if(!field_idx){
        return g_di_model_string;
    }
    bzero(field_idx, g_di_field_count);

    start = (char*)dl;
    end = (char*)dl+dllen;
    if(end[-1]=='\001'){
        end --;
    }
    for(ptr=start; ptr<=end; ptr++){
        if(*ptr=='\001' || *ptr==0){
            if(start==ptr){
                return g_di_model_string;
            }
            for(i=0; i<g_di_field_count; i++){
                if(g_di_flen_array[i]==ptr-start && strncmp(g_di_field_array[i], start, g_di_flen_array[i])==0){
                    break;
                }   
            }   
            if(i==g_di_field_count){
                all_field = 0;
                break;
            }
            field_idx[i] = 1;
            start = ptr+1;
        }
    }

    if(all_field){
        for(i=0; i<g_di_field_count; i++){
            if(field_idx[i]){
                memcpy(dl_ptr, g_di_field_array[i], g_di_flen_array[i]);
                dl_ptr += g_di_flen_array[i]+1;
                dl_ptr[-1] = '\001';
            }
        }
        dl_ptr[-1] = 0;
        free(field_idx);
        return dl;
    }   
    else{
        free(field_idx);
        return g_di_model_string;
    }
}

/*
 * 获取处理后展示的字段样式长度
 *
 * @return 字段样式长度
 */
extern const int di_get_model_length(const char* dl)
{
    int i = 0;
    int dllen = 0;
    char* start = 0;
    char* end = 0;
    char* ptr = 0;
    int all_field = 1;

    if(dl){
        dllen = strlen(dl);
    }   
    else{
        return g_di_model_length;
    }   

    start = (char*)dl;
    end = (char*)dl+dllen;
    if(end[-1]=='\001'){
        end --;
    }
    for(ptr=start; ptr<=end; ptr++){
        if(*ptr=='\001' || *ptr==0){
            if(start==ptr){
                return g_di_model_length;
            }
            for(i=0; i<g_di_field_count; i++){
                if(g_di_flen_array[i]==ptr-start && strncmp(g_di_field_array[i], start, g_di_flen_array[i])==0){
                    break;
                }   
            }   
            if(i==g_di_field_count){
                all_field = 0;
                break;
            }
            start = ptr+1;
        }
    }

    if(all_field){
        return (dllen+1);
    }   
    else{
        return g_di_model_length;
    }
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
    output[len] = 0;

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

static int di_real_post_fee_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
    char* postcodes[POSTCODE_CNT_MAX];
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
    int ret = 0;

    if(userloc==-1){
        if((ret=snprintf(output, outmax, "0.00\001"))>outmax){
            return -1;
        }
        else{
            return 5;
        }
    }

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
            return 5;
        }
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
            if((ret=snprintf(output, outmax, "0.00\001"))>outmax){
                return -1;
            }
            else{
                return 5;
            }
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

        postcode_cnt = str_split_ex(substncur, '_', postcodes, POSTCODE_CNT_MAX);
        if(postcode_cnt<=0){
            if((ret=snprintf(output, outmax, "0.00\001"))>outmax){
                return -1;
            }
            else{
                return 5;
            }
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
            if((ret=snprintf(output, outmax, "%s\001", postfee))>outmax){
                return -1;
            }
            else{
                return ret;
            }
        }

        if(semicolon==substnend){
            break;
        }
        else{
            substncur = semicolon+1;
        }
    }while(1);

    if(nationwide){
        if((ret=snprintf(output, outmax, "%s\001", nationwide))>outmax){
            return -1;
        }   
        else{
            return ret;
        }
	}
    else{
        if((ret=snprintf(output, outmax, "0.00\001"))>outmax){
            return -1;
        }
        else{
            return 5;
        }
    }
}

static int di_promotions_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
{
    int discount = 10000;

    char* promotions[PROMOTION_CNT_MAX];
    int promotion_cnt = 0;

    char* substnend = 0;
    char* substncur = 0;

    time_t et = (arg)?(atoi(arg)):(time(0));
    time_t promot_begin = 0;
    time_t promot_end = 0;
    int promot_discont = 0;
    unsigned long long number = 0;

    int i = 0;
    int ret = 0;
    char* endptr = 0;

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

    promotion_cnt = str_split_ex(substncur, ' ', promotions, PROMOTION_CNT_MAX);
    if(promotion_cnt<=0){
        if((ret=snprintf(output, outmax, "10000\001"))>outmax){
            return -1;
        }
        else{
            return ret;
        }
    }

    for(i=0; i<promotion_cnt; i++){
        promotions[i] = str_trim(promotions[i]);
        if(promotions[i][0]==0){
            continue;
        }
        number = strtoull(promotions[i], &endptr, 0);
        promot_begin = ((number>>32)&0xFFFFFFFF);
        promot_end = ((number>>16)&0xFFFF)*60+promot_begin;
        promot_discont = (number&0xFFFF);
        if(et>=promot_begin && et<promot_end){
            discount = promot_discont;
            break;
        }
    }

    if((ret=snprintf(output, outmax, "%d\001", discount))>outmax){
        return -1; 
    }   
    else{
        return ret;
    }
}

static int di_price_postprocessor(char* input, int inlen, char* output, int outmax, char* arg)
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
	
	char* ends[ENDTIME_CNT_MAX];
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
	
	end_cnt = str_split_ex(substncur, ' ', ends, ENDTIME_CNT_MAX);
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
