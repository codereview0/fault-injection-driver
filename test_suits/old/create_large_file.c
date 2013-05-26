#include "fslib.h"

int main()
{
	int argc = 3;
	char *argv[3];

	argv[0] = "create_file";
	argv[1] = "/mnt/sba/large_file";
	argv[2] = "0";

	create_file(argc, argv);

	for (i = 0; i < 1549612; i ++) {
	}
	
	return 1;
}
