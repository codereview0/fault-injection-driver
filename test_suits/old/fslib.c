#include "fslib.h"

/*stream on which error messages are logged*/
FILE *out_stream = NULL;
char *fslib_logfile = NULL;
char *sys_logfile	= "/var/log/messages";
char *tmp_logfile	= "/tmp/messages";

/*to control the logging of messages*/
int logging = 1;

/*error code*/
extern int errno;

/*for vp_llseek*/
_syscall5(int, _llseek, unsigned int, fd,
	unsigned long, offset_high, unsigned long, offset_low,
	long long *, result, unsigned int, origin);

int vp_init_lib(char *logfile)
{
	/*initialize the output stream*/
	if (logfile) {
		fslib_logfile = logfile;
		if ((out_stream = fopen(logfile, "a+")) == NULL) {
			perror("log file open");
			out_stream = OUT_STREAM;
		}
	}
	else {
		out_stream = OUT_STREAM;
	}

	return 1;
}

int vp_init_syslog()
{
	char cmd[4096];

	sprintf(cmd, "cp -f %s %s", sys_logfile, tmp_logfile);
	if (system(cmd) < 0) {
		perror("system");
		return -1;
	}

	return 1;
}

int vp_mine_syslog()
{
	char cmd[4096];
	int ret;

	/*
	 * Doing 'sync' here flushes even the checkpoint blocks. Currently I'm
	 * stopping it.
	 */

#if 0
	if (fslib_logfile) {
		sprintf(cmd, "sync; diff %s %s >> %s", sys_logfile, tmp_logfile, fslib_logfile);
	}
	else {
		sprintf(cmd, "sync; diff %s %s", sys_logfile, tmp_logfile);
	}
#else
	if (fslib_logfile) {
		sprintf(cmd, "diff %s %s >> %s", sys_logfile, tmp_logfile, fslib_logfile);
	}
	else {
		sprintf(cmd, "diff %s %s", sys_logfile, tmp_logfile);
	}
#endif

	if ((ret = system(cmd)) < 0) {
		perror("system");
		return -1;
	}

	return 1;
}

int vp_log_error(char *err_msg, int err_code, int exit_on_err)
{
	if (err_msg) {
		fprintf(out_stream, "%s\n", err_msg);
	}

	if (err_code != NO_ERROR) {
		fprintf(out_stream, "Sys Error: %s\n", strerror(err_code));
		vp_print_err_code(err_code, err_msg);
		fprintf(out_stream, "Error code: %s\n", err_msg);
	}
	
	fflush(out_stream);

	if (exit_on_err) {
		vp_mine_syslog();
		fprintf(out_stream, "Exiting on error\n\n");
		//fprintf(out_stream, "%s\n", ERR_SEP);
		//fflush(out_stream);
		//exit(1);
		return -1;
	}

	return 1;
}

int vp_start_log()
{
	logging = 1;

	return 1;
}

int vp_dont_log()
{
	logging = 0;

	return 1;
}

int vp_diff_time(struct timeval st, struct timeval et)
{
	int diff;
	if (et.tv_usec < st.tv_usec) {
		et.tv_usec += 1000000;
		et.tv_sec --;
	}

	diff = (et.tv_sec - st.tv_sec)*1000000 + (et.tv_usec - st.tv_usec);
	return diff;
}

char *vp_get_test_name(int test)
{
	switch(test) {
		case CREATE_FILE:
			return "create_file";
		break;

		case READ_FILE:
			return "read_file";
		break;

		case WRITE_FILE:
			return "write_file";
		break;

		case APPEND_FILE:
			return "append_file";
		break;

		case CREATE_DIR:
			return "create_dir";
		break;

		case READ_DIR:
			return "read_dir";
		break;

		case WRITE_DIR:
			return "write_dir";
		break;

		case MOUNT_FS:
			return "mount_fs";
		break;

		case UNMOUNT_FS:
			return "unmount_fs";
		break;

		default:
			return "unknown_test_routine";
	}
}

