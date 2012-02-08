#ifndef SFSLIB_H
#define SFSLIB_H 

int get(char *, char**);
int get_writer(char *, size_t, size_t, char *);
int isdir(char*);
int mkdefault(char*);
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);
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
#endif
