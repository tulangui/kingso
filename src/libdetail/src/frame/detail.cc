/***********************************************************************************
 * Describe : detail核心代码
 *
 * Author   : gongyi, gongyi.cl@taobao.com
 *
 * Modify   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-05-05
 **********************************************************************************/


#include "detail.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <dirent.h>
#include <iconv.h>
#include "util/Log.h"
#include <ctype.h>
#include "di_postprocessor_manager.h"
#include "di_default_postprocessor.h"
#include "util/kspack.h"


/* some define */
#define MAX_DI_DOC 256                           // 一次获取detail的最大文档数
#define UMAX_DI_DOC 10240                        // 一次获取detail的终极文档数

#define DI_DATA_FILE "detail.dat"                // detail数据文件名 
#define DI_INDEX_FILE "detail.idx"               // detail索引文件名
#define DI_TITLE_FILE "detail.mdl"               // detail样式文件名


/* detail内存索引节点 */
typedef struct di_node_s
{
	int64_t nid;               // 文档nid
	int64_t off;               // 文档信息在数据文件中的偏移
	int32_t len;               // 文档信息长度
	struct di_node_s *next;    // 下一个节点指针
}
di_node_t;


/* detail存储相关信息 */
static char g_di_index_file[PATH_MAX] = {0};   // detail索引文件路径
static char g_di_data_file[PATH_MAX] = {0};    // detail数据文件路径
static char g_di_msgcnt_file[PATH_MAX] = {0};  // detail消息持久化数量文件路径
static char g_di_plugin_file[PATH_MAX] = {0};  // detail后处理器插件路径

/* detail更新相关信息 */
static FILE* g_di_index_update = 0;            // detail索引文件更新句柄
static FILE* g_di_data_update = 0;             // detail数据文件更新句柄
static uint64_t g_di_data_length = 0;          // detail数据文件长度

/* detail查询相关信息 */
static const int g_di_buckets = 33554432;      // detail内存索引hash表桶大小
static di_node_t** g_di_table = 0;             // detail内存索引hash表
static int g_fd_data = -1;                     // detail数据文件句柄

/* detail样式相关信息 */
static char g_di_model[MAX_MODEL_LEN] = {0};   // detail数据样式
static int g_di_model_len = 0;                 // detail数据样式长度
static char g_di_field[MAX_MODEL_LEN] = {0};   // 用于存储detail字段名的内存
static int g_di_field_length = 0;              // 用于存储detail字段名的内存长度
static char* g_di_field_array[MAX_MODEL_LEN];  // detail字段名数组
static int g_di_flen_array[MAX_MODEL_LEN];     // detail字段名长度数组
static int g_di_field_count = 0;               // detail中存储的字段数量

/* detail处理相关信息 */
static di_postprocessor_manager_t manager;


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

		//if(end>begin){
		if(cnt==item_size){
		    return -1;
		}
		items[cnt] = begin;
		//}
		
		if(*end){
			*end = 0;
			begin = end+1;
            while(items[cnt]<end && (*(items[cnt])==' ' || *(items[cnt])=='\t')){
                items[cnt] ++;
            }
            end --;
            while(end>items[cnt] && (*end==' ' || *end=='\t')){
                *(end--) = 0;
            }
            if(*(items[cnt])){
                cnt ++;
            }
		}
		else{
            while(items[cnt]<end && (*(items[cnt])==' ' || *(items[cnt])=='\t')){
                items[cnt] ++;
            }
            end --;
            while(end>items[cnt] && (*end==' ' || *end=='\t')){
                *(end--) = 0;
            }
            if(*(items[cnt])){
                cnt ++;
            }
			break;
		}
	}
	
	return cnt;
}

/*
 * 根据detail样式信息，分解detail中存储的字段名
 */
static void split_di_field()
{
	char* begin = g_di_field;
	char* end = g_di_field+g_di_field_length;
	char* tag = 0;
	int i = 0;
	
	// 按'\001'分解样式表，形成字段名数组
	while(tag=strchr(begin, '\001')){
		*tag = 0;
		g_di_field_array[g_di_field_count] = begin;
		g_di_flen_array[g_di_field_count] = strlen(begin);
		if(g_di_flen_array[g_di_field_count]>0){
			g_di_field_count ++;
		}
		begin = tag+1;
	}
	if(begin<end){
		g_di_field_array[g_di_field_count] = begin;
		g_di_flen_array[g_di_field_count] = strlen(begin);
		if(g_di_flen_array[g_di_field_count]>0){
			g_di_field_count ++;
		}
	}
}

