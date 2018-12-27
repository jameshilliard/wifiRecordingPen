#include <speex/speex.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

/*The frame size in hardcoded for this sample code but it doesn't have to be*/
#define ENCODE_FRAME_SIZE 160

static int 	  encodeFrameSize = ENCODE_FRAME_SIZE;
/*Holds the state of the decoder*/
static void  *state=NULL;
static char  *cbits=NULL;
/*Holds bits so they can be read and written to by the Speex routines*/
static SpeexBits bits;

int initEncodeModule(void)
{
    if(state)
    {
        return 0;
    }   
    /*Create a new encoder state in narrowband mode*/
    state = speex_encoder_init(&speex_nb_mode);
    /*Set the quality to 8 (15 kbps)*/
    int tmp=8;
    speex_encoder_ctl(state, SPEEX_SET_QUALITY, &tmp);
    speex_encoder_ctl(state, SPEEX_GET_FRAME_SIZE, &encodeFrameSize);
	printf("frame_size=%d\n",encodeFrameSize);
	cbits=(char *)malloc(encodeFrameSize+40);
    if(cbits==NULL)
    {
        printf("cbits is malloc failure\n");
        return -1;
    }
    speex_bits_init(&bits); 
    return 0;
}

int destroyEncodeModule(void)
{
    if(state)
    {
        /*Destroy the encoder state*/
       speex_encoder_destroy(state);
       /*Destroy the bit-packing struct*/
       speex_bits_destroy(&bits);  
       state=NULL;
    }
    if(cbits)
    {
        free(cbits);
    }
    return 0;
}

int encodePcmToSpeex(const char *pcmBuf,int length,char *speexBuf,int bufSize,int *outLength)
{
    if(pcmBuf==NULL || speexBuf==NULL)
    {
        printf("encodePcmToSpeex error\n");
        return -1;
    }
    int nbBytes=0;
    *outLength=0;
    /*Flush all the bits in the struct so we can encode a new frame*/
    speex_bits_reset(&bits);
    /*Encode the frame*/
    speex_encode_int(state,(short int *)pcmBuf, &bits);
    /*Copy the bits to an array of char that can be written*/
    nbBytes = speex_bits_write(&bits,cbits,encodeFrameSize+40);
	if(nbBytes>0){
		memcpy(speexBuf,cbits,nbBytes);
		*outLength=nbBytes;
	}
    //printf("encodePcmToSpeex %d %d\n",nbBytes,encodeFrameSize);
    return *outLength;
}
