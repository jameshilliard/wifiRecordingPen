#ifndef __TCP_STACK_H_
#define __TCP_STACK_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct Elem_
{
	char *audioBuffer;
	int   length;
}Elem;
//#define Elem int

//define the node of the stack
//we still use a struct to store the node

typedef struct node{
Elem data;
struct node *next;
}SNode;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	//initialize the stack,if success return 1,else return -1
	int initStack(SNode **top);
	
	//push,success,return 1,else return -1;
	int push(SNode **top,Elem e);
	
	//pop,return the Elem,else return -1
	Elem pop(SNode **top);
	
	//judge whether the stack is empty,true return 1,no return 0,fail return -1
	int isEmpty(SNode *top);
	
	//getLength
	int getLength(SNode *top);
	
	//destory the stack
	int destoryStack(SNode **top);

	//get memory sum 
	int geBufLength(SNode *top);

	int clearBuf(SNode **top);
	
	int pushBuf(SNode **top,const char *buf,int length);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

