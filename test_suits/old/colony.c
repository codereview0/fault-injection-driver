#include "colony.h"

char *logfile;

int print_args(int argc, char *argv[])
{
	int i;

	for (i = 0; i < argc; i ++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
	
	return 1;
}

/*
 * Usage: colony <function name> <other args> 
 */
int main(int argc, char *argv[])
{
	char **my_argv;
	
	if (argc < 3) {
		fprintf(stderr, "Usage: colony <log file> <function_name> <function_args>\n");
		fprintf(stderr, "     - mount_fs <source> <target> <filesystemtype>\n");
		fprintf(stderr, "     - unmount_fs <target>\n");
		fprintf(stderr, "     - create_dir <dirname> <#subfiles>\n");
		fprintf(stderr, "     - read_dir <dirname>\n");
		fprintf(stderr, "     - write_dir <dirname> <#subdirs> <#subfiles>\n");
		fprintf(stderr, "     - create_file_async <filename> <size(KB)>\n");
		fprintf(stderr, "     - create_file <filename> <size(KB)>\n");
		fprintf(stderr, "     - read_file <filename> [<block#>, <block#>, ...]\n");
		fprintf(stderr, "     - write_file <filename> [<block#>, <block#>, ...]\n");
		fprintf(stderr, "     - append_file <filename> <append_size(KB)>\n");
		return -1;
	}
	
	if (strcmp(argv[1], "NULL") == 0) {
		logfile = NULL;
	}
	else {
		logfile = argv[1];
	}
	vp_init_lib(logfile);

	my_argv = &argv[2];
	if (strcmp(argv[2], "mount_fs") == 0) {
		mount_fs(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "unmount_fs") == 0) {
		unmount_fs(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "create_dir") == 0) {
		create_dir(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "read_dir") == 0) {
		read_dir(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "write_dir") == 0) {
		write_dir(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "create_file") == 0) {
		create_file(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "create_file_async") == 0) {
		create_file_async(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "read_file") == 0) {
		read_file(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "write_file") == 0) {
		write_file(argc-2, my_argv);
	}
	else
	if (strcmp(argv[2], "append_file") == 0) {
		append_file(argc-2, my_argv);
	}
	else {
		fprintf(stderr, "Invalid test routine\n");
	}

	return 1;
}
