/*
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static char *sfs_str = "Hello World!\n";
static const char *sfs_path = "/sfs";

static int sfs_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, sfs_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(sfs_str);
	} else
		res = -ENOENT;

	return res;
}

static int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
    char* getstat;
    int gsize=0;
	(void) offset;
	(void) fi;

    
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, sfs_path + 1, NULL, 0);
    filler(buf, "admin", NULL, 0);

    //read the streamfs path
    gsize = get(path, &getstat);
    if(gsize>0)
	    free(getstat);

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
    fprintf("getting: %s\n", path);
    gsize = get(path, &getstat);
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
