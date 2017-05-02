#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h> 

int main(void)
{
    FILE* fd = NULL;
    errno = 0;  
    fd = fopen("/dev/sdb","w+");
    if(NULL == fd){
	printf("fopen err %d \n", errno);
        return 1;
    }

    FILE* spc_file = fopen("sample.spc", "r");
    char tmp_str[100];
    char mode;
    int sector, read_size, disk_no;
    while(fscanf(spc_file, "%d,%d,%d,%c,%s", &disk_no, &sector, &read_size, &mode, &tmp_str) != EOF){
        
	
	if(disk_no != 0){
	    continue;
	}
	printf("%d, %d, %d, %c\n", disk_no, sector, read_size, mode);
	off_t offset = (off_t)(sector) * 512;
	if(fseeko(fd, offset, SEEK_SET) == 0){
            char* buff = malloc(read_size);
	    if(mode == 'W'){
		memset(buff, 0, read_size);
		int size_write = fwrite(buff, 1, read_size, fd);
		if(size_write != read_size){
		    printf("\n fwrite() failed %d \n", size_write);
		}
	    }else if(mode == 'R'){
		int size_read = fread (buff, 1, read_size, fd);
		if(size_read != read_size){
		    printf("\n fwrite() failed %d \n", size_read);
		}
	    }
	    
	}else{
	    printf("fseek() failed, sector: %d, read_size: %d, mode: %c, offset: %ld \n", sector, read_size, mode, offset);
	}
    }

    fclose(spc_file);
    fclose(fd);


    return 0;
}
