#include<stdio.h>
#include<string.h>

void main0()
{
    
	char *destipStr="112.113.114.115";
	unsigned char destip[4]={0};
	sscanf(destipStr,"%d.%d.%d.%d",(int *)destip, (int *)(destip+1),(int *)(destip+2), (int *)(destip+3));
	printf("connect ip=%d.%d.%d.%d,port %d",*destip, *(destip+1),*(destip+2), *(destip+3),7111);

}


int  analysisHttpStr(char *httpStr)
{
    char *result = NULL;
    result = strtok(httpStr,";");
    while(result != NULL)
    {
       printf("result is %s\n", result);
       result = strtok(NULL, ";");
    }
    return 0;
}

void main()
{
    char *http="http://audio.xmcdn.com/group41/M02/95/71/wKgJ8VqZIB_zIOQYABMxSBADCxQ734.mp3;http://audio.xmcdn.com/group41/M02/95/71/wKgJ8VqZIB_zIOQYABMxSBADCxQ734.mp3";
    char *httpStr=malloc(1024);
    strcpy(httpStr,http);
    analysisHttpStr(httpStr);
}
