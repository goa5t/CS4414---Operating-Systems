#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "disk.h"

#define MAX_SIZE 4
static char valid_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int virtual_disk;

struct OFT_elem{
    int status;
    int file_offset;
    int dir_index;
};

struct FAT_elem{
    int block_num;
    int status;
	int next_block;
};

struct directory_elem{
	int status;
	int block;
	char * fn; // file name, size of MAX_SIZE
	int length; // length of file
	
};

/****************************
Declare Functions
 **************************/
void updateDir();
void updateFAT();
void printMeta();


/****************************
GLOBAL VARIABLES
**************************/
struct OFT_elem OFT[4];
struct FAT_elem FAT[32];
struct directory_elem directory[8];

/****************************
FILE FUNCTIONS
**************************/

int fs_create(char * name){
	int status = -1;
	int i=0;
	int p=0;
	int created = 0;
	int length = strlen(name);
	//error checking first
	if(length > MAX_SIZE){
		printf("Error fs_create: File name is more than 4 characters.\n");
        	return -1;
	}
	if(length != strspn(name, valid_chars)){
      	 	 printf("Error fs_create: File name contains invalid characters.\n");
       		 return -1;
    	}
	for(i=0; i < 8; i++){
		if(strcmp(name, directory[i].fn) == 0){
			printf("Error fs_create: File name is already in use.\n");
			return -1;
		}
		if(directory[i].status == 0){
			//create file here
			directory[i].status = 1;
			directory[i].fn = name;
			created = 1;
			return 0;
		}
	}
	if(created==0){
		printf("Error fs_create: There are already 8 files in the root directory.\n");
		return -1;
	}
}

int fs_open(char * name){
	int i=0;
	int p=0;
	int found = 0;
	int openFiles = 0;
	for(i=0; i < 4; i++){
		if(OFT[i].status == 1){
			openFiles++;
			//printf("directory[OFT[%i].dir_index] = %s\n", i, directory[OFT[i].dir_index].fn); 
			if(strcmp(name, directory[OFT[i].dir_index].fn) == 0){
				printf("Error fs_open: File is already open.\n");
				return -1;
			}
		}
		
	}
	if(openFiles == 4 ){
		printf("Error fs_open: Max number of files are open (four).\n");
		return -1;
	}
	for(i=0; i < 4; i++){
		if(OFT[i].status ==0){
			OFT[i].status = 1;
			OFT[i].file_offset = 0;
			for(p=0; p < 8; p++){
				if(strcmp(name, directory[p].fn)==0){
					OFT[i].dir_index = p;
					found = 1;
					printf("file named ->%s is assigned fildes->%i\n", name, i);
					return i;
				}
			}
			
		}
	}
	if(found==0){
		printf("Error fs_open: File is not found.\n");
		return -1;
	}
	
}

int fs_close(int fildes){
	printf("in fs_close... attempting to close fildes=%i\n", fildes);
	if(OFT[fildes].status == 0){
		printf("Error fs_close: File does not exist or is not open.\n");
		return -1;
	}
	//int dir = OFT[fildes].dir_index;
	// update directory for this file
	int dindex = OFT[fildes].dir_index;
	updateDir();

	//clear out OFT
	OFT[fildes].status = 0;
	OFT[fildes].file_offset = 0;
	OFT[fildes].dir_index = -1;
	return 0;
}

int fs_delete(char * name){
	int i=0;
	char * zeroes = "0000000000000000";
	for(i=0; i < 4; i++){
		if(OFT[i].status == 1){
			if(strcmp(name, directory[OFT[i].dir_index].fn)==0){
				printf("Error fs_delete: File is currently open.\n");
				return -1;
			}
		}
	}
	
	int dir = -1;
	for(i=0; i < 8; i++){
		if(directory[i].status == 1 && strcmp(name, directory[i].fn) == 0){
			//found the file
			dir = i;
		}
	}
	int block_status = 1;
	int block = directory[dir].block;
	while(block!=0){
		//clear data
		block_write(block, zeroes);

		//clear FAT entry
		FAT[block-32].status = 0;
		block = FAT[block-32].next_block;
		FAT[block-32].next_block = 0;
		
	}
	//clear our directory
	directory[dir].status = 0;
	directory[dir].fn = "0000";
	directory[dir].block = 0;
	directory[dir].length = 0;
	//write directory to disk
	updateDir();
	
	//write FAT to disk
	updateFAT();
	
	if(dir==-1){
		printf("Error fs_delete: File not found.\n");
		return -1;
	}
	else
		return 0;
	
}

