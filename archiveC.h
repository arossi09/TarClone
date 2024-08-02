#include<stdio.h>

/*-----------Note-------------*
 * comments of file use and their
 * description in archiveC.c
 * ---------------------------*/


int create_archive(char **all_paths, int archive_fd, 
			int num_of_paths, int recurse);
int write_header(int archive_fd, char *path);
void write_file(int infd, int outfd);	
	