/*
 * 初始化detail内存索引
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
static int init_di_table()
{
	g_di_table = (di_node_t**)malloc(sizeof(di_node_t*)*g_di_buckets);
	if(!g_di_table){
		TERR("Out of memory!");
		return -1;
	}
	
	memset(g_di_table, 0, sizeof(di_node_t*)*g_di_buckets);
	
	return 0;
}

/*
 * 向hash表中添加新索引项
 *
 * @param nid 文档nid
 * @param off detail信息在数据文件中的偏移
 * @param len detail信息长度
 *
 * @return < 0 添加出错
 *         ==0 添加成功
 */
static int di_table_insert(int64_t nid, int64_t off, int32_t len)
{
	int index = 0;
	di_node_t* pnew = 0;
	
	// 计算映射到的hash桶编号
	index = nid%g_di_buckets;
	
	pnew = (di_node_t*)malloc(sizeof(di_node_t));
	if(!pnew){
		TERR("Out of memory!");
		return -1;
	}
	pnew->nid = nid;
	pnew->off = off;
	pnew->len = len;
	
	// 将新的索引节点加入hash表中
	pnew->next = g_di_table[index];
	g_di_table[index] = pnew;
	
	return 0;
}

/*
 * 加载detail内存索引
 *
 * @param fn_index 索引文件路径
 * @param fn_data  数据文件路径
 * @param fn_model 样式文件路径
 * @param preload  是否预读data数据
 *
 * @return < 0 加载出错
 *         ==0 加载成功
 */
static int load_di_index(const char* fn_index, const char* fn_data, const char* fn_model, bool preload)
{
	FILE* fp_model = 0;
	FILE* fp_index = 0;
	char line[4096];
	int64_t nid = 0;
	int64_t off = 0;
	int32_t len = 0;
	int ret = 0;
	int count = 0;
	
	if(!fn_index || !fn_data || !fn_model){
		return -1;
	}
		
	// 读取detail数据样式信息
	fp_model = fopen(fn_model, "rb");
	if(!fp_model){
		TERR("fopen model error, return![path:%s]", fn_model);
		return -1;
	}
	g_di_model_len = fread(g_di_model, 1, MAX_MODEL_LEN, fp_model);
	fclose(fp_model);
	if(g_di_model_len<=0){
		TERR("fread model error, return!");
		return -1;
	}
	if(g_di_model_len>=MAX_MODEL_LEN){
		TERR("model len large than 4096, return!");
		return -1;
	}
	g_di_model[g_di_model_len] = 0;
	while(g_di_model_len>0 && (g_di_model[g_di_model_len]=='\n' || g_di_model[g_di_model_len]==0)){
		g_di_model[g_di_model_len] = 0;
		g_di_model_len --;
	}
	++ g_di_model_len; //之前g_di_model_len指向最后一个字节,向后移使其指向0,即为长度
	memcpy(g_di_field, g_di_model, g_di_model_len+1);
	g_di_field_length = g_di_model_len;
	split_di_field();
	g_di_model[g_di_model_len] = '\001';
	g_di_model_len ++;
	g_di_model[g_di_model_len] = 0;
	
	// 加载detail索引到内存
	fp_index = fopen(fn_index, "r+b");
	if(!fp_index){
		TERR("fopen index error, return![path:%s]", fn_index);
		return -1;
	}
	if(init_di_table()){
		TERR("init_di_table error, return!");
		fclose(fp_index);
		return -1;
	}
	while(fgets(line, 1024, fp_index)){
		ret = sscanf(line, "%lld %lld %d", (long long*)&nid, (long long*)&off, (int*)&len);
		if(ret!=3){
			TERR("Parse line error. LINE:%s\n", line);
			continue;
		}
		ret = di_table_insert(nid, off, len);
		if(ret!=0){
			TERR("Insert error.\n");
			continue;
		}
		count++;
	}
	fclose(fp_index);
	
	// 打开detail数据文件句柄
	g_fd_data = open(fn_data, 0);
	if(g_fd_data<0){
		TERR("open data error, return![path:%s]", fn_data);
		return -1;
	}

    if (preload) {
        while(read(g_fd_data, line, 4096)==4096);
    }
	
	// 复制文件路径
	snprintf(g_di_index_file, PATH_MAX, "%s", fn_index);
	snprintf(g_di_data_file, PATH_MAX, "%s", fn_data);
	
	TNOTE("%d docs in table.\n", count);
	return 0;
}

