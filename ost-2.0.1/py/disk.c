#include<stdio.h>
#include<string.h>
#include<errno.h> 

#define SIZE 1
#define NUMELEM 5

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
        printf("%d, %d, %d, %c\n", disk_no, sector, read_size, mode);
    }


    if(0 != fseek(fd,11,SEEK_CUR))
    {
        printf("\n fseek() failed\n");
        return 1;
    }

    char* buff = "12345";

    int sizeWrite =  fwrite(buff,SIZE,strlen(buff),fd);
    if(sizeWrite != 5)
    {
        printf("\n fwrite() failed %d \n", sizeWrite);
        return 1;
    }

    fclose(spc_file);
    fclose(fd);


    return 0;
}
