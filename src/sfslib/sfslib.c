#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <pthread.h>

int get(char *, char**);
//int main(int, char*[]);
int get_writer(char *, size_t, size_t, char *);

//GLOBALS
char* get_resp;
char* post_resp;
char* put_resp;
char* delete_resp;

pthread_mutex_t get_lock;
pthread_mutex_t put_lock;
pthread_mutex_t post_lock;
pthread_mutex_t delete_lock;

int get(char * path, char** buffer){
	CURL *curl;
	CURLcode res;
    int size=0;

	curl = curl_easy_init();
	if(curl) {

		curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080");
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_writer);
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, get_resp);

        //We lock here for safety, bit this is a major performance
        //bottlebeck at scale; must be re-designed
        pthread_mutex_lock (&get_lock);
		res = curl_easy_perform(curl);
        size = strlen(get_resp);
        if(size>sizeof(*buffer))
            *buffer = malloc(strlen(get_resp));
        strcpy(*buffer, get_resp);
        free(get_resp);
        pthread_mutex_unlock (&get_lock);

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
	return size;
}

int get_writer(char *data, size_t size, size_t nmemb, char *buffer)
{
    int result = 0;
    if(get_resp == NULL || sizeof(get_resp)<(size * nmemb + 1))
        get_resp = (char*)malloc(strlen(data)*sizeof(char) + 1);
    strcpy(get_resp, data);
    get_resp[strlen(data)] = '\0';
    result = strlen(get_resp);
    return result;
}

/*bool isDir(char * path){
    char* resp;
    int s =0;
    s=get(path,resp);
    if(resp !=null){

        //convert resp to json object, return true if "type=DEFAULT" is set in the response

        free(resp);
    }
}*/

/*int main(int argc, char*argv[]){
	char* data;
	if(get("/", &data)>0)
		fprintf(stdout,"get(\"/\")=%s",data);
}*/