/*
 * 在hash表中查找索引项
 *
 * @param nid 文档nid
 * @param off 保存detail信息在数据文件中的偏移
 * @param len 保存detail信息长度
 *
 * @return < 0 查找出错
 *         ==0 查找成功
 */
static int di_table_search(int64_t nid, int64_t* off, int32_t* len)
{
	int index = nid%g_di_buckets;
	di_node_t* pnode = 0;
	
	if(!off || !len){
		return -1;
	}
	
	for(pnode=g_di_table[index]; pnode; pnode=pnode->next){
		if(nid==pnode->nid){
			*off = pnode->off;
			*len = pnode->len;
			return 0;
		}
	}
	
	return -1;
}

/*
 * 初始化所有后处理器
 *
 * @param manager 后处理器管理器
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
static int di_postprocessors_init(di_postprocessor_manager_t* manager)
{
	int i = 0;
	
	for(i=0; i<manager->postprocessor_cnt; i++){
		if(manager->postprocessors[i].init_func(g_di_field_count, (const char**)g_di_field_array, g_di_flen_array)){
			TERR("di_postprocessor_init %s error, return!", manager->postprocessors[i].name_string_func());
			return -1;
		}
	}
	
	return 0;
}

/*
 * 清理所有后处理器
 *
 * @param manager 后处理器管理器
 *
 * @return < 0 清理出错
 *         ==0 清理成功
 */
static int di_postprocessors_dest(di_postprocessor_manager_t* manager)
{
	int i = 0;
	
	for(i=0; i<manager->postprocessor_cnt; i++){
		manager->postprocessors[i].dest_func();
	}
	
	return 0;
}

/*
 * 替换字符串中的字符为另一字符
 *
 * @param str  待处理字符串
 * @param from 待替换字符
 * @param to   替换后的字符
 *
 * @return str
 */
static char* str_replace(char* str, char from, char to)
{
    char* start = 0;
    char* end = 0;
    char* ptr = 0;
    if(!str){
        return 0;
    }
    start = str;
    end = str+strlen(str);
    for(ptr=start; ptr<end; ptr++){
        if(*ptr==from){
            *ptr = to;
        }
    }
    return str;
}

/*
 * 初始化detail
 *
 * @param config 配置
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
extern int di_detail_init(mxml_node_t* config)
{
	mxml_node_t* index = 0;
	mxml_node_t* data = 0;
	mxml_node_t* model = 0;
	mxml_node_t* msgcnt = 0;
	mxml_node_t* plugin = 0;
	const char* index_path = 0;
	const char* data_path = 0;
	const char* data_pre_str = 0;
	const char* model_path = 0;
	const char* msgcnt_path = 0;
	const char* plugin_path = 0;

	bool data_preload = true;
	
	if(!config){
		TERR("argument config is NULL, return!");
		return -1;
	}
	
	// 获取索引文件的路径配置
	index = mxmlFindElement(config, config, "index", 0, 0, MXML_DESCEND_FIRST);
	if(!index){
		TERR("mxmlFindElement index error, return!");
		return -1;
	}
	index_path = mxmlElementGetAttr(index, "path");
	if(!index_path){
		TERR("mxmlElementGetAttr index path error, return!");
		return -1;
	}
	
	// 获取数据文件的路径配置
	data = mxmlFindElement(config, config, "data", 0, 0, MXML_DESCEND_FIRST);
	if(!data){
		TERR("mxmlFindElement data error, return!");
		return -1;
	}
	data_path = mxmlElementGetAttr(data, "path");
	if(!data_path){
		TERR("mxmlElementGetAttr data path error, return!");
		return -1;
	}
	data_pre_str = mxmlElementGetAttr(data, "preload");
	if(data_pre_str && strcasecmp(data_pre_str, "false") == 0){
		TLOG("no need to pre-read %s!", data_path);
		data_preload = false;
	}
	
	// 获取样式文件的路径配置
	model = mxmlFindElement(config, config, "model", 0, 0, MXML_DESCEND_FIRST);
	if(!model){
		TERR("mxmlFindElement model error, return!");
		return -1;
	}
	model_path = mxmlElementGetAttr(model, "path");
	if(!model_path){
		TERR("mxmlElementGetAttr model path error, return!");
		return -1;
	}
	
	if(load_di_index(index_path, data_path, model_path, data_preload)){
		TERR("load_di_index error, return!");
		return -1;
	}
	
	// 获取消息持久化数量文件的路径配置
	msgcnt = mxmlFindElement(config, config, "msgcnt", 0, 0, MXML_DESCEND_FIRST);
	if(!msgcnt){
		TERR("mxmlFindElement msgcnt error, return!");
		return -1;
	}
	msgcnt_path = mxmlElementGetAttr(msgcnt, "path");
	if(!msgcnt_path){
		TERR("mxmlElementGetAttr msgcnt path error, return!");
		return -1;
	}
	snprintf(g_di_msgcnt_file, PATH_MAX, "%s", msgcnt_path);
	
	// 获取后处理器插件的路径配置
	plugin = mxmlFindElement(config, config, "plugin", 0, 0, MXML_DESCEND_FIRST);
	if(plugin){
		plugin_path = mxmlElementGetAttr(plugin, "path");
		if(plugin_path){
			snprintf(g_di_plugin_file, PATH_MAX, "%s", plugin_path);
		}
	}
	
	if(di_postprocessor_manager_init(&manager, plugin_path)){
		TERR("di_postprocessor_manager_init error, return!");
		di_detail_dest();
		return -1;
	}
	
	if(di_postprocessors_init(&manager)){
		TERR("di_postprocessors_init error, return!");
		di_detail_dest();
		return -1;
	}
	
	return 0;
}

/*
 * 销毁detail
 */
