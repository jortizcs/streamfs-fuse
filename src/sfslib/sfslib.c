#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <curl/curl.h>
#include <pthread.h>
#include "../cJSON/cJSON.h"
#include "sfslib.h"

//int main(int, char*[]);
static int get_writer(char *, size_t, size_t, char *);
static size_t read_cr_callback(void *ptr, size_t size, size_t nmemb, void *stream);
static size_t read_cs_callback(void *ptr, size_t size, size_t nmemb, void *stream);
static char* split_parent_child(const char* path, int parent_child);

//GLOBALS
static char* get_resp;
static char* post_resp;
static char* put_resp;
static char* delete_resp;
static const char* sfs_server = "http://localhost:8080";

pthread_mutex_t get_lock;
pthread_mutex_t put_lock;
pthread_mutex_t post_lock;
pthread_mutex_t delete_lock;

static char* new_node_name;
static int CR_BASE_SIZE;
static int CS_BASE_SIZE;

static int globals_set=0;

inline static void set_globals(){
    if(globals_set==0){
        //set up global constants
        //create default
        char* out;
        cJSON* json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "operation", "create_resource");
        cJSON_AddStringToObject(json, "resourceName", "");
        cJSON_AddStringToObject(json, "resourceType", "default");
        out = cJSON_Print(json);
        CR_BASE_SIZE = strlen(out);
        cJSON_Delete(json);
        free(out);

        //create stream
        json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "operation", "create_generic_publiser");
        cJSON_AddStringToObject(json, "resourceName", "");
        out = cJSON_Print(json);
        CS_BASE_SIZE = strlen(out);
        cJSON_Delete(json);
        free(out);
    
        globals_set=1;
    }
}
#define CREATE_DEFAULT_SIZE CR_BASE_SIZE*sizeof(char)
#define CREATE_STREAM_SIZE CS_BASE_SIZE*sizeof(char)

int get(const char * path, char** buffer){
	CURL *curl;
	CURLcode res;
    char* fpath;
    int size=0;

	curl = curl_easy_init();
    fprintf(stdout, "input_path=%s\n", path);
	if(curl && path!=NULL) {
        fpath = (char*)malloc(strlen(sfs_server) + strlen(path));
        memset(fpath, 0, strlen(sfs_server) + strlen(path));
        strcpy(fpath, sfs_server);
        strcpy(&fpath[strlen(fpath)], path);
        fprintf(stdout, "GET fpath=%s\n", fpath);

		curl_easy_setopt(curl, CURLOPT_URL, fpath);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_writer);
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, get_resp);

        //We lock here for safety, bit this is a major performance
        //bottlebeck at scale; must be re-designed
        pthread_mutex_lock (&get_lock);
		res = curl_easy_perform(curl);
        fprintf(stdout, "res=%d,\tmsg=%s\n", (int)res, curl_easy_strerror(res));
        if(res != CURLE_HTTP_RETURNED_ERROR){//res == CURLE_OK){
            size = strlen(get_resp);
            if(size>sizeof(*buffer))
                *buffer = malloc(strlen(get_resp));
            strcpy(*buffer, get_resp);
            free(get_resp);
        }
        pthread_mutex_unlock (&get_lock);

		/* always cleanup */ 
		curl_easy_cleanup(curl);
        free(fpath);
	}

    if(res!=CURLE_HTTP_RETURNED_ERROR)
        return 1;
    return 0;
}

int delete_(const char * path){
	CURL *curl;
	CURLcode res;
    char* fpath;
    int size=0;
    long http_code=200L;

	curl = curl_easy_init();
    fprintf(stdout, "input_path=%s\n", path);
	if(curl && path!=NULL) {
        fpath = (char*)malloc(strlen(sfs_server) + strlen(path));
        memset(fpath, 0, strlen(sfs_server) + strlen(path));
        strcpy(fpath, sfs_server);
        strcpy(&fpath[strlen(fpath)], path);
        fprintf(stdout, "DELETE fpath=%s\n", fpath);

		curl_easy_setopt(curl, CURLOPT_URL, fpath);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

        res = curl_easy_perform(curl);
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
        //We lock here for safety, bit this is a major performance
        //bottlebeck at scale; must be re-designed
        /*pthread_mutex_lock (&delete_lock);
		res = curl_easy_perform(curl);
        fprintf(stdout, "res=%d,\tmsg=%s\n", (int)res, curl_easy_strerror(res));
        if(res != CURLE_HTTP_RETURNED_ERROR){//res == CURLE_OK){
            size = strlen(get_resp);
            if(size>sizeof(*buffer))
                *buffer = malloc(strlen(get_resp));
            strcpy(*buffer, get_resp);
            free(get_resp);
        }
        pthread_mutex_unlock (&delete_lock);*/

		/* always cleanup */ 
		curl_easy_cleanup(curl);
        free(fpath);
	}

    if(http_code != 200L)
        return -1;
    return 0;
}



