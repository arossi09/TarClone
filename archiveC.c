#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <fcntl.h>

#include "ustar.h"
#include "archiveC.h"
#include "util.h"

#define MASK_MODE 07777
#define CONTENT_BLOCK 4096


/*------Program Overview-------
 * archiveC.c - creates an archive
 * based on the command arguments 
 * given
 * ----------------------------*/

/*Paramters:
 * all_paths: array of all paths to create
 * archive_fd: ...
 * recurse: flag thats set if its a recursive call
 *
 * Output:  process the path file go through each path and if
 * its directory add the direct recursivley and 
 * gather each file in it and create header for each file and add it */
int create_archive(char **all_paths, int archive_fd, 
				int num_of_paths, int recurse){
	int i;
	struct stat buf;

	for(i = 0; i< num_of_paths; i++){
		if(lstat(all_paths[i], &buf) == -1){
			/*if error stating do nothing*/
			continue;
		}

	/* if the path is a directory then recurse and add all of its contents
 	 * including it*/ 
		if(S_ISDIR(buf.st_mode)){
			if(S_ISLNK(buf.st_mode)){
				printf("Skipping Symbolic Link->\n");
				continue;
			}
	
			if(!recurse){
				/*write header*/
				if(write_header(archive_fd, all_paths[i]) > 0){
					return 1;
				}
				
			}
			DIR *dir = opendir(all_paths[i]);
			if(!dir){
				perror("proccess_paths");
				return 1;
			}	

			struct dirent *entry;

			while((entry = readdir(dir)) != NULL){
			
/*ignore . and .. because this causes infinite while loop which is not good
*and took me longer then it should of to figure out (:*/ 
				if(strcmp(entry->d_name, ".") == 0||
					strcmp(entry->d_name, "..") == 0){
					continue;
				}

				char *full_path = 
				  malloc(strlen(all_paths[i])+
					strlen(entry->d_name) + 2);
				
				sprintf(full_path, "%s/%s", 
					all_paths[i], entry->d_name);
				
				/*write_header()*/				
				
				if(write_header(archive_fd, full_path) > 0 ){
					return 1;
				}


/*recurse through the directory with new sub_dir array passed for single path
  *and also enable recurse paramter so it doesnt add dir twice*/ 
	
				if(entry->d_type == DT_DIR){
					char **sub_dir = malloc(sizeof(char*));
					if(!sub_dir){
						perror("archive.c");
						return 1;

					}
					sub_dir[0] = strdup(full_path);
					create_archive(sub_dir, 
						archive_fd, 1, 1);
					free(sub_dir[0]);
					free(sub_dir);
				}


				free(full_path);
				 

			}
			
			
			closedir(dir);
		}else{
	
	/*if its any other file just archive normally*/

			
			/*write_header*/
			if(write_header(archive_fd, all_paths[i]) > 0){
				return 1;				
			}

		}
		
	}
	
	return 0;
}

/*Paramter:
 * archive_fd:...
 * path:the path to write
 *
 * Output: when writing skip files you cant read but report it
 * this function should take the archive file and the name
 * of the path being inputed*/
