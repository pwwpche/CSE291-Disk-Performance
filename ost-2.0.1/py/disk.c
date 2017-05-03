#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h> 
#include<time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define BLOCKSIZE 512

struct  timeval start;
struct  timeval end;


void timer_start(){
	gettimeofday(&start,NULL);
}

void timer_stop(){
	gettimeofday(&end,NULL);
}

long long get_interval(){
	long long diff = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;
	return diff;
	
}
int main(void)	
{
    struct timeval start,finish;
    double seekcost=0.0;
    double readcost=0.0;
    int total=0;
    int flag;
    errno = 0;  
    int fd = open("/dev/sdb", O_RDWR|O_DIRECT, S_IRWXU);
    if(-1 == fd){
	printf("open err %d \n", errno);
        return 1;
    }

    FILE* spc_file = fopen("sample.spc", "r");
    char tmp_str[100];
    char mode;
    int sector, read_size, disk_no;
    long long total_read_time = 0, total_seek_time = 0, total_write_time = 0;
    double total_read_size = 0, total_write_size = 0;

    while(fscanf(spc_file, "%d,%d,%d,%c,%s", &disk_no, &sector, &read_size, &mode, &tmp_str) != EOF){
	if(disk_no != 0){
	    continue;
	}
	printf("%d, %d, %d, %c\n", disk_no, sector, read_size, mode);
	off_t offset = (off_t)(sector);
	timer_start();
	flag = lseek(fd, (sector / 16) * 512, SEEK_SET);
	timer_stop();
	total_seek_time += get_interval();
	
	printf("offset: %ld\n", offset);
	if(flag != -1){
            void *buff = malloc(read_size);
	    posix_memalign(&buff, BLOCKSIZE, read_size);
	    memset(buff, 0, sizeof(buff));

	    if(mode == 'W'){
		timer_start();
		int size_write = write(fd, buff, sizeof(char) * read_size);
		timer_stop();
		if(size_write != read_size){
		    printf("\n write() failed %d \n", size_write);
		}else{
		    total_write_time += get_interval();
		    total_write_size += size_write;
		}
	    }else if(mode == 'R'){
		timer_start();
		int size_read = read(fd, buff, sizeof(char) * read_size);
		timer_stop();
		if(size_read != read_size){
		    printf("\n read() failed %d \n", size_read);
		    if(errno){
			printf("read err %d \n", errno);
		    }
		}else{
		    total_read_time += get_interval();
		    total_read_size += size_read;
		}
	    }
	    free(buff);
	}else{
	    printf("fseek() failed, sector: %d, read_size: %d, mode: %c, offset: %ld \n", sector, read_size, mode, offset);
	}
    }
    printf("seek: %d, read: %d, write: %d\n", total_seek_time, total_read_time, total_write_time);
    printf("total read speed: %fMB/s\n", total_read_size / 1000.0 / 1000.0 / ((total_read_time + 1) / 1000.0 / 1000.0));
    printf("total write speed: %fMB/s\n", total_write_size / 1000.0 / 1000.0 / ((total_write_time + 1) / 1000.0 / 1000.0));
    fclose(spc_file);
    close(fd);
    return 0;
}

/*



int main()
{
	void *buffer = malloc(BLOCKSIZE);
	posix_memalign(&buffer, BLOCKSIZE, BLOCKSIZE);
	memset(buffer, 0, sizeof(buffer));
	int f = open("/dev/sdb", O_CREAT|O_TRUNC|O_WRONLY|O_DIRECT, S_IRWXU);
	lseek(f, 512*1024*512, SEEK_SET);
	write(f, buffer, BLOCKSIZE);
	close(f);
	free(buffer);
	return 0;
}

*/