extern void di_detail_dest()
{
	// 清理内存索引
	if(g_di_table){
		di_node_t* pnode = 0;
		int i = 0;
		for(; i<g_di_buckets; i++){
			while(g_di_table[i]){
				pnode = g_di_table[i];
				g_di_table[i] = pnode->next;
				free(pnode);
				pnode = 0;
			}
		}
		free(g_di_table);
		g_di_table = 0;
	}
	
	// 关闭数据文件句柄
	if(g_fd_data>=0){
		close(g_fd_data);
		g_fd_data = -1;
	}
	
	// 关闭索引文件更新句柄
	if(g_di_index_update){
		fclose(g_di_index_update);
		g_di_index_update = 0;
	}
	
	// 关闭数据文件更新句柄
	if(g_di_data_update){
		fclose(g_di_data_update);
		g_di_data_update = 0;
	}
	
	di_postprocessor_manager_dest(&manager);
	di_postprocessors_dest(&manager);
}
/*
 * @brief 从detail中获取v3格式的展示信息
 *
 * @param id_list nid列表
 * @param args 各个字段的特殊处理参数
 * @param argcnt 参数个数
 * @param res 返回字符串数组
 * @param res_size 返回字符串数组长度
 *
 * @return < 0 获取出错
 *         ==0 获取成功
 */