int fs_read(int fildes, void *buf, size_t nbyte){
	printMeta();
    printf("***** START of fs_read **********\n");
	int dindex = OFT[fildes].dir_index;
	if(dindex == -1 || directory[dindex].status == 0){
		printf("Error in fs_read: invalid file descriptor.\n");
		return -1;
	}
	if(OFT[fildes].file_offset == directory[dindex].length){
		printf("Error fs_read: file_offset already at end of file.\n");
		return 0;
	}
	if((nbyte + OFT[fildes].file_offset) > directory[dindex].length){
		nbyte = directory[dindex].length - OFT[fildes].file_offset;
	}
	
	//locate offset, find corresponding block
	int block = directory[dindex].block;
	int offset = OFT[fildes].file_offset;
	printf("starting block = %i, offset = %i\n", block, offset);
	int nblock = FAT[block-32].next_block;
	
	while( (nblock != 0) && (offset > 16) ){
		block = FAT[nblock-32].block_num;
		nblock = FAT[block-32].next_block;
		offset = offset - 16;
	}
	printf("after traversing, block = %i\n", block);
	//block is now set to be the right data block
	char * readString = calloc(nbyte, sizeof(char));
	char * tmp = calloc(16, sizeof(char));

	int total_read = 0;
	if(offset>0)
		offset++;
	while(total_read < nbyte){
		block_read(block, tmp);
		printf("tmp -> %s\n", tmp);
		int i=0;
		for(i=offset; i < 16 && total_read < nbyte; i++){
			readString[total_read] = tmp[i];
			total_read++;
		}
		printf("readString = %s\n", readString);
		offset = 0;
		block = FAT[block-32].next_block;
	}
	
	memcpy(buf, readString, nbyte);
	printf("***** END of fs_read **********\n");
	return nbyte;

}

int fs_get_filesize(int fildes){
	int dindex = OFT[fildes].dir_index;
	if(dindex == -1 || directory[dindex].status == 0){
		printf("Error in fs_read: invalid file descriptor.\n");
		return -1;
	}

	return directory[dindex].length;
}


int fs_write(int fildes, void *buf, size_t nbyte){
	//printMeta();
    printf("---START OF fs_write---\n");
    int dindex = OFT[fildes].dir_index;
    char * buf_char = (char*)buf;
	printf("writing-> %s<- to directory[%i]\n", buf_char, dindex);

    if(dindex == -1 || directory[dindex].status == 0){
        printf("Error in fs_write: invalid file descriptor.\n");
        return -1;
    }
	int offset = 0;
    //if we have a single file, and we're trying to write more than 512 bytes
    if(nbyte + directory[dindex].length > 512){
        nbyte = 512 - directory[dindex].length;
    }
	offset = OFT[fildes].file_offset;
	int new_length = offset + nbyte;
	int total_blocks = 0;
	if((offset + nbyte) % 16 ==0)
		total_blocks = ((new_length) / 16);
	else
		total_blocks = ((new_length) / 16) + 1;
	int i=0;
	int p=0;
	int prevind = 0;
	// allocate total_blocks for the file
	if(directory[dindex].length == 0){
		while(i < total_blocks){
			for(p=0; p < 32; p++){
				if(FAT[p].status==0){
					if(directory[dindex].block ==0)
						directory[dindex].block = (p+32);
					FAT[p].status = 1;
					if(i > 0)
						FAT[prevind].next_block = p+32;
					prevind = p;
					if(i == total_blocks-1)
						FAT[p].next_block = 0;
					p = 32;
				}
				
			}
			i++;
		}
	
	
	//allocating done

	//write to disk
	i=0;
	int bytesToWrite = nbyte;
	int bindex = 0;
	int block = directory[dindex].block;
	while(i < total_blocks){
		char * tmp = calloc(16, sizeof(char));
		for(p=0; p < 16; p++){
			tmp[p] = buf_char[bindex];
			bindex++;
		}
		printf("writing ->%s<- to block = %i\n", tmp, block);
		block_write(block, tmp);
		
		block = FAT[block-32].next_block;
		i++;
	}
	directory[dindex].length = new_length;
	}
	//printMeta();
    
    printf("---END OF fs_write---\n");
	OFT[fildes].file_offset+=nbyte;
    return nbyte;
}

