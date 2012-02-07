/*
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "cJSON/cJSON.h"
#include "sfslib/sfslib.h"

static char *sfs_str = "Hello World!\n";
static const char *sfs_path = "/temp";

static int sfs_getattr(const char *path, struct stat *stbuf)
{
    char* getresp;
    cJSON* json;
	int res = 0, isdir_t;

	memset(stbuf, 0, sizeof(struct stat));
	/*if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, sfs_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(sfs_str);
	} else if(strcmp(path, "/admin") == 0){
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_size=25;
    } else 
		res = -ENOENT;*/
    get((char*)path,&getresp);
    if(getresp!=NULL){
        isdir_t = isdir((char*)path);
        json = cJSON_Parse(getresp);
        fprintf(stdout, "path=%s, resp=%s, isdir=%d\n", path, getresp, isdir_t);
        if(isdir_t==1){
            stbuf->st_mode = S_IFDIR | 0x0755;
            //stbuf->st_nlink = 2;
        } else {
            stbuf->st_mode = S_IFREG | 0x0444;
		    stbuf->st_nlink = 1;
        } 
        cJSON_Delete(json);
        free(getresp);
    } else
        res = -ENOENT;

	return res;
}

static int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
    char* getstat;
    int gsize=0, i, ch_size, isdir_t;
    //struct stat info;
    cJSON* json;
    struct cJSON* ch_obj;
	(void) offset;
	(void) fi;

    filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

    //read the streamfs path
    gsize = get((char*)path, &getstat);
    if(gsize>0){
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
	    free(getstat);
    } else
        return -ENOENT;
	/*if (strcmp(path, "/") != 0)
		return -ENOENT;

    info.st_mode = S_IFDIR | 0x0755;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, sfs_path + 1, NULL, 0);
    filler(buf, "admin", &info, 0);
    */

	return 0;
}

static int sfs_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, sfs_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int sfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
    char* getstat;
    int gsize=0;
	size_t len;
	(void) fi;
	if(strcmp(path, sfs_path) != 0)
		return -ENOENT;

    //read the streamfs path
    fprintf(stdout,"getting: %s\n", path);
    gsize = get((char*)path, &getstat);
    if(gsize>0){
        sfs_str = (char*) malloc(gsize+1);
        strcpy(sfs_str, getstat);
	    free(getstat);
    } else {
        return 0;
    }

	len = strlen(sfs_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, sfs_str + offset, size);
	} else
		size = 0;

	return size;
}

static struct fuse_operations sfs_oper = {
	.getattr	= sfs_getattr,
	.readdir	= sfs_readdir,
	.open		= sfs_open,
	.read		= sfs_read,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &sfs_oper, NULL);
}