extern int di_detail_get_v3(const char* id_list, const char* dl, char* args[], const int argcnt, char** res, int* res_size)
{
	char* ilist = 0;
	int ret = 0;
	int docs = 0;
	char* items_array[MAX_DI_DOC];
	char* hl_items_array[MAX_DI_DOC];
	int hl_lens_arry[MAX_DI_DOC];
	char** items = items_array;
	char** hl_items = hl_items_array;
	int* hl_lens = hl_lens_arry;
	int item_size = 0;
	int head_size = 0;
	int res_count = 0;
	int64_t docid = 0;
	int i = 0;
	int j = 0;
	char* pe = 0;
	int64_t off = 0;
	int32_t len = 0;
	char* tmp_item = 0;
	char* hl_item = 0;
	int total_size = 0;
	int total_field = 0;
	char header[32];
	int ptr = 0;
	char* result = 0;
	
	di_postprocessor_t* postprocessor = 0;
	const char* model_string = 0;
	int model_length = 0;
	int field_count = 0;
	di_postprocessor_work_t* work_func = 0;
	
	if(!id_list || !args || argcnt!=g_di_field_count || !res || !res_size){
		TWARN("argumet error, return!");
		return -1;
	}
	
	if(dl){
		postprocessor = di_postprocessor_manager_get(&manager, dl, strlen(dl));
	}
	else{
		postprocessor = di_postprocessor_manager_get(&manager, "default", 7);
	}
	if(postprocessor){
		model_string = postprocessor->model_string_func(dl);
		model_length = postprocessor->model_length_func(dl);
		field_count = postprocessor->field_count_func(dl);
		work_func = postprocessor->work_func;
	}
	else{
		model_string = di_get_model_string(dl);
		model_length = di_get_model_length(dl);
		field_count = di_get_field_count(dl);
		work_func = di_postprocessor_work;
	}
	
	*res = 0;
	*res_size = 0;
	
	// 分割nid
	ilist = strdup(id_list);
	if(!ilist){
		TWARN("strdup id_list error, return!");
		return -1;
	}
	ret = split_with_blank(ilist, items, MAX_DI_DOC, ",");
	if(ret<0){
		char* mem = (char*)malloc((sizeof(char*)*2+sizeof(int))*UMAX_DI_DOC);
		if(!mem){
			TERR("malloc UMAX_DI_DOC memory error, return!");
			free(ilist);
			return -1;
		}
		items = (char**)mem;
		hl_items = (char**)(mem+(sizeof(char*)*UMAX_DI_DOC));
		hl_lens = (int*)(mem+(sizeof(char*)*2*UMAX_DI_DOC));
		free(ilist);
		ilist = strdup(id_list);
		if(!ilist){
			TWARN("strdup id_list error, return!");
			if(items!=items_array){
				free(items);
			}
			return -1;
		}
		ret = split_with_blank(ilist, items, UMAX_DI_DOC, ",");
		if(ret<0){
			TWARN("Parse docid list failed. id_list:%s\n", id_list);
			free(ilist);
			if(items!=items_array){
				free(items);
			}
			return -1;
		}
	}
	docs = ret;

	// 循环处理每个文档
	for(i=0; i<docs; i++){
		hl_items[i] = 0;
		hl_lens[i] = 0;
		
		// 翻译文档nid
		pe = 0;
		docid = strtoll(items[i], &pe, 10);
		if(!pe || (*pe)!=0 || docid<0){
			TWARN("Parser docid:%s failed",items[i]);
			continue;
		}
		
		// 获取文档信息在数据文件中的偏移，及数据长度
		ret = di_table_search(docid, &off, &len);
		if(ret!=0){
			TWARN("di_table_search not found docid=%lld", (long long)docid);
			continue;
		}
		if(len==0){
			TWARN("detail lenght is 0 docid=%lld", (long long)docid);
			continue;
		}
		
		// 为文档信息分配内存
		tmp_item = (char*)malloc(len+2);
		if(!tmp_item){
			TERR("Out of memory!");
			free(ilist);
			if(items!=items_array){
				free(items);
			}
			for(j=0; j<i; j++){
				if(hl_items[j]){
					free(hl_items[j]);
				}
			}
			return -1;
		}
		
		// 读取文档信息
		ret = pread(g_fd_data, tmp_item, len, off);
		if(ret!=len){
			free(ilist);
			if(items!=items_array){
				free(items);
			}
			free(tmp_item);
			for(j=0; j<i; j++){
				if(hl_items[j]){
					free(hl_items[j]);
				}
			}
			if(ret<0 && errno==EINTR){
				TWARN("pread data interrupted by a signal, return!");
			}
			else{
				TERR("pread data error, return!");
				close(g_fd_data);
				g_fd_data = open(g_di_data_file, 0);
			}
			return -1;
		}
		tmp_item[len] = '\001';
		tmp_item[len+1] = 0;
		len ++;
		
		// 后处理文档
		hl_item = work_func(tmp_item, len, args);
		if(!hl_item){ //fault. Not hl.
			hl_item = tmp_item;
			tmp_item = 0;
		}
		else{ //correct
			free(tmp_item);
			tmp_item = 0;
		}
		
		hl_items[i] = hl_item;
		hl_lens[i] = strlen(hl_item);
		
		++ res_count;
		item_size += hl_lens[i];
	}
	
	// 为最终结果分配空间
	total_field = ((1+res_count)*field_count); // 字段数*(宝贝数+1) + 1
	snprintf(header, sizeof(header), "%d\001%d\001", total_field, field_count);
	head_size = strlen(header);
	total_size = head_size+model_length+item_size;
	result = (char*)malloc(total_size+1);
	if(!result){
		TERR("Out of memory.");
		free(ilist);
		ilist=0;
		if(items!=items_array){
			free(items);
		}
		for(i=0; i<docs; i++){
			if(hl_items[i]){
				free(hl_items[i]);
			}
		}
		return -1;
	}

	// 拷贝头到最终结果
	memcpy(result, header, head_size);
	// 拷贝样式到最终结果
	memcpy(result+head_size, model_string, model_length);
	
	// 循环添加每个nid对应的detail到最终结果
	ptr = head_size+model_length;
	for(i=0; i<docs; i++){
		if(hl_lens[i]==0){
			continue;
		}
		memcpy(result+ptr, hl_items[i], hl_lens[i]);
		ptr += hl_lens[i];
		free(hl_items[i]);
		hl_items[i] = 0;
		hl_lens[i] = 0;
	}

	*(result+ptr) = 0;
	++ ptr;

	if(total_size==ptr-1){ //correct
		*res = result;
		*res_size = ptr-1; // the last  \001
		free(ilist);
		ilist = 0;
		if(items!=items_array){
			free(items);
		}
		return 0;
	}
	
	free(result);
	result = 0;
	free(ilist);
	ilist = 0;
	if(items!=items_array){
		free(items);
	}

	return -1;
}

