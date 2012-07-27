#include "di_postprocessor_manager.h"
#include "di_default_postprocessor.h"
#include "util/Log.h"
#include <dirent.h>
#include <dlfcn.h>

extern int di_postprocessor_manager_init(di_postprocessor_manager_t* manager, const char* plugin_path)
{
	DIR* dir = 0;
	struct dirent file;
	struct dirent* retptr = 0;
	
	void* handle = 0;
	di_get_name_string_t* get_name_string_func = 0;
	di_get_name_length_t* get_name_length_func = 0;
	di_get_field_count_t* get_field_count_func = 0;
	di_get_model_string_t* get_model_string_func = 0;
	di_get_model_length_t* get_model_length_func = 0;
	di_postprocessor_init_t* init_func = 0;
	di_postprocessor_dest_t* dest_func = 0;
	di_postprocessor_work_t* work_func = 0;
	di_argument_set_t* arg_set_func = 0;
	
	char path_buf[PATH_MAX+1];
	int len = 0;
	
	if(!manager){
		TERR("di_postprocessor_manager_init argument error, return!");
		return -1;
	}
	
	manager->postprocessor_cnt = 0;
	
	if(plugin_path && plugin_path[0]){
		dir = opendir(plugin_path);
		if(!dir){
			TERR("opendir plugin_path error, return!");
			return -1;
		}
		
		while(readdir_r(dir, &file, &retptr)==0 && retptr){
			len = strlen(file.d_name);
			if((file.d_type!=DT_REG && file.d_type!=DT_LNK) || len<4 || strstr(file.d_name, ".so")==0){
				continue;
			}
			snprintf(path_buf, PATH_MAX+1, "%s/%s", plugin_path, file.d_name);
			handle = dlopen(path_buf, RTLD_NOW);
			if(!handle){
				TERR("dlopen %s error, return![error=%s]", path_buf, dlerror());
				return -1;
			}
			get_name_string_func = (di_get_name_string_t*)dlsym(handle, GET_NAME_STRING_FUNC_NAME);
			get_name_length_func = (di_get_name_length_t*)dlsym(handle, GET_NAME_LENGTH_FUNC_NAME);
			get_field_count_func = (di_get_field_count_t*)dlsym(handle, GET_FIELD_COUNT_FUNC_NAME);
			get_model_string_func = (di_get_model_string_t*)dlsym(handle, GET_MODEL_STRING_FUNC_NAME);
			get_model_length_func = (di_get_model_length_t*)dlsym(handle, GET_MODEL_LENGTH_FUNC_NAME);
			init_func = (di_postprocessor_init_t*)dlsym(handle, INIT_FUNC_NAME);
			dest_func = (di_postprocessor_dest_t*)dlsym(handle, DEST_FUNC_NAME);
			work_func = (di_postprocessor_work_t*)dlsym(handle, WORK_FUNC_NAME);
			arg_set_func = (di_argument_set_t*)dlsym(handle, ARG_SET_FUNC_NAME);
			if(!get_name_string_func || !get_name_length_func || !get_field_count_func ||
			   !get_model_string_func || !get_model_length_func ||
			   !init_func || !dest_func || !work_func || !arg_set_func){
				TERR("dlsym %s error, return!", path_buf);
				return -1;
			}
			if(di_postprocessor_manager_put(manager, handle,
			   get_name_string_func, get_name_length_func,
			   get_field_count_func, get_model_string_func, get_model_length_func,
			   init_func, dest_func, work_func, arg_set_func)){
				TERR("di_postprocessor_manager_put %s error, return!", path_buf);
				return -1;
			}
			TNOTE("put %s in di_postprocessor_manager success!", path_buf);
		}
		
		closedir(dir);
	}
	
	if(di_postprocessor_manager_put(manager, NULL,
	   di_get_name_string, di_get_name_length,
	   di_get_field_count, di_get_model_string, di_get_model_length,
	   di_postprocessor_init, di_postprocessor_dest, di_postprocessor_work, di_argument_set)){
		TERR("di_postprocessor_manager_put %s error, return!", path_buf);
		return -1;
	}
	TNOTE("put default_postprocessor in di_postprocessor_manager success!");
	
	return 0;
}

extern void di_postprocessor_manager_dest(di_postprocessor_manager_t* manager)
{
	int i = 0;
	
	if(!manager){
		TWARN("di_postprocessor_manager_dest argument error, return!");
		return;
	}
	
	for(i=0; i<manager->postprocessor_cnt; i++){
		if(manager->postprocessors[i].handle){
			dlclose(manager->postprocessors[i].handle);
		}
	}
}

extern int di_postprocessor_manager_put(di_postprocessor_manager_t* manager, void* handle,
	di_get_name_string_t* get_name_string_func,
	di_get_name_length_t* get_name_length_func,
	di_get_field_count_t* get_field_count_func,
	di_get_model_string_t* get_model_string_func,
	di_get_model_length_t* get_model_length_func,
	di_postprocessor_init_t* init_func,
	di_postprocessor_dest_t* dest_func,
	di_postprocessor_work_t* work_func,
	di_argument_set_t* arg_set_func)
{
	int no = 0;
	
	if(!manager || !get_name_string_func || !get_name_length_func || !get_field_count_func ||
	   !get_model_string_func || !get_model_length_func || !init_func || !dest_func || !work_func || !arg_set_func){
		TWARN("di_postprocessor_manager_put argument error, return!");
		return -1;
	}
	
	no = manager->postprocessor_cnt;
	if(no==POSTPROCESSER_MAX){
		TWARN("di_postprocessor_manager full error, return!");
		return -1;
	}
	manager->postprocessor_cnt ++;
	manager->postprocessors[no].handle = handle;
	manager->postprocessors[no].name_string_func = get_name_string_func;
	manager->postprocessors[no].name_length_func = get_name_length_func;
	manager->postprocessors[no].field_count_func = get_field_count_func;
	manager->postprocessors[no].model_string_func = get_model_string_func;
	manager->postprocessors[no].model_length_func = get_model_length_func;
	manager->postprocessors[no].init_func = init_func;
	manager->postprocessors[no].dest_func = dest_func;
	manager->postprocessors[no].work_func = work_func;
	manager->postprocessors[no].arg_set_func = arg_set_func;
	
	return 0;
}

extern di_postprocessor_t* di_postprocessor_manager_get(di_postprocessor_manager_t* manager, const char* name, const int name_len)
{
	int i = 0;
	const char* name_string = 0;
	int name_length = 0;
	
	if(!manager || !name || !name_len){
		TWARN("di_postprocessor_manager_init argument error, return!");
		return 0;
	}
	
	for(i=0; i<manager->postprocessor_cnt; i++){
		name_string = manager->postprocessors[i].name_string_func();
		name_length = manager->postprocessors[i].name_length_func();
		if(name_length==name_len && strncmp(name, name_string, name_length)==0){
			return (&manager->postprocessors[i]);
		}
	}
	
	return 0;
}