int fs_lseek (int fildes, off_t offset){
    int dindex = OFT[fildes].dir_index;
    if(dindex == -1 || directory[dindex].status == 0){
        printf("Error in fs_lseek: invalid file descriptor.\n");
        return -1;
    }
    int cur_offset = OFT[fildes].file_offset;
    //printf("in lseek... current offset = %i\n.", cur_offset);
    int length = directory[dindex].length;
    if(cur_offset + offset > length || cur_offset + offset < 0){
        printf("Error in fs_lseek: Offset is out of bounds of the file.\n");
        return -1;
    }
    OFT[fildes].file_offset += offset;
    
    return 0;
}

int fs_truncate (int fildes, off_t length){
    int dindex = OFT[fildes].dir_index;
    if(dindex == -1 || directory[dindex].status == 0){
        printf("Error in fs_truncate: invalid file descriptor.\n");
        return -1;
    }
    int filesize = directory[dindex].length;
    if(length > filesize){
        printf("Error in fs_truncate: requested length is larger than the file size.\n");
        return -1;
    }
   /* int cur_offset = 0;
    int i=length;
    int block = directory[dindex].block;
    char * tmp = calloc(16, sizeof(char));
    char * readin = calloc(16, sizeof(char));
    //find last block of file
    while(i > 16){
        block = FAT[block-32].next_block;
        i = i - 16;
    }
    block_read(block, readin);
    while(cur_offset < i){
        tmp[cur_offset] = readin[cur_offset];
        cur_offset++;
    }
    block_write(block, tmp);
    
    //empty FAT and data after length
    int fat_index = block-32;
    block = FAT[fat_index].next_block;
    fat_index = block-32;
    int stat = FAT[fat_index].status;
    char * zeroes = "0000000000000000";
    while(stat == 1){
        block_write(block, zeroes);
        FAT[fat_index].status = 0;
        block = FAT[fat_index].next_block;
        FAT[fat_index].next_block = 0;
        
        //next block from FAT
        fat_index = block-32;
        stat = FAT[fat_index].status;
    }
    directory[dindex].length = length;*/
    OFT[fildes].file_offset = 0;
    return 0;
}
/*************************

UPDATE FUNCTIONS

*****************************/
void updateDir(){

	int i=0;
	int dir_start = 1;
	int dir_end = 8;
	int dir_index = 0;
	for(i=dir_start; i < dir_end + 1; i++){
		char * buf = calloc(16, sizeof(char));
		//status byte
		buf[0] = directory[dir_index].status + '0';

		buf[1] = (directory[dir_index].block / 10)+ '0';
		buf[2] = (directory[dir_index].block % 10)+ '0';

		//file name, 4 bytes
		buf[3] = directory[dir_index].fn[0];
		buf[4] = directory[dir_index].fn[1];
		buf[5] = directory[dir_index].fn[2];
		buf[6] = directory[dir_index].fn[3];

		//file length, 3 bytes
		buf[7] = (directory[dir_index].block / 100)+ '0';
		buf[8] = (directory[dir_index].block / 10)+ '0';
		buf[9] = (directory[dir_index].block % 10)+ '0'; 

		block_write(i, buf);

		dir_index++;
	}
}

