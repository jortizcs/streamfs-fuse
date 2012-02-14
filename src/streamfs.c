/*
*/

//#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include "cJSON/cJSON.h"
#include "sfslib/sfslib.h"

#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)

static char *sfs_str = "Hello World!\n";
//static const char *sfs_path = "/temp";
static cJSON* queryres_cache=NULL;
static pthread_mutex_t qres_lock;

//the returned string MUST be freed after it's used.
static char* fmt_ts_query_result(cJSON* prev_reply_json, const char* path, int* size){
    char* resp_cpy_str;         //feed()'d locally
    char* fmtstr;               //free()'d by caller
    char* ts;
    char* v;
    cJSON* data_obj;            //delete not needed
    cJSON* tsarray;             //delete not needed
    int i=0,dtptct=0;

    /*pthread_mutex_lock(&qres_lock);
    if((prev_reply_json=cJSON_GetObjectItem(queryres_cache, path)) != NULL){*/
    resp_cpy_str = cJSON_PrintUnformatted(prev_reply_json);
    fprintf(stdout, "resp_cpy_str=%s\n", resp_cpy_str);
    if(prev_reply_json == NULL)
        fprintf(stdout, "Previously reply is NULL");
    //resp_cpy = cJSON_Parse(resp_cpy_str);
    fmtstr = (char*)calloc(strlen(resp_cpy_str),sizeof(int));
    free(resp_cpy_str);
    /*}
    pthread_mutex_unlock(&qres_lock);*/
    tsarray=cJSON_GetObjectItem(prev_reply_json, "ts_query_results");
    if(tsarray!=NULL){
        fprintf(stdout, "tsarray NOT NULL\n");
        dtptct = cJSON_GetArraySize(tsarray);
        fprintf(stdout, "array_size=%d\n", dtptct);
        if(dtptct>0){
            *size=0;
            for(i=0; i<dtptct; ++i){
                data_obj = cJSON_GetArrayItem(tsarray, i);
                ts = cJSON_Print(cJSON_GetObjectItem(data_obj,"ts"));
                v = cJSON_Print(cJSON_GetObjectItem(data_obj, "value"));
                fprintf(stdout, "\tts=%s, v=%s\n", ts, v);
                sprintf(fmtstr + strlen(fmtstr), "%s %s\n",ts, v);
                (*size) += strlen(ts) + strlen(v) + 2;
                free(ts);
                free(v);
            }
            if(size>0)
                fmtstr[*size]='\0';
            
        }
        fprintf(stdout, "fmtstr=\n%s", fmtstr);
        //cJSON_Delete(resp_cpy);
    }else {
        fprintf(stdout, "tsarray NULL\n");
    }

    return fmtstr;
}

static int sfs_getattr(const char *path, struct stat *stbuf)
{
    char* getresp;
    cJSON* json;
    cJSON* prev_reply_json;
	int res = 0, isdir_t, getresp_size=-1;

	memset(stbuf, 0, sizeof(struct stat));
    getresp=get(path);
    if(strlen(getresp)>0){
        isdir_t = isdir((char*)path);
        json = cJSON_Parse(getresp);
        fprintf(stdout, "path=%s, resp=%s, isdir=%d\n", path, getresp, isdir_t);
        if(isdir_t==1){
            stbuf->st_mode = S_IFDIR | 0755;
            //stbuf->st_nlink = 2;
        } else {
            stbuf->st_mode = S_IFREG | 0666;
		    //stbuf->st_nlink = 1;
            pthread_mutex_lock(&qres_lock);
            if((prev_reply_json=cJSON_GetObjectItem(queryres_cache, path)) != NULL){
                //tsres = cJSON_GetObjectItem(prev_reply_json, "ts_query_results");
                getresp = fmt_ts_query_result(prev_reply_json, path, &getresp_size);
                stbuf->st_size = getresp_size;
                free(getresp);
            } else {
                stbuf->st_size = strlen(getresp);
            }
            pthread_mutex_unlock(&qres_lock);
        } 
        cJSON_Delete(json);
    } else {
        fprintf(stdout, "%s does not exist\n", path);
        res = -ENOENT;
    }
	return res;
}

