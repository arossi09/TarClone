#include <stdio.h>

extern int v_option;
extern int S_option;


/* chars resembling file types*/
#define DIRECT '5'
#define SYM '2'
#define REG '0'

#define MAX_PATH 256 /*max path size*/

#define OCTAL 8 /*octal size*/

/*size defs for contents
 * we need*/
#define PERM_SIZE 10
#define MTIME_WIDTH 16
#define BLOCK_SIZE 512
#define NAME_SIZE 100
#define UID_SIZE 8
#define GID_SIZE 8
#define SIZE_SIZE 12
#define MTIME_SIZE 12
#define LINKNAME_SIZE 100
#define MAGIC_SIZE 6
#define CHKSUM_SIZE 8
#define CHKSUM_OFF 148
#define VERSION_SIZE 2


/*ustar header format*/
struct USTAR_Header{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};

/*-----------Note-------------*
 * comments of file use and their
 * description in ustar.c
 * ---------------------------*/


int is_corrupt(struct USTAR_Header *header);
int path_matches_header(const char *path, 
		const struct USTAR_Header *Header, int flip);
void expand_file_info(int is_dir, const struct USTAR_Header *header);
int extract_content(const struct USTAR_Header *header, int archive_fd);	
char *name_joiner(const char *prefix, const char *suffix);
int min(int x, int y);
