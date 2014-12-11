/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR


//encryption constants
#define ENCRYPT 1
#define DECRYPT 0
#define PASS -1
//some numbers
#define PATH_MAX 255


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include "aes-crypt.h"

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>


#endif

//./encrypt-fs password mirror/ mnt/ -d

struct xmp_info
{
	//our mountpoint
	char *mountpoint;
	//mirrored directory path
	char *mirror;
	//password for encryption/decryption
	char *password;
};

#define XMP_DATA ((struct xmp_info *) fuse_get_context()->private_data)

static int fixPath(char fixedpath[PATH_MAX], const char *path)
{


	char *mir = XMP_DATA->mirror;
	//copy the mirrored directory into our mirrored path
	fixedpath = strcpy(fixedpath, mir);
	//concatenate our path
	fixedpath = strcat(fixedpath, path);
	return 0;
}


#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char fpath[PATH_MAX];
	fixPath(fpath,path);

	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name,char *value,
			size_t size)
{
	char fpath[PATH_MAX];
	fixPath(fpath,path);

	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char fpath[PATH_MAX];
	fixPath(fpath,path);
	int res = llistxattr(fpath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char fpath[PATH_MAX];
	fixPath(fpath,path);

	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

//returns the path of the temp file 
static char* tmp_path(const char* old_path, const char *suffix, const * return_path){
    char* new_path;
    int len=0;
    len=strlen(old_path) + strlen(suffix) + 1;
    new_path = malloc(sizeof(char)*len);
    
    if(new_path == NULL){
        return NULL;
    }

    new_path[0] = '\0';
    strcat(new_path, old_path);
    strcat(new_path, suffix);
    strcpy(return_path,new_path);
    
    //free it, so we don't end up with a memory leak
    free(new_path);
    return 0;
}




/* gets the characteristics of a file from lstat and stores them.*/
static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

//Checks flags if the file is encrypted
//see xattr-util.c
static int isenc(const char *path)
{
	ssize_t valuelength;
	char* value;
	
	//get the length of the memory space for the attribute
	valuelength = xmp_getxattr(path, "user.encrypted", NULL, 0);
	if (valuelength < 0) { 
		return -errno;
	}
	
	//allocate space for the value
	value = malloc(sizeof(*value)*(valuelength+1));
	
	//get the value of the attribute
	
	valuelength = xmp_getxattr(path, ENCRYPT, value, valuelength);

    value[valuelength] = '\0';
	
	//check if it is encrypted
	if (!strcmp(value, "true")){
		free(value);
		return 1;
	}
	free(value);
	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = access(fpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	char fpath[PATH_MAX];
	fixPath(fpath,path);
	res = mkdir(fpath, mode);
	
	//this checks if it can make a directory 
	if (res == -1)
		return -errno;
	
	//do any function calls here
	printf("PAATH IS %s\n",path );

	printf("%s\n", "I AM MAKING A DIRECTORY!" );
	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = unlink(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = open(fpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}


//big changes here
static int xmp_read(const char *path, char *buf, size_t size, off_t offset)
{
	FILE *fp, *temp;
	int fd;
	int res;

	char fpath[PATH_MAX];

	//this will be our new path we will use
	char temp_path[PATH_MAX];

	fixPath(fpath,path);


	if(isenc(fpath)){

		printf("FPath of file to open: %s\n",fpath );
		
		//create a new temp file		
		fp = fopen(fpath, "rb");

		//string for our temp path 
		tmp_path(fpath, ".tmp",temp_path);
		printf("temp path is %s\n", temp_path);
		
		//this tries to open our temp path, and if it doesn't exist it creates it 
		temp = fopen(temp_path, "wb+");
		
		//decrypt into the temp file
		printf("about to decrypt file\n");
		
		int crypt = do_crypt(fp, temp, DECRYPT, XMP_DATA->password);
		//read that bullshit
		printf("Crypt is: %d\n",crypt);
		fflush(stdout);

		fseek(temp, 0, SEEK_END);
		size_t templen = ftell(temp);
		fseek(temp, 0, SEEK_SET);
		fprintf(stderr, "Read: size given by read: %zu\nsize of tmpFile: %zu\nsize of offset: %zu\n", size, templen, offset);
		/* Read the decrypted contents of original file to the application widow */
		res = fread(buf, 1, templen, temp);

		fflush(stdout);
		if (res == -1)
			res = -errno;
		
		//close our file pointers
		fclose(fp);
		fclose(temp);
		remove(temp_path);
	}
	//standard behavior
	else
	{
		fd = open(fpath, O_RDONLY);
		if (fd == -1)
			return -errno;

		res = pread(fd, buf, size, offset);
		if (res == -1)
			res = -errno;

		close(fd);
	}
	
	return res;
}


//writes to a file
static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset)
{

	FILE *fp, *temp;
	int fd;
	int res;

	char fpath[PATH_MAX];

	//our new path to work with 
	char temp_path[PATH_MAX];

	fixPath(fpath,path);

	//(void) fi;

	if(isenc(path))
	{
		/*file we want to actually read, this is encrypted!*/
		fp = fopen(fpath, "rb+");

		tmp_path(fpath, ".tmp",temp_path);
		temp = fopen(temp_path, "wb+");
		//write into temp file


		do_crypt(fp, temp, DECRYPT, XMP_DATA->password);
		fclose(fp);
		
		//travel to the offset we want to write to in the file located in memory stream
		fseek(temp, offset, SEEK_SET);
		//size_t templen = ftell(temp);

		res = fwrite(buf, sizeof(char), size, temp);
		if (res == -1)
			res = -errno;
		fflush(temp);
		//fclose(temp);
		fp = fopen(fpath, "wb+");
		fseek(temp, 0, SEEK_SET);
		
		//temp = fopen(temp_path, "w");
		if(!do_crypt(temp, fp, ENCRYPT, XMP_DATA->password))
		{
				fprintf(stderr, "do _crypt failed\n");
		}

		//close our file pointers
		fclose(fp);
		fclose(temp);
		remove(temp_path);
	}
	else
	{
		fd = open(fpath, O_WRONLY);
		if (fd == -1)
			return -errno;
		//set encrypted flag
		//get the cipher text passing in the buf
		//write the ciphertext to the file
		res = pwrite(fd, buf, size, offset);
		if (res == -1)
			res = -errno;
		close(fd);	
	}
	
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;

	char fpath[PATH_MAX];
	fixPath(fpath,path);

    int res;
    res = creat(fpath, mode);
    if(res == -1)
	return -errno;

    close(res);

    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

//Tutorial by Joseph J Pfeiffer at
//cs.nmsu.edu/~pfeiffer/fuse-tutorial
//USAGE: ./pa5-encfs <Key Phrase> <Mirror Directory> <Mount Point>
//sudo ./fusexmp password mirror mnt
int main(int argc, char *argv[])
{
	umask(0);
	//Make sure the proper number of arguments are passed in
	if(argc < 4){
		printf("%s\n", "ERROR: NOT ENOUGH ARGUMENTS." );
		return -errno;
	}

	/*I have to include these for the realpath function. If we put NULL as the second argument, it will allocate the memory and never deallocate it (possible bug?) 

	These two arrays are guarenteed to deallocate the memory when the program ends. Valgrind says there is no memory leaks
	*/

	char mnt[PATH_MAX + 1];
	char mir[PATH_MAX + 1];

	//initialize a struct to hold mirrored directory and password
	struct xmp_info *xmp_data;
	xmp_data = malloc(sizeof(struct xmp_info));
	//TODO catch for any errors calling malloc
	xmp_data->mountpoint = realpath(argv[3],mnt);
	//stores the path to the mirroed directory
	xmp_data->mirror = realpath(argv[2], mir);
	//Stores the password
	xmp_data->password = argv[1];

	//passwrod 

	printf("password = %s\n", xmp_data->password);
	printf("mirrored directory = %s\n", xmp_data->mirror);
	printf("mountpoint = %s\n", xmp_data->mountpoint);

	//fuse will now execute normally. It will get it's normal paramters passed in the correct order.
	argv[1] = argv[3]; // store the mountpoint as the first argument
	argv[2] = argv[4]; // moving the option flag to the second argument
	argc -= 2; //reduces the total number of passed aruments in.
	//call the fuse_main, passing in our data as well.
	fuse_main(argc, argv, &xmp_oper, xmp_data);
	free(xmp_data);
	return 0;
}