int write_header(int archive_fd, char *path){
	char *name = NULL;
	struct passwd *user;
	struct group *group;
	int sum = 0;
	int i = 0;
	int content_fd;
	
	struct stat *buf = (struct stat *)malloc(sizeof(struct stat));
	struct USTAR_Header *header = 
	      (struct USTAR_Header*)calloc(1, sizeof(struct USTAR_Header)*8);
	char *new_path = NULL;

	if(!buf){
		perror("write_header");
		exit(EXIT_FAILURE);

	}
	if(!header){
		perror("write_header");
		exit(EXIT_FAILURE);
	}
	if(lstat(path, buf) == -1){
		perror("writing header.");
		exit(EXIT_FAILURE);
	}

	if(S_ISDIR(buf->st_mode)){
		new_path = malloc(strlen(path) +2);
	}else{
		new_path = malloc(strlen(path) +1);
	
	}
	

	/*realicate memory for path because other 
 	 *functions might free original*/		
	if(!new_path){
		perror("write_header");
		exit(EXIT_FAILURE);
	}

	strcpy(new_path, path);
	if(S_ISDIR(buf->st_mode)){
		strcat(new_path, "/");
	}
	
	
	/*put the whole path in name if it fits*/
	if(strlen(path) < NAME_SIZE){
	

		strcpy(header->name, new_path);		
		memset(header->prefix, 0, 155);	
	}

	/*if path does not fit then split path into 
 	 *prefix and name with as much as can fit in name
	 *and the rest in prefix*/ 
	else{

		name =	name_splitter(new_path);
		if(!name){
			printf("path: %s. Could not be partitioned\n",
							 new_path);
			free(new_path);
			return 0;
		}
		strcpy(header->name, name);
		strcpy(header->prefix, new_path);
		
	}

	/*insert the mode into header & with set octal to extract permission
 	 *bits */	


/* mode */
	sprintf(header->mode, "%07o", buf->st_mode & MASK_MODE);
/* uid */	
	if(exceeds_max_octal8(buf->st_uid)){
	
		if(s_check(buf->st_uid, path)){
			free(new_path);
			free(buf);
			free(header);
			return 0;
		}	
		insert_special_int(header->uid, UID_SIZE, buf->st_uid);	
	}else{
		sprintf(header->uid, "%07o", buf->st_uid);
	}

/* gid */
	if(exceeds_max_octal8(buf->st_gid)){
		if(s_check(buf->st_gid, path)){
			free(new_path);
			free(buf);
			free(header);
			return 0;
		}	

		insert_special_int(header->gid, GID_SIZE, buf->st_gid);
	}else{
		sprintf(header->gid, "%07o", buf->st_gid);
	}


/* size */
	/*if file is symlink or directory size is 0*/
	if(S_ISLNK(buf->st_mode) || S_ISDIR(buf->st_mode)){
		sprintf(header->size, "%011o", 0);
	}else if(exceeds_max_octal12(buf->st_size)){
		if(s_check(buf->st_size, path)){
			free(new_path);
			free(buf);
			free(header);
			return 0;
		}
		insert_special_int(header->size, SIZE_SIZE, buf->st_size);
	}else{
		sprintf(header->size, "%011o", (int)buf->st_size);	
	}

/* mtime */
	if(exceeds_max_octal12(buf->st_mtime)){
		if(s_check(buf->st_mtime, path)){
			free(new_path);
			free(buf);
			free(header);
			return 0;
		}					
		insert_special_int(header->mtime, MTIME_SIZE, buf->st_mtime);
	}else{
		sprintf(header->mtime, "%011o", (int) buf->st_mtime);
	}

/* typeflag & linkname*/
	if(S_ISREG(buf->st_mode)){
		*(header->typeflag) = '0';

	}else if(S_ISLNK(buf->st_mode)){
		*(header->typeflag) = '2';
		readlink(path, header->linkname, LINKNAME_SIZE);
		/*read the link into the flag?*/

	}else if(S_ISDIR(buf->st_mode)){
		*(header->typeflag) = '5';
	}

/* ustar & version*/
	strcpy(header->magic, "ustar");
	memcpy(header->version, "00", VERSION_SIZE);

/* uname & gname */
	user = getpwuid(buf->st_uid);
	group = getgrgid(buf->st_gid);

	strcpy(header->uname, user->pw_name);
	strcpy(header->gname, group->gr_name);

/* stdev & stminor ---- including this messed up diffs ):*/
	/*sprintf(header->devmajor, "%07o", major(buf->st_rdev));*/
/*	sprintf(header->devminor, "%07o", minor(buf->st_rdev));*/

/* check sum */
	memset(header->chksum, ' ', CHKSUM_SIZE); 
	
	for(i = 0; i < BLOCK_SIZE; i++){
		sum += (unsigned char)((unsigned char *)header)[i];		
	}
	sprintf(header->chksum, "%07o", sum);

/* write the header */

	if(write(archive_fd, header, BLOCK_SIZE) == -1){
		perror("Could not write header");
		return 1;
	}

/* write content if it is not a directory and size isnt 0*/
	if(!S_ISDIR(buf->st_mode) && buf->st_size){
		if((content_fd = open(path, O_RDONLY, S_IRUSR)) == -1){
			perror("Could not write content");
			return 1;
		}
	
		write_file(content_fd, archive_fd);
		close(content_fd);
	}
	if(v_option){
		if(!header->prefix[0]){
			printf("%s\n", header->name);
		}else{
			printf("%s/%s\n", header->prefix, header->name);
		}
	}
/*

	This was for debugging...
	if(v_option){
		printf("--------contents---------\n");	
		printf("name: %s, prefix: %s\n", 
			header->name, header->prefix);
		printf("mode: %s\n", header->mode);
		printf("uid: %s\n", header->uid);
		printf("gid: %s\n", header->gid);
		printf("size: %s\n", header->size);
		printf("mtime: %s\n", header->mtime);	
		printf("typeflag: %c\n", *(header->typeflag));
		printf("magic: %s\n", header->magic);	
		printf("version: %s\n", header->version);	
		printf("uname: %s\n", header->uname);
		printf("gname: %s\n", header->gname);
		printf("devmajor: %s\n", header->devmajor);
		printf("devminor: %s\n", header->devminor);
		printf("chksum: %s\n", header->chksum);
		printf("-------------------------\n");
	}
*/
	free(new_path);
	free(buf);
	free(header);
	
	
	return 0;

}


/*
 *Output: reads file contents and writes them
 * to archive in a 512 byte block.
 * If the file is too big it will end at 512
 * bytes if it is smaller it will pad the rest
 * with NULL bytes*/
void write_file(int infd, int outfd){
	int bytes_read = 0;
	int bytes_written = 0;
	char buf[CONTENT_BLOCK];

	while((bytes_read = read(infd, buf, CONTENT_BLOCK)) > 0){
		write(outfd, buf, bytes_read);
		bytes_written += bytes_read;
	}

	memset(buf, 0, BLOCK_SIZE);
	int accomodation = ((bytes_written / BLOCK_SIZE) +1) * BLOCK_SIZE;
	if(accomodation > 0){	
		write(outfd, buf, accomodation - bytes_written);
	}
}
