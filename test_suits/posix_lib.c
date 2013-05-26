/*NOTE: if you are planning to add more calls, change the #def MAX_WORKLOAD*/

#include "posix_lib.h"

int pos_access(const char *pathname, int mode)
{
	int ret;

	vp_log_error("access: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = access(pathname, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to access file (path = %s)", pathname);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("access: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_chdir(const char *path)
{
	int ret;

	vp_log_error("chdir: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = chdir(path)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to chdir (path = %s)", path);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("chdir: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_chmod(const char *path, mode_t mode)
{
	int ret;

	vp_log_error("chmod: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = chmod(path, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to chmod (path = %s)", path);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("chmod: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_chown(const char *path, uid_t owner, gid_t group)
{
	int ret;

	vp_log_error("chown: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = chown(path, owner, group)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to chown (path = %s)", path);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("chown: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_chroot(const char *path)
{
	int ret;

	vp_log_error("chroot: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = chroot(path)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to chroot (path = %s)", path);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("chroot: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_creat(const char *pathname, mode_t mode)
{
	int ret;

	vp_log_error("creat: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = creat(pathname, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to create file (path = %s mode = %d)", 
		pathname, mode);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("creat: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_stat(const char *file_name, struct stat *buf)
{
	int ret;

	vp_log_error("stat: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = stat(file_name, buf)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to stat file (file_name = %s)", file_name);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("stat: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_statfs(const char *path, struct statfs *buf)
{
	int ret;

	vp_log_error("statfs: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = statfs(path, buf)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to statfs (path = %s)", path);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("statfs: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_fsync(int fd)
{
	int ret;

	vp_log_error("fsync: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = fsync(fd)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to fsync file (fd = %d)", fd);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("fsync: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_truncate(const char *path, off_t length)
{
	int ret;

	vp_log_error("truncate: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = truncate(path, length)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to truncate (path = %s)", path);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("truncate: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

ssize_t pos_getdirentries(int fd, char *buf, size_t  nbytes, off_t *basep)
{
	int ret;

	vp_log_error("getdirentries: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = getdirentries(fd, buf, nbytes, basep)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to getdirentries (fd = %d)", fd);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("getdirentries: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_link(const char *oldpath, const char *newpath)
{
	int ret;

	vp_log_error("link: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = link(oldpath, newpath)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to create link (oldpath = %s newpath = %s)", oldpath, newpath);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("link: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_lstat(const char *file_name, struct stat *buf)
{
	int ret;

	vp_log_error("lstat: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = lstat(file_name, buf)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to lstat (file_name = %s)", file_name);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("lstat: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_mkdir(const char *pathname, mode_t mode)
{
	int ret;

	vp_log_error("mkdir: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = mkdir(pathname, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to mkdir (pathname = %s)", pathname);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("mkdir: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data)
{
	int ret_val;

	vp_log_error("mount: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret_val = mount(source, target, filesystemtype, mountflags, data)) != 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to mount %s in %s as %s", 
		source, target, filesystemtype);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("mount: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret_val;
}

int pos_creat_open(const char *pathname, int flags, mode_t mode)
{
	int ret;

	vp_log_error("creat-open: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = open(pathname, flags, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to create-open file (path = %s flags = %d mode = %d)", 
		pathname, flags, mode);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("creat-open: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_open(const char *pathname, int flags)
{
	int ret;
	char err_msg[4096];

	vp_log_error("open: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = open(pathname, flags)) < 0) {
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to open file (path = %s flags = %d)", 
		pathname, flags);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("open: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_read(int fd, void *buf, size_t count)
{
	int ret;

	vp_log_error("read: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = read(fd, buf, count)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to read file (fd = %d buf = %x count = %d)", 
		fd, (int)buf, count);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("read: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_readlink(const char *path, char *buf, size_t bufsiz)
{
	int ret;

	vp_log_error("readlink: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = readlink(path, buf, bufsiz)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to readlink (path = %s)", path);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("readlink: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_rename(const char *oldpath, const char *newpath)
{
	int ret;

	vp_log_error("rename: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = rename(oldpath, newpath)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to rename (oldpath = %s newpath = %s)", oldpath, newpath);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("rename: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_rmdir(const char *pathname)
{
	int ret;

	vp_log_error("rmdir: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = rmdir(pathname)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to rmdir (pathname = %s)", pathname);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("rmdir: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_symlink(const char *oldpath, const char *newpath)
{
	int ret;

	vp_log_error("symlink: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = symlink(oldpath, newpath)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to create symlink (oldpath = %s newpath = %s)", oldpath, newpath);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("symlink: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

void pos_sync(void)
{
	vp_log_error("sync: start", NO_ERROR, !(EXIT_ON_ERROR));

	sync();

	vp_log_error("sync: end", NO_ERROR, !(EXIT_ON_ERROR));
}

mode_t pos_umask(mode_t mask)
{
	int ret;

	vp_log_error("umask: start", NO_ERROR, !(EXIT_ON_ERROR));

	ret = umask(mask);

	vp_log_error("umask: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_unlink(const char *pathname)
{
	int ret;

	vp_log_error("unlink: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = unlink(pathname)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to unlink (pathname = %s)", pathname);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("unlink: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_umount(const char *target)
{
	int ret_val;

	vp_log_error("umount: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret_val = umount(target)) != 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to unmount %s", target);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("umount: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret_val;
}

int pos_utimes(char *filename, struct timeval *tvp)
{
	int ret;

	vp_log_error("utimes: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = utimes(filename, tvp)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to utimes (filename = %s)", filename);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	vp_log_error("utimes: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_write(int fd, const void *buf, size_t count)
{
	int ret;

	vp_log_error("write: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = write(fd, buf, count)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to write file (fd = %d buf = %x count = %d)", 
		fd, (int)buf, count);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("write: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

int pos_close(int fd)
{
	int ret;

	vp_log_error("close: start", NO_ERROR, !(EXIT_ON_ERROR));

	if ((ret = close(fd)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to close file (fd = %d)", fd);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	vp_log_error("close: end", NO_ERROR, !(EXIT_ON_ERROR));

	return ret;
}

/*NOTE: if you are planning to add more calls, change the #def MAX_WORKLOAD*/
