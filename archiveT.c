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
#include "archiveT.h"
#include "util.h"


/*-------Program Overview--------
 * lists the contents of the archive
 * explicity with Verbose option set
 * or regularly if not.
 * -----------------------------*/

/*Paramters
 * archive_name:...
 * argc: ...
 * argv: ...
 *
 * Output: does the processing for listing the archive
 * either explicity with verbose option or not.
 */
void archiveT(const char *archive_name, int argc, char *argv[]){
	/*for each path in path
 	 *check if that path is in the name of header*/


	
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

/* need to check if prefix if there is one then join them for name*/
/* fix the prefix making when printing maybe helped*/
	while((size = read(archive_fd, &header, BLOCK_SIZE)>0)){
	
		if(!header.name[0]){
			break;
		}	
		if(is_corrupt(&header) == 1){
			exit(EXIT_FAILURE);
		}


		if(restricted_bound){
			for(i =3; i < argc; i++){
				if(path_matches_header(argv[i], &header, 0)){
					if(v_option){
						expand_file_info(0, &header);
						break;
						
					}
					else if(header.prefix[0]){
						
						printf("%.155s/%.100s\n", 
						   header.prefix, header.name);
						break;
					}else{

						printf("%.100s\n", header.name);
						break;
					}
				}				
			}				
		}else{
		
			if(v_option){
				expand_file_info(0, &header);
			}
			else if(header.prefix[0]){
				printf("%.155s/%.100s\n", 
					header.prefix, header.name);
			}else{
				printf("%.100s\n", header.name);
			}
		}
	
	
		/*if there are file contents skip past it*/		
		filesize = strtol(header.size, NULL, OCTAL);
		if(filesize > 0){
			int move = (((filesize/BLOCK_SIZE)+1)*BLOCK_SIZE);
			lseek(archive_fd,move, SEEK_CUR);		
		}
	}

}