int mkdefault(const char * path){
	CURL *curl;
	CURLcode res;
    char* fpath;
    char* parent;       //the directory to create the sub-directory in
    char* hd_src;
    long http_code;
    int send_size=0;

	curl = curl_easy_init();
    fprintf(stdout, "mkdefault::input_path=%s\n", path);
	if(curl && path!=NULL) {
        set_globals();
        parent = split_parent_child(path, 0);
        fpath = (char*)malloc(strlen(sfs_server) + strlen(parent));
        memset(fpath, 0, strlen(sfs_server) + strlen(parent));
        strcpy(fpath, sfs_server);
        strcpy(&fpath[strlen(fpath)], parent);
        fprintf(stdout, "PUT fpath=%s\n", fpath);

        new_node_name = split_parent_child(path,1);
        send_size = CREATE_DEFAULT_SIZE + strlen(new_node_name);
        hd_src =(char*)malloc(send_size + 5);
		curl_easy_setopt(curl, CURLOPT_URL, fpath);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_cr_callback);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)send_size);
                     

        //We lock here for safety, bit this is a major performance
        //bottlebeck at scale; must be re-designed
        pthread_mutex_lock (&put_lock);
		res = curl_easy_perform(curl);
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
        pthread_mutex_unlock (&put_lock);

		curl_easy_cleanup(curl);
        free(fpath);
        free(parent);
        free(hd_src);
        free(new_node_name);

        if(http_code==201L)
            return 0;
	}

    errno = ENOENT;
    return -1;
}

int mkstream(const char * path){
	CURL *curl;
	CURLcode res;
    char* fpath;
    char* parent;       //the directory to create the sub-directory in
    char* hd_src;
    long http_code;
    int send_size=0;

	curl = curl_easy_init();
    fprintf(stdout, "mkstream::input_path=%s\n", path);
	if(curl && path!=NULL) {
        set_globals();
        parent = split_parent_child(path, 0);
        fpath = (char*)malloc(strlen(sfs_server) + strlen(parent));
        memset(fpath, 0, strlen(sfs_server) + strlen(parent));
        strcpy(fpath, sfs_server);
        strcpy(&fpath[strlen(fpath)], parent);
        fprintf(stdout, "PUT fpath=%s\n", fpath);

        new_node_name = split_parent_child(path,1);
        send_size = CREATE_DEFAULT_SIZE + strlen(new_node_name);
        hd_src =(char*)malloc(send_size + 5);
		curl_easy_setopt(curl, CURLOPT_URL, fpath);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_cs_callback);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)send_size);
                     
        //We lock here for safety, bit this is a major performance
        //bottlebeck at scale; must be re-designed
        pthread_mutex_lock (&put_lock);
		res = curl_easy_perform(curl);
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
        pthread_mutex_unlock (&put_lock);

		curl_easy_cleanup(curl);
        free(fpath);
        free(parent);
        free(hd_src);
        free(new_node_name);

        if(http_code==201L)
            return 0;
	}

    errno = ENOENT;
    return -1;
}

int get_writer(char *data, size_t size, size_t nmemb, char *buffer)
{
    int result = 0;
    if(data !=NULL && (get_resp == NULL || sizeof(get_resp)<(size * nmemb + 1)) )
        get_resp = (char*)malloc(strlen(data)*sizeof(char) + 1);
    strcpy(get_resp, data);
    get_resp[strlen(data)] = '\0';
    result = strlen(get_resp);
    return result;
}

int isdir(const char * path){
    char* resp;
    cJSON* obj;
    int s =0;
    if(strcmp(path, "/")==0)
        return 1;
    s=get(path, &resp);
    if(s>0 && resp !=NULL){
        //convert resp to json object, return true if "type=DEFAULT" is set in the response
        obj = cJSON_Parse(resp);
        if(cJSON_GetObjectItem(obj, "type") != NULL &&
            strcmp(cJSON_GetObjectItem(obj, "type")->valuestring, "DEFAULT")==0)
            return 1;
        free(resp);
        cJSON_Delete(obj);
    }

    return 0;
}