static int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
    char* getstat;
    int i, ch_size, isdir_t;
    //struct stat info;
    cJSON* json;
    struct cJSON* ch_obj;
	(void) offset;
	(void) fi;

    filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
    filler(buf, "._.", NULL, 0);
    filler(buf, "Backups.backupdb", NULL, 0);
    filler(buf, "mach_kernel", NULL, 0);

    //read the streamfs path
    getstat = get(path);
    if(strlen(getstat)>0){
        fprintf(stdout, "sfs_readdir::getstat=%s\n", getstat);
        json = cJSON_Parse(getstat);
        isdir_t = isdir((char*)path);
        if(isdir_t==1){
            ch_obj = cJSON_GetObjectItem(json, "children");
            if(ch_obj != NULL){
                ch_size = cJSON_GetArraySize(ch_obj);
                fprintf(stdout, "\tarray_size=%d\n", ch_size);
                for(i=0; i<ch_size; i++){
                    if(strcmp(cJSON_GetArrayItem(ch_obj, i)->valuestring, "ibus")!=0 &&
                        strcmp(cJSON_GetArrayItem(ch_obj, i)->valuestring, "time")!=0 &&
                        strcmp(cJSON_GetArrayItem(ch_obj, i)->valuestring, "resync")!=0)
                        filler(buf, cJSON_GetArrayItem(ch_obj, i)->valuestring, NULL, 0);
                    fprintf(stdout, "\titem %d=%s\n", i, cJSON_GetArrayItem(ch_obj, i)->valuestring);
                }
            }
        }
        cJSON_Delete(json);
    } else
        return -ENOENT;

	return 0;
}

/*static int sfs_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, sfs_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}*/

int sfs_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    return retstat;
}

int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    return retstat;
}

int sfs_unlink(const char *path)
{
    int retstat = 0;
    retstat = delete_(path);
    return retstat;
}

int sfs_rmdir(const char *path)
{
    int retstat = 0;
    retstat = delete_(path);
    return retstat;
}

static int sfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
    char* getstat;
    int gsize=-1;
    cJSON* prev_reply_json;
    int retqrep=0;
	size_t len;
	(void) fi;

    //check if this stream file was previously queried
    //if it was, return that reply.
    pthread_mutex_lock(&qres_lock);
    if((prev_reply_json=cJSON_GetObjectItem(queryres_cache, path)) != NULL){
        //getstat = cJSON_Print(prev_reply_json);
        getstat = fmt_ts_query_result(prev_reply_json, path, &gsize);
        //gsize=strlen(getstat);
        fprintf(stdout, "prev_qres=%s\n", getstat);
        cJSON_DetachItemFromObject(queryres_cache, path);
        fprintf(stdout, "\tgsize=%d strlen=%d\n", (int)gsize, (int)strlen(getstat));
        retqrep = 1;
    }
    pthread_mutex_unlock(&qres_lock);

    //read the streamfs path
    fprintf(stdout,"getting: %s\n", path);
    //if the file was not previously queried, return the GET reply from this file
    if(retqrep==0){
        getstat = get((char*)path);
        gsize=strlen(getstat);
    }
    if(gsize>0){
        sfs_str = (char*) malloc(gsize+1);
        memset(sfs_str, 0, sizeof(sfs_str));
        strcpy(sfs_str, getstat);
    } else {
        return 0;
    }

	len = strlen(sfs_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
        else
            size = len;
        memset(buf, 0, size);
		memcpy(buf, sfs_str + offset, size);
        fprintf(stdout, "\tsfs_read::buf=%s\n\tsize=%d\n", buf,(int)size);
	} else
		size = 0;

    fprintf(stdout, "returning size=%d\n", (int)size);
	return size;
}

/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    fprintf(stdout, "\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode); 
    retstat = mkdefault(path);
    return retstat;
}

