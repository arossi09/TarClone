#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


#include "ustar.h"


/*-----------FILE OVERVIEW---------------*/
/*Brain of the program establishes the ustar
 * format and any functions that directly take
 * a header as its parameters
 * --------------------------------------*/

/*Paramters:
 * header: the header of the current file to check
 *
 * Output:
 * checks if header is corrupt based off guidlines*/
int is_corrupt(struct USTAR_Header *header){
	int i;
	int sum = 0;
	int header_sum = strtol(header->chksum, 0, OCTAL);

	for(i = 0; i < BLOCK_SIZE; i++){
		if(i < CHKSUM_OFF || i>= CHKSUM_OFF + CHKSUM_SIZE){
			sum += (unsigned char)((unsigned char *)header)[i];
		}else{
			sum += ' ';
		}
	}
	int strict = S_option && 
			(strncmp(header->magic, "ustar\0", MAGIC_SIZE) ||
			strncmp(header->version, "00", VERSION_SIZE) ||
			(header->uid[0] < '0' || header->uid[0] > '7'));;

	int not_strict =  strncmp(header->magic, "ustar", MAGIC_SIZE-1);
	if(sum != header_sum || strict || not_strict){
		perror(" Corrupted Header. Bailing.\n");	
		return 1;
	}
	return 0;

}

/*Paramters:
 * path: the current path taken in to check
 * header: the ustar header to check with path
 * flip: int that is either set or not to flip
 * the check
 *
 *Output:
 * returns 1 if the path is included in the header
 * or vice verse if flip is set*/
int path_matches_header(const char *path, 
			const struct USTAR_Header *header, int flip){
	
	char header_path[MAX_PATH];
	if(header->prefix[0]){
		sprintf(header_path, "%s/%s", header->prefix, header->name);
	}else{
		sprintf(header_path, "%s", header->name);
	}
	
	int i = 0;

	if(flip){
		while(header_path[i]){
			if(path[i] != header_path[i]){
				return 0;
			}
			i++;
		}
		if(strlen(header_path) == i){
			return 1;
		}
		return 0;
	}
	while(path[i]){
		if(path[i] != header_path[i]){
			return 0;
		}
		i++;
	}
	return 1;
	
	


}

/*Paramters
 * header: the headers information to expand for verbose option
 * of listing mode
 *
 * output: prints out the expanded info of the header*/
void expand_file_info(int is_dir, const struct USTAR_Header *header){
    char permissions[PERM_SIZE + 1];
    int mode = strtol(header->mode, NULL, OCTAL);
    int file_size = strtol(header->size, NULL, OCTAL);

    memset(permissions, '-', PERM_SIZE);
    permissions[PERM_SIZE] = '\0';

    time_t mod_time = strtol(header->mtime, NULL, OCTAL);
    struct tm *tm_info = localtime(&mod_time);
    char time[MTIME_WIDTH + 1];
    strftime(time, sizeof(time), "%Y-%m-%d %H:%M", tm_info);

    if (*(header->typeflag) == '5') {
        permissions[0] = 'd';
    } else if (*(header->typeflag) == '2') {
        permissions[0] = 'l';
    }

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';
    int owner_group_len = strlen(header->uname) + strlen(header->gname) + 2;
    char *owner_group = (char *)malloc(owner_group_len);	
    snprintf(owner_group, owner_group_len, "%s/%s", header->uname,
				 header->gname);

    if (header->prefix[0]) {
        printf("%s %-17s %8u %s %.155s/%.100s\n", permissions, owner_group,
			  file_size, time,header->prefix, header->name);
	free(owner_group);
        return;
    }
    printf("%s %-17s %8u %s %.100s\n", permissions, owner_group,
		 file_size, time, header->name);
	free(owner_group);

}

/*Paramters:
 * header: the header to extract contents from
 * archive_fd: the file descriptor of the archive to increment and use
 * to write to
 *
 * Output: extracts contents from archive if can't read file then skip*/
