#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int dismount_fs(char *disk_name);
int fs_open(char *name);
int fs_close(int fildes);
int fs_create(char *name);
int fs_delete(char *name);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);

size_t nbyte;
off_t offset;
off_t length;
int temp;

#define BUFFER_SIZE 80

int i;

int status;

int totalNum;

char tempBuf[BUFFER_SIZE];

int fda, fdb, fdc, fdd, fde; // 5 file descriptors

int main() {


  status = make_fs("mydk");
  printf("status for make_fs = %d\n", status); // "status for make_fs = 0" should be printed

  status = mount_fs("mydk");
  printf("status for mount_fs = %d\n", status); // "status for mount_fs = 0" should be printed

  status = fs_create("fa");
  printf("status for fs_create(""fa"") = %d\n", status); // "status for fs_create("fa") = 0" should be printed

  fda = fs_open("fa");
  printf("fda for fs_open(""fa"") = %d\n", fda); // "fda for fs_open("fa") = a nonnegative int (0..3)" should be printed

  char bufb[] = "The goal of this project";
  nbyte = 24;
  totalNum = fs_write(fda,bufb,nbyte);
  printf("total number of bytes written into file fa is %d\n", totalNum); // "total number of bytes written into file fa is 24" should be printed
 
  offset = -12;
  status = fs_lseek(fda,offset);
  printf("status for fs_lseek(fda,offset) = %d\n", status); // "status for fs_lseek(fda,offset) = 0" should be printed

char bufc[] = "TEST STRING IS VERY LONG STRING YES";
  nbyte = 36;
  totalNum = fs_write(fda,bufc,nbyte);
  printf("total number of bytes written into file fa is %d\n", totalNum); // "total number of bytes written into file fa is 24" should be printed

offset = -48;
  status = fs_lseek(fda,offset);
  printf("status for fs_lseek(fda,offset) = %d\n", status); // "status for fs_lseek(fda,offset) = 0" should be printed

nbyte = 48;
  totalNum = fs_read(fda,tempBuf,nbyte);
  printf("total number of bytes read from file fa = %d\n", totalNum); // "total number of bytes read from file fa = 12" should be printed
  
for (i = 0; i < nbyte; i++)
	  printf("%c",tempBuf[i]); // "to do something" should be printed
  printf("\n");


dismount_fs("mydk");
  printf("status for dismount_fs(""mydk"") = %d\n", status); // "status for dismount_fs("mydk") = 0" should be printed
 
  return 0;
}

 