void updateFAT(){
	int i=0;
	int fat_start = 9;
	int fat_end = 16;
	int fat_index = 0;
	int p = 0;
	for(i=fat_start; i<fat_end + 1; i++){
		char * buf = calloc(16, sizeof(char));
		for(p=0; p < 4; p++){
			//status
			buf[p * 4] = FAT[fat_index].status + '0';
			//block
			buf[p*4 + 1] = (FAT[fat_index].next_block / 100) + '0';
			buf[p*4 + 2] = (FAT[fat_index].next_block / 10) + '0';
			buf[p*4 + 3] = (FAT[fat_index].next_block % 10) + '0';
			fat_index++;
		}
		block_write(i, buf);
	}
}
/******************

VIRTUAL DISK FUNCTIONS

*****************/
int make_fs(char * disk_name){
    int status;
    int length = strlen(disk_name);
    
    //check disk_name
    if(length > MAX_SIZE){
        printf("Disk name is more than 4 characters.\n");
        return -1;
    }
    if(length != strspn(disk_name, valid_chars)){
        printf("Disk name contains invalid characters.\n");
        return -1;
    }
    
    //make the disk
    if( (status = make_disk(disk_name) < 0))
        return -1;
    
    //open disk and initialize
    if( (status = open_disk(disk_name) < 0) )
        return -1;
    
    // superblock
    // every char is 1 byte, every 2 bytes represents the start/end point for that section
    // 4 bytes for directory, 4 bytes for FAT, 4 bytes for data blocks, 4 unused
    char * superblock = "0108091632630000";
    if(block_write(0, superblock) < 0){
        printf("Error writing superblock.\n");
        return -1;
    }
    
    //directory
    // 8 blocks, each block contains 16 bytes
    // initialize to all 0s, since there are no files yet
   
	 char * zeroes = "0000000000000000";

	int i=1;
	for(i=1; i<9; i++){
   		 if(block_write(i, zeroes) < 0){
       			 printf("Error writing directory.\n");
       			 return -1;
   		 }
	}

    //FAT
    // initialize to all 0s at first
    //char * fat = calloc(8, 16 * (sizeof(char)));

	for(i=9; i<17; i++){
    		if(block_write(i, zeroes) < 0){
       			 printf("Error writing fat.\n");
       			 return -1;
  		  }
	}
    
    //data blocks
    //char * data = calloc(32, 16 * sizeof(char));
    for(i=32; i<63; i++){
    		if(block_write(i, zeroes) < 0){
       			 printf("Error writing data.\n");
       			 return -1;
  		  }
	}

    
    //close disk
    if ( (status = close_disk(disk_name) < 0) )
        return -1;
    
       return 0;
}