/*
 * 从detail中获取展示信息
 *
 * @param id_list nid列表
 * @param args 各个字段的特殊处理参数
 * @param argcnt 参数个数
 * @param res 返回字符串数组
 * @param res_size 返回字符串数组长度
 *
 * @return < 0 获取出错
 *         ==0 获取成功
 */
extern int di_detail_get(const char* id_list, const char* dl, char* args[], const int argcnt, char** res, int* res_size)
{
	char* ilist = 0;
	int ret = 0;
	int docs = 0;
	char* items_array[MAX_DI_DOC];
	char* hl_items_array[MAX_DI_DOC];
	int hl_lens_arry[MAX_DI_DOC];
	char** items = items_array;
	char** hl_items = hl_items_array;
	int* hl_lens = hl_lens_arry;
	int item_size = 0;
	int head_size = 0;
	int res_count = 0;
	int64_t docid = 0;
	int i = 0;
	int j = 0;
	char* pe = 0;
	int64_t off = 0;
	int32_t len = 0;
	char* tmp_item = 0;
	char* hl_item = 0;
	int total_size = 0;
	int total_field = 0;
	char header[32];
	int ptr = 0;
	char* result = 0;
	
	di_postprocessor_t* postprocessor = 0;
	const char* model_string = 0;
	int model_length = 0;
	int field_count = 0;
	di_postprocessor_work_t* work_func = 0;

    kspack_t* pack = 0;
    int dl_len = 0;
    char* dl_tmp = 0;
	
	if(!id_list || !args || argcnt!=g_di_field_count || !res || !res_size){
		TWARN("argumet error, return!");
		return -1;
	}

    if(dl){
        dl_len = strlen(dl);
        dl_tmp = strdup(dl);
        if(!dl_tmp){
            TWARN("strdup dl error, return!");
            return -1;
        }
        str_replace(dl_tmp, ',', '\001');
    }
	
	if(dl_tmp){
		postprocessor = di_postprocessor_manager_get(&manager, dl_tmp, dl_len);
	}
	else{
		postprocessor = di_postprocessor_manager_get(&manager, "default", 7);
	}
	if(postprocessor){
		model_string = postprocessor->model_string_func(dl_tmp);
		model_length = postprocessor->model_length_func(dl_tmp);
		field_count = postprocessor->field_count_func(dl_tmp);
		work_func = postprocessor->work_func;
	}
	else{
		model_string = di_get_model_string(dl_tmp);
		model_length = di_get_model_length(dl_tmp);
		field_count = di_get_field_count(dl_tmp);
		work_func = di_postprocessor_work;
	}
	
	*res = 0;
	*res_size = 0;
	
	// 分割nid
	ilist = strdup(id_list);
	if(!ilist){
		TWARN("strdup id_list error, return!");
        free(dl_tmp);
		return -1;
	}
	ret = split_with_blank(ilist, items, MAX_DI_DOC, ",");
	if(ret<0){
		char* mem = (char*)malloc((sizeof(char*)*2+sizeof(int))*UMAX_DI_DOC);
		if(!mem){
			TWARN("malloc UMAX_DI_DOC memory error, return!");
            free(dl_tmp);
			free(ilist);
			return -1;
		}
		items = (char**)mem;
		hl_items = (char**)(mem+(sizeof(char*)*UMAX_DI_DOC));
		hl_lens = (int*)(mem+(sizeof(char*)*2*UMAX_DI_DOC));
		free(ilist);
		ilist = strdup(id_list);
		if(!ilist){
			TWARN("strdup id_list error, return!");
            free(dl_tmp);
			if(items!=items_array){
				free(items);
			}
			return -1;
		}
		ret = split_with_blank(ilist, items, UMAX_DI_DOC, ",");
		if(ret<0){
			TWARN("Parse docid list failed. id_list:%s\n", id_list);
            free(dl_tmp);
			free(ilist);
			if(items!=items_array){
				free(items);
			}
			return -1;
		}
	}
	docs = ret;

	// 循环处理每个文档
	for(i=0; i<docs; i++){
		hl_items[i] = 0;
		hl_lens[i] = 0;
		
		// 翻译文档nid
		pe = 0;
		docid = strtoll(items[i], &pe, 10);
		if(!pe || (*pe)!=0 || docid<0){
			TWARN("Parser docid:%s failed",items[i]);
			continue;
		}
		
		// 获取文档信息在数据文件中的偏移，及数据长度
		ret = di_table_search(docid, &off, &len);
		if(ret!=0){
			//TNOTE("di_table_search not found docid=%lld", (long long)docid);
			continue;
		}
		if(len==0){
			TWARN("detail lenght is 0 docid=%lld", (long long)docid);
			continue;
		}
		
		// 为文档信息分配内存
		tmp_item = (char*)malloc(len+2);
		if(!tmp_item){
			TWARN("Out of memory!");
			free(ilist);
			if(items!=items_array){
				free(items);
			}
			for(j=0; j<i; j++){
				if(hl_items[j]){
					free(hl_items[j]);
				}
			}
			return -1;
		}
		
		// 读取文档信息
		ret = pread(g_fd_data, tmp_item, len, off);
		if(ret!=len){
            free(dl_tmp);
			free(ilist);
			if(items!=items_array){
				free(items);
			}
			free(tmp_item);
			for(j=0; j<i; j++){
				if(hl_items[j]){
					free(hl_items[j]);
				}
			}
			if(ret<0 && errno==EINTR){
				TWARN("pread data interrupted by a signal, return!");
			}
			else{
				TERR("pread data error, return!");
				close(g_fd_data);
				g_fd_data = open(g_di_data_file, 0);
			}
			return -1;
		}
		tmp_item[len] = '\001';
		tmp_item[len+1] = 0;
		len ++;
		
		// 后处理文档
		hl_item = work_func(tmp_item, len, args);
		if(!hl_item){ //fault. Not hl.
			hl_item = tmp_item;
			tmp_item = 0;
		}
		else{ //correct
			free(tmp_item);
			tmp_item = 0;
		}
		
		hl_items[i] = hl_item;
		hl_lens[i] = strlen(hl_item);
		
		++ res_count;
		item_size += hl_lens[i];
	}

    pack = kspack_open('w', 0, 0);
    if(!pack){
        TWARN("kspack_open error, return!");
        free(dl_tmp);
        free(ilist);
		ilist=0;
		if(items!=items_array){
			free(items);
		}
		for(i=0; i<docs; i++){
			if(hl_items[i]){
				free(hl_items[i]);
			}
		}
		return -1;
    }

    if(kspack_put(pack, "field_count", 11, &field_count, sizeof(int), KS_BYTE_VALUE)){
        TWARN("kspack_put field_count error, return!");
        free(dl_tmp);
        free(ilist);
		ilist=0;
		if(items!=items_array){
			free(items);
		}
		for(i=0; i<docs; i++){
			if(hl_items[i]){
				free(hl_items[i]);
			}
		}
        kspack_close(pack, 1);
		return -1;
    }

    if(kspack_put(pack, "field_name", 10, model_string, model_length+1, KS_BYTE_VALUE)){
        TWARN("kspack_put field_name error, return!");
        free(dl_tmp);
        free(ilist);
		ilist=0;
		if(items!=items_array){
			free(items);
		}
		for(i=0; i<docs; i++){
			if(hl_items[i]){
				free(hl_items[i]);
			}
		}
        kspack_close(pack, 1);
		return -1;
    }

    if(kspack_put(pack, "data_count", 10, &docs, sizeof(int), KS_BYTE_VALUE)){
        TWARN("kspack_put data_count error, return!");
        free(dl_tmp);
        free(ilist);
		ilist=0;
		if(items!=items_array){
			free(items);
		}
		for(i=0; i<docs; i++){
			if(hl_items[i]){
				free(hl_items[i]);
			}
		}
        kspack_close(pack, 1);
		return -1;
    }

	for(i=0; i<docs; i++){
        if(hl_lens[i]==0){
            TDEBUG("nid=%s, data=NULL", items[i]);
            ret = kspack_put(pack, items[i], strlen(items[i]), 0, 0, KS_NONE_VALUE);
        }
        else{
            TDEBUG("nid=%s, data=%s", items[i], hl_items[i]);
            ret = kspack_put(pack, items[i], strlen(items[i]), hl_items[i], hl_lens[i]+1, KS_BYTE_VALUE);
        }
        if(ret!=0){
            TWARN("kspack_put doc[%d] error, return!", i);
            free(dl_tmp);
            free(ilist);
            ilist=0;
            if(items!=items_array){
                free(items);
            }
            for(; i<docs; i++){
                if(hl_items[i]){
                    free(hl_items[i]);
                }
            }
            kspack_close(pack, 1);
            return -1;
        }
		free(hl_items[i]);
		hl_items[i] = 0;
		hl_lens[i] = 0;
	}

    free(ilist);
    ilist=0;
    if(items!=items_array){
        free(items);
    }

    if(kspack_done(pack)){
        TWARN("kspack_done error, return!");
        free(dl_tmp);
        kspack_close(pack, 1);
        return -1;
    }
    *res = (char*)kspack_pack(pack);
    *res_size = kspack_size(pack);
    kspack_close(pack, 0);
    free(dl_tmp);

	return 0;
}

