#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "ustar.h"
#include "archiveX.h"
#include "util.h"


/*Paramters:
 * archive_name: name of the archive to be passed as char
 * argc: ...
 * argv: ...
 *
 * output: extracts the archive*/
void archiveX(const char *archive_name, int argc, char *argv[]){

	/*if it is then do the processing*/ 
	int archive_fd = open(archive_name, O_RDONLY, S_IRUSR);
	/*flag if paths are passed on command line*/
	int restricted_bound = 0;	
	int i;
	int size;
	int filesize;
	if(archive_fd == -1){
		perror("archiveT");
		exit(EXIT_FAILURE);
	}

	struct USTAR_Header header;

	if(argc >= 4){
		restricted_bound = 1;
	}

	while((size = read(archive_fd, &header, BLOCK_SIZE))>0){	

		
		if(!header.name[0]){
			break;
		}	
		if(is_corrupt(&header) == 1){
			exit(EXIT_FAILURE);	
		}
	
		if(restricted_bound){
			
			for(i =3; i < argc; i++){
				if(path_matches_header(argv[i], &header, 0)||
				(path_matches_header(argv[i], &header, 1) 
					&& *(header.typeflag) == DIRECT)){
					extract_content(&header, 
							archive_fd);
					break;
				}
				/*if its not a header we want and it has a
 				 *skip it*/
			}
			if(i == argc){
				filesize = strtol(header.size, NULL, OCTAL);
				if(filesize > 0){
					int move = ((filesize+BLOCK_SIZE-1)
							/BLOCK_SIZE)*BLOCK_SIZE;
					lseek(archive_fd,move, SEEK_CUR);
				}
			}	
						
		}else{
			extract_content(&header, archive_fd);
				
			
		}
					
		
	}
	close(archive_fd);

}


