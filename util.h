#include <stdio.h>

/*start index for argc*/
#define START_INDEX 3





char **collect_file_paths(int argc, char *argv[], int *num_of_paths);

char *name_splitter(char *path);

int insert_special_int(char *where, size_t size, int32_t val);
	
int exceeds_max_octal8(int value);
	
int exceeds_max_octal12(int value);

int check_archive_size(const char *fname);


void print_usage();

int s_check(int num, const char *path);

void free_file_paths(char **file_paths, int num_paths);
int next_two_blocks_null(int archive_fd);
int is_null_blocks(char *block);
	
	