/*
 * 向detail中添加展示信息
 *
 * @param nid 文档nid
 * @param content 详细内容
 * @param len 详细内容长度
 *
 * @return < 0 添加出错
 *         ==0 添加成功
 */
extern int di_detail_add(int64_t nid, const char* content, int32_t len)
{
	if(!content || len<=0){
		TWARN("argument error, return!");
		return -1;
	}
	
	// open data file
	if(!g_di_data_update){
		g_di_data_update = fopen(g_di_data_file, "a");
		if(!g_di_data_update){
			TWARN("fopen g_di_data_file error, return!");
			return -1;
		}
		g_di_data_length = ftell(g_di_data_update);
	}
	// open index file
	if(!g_di_index_update){
		g_di_index_update = fopen(g_di_index_file, "a");
		if(!g_di_index_update){
			TWARN("fopen g_di_index_file error, return!");
			return -1;
		}
	}
	
	// write content to data file
	if(fwrite(content, 1, len, g_di_data_update)!=len){
		TWARN("fwrite g_di_data_update error, return!");
		return -1;
	}
	fflush(g_di_data_update);
	
	// write index to index file
	if(fprintf(g_di_index_update, "%lld %lld %d\n", nid, g_di_data_length, len)<0){
		TWARN("fprintf g_di_index_update error, return!");
		return -1;
	}
	fflush(g_di_index_update);
	
	g_di_data_length += len;
	
	// 将索引插入内存中
	if(di_table_insert(nid, g_di_data_length-len, len)<0){
		TWARN("di_table_insert error, return!");
		return -1;
	}
	
	return 0;
}

