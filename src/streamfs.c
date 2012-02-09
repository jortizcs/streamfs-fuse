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
//static const char *sfs_path = "/temp";

static int sfs_getattr(const char *path, struct stat *stbuf)
{
    char* getresp;
    cJSON* json;
	int res = 0, isdir_t, getok=0;

	memset(stbuf, 0, sizeof(struct stat));
    getok=get((char*)path,&getresp);
    if(getok>0){
        isdir_t = isdir((char*)path);
        json = cJSON_Parse(getresp);
        fprintf(stdout, "path=%s, resp=%s, isdir=%d\n", path, getresp, isdir_t);
        if(isdir_t==1){
            stbuf->st_mode = S_IFDIR | 0x0755;
            //stbuf->st_nlink = 2;
        } else {
            stbuf->st_mode = S_IFREG | 0x0444;
		    //stbuf->st_nlink = 1;
            stbuf->st_size = strlen(getresp);
        } 
        cJSON_Delete(json);
        free(getresp);
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
    
    //log_msg("\nsfs_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    //log_fi(fi);
	
    return retstat;
}

int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    /*log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    log_fi(fi);

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    retstat = close(fi->fh);*/
    
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
    int gsize=0;
	size_t len;
	(void) fi;

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
        else
            size = len;
		memcpy(buf, sfs_str + offset, size);
        fprintf(stdout, "\tsfs_read::buf=%s\n\tsize=%d\n", buf,(int)size);
	} else
		size = 0;

    fprintf(stdout, "returning size=%d\n", (int)size);
	return size;
    /*size_t len;
	(void) fi;

	len = strlen(sfs_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, sfs_str + offset, size);
	} else
		size = 0;

	return size;*/
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
    return retstat;
}

int sfs_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;
    return retstat;
}

static struct fuse_operations sfs_oper = {
	.getattr	= sfs_getattr,
	.readdir	= sfs_readdir,
    .open       = sfs_open, 
	.read		= sfs_read,

    .flush      = sfs_flush,
    .release    = sfs_release,
    
    .mkdir      = sfs_mkdir,
    .mknod      = sfs_mknod,
    .write      = sfs_write,
    .utime      = sfs_utime,
    .unlink     = sfs_unlink,
    .rmdir      = sfs_rmdir,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &sfs_oper, NULL);
}