int mount_fs(char * disk_name){
    int status;
	char * superblock = calloc(16, sizeof(char));
	int dir_start, dir_end, fat_start, fat_end, data_start= -1;
	int i, p=0;
	
    //open disk
    if( (status = open_disk(disk_name) < 0) )
        return -1;
    
	//load superblock into memory
	if( (block_read(0, superblock) < 0) ){
		printf("Error reading superblock.\n");
		return -1;
	}

	char * tmp = malloc(2);

	tmp[0] = superblock[0];
	tmp[1] = superblock[1];
	dir_start = atoi(tmp);

	tmp[0] = superblock[2];
	tmp[1] = superblock[3];
	dir_end = atoi(tmp);

	tmp[0] = superblock[4];
	tmp[1] = superblock[5];
	fat_start = atoi(tmp);

	tmp[0] = superblock[6];
	tmp[1] = superblock[7];
	fat_end = atoi(tmp);
	
	//grabbing first block of data
	tmp[0] = superblock[8];
	tmp[1] = superblock[9];
	data_start = atoi(tmp);


	//printf("directory start is %i, end is %i\n", dir_start, dir_end);
	//printf("fat start is %i, end is %i\n", fat_start, fat_end);

    
	//initialize OFT
	for(i=0;i<4; i++){
		OFT[i].status = 0;
		OFT[i].file_offset = 0;
		OFT[i].dir_index = -1;
	}

	// load from virtual disk into FAT
	char fat_status = '0';
	char * buffer = calloc(16, sizeof(char));
	int block_index = data_start;
	char * fat_block = malloc(3);
	int tmp_index=0;
	
	for(i=fat_start; i < fat_end+1; i++){
		if( (block_read(i, buffer)) < 0){
			printf("Error reading FAT at block %i\n.", i);
			close_disk(disk_name);
			return -1;
		}
		for(p=0; p < 4; p++){
			
			//printf("buffer = %s\n", buffer);
			//printf("buffer[%i*4] = %i\n", p, buffer[p*4]);
			fat_status = buffer[p*4] - '0';
			
			FAT[tmp_index].block_num = block_index;
			FAT[tmp_index].status = fat_status;

			fat_block[0] = buffer[p*4+1];
			fat_block[1] = buffer[p*4+2];
			fat_block[2] = buffer[p*4+3];
			//printf("test seg\n");
			FAT[tmp_index].next_block = atoi(fat_block);
	

			
			block_index++;	
			tmp_index++;	
		}
		
	}	



	//load from virtual disk into directory
	char dir_status = '0';
	tmp_index = 0;
	char * dir_block = malloc(2);
	char * dir_len = malloc(3);
	for(i=dir_start; i < dir_end + 1; i++){
	
		if( (block_read(i, buffer)) < 0){
			printf("Error reading directory at block %i\n.", i);
			close_disk(disk_name);
			return -1;
		}
		directory[tmp_index].status = buffer[0] - '0';
		//printf("directory[tmp_index].status = %i\n", directory[tmp_index].status);
		dir_block[0] = buffer[1];
		dir_block[1] = buffer[2];
		directory[tmp_index].block = atoi(dir_block);
		
		//directory[tmp_index]->fn = 
		
		directory[tmp_index].fn = malloc(4);
		//memcpy(directory[tmp_index].fn, buffer[3], 4);
		directory[tmp_index].fn[0] = buffer[3];
		directory[tmp_index].fn[1] = buffer[4];
		directory[tmp_index].fn[2] = buffer[5];
		directory[tmp_index].fn[3] = buffer[6];

		dir_len[0] = buffer[7];
		dir_len[1] = buffer[8];
		dir_len[2] = buffer[9];
		directory[tmp_index].length = atoi(dir_len);
		tmp_index++;
	
	}
	//printf("After mounting...\n");
	//printMeta();
    
	

    return 0;
}


