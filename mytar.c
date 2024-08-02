#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "archiveC.h"
#include "archiveT.h"
#include "archiveX.h"
#include "util.h"
#include "ustar.h"

/*------Program Overview--------*
 * mytar.c - archive creation,
 * listing, and extraction system
 * following USTAR archive 
 * structure.
 * -----------------------------*/


int v_option = 0;
int S_option = 0;
int main(int argc, char *argv[]){
	int c_option = 0, t_option = 0, x_option = 0, f_mand = 0;
	int i;
	int opt_amnt = 0;
	char two_blocks[BLOCK_SIZE*2];
	
	
	/*if the command line has atleast options and tar file*/
	if(argc >= 3){
		opt_amnt = strlen(argv[1]);	
		
		/*set each option var to 1 if set*/
		for(i = 0; i < opt_amnt; i++){
			if(argv[1][i] == 'c'){
				c_option = 1;
			}
			else if(argv[1][i] == 't'){
				t_option = 1;
			}
			else if(argv[1][i] == 'x'){
				x_option = 1;
			}
			else if(argv[1][i] == 'v'){
				v_option = 1;
			}
			else if(argv[1][i] == 'S'){
				S_option = 1;
			}
			else if(argv[1][i] == 'f'){
				f_mand = 1;

			}
			else{
				perror("Unknown option.\n");
				return 1;
			}

		}
		if(!(f_mand)){
		perror("f option is mandatory followed by a tar file.\n");
			return 1;
		}		



		if(!(c_option || t_option || x_option)){
		perror("One of ‘c’, ‘t’, or ‘x’is required!!\n");
		     return 1;
		}
		if((c_option + t_option + x_option)> 1){
		perror("You may only choose one of the 'ctx' options.\n");
			print_usage();
			return 1;
		}


		/*if Archive create option is called*/
		if(c_option){
			int num_paths = 0;		
			int fd;

			if(argc >= 4){
				char **file_paths = 
					collect_file_paths(argc, argv,
							 &num_paths);
				fd = open(argv[2], O_WRONLY | O_CREAT | 
						O_TRUNC, S_IRUSR | S_IWUSR);
				if(fd == -1){
					perror("Couldnt open archive.");
					exit(EXIT_FAILURE);
				}
				

				if(create_archive(file_paths, 
						fd, num_paths, 0) != 0){
					
					free_file_paths(file_paths, num_paths);	
					return 1;
				}	
				
	
				/*last two null blocks*/	
				memset(two_blocks, 0, BLOCK_SIZE*2);
				write(fd, two_blocks, BLOCK_SIZE*2);

				
				free_file_paths(file_paths, num_paths);	
				close(fd);
				
	
			}else{
				printf("Archive Create expects paths...\n");
				return 1;
			}	

		}
		else if(t_option){
			archiveT(argv[2], argc, argv);		

		}else{
			archiveX(argv[2], argc, argv);

		}



	}else{
		
		print_usage();
		return 1;
	}
	
	return 0;
	

}