int vp_read(int fd, void *buf, size_t count)
{
	int ret;

	if ((ret = read(fd, buf, count)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to read file (fd = %d buf = %x count = %d)", 
		fd, (int)buf, count);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret;
}

int vp_write(int fd, const void *buf, size_t count)
{
	int ret;

	if ((ret = write(fd, buf, count)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to write file (fd = %d buf = %x count = %d)", 
		fd, (int)buf, count);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	
	return ret;
}

int vp_open(const char *pathname, int flags)
{
	int ret;
	char err_msg[4096];

	if ((ret = open(pathname, flags)) < 0) {
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to open file (path = %s flags = %d)", 
		pathname, flags);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	return ret;
}

int vp_creat_open(const char *pathname, int flags, mode_t mode)
{
	int ret;

	if ((ret = open(pathname, flags, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to create-open file (path = %s flags = %d mode = %d)", 
		pathname, flags, mode);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	return ret;
}

int vp_creat(const char *pathname, mode_t mode)
{
	int ret;

	if ((ret = creat(pathname, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to create file (path = %s mode = %d)", 
		pathname, mode);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	return ret;
}

int vp_close(int fd)
{
	int ret;

	if ((ret = close(fd)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to close file (fd = %d)", fd);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	return ret;
}

int vp_mkdir(const char *pathname, mode_t mode)
{
	int ret;

	if ((ret = mkdir(pathname, mode)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to mkdir (pathname = %s)", pathname);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret;
}

DIR *vp_opendir(const char *name)
{
	DIR *ret;

	if ((ret = opendir(name)) == NULL) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to open dir (name = %s)", name);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret;
}

struct dirent *vp_readdir(DIR *dir)
{
	struct dirent *ret;

	if ((ret = readdir(dir)) == NULL) {
		char err_msg[4096];
		int err_code = errno;
		
		sprintf(err_msg, "Error: Unable to read dir - Could be due to end of file");
		vp_log_error(err_msg, err_code, !(EXIT_ON_ERROR));
	}

	return ret;
}

int vp_closedir(DIR *dir)
{
	int ret;

	if ((ret = closedir(dir)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to close dir (DIR = %x)", (int)dir);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret;
}

int vp_fsync(int fd)
{
	int ret;

	if ((ret = fsync(fd)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to fsync file (fd = %d)", fd);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}	

	return ret;
}

int vp_lseek(int fildes, off_t offset, int whence)
{
	int ret;

	if ((ret = lseek(fildes, offset, whence)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to lseek file (fd = %d offset = %ld whence = %d)", 
		fildes, offset, whence);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret;
}

long long vp_llseek(unsigned int fd, unsigned long long offset, unsigned int origin)
{
	long long result;
	int ret;

	if ((ret = _llseek (fd, offset >> 32, offset & 0xffffffff,
					&result, origin)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to LLseek file (fd = %d offset = %lld whence = %d)", 
		fd, offset, origin);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret;
}


int vp_fstat(int filedes, struct stat *buf)
{
	int ret;

	if ((ret = fstat(filedes, buf)) < 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to fstat file (fd = %d statbuf = %x)", 
		filedes, (int)buf);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret;
}

int vp_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *rq)
{
	if (pthread_create(thread, attr, start_routine, rq) != 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to create thread (thread %x attr %x routine %x rq %x)", 
		(int)thread, (int)attr, (int)start_routine, (int)rq);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
		return -1;
	}

	return 1;
}

int vp_pthread_join(pthread_t th, void **thread_return)
{
	int ret_val;

	if ((ret_val = pthread_join(th, thread_return)) != 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to join thread (th %d return %x)", 
		(int)th, (int)thread_return);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret_val;
}

int vp_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data)
{
	int ret_val;

	if ((ret_val = mount(source, target, filesystemtype, mountflags, data)) != 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to mount %s in %s as %s", 
		source, target, filesystemtype);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}


	return ret_val;
}

int vp_umount(const char *target)
{
	int ret_val;

	if ((ret_val = umount(target)) != 0) {
		char err_msg[4096];
		int err_code = errno;

		sprintf(err_msg, "Error: Unable to unmount %s", target);
		vp_log_error(err_msg, err_code, EXIT_ON_ERROR);
	}

	return ret_val;
}

int vp_print_err_code(int err_code, char *err_msg)
{
	switch(err_code) {
		case EPERM: sprintf(err_msg, "%s", str_EPERM); break;
		case ENOENT: sprintf(err_msg, "%s", str_ENOENT); break;
		case ESRCH: sprintf(err_msg, "%s", str_ESRCH); break;
		case EINTR: sprintf(err_msg, "%s", str_EINTR); break;
		case EIO: sprintf(err_msg, "%s", str_EIO); break;
		case ENXIO: sprintf(err_msg, "%s", str_ENXIO); break;
		case E2BIG: sprintf(err_msg, "%s", str_E2BIG); break;
		case ENOEXEC: sprintf(err_msg, "%s", str_ENOEXEC); break;
		case EBADF: sprintf(err_msg, "%s", str_EBADF); break;
		case ECHILD: sprintf(err_msg, "%s", str_ECHILD); break;
		case EAGAIN: sprintf(err_msg, "%s", str_EAGAIN); break;
		case ENOMEM: sprintf(err_msg, "%s", str_ENOMEM); break;
		case EACCES: sprintf(err_msg, "%s", str_EACCES); break;
		case EFAULT: sprintf(err_msg, "%s", str_EFAULT); break;
		case ENOTBLK: sprintf(err_msg, "%s", str_ENOTBLK); break;
		case EBUSY: sprintf(err_msg, "%s", str_EBUSY); break;
		case EEXIST: sprintf(err_msg, "%s", str_EEXIST); break;
		case EXDEV: sprintf(err_msg, "%s", str_EXDEV); break;
		case ENODEV: sprintf(err_msg, "%s", str_ENODEV); break;
		case ENOTDIR: sprintf(err_msg, "%s", str_ENOTDIR); break;
		case EISDIR: sprintf(err_msg, "%s", str_EISDIR); break;
		case EINVAL: sprintf(err_msg, "%s", str_EINVAL); break;
		case ENFILE: sprintf(err_msg, "%s", str_ENFILE); break;
		case EMFILE: sprintf(err_msg, "%s", str_EMFILE); break;
		case ENOTTY: sprintf(err_msg, "%s", str_ENOTTY); break;
		case ETXTBSY: sprintf(err_msg, "%s", str_ETXTBSY); break;
		case EFBIG: sprintf(err_msg, "%s", str_EFBIG); break;
		case ENOSPC: sprintf(err_msg, "%s", str_ENOSPC); break;
		case ESPIPE: sprintf(err_msg, "%s", str_ESPIPE); break;
		case EROFS: sprintf(err_msg, "%s", str_EROFS); break;
		case EMLINK: sprintf(err_msg, "%s", str_EMLINK); break;
		case EPIPE: sprintf(err_msg, "%s", str_EPIPE); break;
		case EDOM: sprintf(err_msg, "%s", str_EDOM); break;
		case ERANGE: sprintf(err_msg, "%s", str_ERANGE); break;
		case EDEADLK: sprintf(err_msg, "%s", str_EDEADLK); break;
		case ENAMETOOLONG: sprintf(err_msg, "%s", str_ENAMETOOLONG); break;
		case ENOLCK: sprintf(err_msg, "%s", str_ENOLCK); break;
		case ENOSYS: sprintf(err_msg, "%s", str_ENOSYS); break;
		case ENOTEMPTY: sprintf(err_msg, "%s", str_ENOTEMPTY); break;
		case ELOOP: sprintf(err_msg, "%s", str_ELOOP); break;
		case ENOMSG: sprintf(err_msg, "%s", str_ENOMSG); break;
		case EIDRM: sprintf(err_msg, "%s", str_EIDRM); break;
		case ECHRNG: sprintf(err_msg, "%s", str_ECHRNG); break;
		case EL2NSYNC: sprintf(err_msg, "%s", str_EL2NSYNC); break;
		case EL3HLT: sprintf(err_msg, "%s", str_EL3HLT); break;
		case EL3RST: sprintf(err_msg, "%s", str_EL3RST); break;
		case ELNRNG: sprintf(err_msg, "%s", str_ELNRNG); break;
		case EUNATCH: sprintf(err_msg, "%s", str_EUNATCH); break;
		case ENOCSI: sprintf(err_msg, "%s", str_ENOCSI); break;
		case EL2HLT: sprintf(err_msg, "%s", str_EL2HLT); break;
		case EBADE: sprintf(err_msg, "%s", str_EBADE); break;
		case EBADR: sprintf(err_msg, "%s", str_EBADR); break;
		case EXFULL: sprintf(err_msg, "%s", str_EXFULL); break;
		case ENOANO: sprintf(err_msg, "%s", str_ENOANO); break;
		case EBADRQC: sprintf(err_msg, "%s", str_EBADRQC); break;
		case EBADSLT: sprintf(err_msg, "%s", str_EBADSLT); break;
		case EBFONT: sprintf(err_msg, "%s", str_EBFONT); break;
		case ENOSTR: sprintf(err_msg, "%s", str_ENOSTR); break;
		case ENODATA: sprintf(err_msg, "%s", str_ENODATA); break;
		case ETIME: sprintf(err_msg, "%s", str_ETIME); break;
		case ENOSR: sprintf(err_msg, "%s", str_ENOSR); break;
		case ENONET: sprintf(err_msg, "%s", str_ENONET); break;
		case ENOPKG: sprintf(err_msg, "%s", str_ENOPKG); break;
		case EREMOTE: sprintf(err_msg, "%s", str_EREMOTE); break;
		case ENOLINK: sprintf(err_msg, "%s", str_ENOLINK); break;
		case EADV: sprintf(err_msg, "%s", str_EADV); break;
		case ESRMNT: sprintf(err_msg, "%s", str_ESRMNT); break;
		case ECOMM: sprintf(err_msg, "%s", str_ECOMM); break;
		case EPROTO: sprintf(err_msg, "%s", str_EPROTO); break;
		case EMULTIHOP: sprintf(err_msg, "%s", str_EMULTIHOP); break;
		case EDOTDOT: sprintf(err_msg, "%s", str_EDOTDOT); break;
		case EBADMSG: sprintf(err_msg, "%s", str_EBADMSG); break;
		case EOVERFLOW: sprintf(err_msg, "%s", str_EOVERFLOW); break;
		case ENOTUNIQ: sprintf(err_msg, "%s", str_ENOTUNIQ); break;
		case EBADFD: sprintf(err_msg, "%s", str_EBADFD); break;
		case EREMCHG: sprintf(err_msg, "%s", str_EREMCHG); break;
		case ELIBACC: sprintf(err_msg, "%s", str_ELIBACC); break;
		case ELIBBAD: sprintf(err_msg, "%s", str_ELIBBAD); break;
		case ELIBSCN: sprintf(err_msg, "%s", str_ELIBSCN); break;
		case ELIBMAX: sprintf(err_msg, "%s", str_ELIBMAX); break;
		case ELIBEXEC: sprintf(err_msg, "%s", str_ELIBEXEC); break;
		case EILSEQ: sprintf(err_msg, "%s", str_EILSEQ); break;
		case ERESTART: sprintf(err_msg, "%s", str_ERESTART); break;
		case ESTRPIPE: sprintf(err_msg, "%s", str_ESTRPIPE); break;
		case EUSERS: sprintf(err_msg, "%s", str_EUSERS); break;
		case ENOTSOCK: sprintf(err_msg, "%s", str_ENOTSOCK); break;
		case EDESTADDRREQ: sprintf(err_msg, "%s", str_EDESTADDRREQ); break;
		case EMSGSIZE: sprintf(err_msg, "%s", str_EMSGSIZE); break;
		case EPROTOTYPE: sprintf(err_msg, "%s", str_EPROTOTYPE); break;
		case ENOPROTOOPT: sprintf(err_msg, "%s", str_ENOPROTOOPT); break;
		case EPROTONOSUPPORT: sprintf(err_msg, "%s", str_EPROTONOSUPPORT); break;
		case ESOCKTNOSUPPORT: sprintf(err_msg, "%s", str_ESOCKTNOSUPPORT); break;
		case EOPNOTSUPP: sprintf(err_msg, "%s", str_EOPNOTSUPP); break;
		case EPFNOSUPPORT: sprintf(err_msg, "%s", str_EPFNOSUPPORT); break;
		case EAFNOSUPPORT: sprintf(err_msg, "%s", str_EAFNOSUPPORT); break;
		case EADDRINUSE: sprintf(err_msg, "%s", str_EADDRINUSE); break;
		case EADDRNOTAVAIL: sprintf(err_msg, "%s", str_EADDRNOTAVAIL); break;
		case ENETDOWN: sprintf(err_msg, "%s", str_ENETDOWN); break;
		case ENETUNREACH: sprintf(err_msg, "%s", str_ENETUNREACH); break;
		case ENETRESET: sprintf(err_msg, "%s", str_ENETRESET); break;
		case ECONNABORTED: sprintf(err_msg, "%s", str_ECONNABORTED); break;
		case ECONNRESET: sprintf(err_msg, "%s", str_ECONNRESET); break;
		case ENOBUFS: sprintf(err_msg, "%s", str_ENOBUFS); break;
		case EISCONN: sprintf(err_msg, "%s", str_EISCONN); break;
		case ENOTCONN: sprintf(err_msg, "%s", str_ENOTCONN); break;
		case ESHUTDOWN: sprintf(err_msg, "%s", str_ESHUTDOWN); break;
		case ETOOMANYREFS: sprintf(err_msg, "%s", str_ETOOMANYREFS); break;
		case ETIMEDOUT: sprintf(err_msg, "%s", str_ETIMEDOUT); break;
		case ECONNREFUSED: sprintf(err_msg, "%s", str_ECONNREFUSED); break;
		case EHOSTDOWN: sprintf(err_msg, "%s", str_EHOSTDOWN); break;
		case EHOSTUNREACH: sprintf(err_msg, "%s", str_EHOSTUNREACH); break;
		case EALREADY: sprintf(err_msg, "%s", str_EALREADY); break;
		case EINPROGRESS: sprintf(err_msg, "%s", str_EINPROGRESS); break;
		case ESTALE: sprintf(err_msg, "%s", str_ESTALE); break;
		case EUCLEAN: sprintf(err_msg, "%s", str_EUCLEAN); break;
		case ENOTNAM: sprintf(err_msg, "%s", str_ENOTNAM); break;
		case ENAVAIL: sprintf(err_msg, "%s", str_ENAVAIL); break;
		case EISNAM: sprintf(err_msg, "%s", str_EISNAM); break;
		case EREMOTEIO: sprintf(err_msg, "%s", str_EREMOTEIO); break;
		case EDQUOT: sprintf(err_msg, "%s", str_EDQUOT); break;
		case ENOMEDIUM: sprintf(err_msg, "%s", str_ENOMEDIUM); break;
		case EMEDIUMTYPE: sprintf(err_msg, "%s", str_EMEDIUMTYPE); break;
		default: sprintf(err_msg, "Unknown error code %d", err_code); break;
	}

	return 1;
}

int log_start_end_msgs(int start, char *routine)
{
	char err_msg[4096];

	if (start) {
		vp_init_syslog();
		sprintf(err_msg, "%s start", routine);
		vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	}
	else {
		vp_mine_syslog();
		sprintf(err_msg, "%s over", routine);
		vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
		//sprintf(err_msg, "%s", ERR_SEP);
		//vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	}

	return 1;
}

int build_block(char *buffer, int index)
{
	memset(buffer, ' ', 4096);
	sprintf(buffer, "%d %s", index, QUOTE);
	return 1;
}

/*This compares the block read from a file to its original 
 *contents and logs any errors*/
int verify_block(char *buffer, int index)
{
	char dummy[4096];

	memset(dummy, ' ', 4096);
	sprintf(dummy, "%d %s", index, QUOTE);
	if (strcmp(buffer, dummy) == 0) {
		return 1;
	}
	else {
		char err_msg[8192];
		sprintf(err_msg, "App_Error: Block %d does not match with original contents", index);
		vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
		#if 1
		sprintf(err_msg, "App_Error: original contents: [%s]", dummy);
		vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
		sprintf(err_msg, "App_Error: new contents: [%s]", buffer);
		vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
		#endif
	}

	return 0;
}

/*This compares the dir contents read from the dir*/
/*FIXME:*/
int verify_dir_contents(struct dirent *dent, int index)
{
	char fname[4096];

	switch (index) {
		case -2:
			sprintf(fname, ".");
		break;

		case -1:
			sprintf(fname, "..");
		break;

		default:
			sprintf(fname, "file%d", index);
	}

	if (strcmp(dent->d_name, fname) == 0) {
		return 1;
	}
	else {
		char err_msg[4096];
		sprintf(err_msg, "App_Error: Dir entry %d (%s) does not match with original contents (%s)", 
		index, dent->d_name, fname);
		vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	}

	return 0;
}

/* 
 * mount_fs parameters: <source> <target> <filesystemtype>
 */
int mount_fs(int argc, char *argv[])
{
	char *source = argv[1];
	char *target = argv[2];
	char *fstype = argv[3];
	int mylogging = logging;
	
	if (mylogging) {
		log_start_end_msgs(1, "mount_fs");
	}

	vp_mount(source, target, fstype, 0, NULL);

	if (mylogging) {
		log_start_end_msgs(0, "mount_fs");
	}

	return 1;
}

/* 
 * unmount_fs parameters: <target>
 */
int unmount_fs(int argc, char *argv[])
{
	char *target = argv[1];
	int mylogging = logging;
	
	if (mylogging) {
		log_start_end_msgs(1, "unmount_fs");
	}

	vp_umount(target);

	if (mylogging) {
		log_start_end_msgs(0, "unmount_fs");
	}

	return 1;
}

/* 
 * create_dir parameters: <dir name> <no of subfiles>
 */
int create_dir(int argc, char *argv[])
{
	int i;
	int mode = 0777;
	char *dname = argv[1];
	int no_subfiles = atoi(argv[2]);
	int mylogging = logging;
	
	if (mylogging) {
		log_start_end_msgs(1, "create_dir");
	}

	vp_mkdir(dname, mode);

	/*create the sub files*/
	for (i = 0; i < no_subfiles; i ++) {
		char *create_file_argv[3];

		create_file_argv[0] = (char *)malloc(strlen("create_file")+1);
		strcpy(create_file_argv[0], "create_file");
		create_file_argv[1] = (char *)malloc(4096);
		sprintf(create_file_argv[1], "%s/file%d", dname, i);
		create_file_argv[2] = (char *)malloc(8);
		strcpy(create_file_argv[2], "0");

		logging = 0;	
		create_file(3, create_file_argv);
		logging = mylogging;	
	}

	if (mylogging) {
		log_start_end_msgs(0, "create_dir");
	}

	return 1;
}

/*
 * read_dir parameters: <dir name>
 */
int read_dir(int argc, char *argv[])
{
	DIR *dir;
	struct dirent *dent;
	int i = -2; // -2 and -1 are for the current dir and its parent
	char *dname = argv[1];
	
	if (logging) {
		log_start_end_msgs(1, "read_dir");
	}

	dir = vp_opendir(dname);

	while ((dent = vp_readdir(dir)) != NULL) {
		verify_dir_contents(dent, i);
		i ++;
	}
	
	if (logging) {
		log_start_end_msgs(0, "read_dir");
	}

	return 1;
}

/*
 * write_dir parameters: <dir name> <no of subdirs> <no of subfiles>
 * no of subdir gives the number of sub directories under which the 
 * files will be created.
 */
int write_dir(int argc, char *argv[])
{
	DIR *dir;
	int i;
	char *dname = argv[1];
	int no_subdirs = atoi(argv[2]);
	int no_subfiles = atoi(argv[3]);
	//int dirfd;
	int mylogging = logging;
	
	if (mylogging) {
		log_start_end_msgs(1, "write_dir");
	}

	dir = vp_opendir(dname);
	//dirfd = dirfd(dir);

	if (no_subdirs > 0) {
		int files_in_each_dir = no_subfiles/no_subdirs;

		/*create at least one file in each subdir*/
		if (files_in_each_dir == 0) {
			if (no_subfiles != 0) {
				files_in_each_dir = 1;
			}
		}
		
		/*first create all the sub dirs*/
		for (i = 0; i < no_subdirs; i ++) {
			char *create_dir_argv[3];

			create_dir_argv[0] = (char *)malloc(strlen("create_dir")+1);
			strcpy(create_dir_argv[0], "create_dir");
			create_dir_argv[1] = (char *)malloc(4096);
			sprintf(create_dir_argv[1], "%s/dir%d", dname, i);
			create_dir_argv[2] = (char *)malloc(8);
			strcpy(create_dir_argv[2], "0");
			sprintf(create_dir_argv[2], "%d", files_in_each_dir);

			/*we have created the some files - subtract them from the total*/
			no_subfiles -= files_in_each_dir;
			
			/*if we have created all the subfiles, create no more*/
			if (no_subfiles <= 0) {
				files_in_each_dir = 0;
			}

			logging = 0;
			create_dir(3, create_dir_argv);
			logging = mylogging;	
		}
	}
	else {
		/*create the sub files*/
		for (i = 0; i < no_subfiles; i ++) {
			char *create_file_argv[3];

			create_file_argv[0] = (char *)malloc(strlen("create_file")+1);
			strcpy(create_file_argv[0], "create_file");
			create_file_argv[1] = (char *)malloc(4096);
			sprintf(create_file_argv[1], "%s/file%d", dname, i);
			create_file_argv[2] = (char *)malloc(8);
			strcpy(create_file_argv[2], "0");

			logging = 0;
			create_file(3, create_file_argv);
			logging = mylogging;	
		}
	}

	vp_closedir(dir);
	//printf("Calling fsync on dir contents ...\n");
	//vp_fsync(dirfd);
	//vp_close(dirfd);
	
	if (mylogging) {
		log_start_end_msgs(0, "write_dir");
	}
	
	return 1;
}

/* 
 * create_file_async parameters: <file name> <size(KB)>
 */
int create_file_async(int argc, char *argv[])
{
	char buffer[4096];
	int fd;
	int i;
	char *fname = argv[1];
	char logmsg[512];
	int size = atoi(argv[2])*1024;  //in bytes

	sprintf(logmsg, "create_file_async: %s", fname);

	if (logging) {
		log_start_end_msgs(1, logmsg);
	}

	fd = vp_creat_open(fname, O_CREAT|O_RDWR, S_IRWXU);

	memset(buffer, 1, 4096);

	/*write the whole file*/
	for (i = 0; i < size/MY_BLOCK_SIZE; i ++) {
		build_block(buffer, i);
		vp_write(fd, buffer, MY_BLOCK_SIZE);
	}
	
	if (size%MY_BLOCK_SIZE > 0) {
		/*write the last bytes that didn't make to full block*/
		//build_block(buffer, size/MY_BLOCK_SIZE + 1);
		vp_write(fd, buffer, size%MY_BLOCK_SIZE);
	}

	vp_close(fd);
	
	if (logging) {
		log_start_end_msgs(0, logmsg);
	}

	return 1;
}

/* 
 * create_file parameters: <file name> <size(KB)>
 */
int create_file(int argc, char *argv[])
{
	char buffer[4096];
	int fd;
	int i;
	char *fname = argv[1];
	char logmsg[512];
	int size = atoi(argv[2])*1024;  //in bytes

	sprintf(logmsg, "create_file: %s", fname);

	if (logging) {
		log_start_end_msgs(1, logmsg);
	}

	fd = vp_creat_open(fname, O_CREAT|O_RDWR, S_IRWXU);

	memset(buffer, 1, 4096);

	/*write the whole file*/
	for (i = 0; i < size/MY_BLOCK_SIZE; i ++) {
		build_block(buffer, i);
		vp_write(fd, buffer, MY_BLOCK_SIZE);
	}
	
	if (size%MY_BLOCK_SIZE > 0) {
		/*write the last bytes that didn't make to full block*/
		//build_block(buffer, size/MY_BLOCK_SIZE + 1);
		vp_write(fd, buffer, size%MY_BLOCK_SIZE);
	}

	//printf("Calling fsync on file %s ...\n", fname);
	vp_fsync(fd);
	vp_close(fd);
	
	if (logging) {
		log_start_end_msgs(0, logmsg);
	}

	return 1;
}

/* 
 * read_file parameters: <file name> [<block#>, <block#>, ...]
 *
 * by default, it reads the whole file. but you can specify any
 * particular blocks to read
 */
int read_file(int argc, char *argv[])
{
	char buffer[4096];
	int fd;
	int i;
	char *fname = argv[1];
	int total_blocks = argc - 2; //all blocks except the prog name and file name

	if (logging) {
		log_start_end_msgs(1, "read_file");
	}
	
	fd = vp_open(fname, O_RDONLY);

	if (total_blocks > 0) {
		/*read specific blocks*/
		for (i = 0; i < total_blocks; i ++) {
			vp_lseek(fd, atoi(argv[2+i])*MY_BLOCK_SIZE, SEEK_SET);
			vp_read(fd, buffer, MY_BLOCK_SIZE);
			verify_block(buffer, atoi(argv[2+i]));
		}
	}
	else {
		/*read the whole file*/
		struct stat buf;
		
		vp_fstat(fd, &buf);
		
		//printf("here i'am ...\n");
		for (i = 0; i < buf.st_size/MY_BLOCK_SIZE; i ++) {
			vp_read(fd, buffer, MY_BLOCK_SIZE);
			verify_block(buffer, i);
			//printf("verified block %d...\n", i);
		}
		
		if (buf.st_size%MY_BLOCK_SIZE > 0) {
			/*read the last bytes that didn't make to full block*/
			vp_read(fd, buffer, buf.st_size%MY_BLOCK_SIZE);
			verify_block(buffer, buf.st_size/MY_BLOCK_SIZE + 1);
		}
	}

	vp_close(fd);

	
	if (logging) {
		log_start_end_msgs(0, "read_file");
	}
	
	return 1;
}

/* 
 * append_file parameters: <file name> <append_size(KB)>
 */
int append_file(int argc, char *argv[])
{
	char buffer[4096];
	int fd;
	int i;
	char *fname = argv[1];
	int append_size = atoi(argv[2])*1024; 
	struct stat buf;
	
	if (logging) {
		log_start_end_msgs(1, "append_file");
	}
	
	fd = vp_open(fname, O_WRONLY);

	/*go to the end of the file*/
	vp_lseek(fd, 0, SEEK_END);

	vp_fstat(fd, &buf);
	if (buf.st_size % 4096 != 0) {
		fprintf(stderr, "File size not a multiple of 4KB ... change this. Exiting ...\n");
		exit(-1);
	}
	
	for (i = buf.st_size/MY_BLOCK_SIZE; i < (buf.st_size + append_size)/MY_BLOCK_SIZE; i ++) {
		build_block(buffer, i);
		vp_write(fd, buffer, MY_BLOCK_SIZE);
	}

	vp_fsync(fd);
	
	vp_close(fd);
	
	if (logging) {
		log_start_end_msgs(0, "append_file");
	}
	
	return 1;
}

/* 
 * write_file parameters: <file name> [<block#>, <block#>, ...]
 *
 * by default, it writes to the whole file. but you can specify any
 * particular blocks to write
 */
int write_file(int argc, char *argv[])
{
	char buffer[4096];
	int fd;
	int i;
	char *fname = argv[1];
	int total_blocks = argc - 2; //all blocks except the prog name and file name
	
	if (logging) {
		log_start_end_msgs(1, "write_file");
	}
	
	fd = vp_open(fname, O_WRONLY);

	if (total_blocks > 0) {
		/*write specific blocks*/
		for (i = 0; i < total_blocks; i ++) {
			printf("Writing to block %d\n", atoi(argv[2+i]));
			vp_llseek(fd, atoi(argv[2+i])*MY_BLOCK_SIZE, SEEK_SET);
			build_block(buffer, atoi(argv[2+i]));
			vp_write(fd, buffer, MY_BLOCK_SIZE);
		}
	}
	else {
		/*write the whole file*/
		struct stat buf;
		
		vp_fstat(fd, &buf);
		
		for (i = 0; i < buf.st_size/MY_BLOCK_SIZE; i ++) {
			build_block(buffer, i);
			vp_write(fd, buffer, MY_BLOCK_SIZE);
		}
		
		if (buf.st_size%MY_BLOCK_SIZE > 0) {
			/*write the last bytes that didn't make to full block*/
			build_block(buffer, buf.st_size/MY_BLOCK_SIZE + 1);
			vp_write(fd, buffer, buf.st_size%MY_BLOCK_SIZE);
		}
	}

	//printf("Write to file over ...\n");
	vp_fsync(fd);
	vp_close(fd);
	
	if (logging) {
		log_start_end_msgs(0, "write_file");
	}
	
	return 1;
}


