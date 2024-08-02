#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/stat.h> 
#include "util.h"
#include "ustar.h"


/*----Program Overview----
 * holds all the utility 
 * functions that are
 * used over different files
 * -----------------------*/



#define MAX_OCTAL_VALUE8 07777777
#define MAX_OCTAL_VALUE12 077777777777

/*collects the paths into an array of paths and returns that
 * array*/
char **collect_file_paths(int argc, char *argv[], int *num_of_paths){
	
	int i;
	*num_of_paths = argc - START_INDEX;

	char **file_paths = malloc(*num_of_paths * sizeof(char*));
	if(file_paths == NULL){
		perror("collect files");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < *num_of_paths; i++){
		/*if there is no link then skip over*/
		if(strlen(argv[START_INDEX+i]) >= MAX_PATH){
			perror("Error path excceeded 256 characters");
			exit(EXIT_FAILURE);
		}
	
		file_paths[i] = malloc(MAX_PATH * sizeof(char));

		strcpy(file_paths[i], argv[START_INDEX + i]);
	}

	return file_paths;

}

/*checks if the block is full of all
 * null character*/
int is_null_blocks(char *block){
	int i;
	for(i = 0; i< BLOCK_SIZE; i++){
		if(block[i] != '\0'){
			return 0;
		}
	}
	return 1;

}

/*checks if the next two blocks are null*/
int next_two_blocks_null(int archive_fd){
	char block1[BLOCK_SIZE];
	char block2[BLOCK_SIZE];


	if(read(archive_fd, block1, BLOCK_SIZE) != BLOCK_SIZE){
		return 0;
	}
	if(read(archive_fd, block2, BLOCK_SIZE) != BLOCK_SIZE){
		return 0;
	}
	lseek(archive_fd, -2*BLOCK_SIZE, SEEK_CUR);
	
	return is_null_blocks(block1) && is_null_blocks(block2);
	
}

/*splits a name if based on its size
 * placing a null terminate byte in the
 * path to split the name into its prefix
 * and name*/
char *name_splitter(char *path){
	char *ptr = strlen(path)-1 + path;
	char *name = NULL;
	int i = 0;	
	for(; i <= NAME_SIZE;i++, ptr--){
		if(*ptr == '/'){
			name = ptr;
		}
	}
	if(!name){
		return NULL;
	}
	*name = '\0';
	name++;
	return name;

}


/*yeah*/
int insert_special_int(char *where, size_t size, int32_t val) {
	int err = 0;
	if ( val < 0 || ( size < sizeof(val)) ) {
		err++;
	} 
	else {
		memset(where,0,size);
		*(int32_t *)(where+size-sizeof(val)) = htonl(val);
		*where |= 0x80;
	
	}
	return err;
}

/*these two check if the value excceeds 8 byte
 * octal digit or 12 byte*/
int exceeds_max_octal8(int value){
	return value > MAX_OCTAL_VALUE8;
}
int exceeds_max_octal12(int value){
	return value > MAX_OCTAL_VALUE12;
}

/*returns size of file*/
int check_archive_size(const char *fname){

	struct stat test;
	if(stat(fname, &test) == -1){
		perror(fname);
		return -1;
	}		
	
	return ((int) test.st_size);			
}

/*for printing usage to user*/
void print_usage(){

	perror("Expected Format: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
	return;

}	

/*Strict check error prompt*/
int s_check(int num, const char *path){

	if(S_option){
		if(v_option){
			perror("(insert_octal:255) octal value too long." 
			" (015634204)\noctal value too long. (015634204)\n");
		}
		printf("%s: Unable to create conforming header." 
					"Skipping.\n", path);
		return 1;
	}
	return 0;
}

/*frees the array of file paths earlier*/
void free_file_paths(char **file_paths, int num_paths) {
	if(!file_paths){
		return;
	}
	int i;
	for(i = 0; i < num_paths; i++){
		free(file_paths[i]);
	}
	free(file_paths);
}


