#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "sba_common_defs.h"

#define DEV		"/dev/SBA"

int main(int argc, char *argv[])
{
	int fd;

	if (argc < 2) {
		printf("Usage: sba <start|stop|print_stat|zero_stat|remove_fault|print_fault|test_system|dont_test|move_2_start|squash_writes|allow_writes|print_jblocks|clean_stats|clean_all_stats|extract_stats|crash_commit|dont_crash_commit|workload_start|workload_end>\n");
		return -1;
	}

	fd = open(DEV, O_RDONLY);

	if (strcmp(argv[1], "start") == 0) {
		fprintf(stderr, "Starting sba ...\n");
		ioctl(fd, START_SBA);
	}
	else 
	if (strcmp(argv[1], "stop") == 0) {
		fprintf(stderr, "Stoping sba ...\n");
		ioctl(fd, STOP_SBA);
	}
	else
	if (strcmp(argv[1], "print_stat") == 0) {
		fprintf(stderr, "printing the statistics ...\n");
		ioctl(fd, PRINT_STAT);
	}
	else
	if (strcmp(argv[1], "zero_stat") == 0) {
		fprintf(stderr, "zeroing the statistics ...\n");
		ioctl(fd, ZERO_STAT);
	}
	else
	if (strcmp(argv[1], "remove_fault") == 0) {
		fprintf(stderr, "removing the fault ...\n");
		ioctl(fd, REMOVE_FAULT);
	}
	else
	if (strcmp(argv[1], "print_fault") == 0) {
		fprintf(stderr, "printing the fault ...\n");
		ioctl(fd, PRINT_FAULT);
	}
	else
	if (strcmp(argv[1], "test_system") == 0) {
		fprintf(stderr, "setting sba to test the system ...\n");
		ioctl(fd, TEST_SYSTEM);
	}
	else
	if (strcmp(argv[1], "dont_test") == 0) {
		fprintf(stderr, "setting sba not to test the system ...\n");
		ioctl(fd, DONT_TEST);
	}
	else
	if (strcmp(argv[1], "move_2_start") == 0) {
		fprintf(stderr, "moving sba to start state ...\n");
		ioctl(fd, MOVE_2_START);
	}
	else
	if (strcmp(argv[1], "squash_writes") == 0) {
		fprintf(stderr, "squashing the writes ...\n");
		ioctl(fd, SQUASH_WRITES);
	}
	else
	if (strcmp(argv[1], "allow_writes") == 0) {
		fprintf(stderr, "allowing the writes ...\n");
		ioctl(fd, ALLOW_WRITES);
	}
	else
	if (strcmp(argv[1], "print_jblocks") == 0) {
		fprintf(stderr, "printing the journaled blocks ...\n");
		ioctl(fd, PRINT_JBLOCKS);
	}
	else
	if (strcmp(argv[1], "clean_stats") == 0) {
		fprintf(stderr, "cleaning the statistics ...\n");
		ioctl(fd, CLEAN_STAT);
	}
	else
	if (strcmp(argv[1], "clean_all_stats") == 0) {
		fprintf(stderr, "cleaning all the statistics ...\n");
		ioctl(fd, CLEAN_ALL_STAT);
	}
	else
	if (strcmp(argv[1], "extract_stats") == 0) {
		char *buf = (char *)malloc(MAX_UBUF_SIZE);
		if (!buf) {
			fprintf(stderr, "unable to allocate 50 MB of mem\n");
			return 1;
		}
		else {
			fprintf(stderr, "Extracting ... \n");	
			do {
				fprintf(stderr, "Looping ...");
				memset(buf, '\0', MAX_UBUF_SIZE);
				ioctl(fd, EXTRACT_STATS, buf);
				fprintf(stderr, "over \n");
				printf("%s\n", buf);
			} while(!(strstr(buf, "COPY")));
		}
		free(buf);
	}
	else
	if (strcmp(argv[1], "crash_commit") == 0) {
		fprintf(stderr, "crashing after commit ...\n");
		ioctl(fd, CRASH_COMMIT);
	}
	else
	if (strcmp(argv[1], "dont_crash_commit") == 0) {
		fprintf(stderr, "clearing the crash_after_commit flag ...\n");
		ioctl(fd, DONT_CRASH_COMMIT);
	}
	else
	if (strcmp(argv[1], "workload_start") == 0) {
		fprintf(stderr, "starting the workload ...\n");
		ioctl(fd, WORKLOAD_START);
	}
	else
	if (strcmp(argv[1], "workload_end") == 0) {
		fprintf(stderr, "ending the workload ...\n");
		ioctl(fd, WORKLOAD_END);
	}
	else {
		fprintf(stderr, "Invalid command\n");
	}
	
	return 1;
}