int sfs_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = 0;
    
    // On Linux this could just be 'mknod(path, mode, rdev)' but this
    //  is more portable
    if (S_ISREG(mode)) {
        //retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        retstat = mkstream(path);
        /*if (retstat < 0)
            retstat = sfs_error("sfs_mknod open");
        else {
            retstat = close(retstat);
            if (retstat < 0)
                retstat = sfs_error("sfs_mknod close");
        }*/
    } /*else
        if (S_ISFIFO(mode)) {
            retstat = mkfifo(fpath, mode);
            if (retstat < 0)
            retstat = sfs_error("sfs_mknod mkfifo");
        } else {
            retstat = mknod(fpath, mode, dev);
            if (retstat < 0)
            retstat = sfs_error("sfs_mknod mknod");
        }*/
    
    return retstat;
}

int sfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd=1;
    fi->fh = fd;
    return retstat;
}

int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    char* queryrep;
    const char* queryresp_;
    char* query_prefix = "?query=true&";
    char* fpath;
    char* p;
    cJSON* resp_json;
    fprintf(stdout, "buf=%s\n", buf);
    fpath = (char*) malloc(sizeof(char)*(strlen(path) + strlen(query_prefix) + strlen(buf)));
    strcpy(fpath, path);
    strcat(fpath, query_prefix);
    strncat(fpath, buf+offset, size);
    p = strchr(fpath,'\n');
    if (p)
      *p = '\0';
    fprintf(stdout, "fullpath=%s\n", fpath);
    queryrep=get(fpath);
    if(fpath != NULL)
        free(fpath);
    if(strlen(queryrep)>0){
        queryresp_ = queryrep;
        resp_json = cJSON_Parse(queryrep);
        retstat = strlen(buf)*sizeof(char);
        fprintf(stdout, "query_reply=%s\n", queryrep);
        pthread_mutex_lock(&qres_lock);
        if(cJSON_GetObjectItem(queryres_cache, path) == NULL)
            cJSON_AddItemToObject(queryres_cache, path, resp_json);
        else
            cJSON_ReplaceItemInObject(queryres_cache, path, resp_json);
        pthread_mutex_unlock(&qres_lock);
    }
    return retstat;
}

int sfs_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;
    return retstat;
}

/** Change the permission bits of a file */
int sfs_chmod(const char *path, mode_t mode)
{
    int retstat = 0;
    return retstat;
}

/** Change the owner and group of a file */
int sfs_chown(const char *path, uid_t uid, gid_t gid)
{
    int retstat = 0;
    return retstat;
}

/** Change the size of a file */
int sfs_truncate(const char *path, off_t newsize)
{
    int retstat = 0;
    return retstat;
}

void* sfs_init(struct fuse_conn_info *conn)
{
    init_sfslib();
    pthread_mutex_lock(&qres_lock);
    queryres_cache = cJSON_CreateObject();
    pthread_mutex_unlock(&qres_lock);
    return SFS_DATA;
}

void sfs_destroy(void* userdata)
{
    shutdown_sfslib();
    pthread_mutex_lock(&qres_lock);
    if(queryres_cache != NULL)
        cJSON_Delete(queryres_cache);
    pthread_mutex_unlock(&qres_lock);
}

static struct fuse_operations sfs_oper = {
    .init       = sfs_init,
    .destroy    = sfs_destroy,
	.getattr	= sfs_getattr,
	.readdir	= sfs_readdir,
    .open       = sfs_open, 
	.read		= sfs_read,
    .flush      = sfs_flush,
    .release    = sfs_release,
    .chmod      = sfs_chmod,
    .chown      = sfs_chown,
    .mkdir      = sfs_mkdir,
    .mknod      = sfs_mknod,
    .write      = sfs_write,
    .utime      = sfs_utime,
    .unlink     = sfs_unlink,
    .rmdir      = sfs_rmdir,
    .truncate   = sfs_truncate,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &sfs_oper, NULL);
}