int extract_content(const struct USTAR_Header *header, int archive_fd){
	int mode;
	char *correct_path = NULL;
	int file_fd;
	int filesize;
	struct utimbuf time_fix;
/*if it is a directory then create directory*/	
	if(*(header->typeflag) == DIRECT){
		mode = strtol(header->mode, NULL, OCTAL);
		if(header->prefix[0]){
			correct_path = name_joiner(header->prefix,
							 header->name);
			
			/*making sure the file name is null terminated*/
			char dir_name[256];
			sprintf(dir_name, "%s", correct_path);
			mkdir(dir_name, mode);
			if(v_option){
				printf("%.255s\n", correct_path);
			}
		}else{
			char dir_name[101];
			sprintf(dir_name, "%.100s", header->name);	
			mkdir(dir_name, mode);
			if(v_option){
				printf("%.100s\n", header->name);
			}
		}
	}else if(*(header->typeflag) == SYM){
		if(header->prefix[0]){
			correct_path = name_joiner(header->prefix, 
							header->name);
			symlink(header->linkname, correct_path);
		}else{
			symlink(header->linkname, header->name);
		}
		
	}
	else{
		filesize = strtol(header->size, NULL, OCTAL);

		char *buf = (char *)malloc(filesize);
/* try to open the files*/
		mode = strtol(header->mode, NULL, OCTAL);
		if(header->prefix[0]){
			correct_path = name_joiner(header->prefix, 
							header->name);
	
			char file_name[256];
			sprintf(file_name, "%.255s", correct_path);	
			if(v_option){
				printf("%s\n", correct_path);
			}
			file_fd = open(file_name, O_CREAT | O_TRUNC |
							 O_WRONLY, mode);
		}else{
			char file_name[101];
			sprintf(file_name, "%.100s", header->name);
			if(v_option){
				printf("%s\n", header->name);
			}
			file_fd = open(file_name, O_CREAT | O_TRUNC |
							 O_WRONLY, mode);
		}
/*if cant open file then exit*/		
		if(file_fd < 0){
			if(filesize>0){
				int move = ((filesize+BLOCK_SIZE-1)
						/BLOCK_SIZE)*BLOCK_SIZE;
				lseek(archive_fd,move, SEEK_CUR);
			}
			free(buf);
			if(correct_path)
				perror(correct_path);
			else
				perror(header->name);
			free(correct_path);
			return 0 ;
		}
/*try to write to the file the contents*/
		int remaining_bytes = filesize;
		int bytes_read = 0;
		while(remaining_bytes >0 && (bytes_read = read(archive_fd,
				buf, min(BLOCK_SIZE, remaining_bytes))) > 0){
			
			if(write(file_fd, buf, bytes_read) == -1){
			    int bytes_left = remaining_bytes - bytes_read;
			    if(bytes_left > 0){
				   lseek(archive_fd, bytes_left,SEEK_CUR);
			           return 0;	
			    }
			} 
			remaining_bytes -= bytes_read;
		}
		

		
/*calculate padding and adjust archive_fd*/
		int padding = BLOCK_SIZE - bytes_read;
		if(padding > 0 && bytes_read != 0){
			if(lseek(archive_fd, padding, SEEK_CUR) < 0){
				return 1;
			}
		}
		free(buf);
		close(file_fd);
	}

/*fix the access and mod time*/
	time_fix.modtime = strtol(header->mtime, NULL, OCTAL);
        time_fix.actime = strtol(header->mtime, NULL, OCTAL);

	
	if(correct_path){
		utime(correct_path, &time_fix);
		free(correct_path);
		return 0;
	}
	
	utime(header->name, &time_fix);
	return 0;		


}

/*Paramters: 
 * preffix: self, explanatory.
 * suffix: self explanatory.
 *
 * output: combines the two strings and returns a pointer
 * to them joined*/
char *name_joiner(const char *prefix, const char *suffix){
	int prefix_len = strlen(prefix);
	int suffix_len = strlen(suffix);
	int total_len = prefix_len + suffix_len;

	char *path = (char *)malloc(total_len+2);
	if(!path){
		perror(path);
		return NULL;
	}
	
	strcpy(path, prefix);
	if(prefix_len > 0 && prefix[prefix_len - 1] != '/') {
        	strcat(path, "/");
    	}

	strcat(path, suffix);
	return path;
	
}
int min(int x, int y){
	if(x < y){
		return x;
	}
	return y;

}
