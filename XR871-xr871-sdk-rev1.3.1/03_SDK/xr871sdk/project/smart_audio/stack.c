#include <string.h>
#include "stack.h" 

int initStack(SNode **top)
{
	*top=(SNode *)malloc(sizeof(SNode));
	if((*top)!=NULL){
	    memset(&(*top)->data,0,sizeof((*top)->data));
		//(*top)->data=0;
		(*top)->next=NULL;
		return 0;
	}
	else{
		printf("%s():%d allocate memory error!\n",__func__, __LINE__);
		return -1;	
	}
}
 
//push,success,return 1,else return -1;
int push(SNode **top,Elem e)
{
	SNode *temp;
	//judge the stack is available
	if(!(*top)){
		printf("%s():%d no such a stack\n",__func__, __LINE__);
		return -1;
	}
	//push,first,the L is the top,
	temp=(SNode*)malloc(sizeof(SNode));
	if(temp==NULL){
		printf("%s():%d allocate memory error!\n",__func__, __LINE__);
		return -1;
		//exit(-1);
	}
	temp->data=e;
	//push the node to the stack
	temp->next=*top;
	*top=temp;
	return 0;
}
 
//pop,return the Elem,else return -1
Elem pop(SNode **top)
{
	SNode *temp;
	Elem e;
	if(!(*top)){
		printf("%s():%d no such a stack\n",__func__, __LINE__);
		//exit(-1);
		e.dataFlag=-1;
		return e;
	}
	temp=(*top);
	*top=(*top)->next;
	e=temp->data;
	free(temp);
	temp=NULL;
	return e;
}
 
//judge whether the stack is empty,true return 1,no return 0,fail return -1
int isEmpty(SNode *top)
{
	//SNode *temp;
	//Elem e;
	if(!top){
		printf("%s():%d no such a stack\n",__func__, __LINE__);
		//exit(-1);
		return -1;
	}
	if(top->next==NULL)
		return 1;
	else 
		return 0;
}
 
//getLength
int getLength(SNode *top)
{
	int num=0;
	SNode *temp;
	if(!top){
		printf("%s():%d no such a stack\n",__func__, __LINE__);
		//exit(-1);
		return -1;
	}
	temp=top;
	while(temp->next!=NULL){
		temp=temp->next;
		num++;
	}
	return num;
 
}
 
//destory the stack
int destoryStack(SNode **top)
{
	SNode *temp;
	temp=*top;
	if(*top==NULL){
		printf("%s():%d no such a stack\n",__func__, __LINE__);
		//exit(-1);
		return -1;
	}
	while((*top)->next!=NULL){
		*top=(*top)->next;
		free(temp);
		temp=*top;
	}
	free(*top);
	*top=NULL;
	return 0;
}


int geBufLength(SNode *top)
{
	int num=0;
	SNode *temp;
	if(!top){
		printf("%s():%d no such a stack\n",__func__, __LINE__);
		//exit(-1);
		return -1;
	}
	temp=top;
	while(temp->next!=NULL){
		num+=temp->data.length;
		temp=temp->next;
		//num++;
	}
	return num;
 
}

int clearBuf(SNode **top)
{
    int length=0;
    int i=0;
    Elem e;
    length=getLength(*top);
	if(length>=0)
		printf("%s():%d after pushed the elem ,the length of the stack is %d\n",__func__, __LINE__,length);
	for(i=0;i<length;i++){
		e=pop(top);
		if(e.audioBuffer!=NULL){  //for aduio buffer;
            printf("%s():%d free %p length=%d\n",__func__, __LINE__,e.audioBuffer,e.length);
			free(e.audioBuffer);
			e.audioBuffer=NULL;
		}	
	}
    return 0;
}

int pushBuf(SNode **top,const char *buf,int length,int type,int flag)
{
	SNode *temp;
	//judge the stack is available
	if(!(*top)){
		printf("%s():%d no such a stack",__func__, __LINE__);
		return -1;
	}
	//push,first,the L is the top,
	temp=(SNode*)malloc(sizeof(SNode));
	if(temp==NULL){
		printf("%s():%d allocate memory error!",__func__, __LINE__);
		return -1;
		//exit(-1);
	}
	char *stackBuf=malloc(length+1);
	if(temp==NULL){
		printf("%s():%d allocate memory buf error!",__func__, __LINE__);
		return -1;
		//exit(-1);
	}
	memcpy(stackBuf,buf,length);
	temp->data.audioBuffer=stackBuf;
	temp->data.length=length;
	temp->data.ts=flag;
	temp->data.dataFlag=type;
	temp->next=NULL;
	//push the node to the stack
	SNode *tempLast;
	tempLast=*top;
	while(tempLast->next!=NULL){
		tempLast=tempLast->next;
	}
	tempLast->next=temp;
	
	return 0;
}