/******************************

DISMOUNT

************/
int dismount_fs(char * disk_name){
	int status = -1;
	char * superblock = calloc(16, sizeof(char));
	
	int dir_start, dir_end, fat_start, fat_end, data_start= -1;
	int i=0;
	int dir_index = 0;

	if( (block_read(0, superblock) < 0) ){
		printf("Error reading superblock.\n");
		return -1;
	}
	//printf("superblock is %s\n", superblock);

	char * tmp = malloc(2);

	tmp[0] = superblock[0];
	tmp[1] = superblock[1];
	//
	//memcpy(tmp, superblock[0], 2);
	dir_start = atoi(tmp);
	//printf("dir_start = %i\n", dir_start);
	tmp[0] = superblock[2];
	tmp[1] = superblock[3];
	dir_end = atoi(tmp);

	tmp[0] = superblock[4];
	tmp[1] = superblock[5];
	fat_start = atoi(tmp);

	tmp[0] = superblock[6];
	tmp[1] = superblock[7];
	fat_end = atoi(tmp);
	
	//grabbing first block of data
	tmp[0] = superblock[8];
	tmp[1] = superblock[9];
	data_start = atoi(tmp);
    
    //not reading superblock properly... should probably comment out
    dir_start = 1;
    dir_end = 8;
    fat_start = 9;
    fat_end = 16;
    
	printf("directory start is %i, end is %i\n", dir_start, dir_end);
	printf("fat start is %i, end is %i\n", fat_start, fat_end);

    char * buf = calloc(16, sizeof(char));
	//write directory 
	for(i=dir_start; i < dir_end + 1; i++){
		
		//status byte
		buf[0] = directory[dir_index].status + '0';
		//printf("status is %c\n", (directory[dir_index].status) + '0');
		//printf("status in dir is %i\n", directory[dir_index].status);
		//block 2 bytes
		buf[1] = (directory[dir_index].block / 10)+ '0';
		buf[2] = (directory[dir_index].block % 10)+ '0';

		//file name, 4 bytes
		buf[3] = directory[dir_index].fn[0];
		buf[4] = directory[dir_index].fn[1];
		buf[5] = directory[dir_index].fn[2];
		buf[6] = directory[dir_index].fn[3];

		//file length, 3 bytes
		buf[7] = (directory[dir_index].block / 100)+ '0';
		buf[8] = (directory[dir_index].block / 10)+ '0';
		buf[9] = (directory[dir_index].block % 10)+ '0'; 

		block_write(i, buf);
		printf("buf being written to disk is %s\n", buf);
        
		dir_index++;
	}
	//write FAT
	int fat_index = 0;
	int p = 0;
	for(i=fat_start; i<fat_end + 1; i++){
		char * buf = calloc(16, sizeof(char));
		for(p=0; p < 4; p++){
			//status
			buf[p * 4] = FAT[fat_index].status + '0';
			//block
			buf[p*4 + 1] = (FAT[fat_index].next_block / 100) + '0';
			buf[p*4 + 2] = (FAT[fat_index].next_block / 10) + '0';
			buf[p*4 + 3] = (FAT[fat_index].next_block % 10) + '0';
			fat_index++;
		}
		block_write(i, buf);
		//printf("buf being written to disk is %s\n", buf);
	}
	//close disk
    if ( (status = close_disk(disk_name) < 0) )
        return -1;    

	return 0;
}

//PRINT FUNCTION

void printMeta(){
	int tmp_index = 0;
	int i=0;
	printf("/**************************************\nPRINT META\n***************************************/\n");
	for(i=0; i < 4; i++){
		printf("OFT[%i] = status = %i, offset = %i, index in directory = %i,\n", i, OFT[i].status, OFT[i].file_offset, OFT[i].dir_index);
	}
	printf("---------------------------------\n");
	for(tmp_index=0; tmp_index < 8; tmp_index++){
	printf("directory[%i] = status = %i, block = %i, fn = %s, length = %i,\n", tmp_index, directory[tmp_index].status, directory[tmp_index].block, directory[tmp_index].fn, directory[tmp_index].length);
	}
	printf("---------------------------------\n");
	for(tmp_index = 0; tmp_index < 32; tmp_index++){
		printf("FAT[%i] = block_num = %i, status = %i, next_block = %i\n", tmp_index, FAT[tmp_index].block_num, FAT[tmp_index].status, FAT[tmp_index].next_block);
	}
	printf("/**************************************\nPRINT META\n***************************************/\n");
}

/*
int main(){
    int status, nbyte, totalNum;
    int fda, fdb, fdc, fdd, fde; // 5 file descriptors
    
    status = make_fs("test");
    printf("status for make_fs is %d\n", status);

	  status = mount_fs("test");
    printf("status for mount_fs is %d\n", status);

	status = fs_create("fa");
	printf("status for fs_create is %d\n", status);

	fda = fs_open("fa");
	printf("status for fs_open is %d\n", fda);

    char bufa[] = "Hi, there";
    nbyte = 9;
    totalNum = fs_write(fda,bufa,nbyte);
    printf("total number of bytes written into file fa is %d\n", totalNum); // "total number of bytes written into file fa is 9" should be printed
    
    
	printMeta();

	void * buf = malloc(3);
	status = fs_read(fda, buf, 1);
	printf("status for fs_read is %d\n", status);
	
	status = fs_get_filesize(0);
	printf("status for fs_get_filesize is %d\n", status);

	status = fs_close(0);	
	printf("status for fs_close is %d\n", status);

	status = fs_delete("fa");
	printf("status for fs_delete is %d\n", status);

	printMeta();

	status = dismount_fs("test");
	printf("status for dismount_fs is %d\n", status);
    return 0;
}
 */