/*
 * 获取detail中存储的字段数量
 *
 * @return detail中存储的字段数量
 */
extern const int get_di_field_count()
{
	return g_di_field_count;
}

/*
 * 获取detail中存储的字段名字数组
 *
 * @return detail中存储的字段名字数组
 */
extern const char** get_di_field_name()
{
	return (const char**)g_di_field_array;
}

/*
 * 获取detail中存储的字段名字长度数组
 *
 * @return detail中存储的字段名字长度数组
 */
extern const int* get_di_field_nlen()
{
	return g_di_flen_array;
}

/*
 * 申请字段参数数组
 *
 * @param args_p 存放申请到的字段参数数组
 *
 * @return < 0 申请出错
 *         >=0 数组长度
 */
extern int di_detail_arg_alloc(char*** args_p)
{
	if(!args_p){
		return -1;
	}
	
	*args_p = (char**)malloc(sizeof(char*)*(g_di_field_count+1));
	if(!*args_p){
		TWARN("Out of memory!");
		*args_p = 0;
		return -1;
	}
	bzero(*args_p, sizeof(char*)*(g_di_field_count+1));
	
	return (g_di_field_count+1);
}

/*
 * 释放字段参数数组
 *
 * @param args_p 释放的字段参数数组
 */
extern void di_detail_arg_free(char** args)
{
	if(args){
		free(args);
	}
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
extern void di_detail_arg_set(char** args, const int argcnt, char* field, int flen, char* arg, const char* dl)
{
	di_postprocessor_t* postprocessor = 0;
	di_argument_set_t* arg_set_func = 0;
	
	if(dl){
		postprocessor = di_postprocessor_manager_get(&manager, dl, strlen(dl));
	}
	else{
		postprocessor = di_postprocessor_manager_get(&manager, "default", 7);
	}
	if(postprocessor){
		arg_set_func = postprocessor->arg_set_func;
	}
	else{
		arg_set_func = di_argument_set;
	}
	
	arg_set_func(args, argcnt, field, flen, arg);
}

/*
 * 返回持久化数量文件路径
 *
 * @return 路径
 */
extern const char* get_di_msgcnt_file()
{
	return g_di_msgcnt_file;
}
