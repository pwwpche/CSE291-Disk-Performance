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

double max(double a,double b){
	if (a>b) return a;
	else return b;
}
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
    double maxread=0.0;
    int flag;
    char device[100];
    printf("device number:");
    scanf("%s",device);
    int fd = open(device, O_RDWR|O_DIRECT, S_IRWXU);
    if(-1 == fd){
	  printf("open err %d \n", errno);
        return 1;
    }
    int write_times=0;
    FILE* spc_file = fopen("Financial1.spc", "r");
    FILE* dat_file = fopen("disk.dat","w");
    char tmp_str[100];
    char mode;
    int sector, read_size, disk_no;
    long long total_read_time = 0, total_seek_time = 0, total_write_time = 0;
    double total_read_size = 0.0, total_write_size = 0.0;
    int task_count = 0;
	printf("start\n");
    while(fscanf(spc_file, "%d,%d,%d,%c,%s", &disk_no, &sector, &read_size, &mode, &tmp_str) != EOF){
	if(disk_no >1){
	    continue;
	}
	if (task_count%30000==0 && task_count!=0){
	    fprintf(dat_file,"%f\t100\t%f\t%f\t100\t0\t128\t%f\t0\t0\t1\t%f\t1\t60\t0\t%f\n",\
30000.0/(total_write_time + total_read_time + total_seek_time + 1) * 1000 * 1000,\
480.0,\
((total_write_time + total_read_time + total_seek_time + 1) / 1000.0 / 30000.0),\
total_read_size/(30000.0-write_times),\
write_times/30000.0,\
1.0*total_write_size/write_times);
	printf("reached%d %f,%f,%f,%f,%d\n",task_count,seekcost,maxread,total_read_size,total_write_size,write_times);

	    seekcost=0.0;
	    maxread=0.0;
	    total_read_size = 0.0;
	    total_write_size = 0.0;
	    total_read_time = 0; 
		total_seek_time = 0; 
		total_write_time = 0;
		write_times=0;
	}
	task_count++;

	if (task_count > 300000){
	    break;
	}
	if(task_count % 5000 == 0){
	    printf("task %d:   %d, %c\n", task_count, read_size, mode);
	}
	off_t offset = (off_t)(sector);
	timer_start();
	flag = lseek(fd, offset * 512, SEEK_SET);
	timer_stop();
	total_seek_time += get_interval();

	if(flag != -1){
            void *buff = malloc(read_size);
	    posix_memalign(&buff, BLOCKSIZE, read_size);
	    memset(buff, 0, sizeof(buff));

	    if(mode == 'w'){
		timer_start();
		int size_write = write(fd, buff, sizeof(char) * read_size);
		timer_stop();
		if(size_write != read_size){
		    printf("\n write() failed %d \n", size_write);
		}else{
		    write_times++;
		    total_write_time += get_interval();
		    total_write_size += size_write;
		}
	    }else if(mode == 'r'){
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
		offset+=read_size;
		read_size*=16;
                void *buff = malloc(read_size);
	        posix_memalign(&buff, BLOCKSIZE, read_size);
	        memset(buff, 0, sizeof(buff));
		timer_start();
		read(fd, buff, sizeof(char) * read_size);
		timer_stop();
		maxread=max(maxread,read_size / 1000.0 / 1000.0 / ((get_interval() + 0.1) / 1000.0 / 1000.0));
	    }
	     else printf("wtf %c",mode);
	    free(buff);
	}else{
	    printf("fseek() failed, sector: %d, read_size: %d, mode: %c, offset: %ld \n", sector, read_size, mode, offset);
	}
    }

    printf("seek: %fms, read: %fms, write: %fms\n", total_seek_time / 1000.0, total_read_time / 1000.0, total_write_time / 1000.0);
    printf("total read speed: %fMB/s\n", total_read_size / 1000.0 / 1000.0 / ((total_read_time + 1) / 1000.0 / 1000.0));
    printf("total write speed: %fMB/s\n", total_write_size / 1000.0 / 1000.0 / ((total_write_time + 1) / 1000.0 / 1000.0));
    printf("average latency: %fms\n", ((total_write_time + total_read_time + total_seek_time + 1) / 1000.0 / task_count));
    printf("iops: %f\n", task_count * 1.0 / (total_write_time + total_read_time + total_seek_time + 1) * 1000 * 1000);
    printf("max read speed (bandwidth) %f MB/s",maxread);
    fclose(spc_file);
    fclose(dat_file);
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