static size_t read_cr_callback(void *ptr, size_t size, size_t nmemb, void *stream){
    if(new_node_name != NULL && ptr != NULL){
        cJSON * json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "operation", "create_resource");
        cJSON_AddStringToObject(json, "resourceName", new_node_name);
        cJSON_AddStringToObject(json, "resourceType", "default");
        strncpy((char*)ptr, cJSON_Print(json), size*nmemb);
        cJSON_Delete(json);
        return strlen(ptr)*sizeof(char);
    }
    return 0;
}

static size_t read_cs_callback(void *ptr, size_t size, size_t nmemb, void *stream){
    if(new_node_name != NULL && ptr != NULL){
        cJSON * json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "operation", "create_generic_publisher");
        cJSON_AddStringToObject(json, "resourceName", new_node_name);
        strncpy((char*)ptr, cJSON_Print(json), size*nmemb);
        cJSON_Delete(json);
        return strlen(ptr)*sizeof(char);
    }
    return 0;
}

/*int main(int argc, char*argv[]){
	char* data;
    char* ptr;
    char* out;
    char* tok;
    char* blah;
    char* last_tok;
    char* parent;
    cJSON* json;
	if(get("/", &data)>0){
        get("/", &data);
		fprintf(stdout,"get(\"/\")=%s",data);
        free(data);

        fprintf(stdout, "isdir? %d\n", isdir("/temp"));
        fprintf(stdout, "isdir? %d\n", isdir("/sfs"));
    }

    set_globals();
    new_node_name = "blah";
    ptr = (char*)malloc(CREATE_DEFAULT_SIZE + strlen(new_node_name) + 1);
    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "operation", "create_resource");
    cJSON_AddStringToObject(json, "resourceName", "blah");
    cJSON_AddStringToObject(json, "resourceType", "default");
    out = cJSON_Print(json);
    fprintf(stdout, "out=\n\t%s\n", out);
    fprintf(stdout, "strlen(out)=%d\n", (int)strlen(out));
    strncpy(ptr,out, CREATE_DEFAULT_SIZE + strlen(new_node_name));
    fprintf(stdout, "ptr=%s\n", ptr);
    fprintf(stdout, "strlen(ptr)=%d\n", (int)strlen(ptr));
    cJSON_Delete(json);
    free(ptr);
    free(out);

    blah = split_parent_child("/temp", 0);
    fprintf(stdout, "parent=%s\n", blah);
    free(blah);

    blah = split_parent_child("/temp", 1);
    fprintf(stdout, "child=%s\n", blah);
    free(blah);

    return 0;
}*/
    

static char* split_parent_child(const char* path, int parent_child){
    char * fullpath;
    char* parent;
    char* last_tok;
    char* tok;
    int new_length=0;
    if(path != NULL){
        //root case
        if(strcmp(path, "/")==0 && parent_child==0){
            fullpath= (char*)malloc(sizeof(char)*strlen(path) + 1);
            memset(fullpath, 0, sizeof(char)*strlen(path) + 1);
            strcpy(fullpath,"/");
            return fullpath;
        } else if(strcmp(path, "/")==0 && parent_child!=0){
            last_tok = (char*)malloc(sizeof(char)*strlen(path) + 1);
            memset(last_tok, 0, sizeof(char)*strlen(path) + 1);
            last_tok = (char*)malloc(sizeof(char)*strlen(path) + 1);
            strcpy(last_tok, "");
            return last_tok;
        }

        //non-corner case
        fullpath= (char*)malloc(sizeof(char)*strlen(path) + 1);
        memset(fullpath, 0, sizeof(char)*strlen(path) + 1);
        last_tok = (char*)malloc(sizeof(char)*strlen(path) + 1);
        memset(last_tok, 0, sizeof(char)*strlen(path) + 1);
        strcpy(fullpath, path);
        tok = strtok(fullpath, "/");
        while(tok != NULL){
            memset(last_tok, 0, sizeof(char)*strlen(path) + 1);
            strcpy(last_tok,tok);
            last_tok[strlen(last_tok)]='\0';
            tok = strtok(NULL, "/");
        }
        strcpy(fullpath, path);
        parent = (char*) malloc(sizeof(char)*(strlen(fullpath)-strlen(last_tok)));
        new_length= strlen(fullpath)-(strlen(last_tok)+1);
        strncpy(parent, fullpath, new_length);
        parent[new_length]='\0';
        free(fullpath);
        if(parent_child==0){
            if(strcmp(parent, "")==0)
                strcpy(parent, "/\0");
            free(last_tok);
            return parent;
        } else {
            free(parent);
            return last_tok;
        }
    }
    return NULL;
}
