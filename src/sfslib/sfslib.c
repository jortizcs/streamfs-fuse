#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <pthread.h>

#include "../cJSON/cJSON.h"

int get(char *, char**);
//int main(int, char*[]);
int get_writer(char *, size_t, size_t, char *);
int isdir(char*);

//GLOBALS
char* get_resp;
char* post_resp;
char* put_resp;
char* delete_resp;
static const char* sfs_server = "http://localhost:8080";


pthread_mutex_t get_lock;
pthread_mutex_t put_lock;
pthread_mutex_t post_lock;
pthread_mutex_t delete_lock;

int get(char * path, char** buffer){
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
	return size;
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

int isdir(char * path){
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

/*int main(int argc, char*argv[]){
	char* data;
	if(get("/", &data)>0){
        get("/", &data);
		fprintf(stdout,"get(\"/\")=%s",data);
        free(data);

        fprintf(stdout, "isdir? %d\n", isdir("/temp"));
        fprintf(stdout, "isdir? %d\n", isdir("/sfs"));
    }
    
    return 0;
}*/
