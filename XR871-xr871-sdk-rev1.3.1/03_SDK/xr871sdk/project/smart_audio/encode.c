#include <speex/speex.h>
#include <string.h>
#include "stack.h" 

/*The frame size in hardcoded for this sample code but it doesn't have to be*/
#define FRAME_SIZE 160

/*Holds the state of the decoder*/
static void  *state=NULL;
static float *inputFloat=NULL;
static char  *cbits=NULL;
/*Holds bits so they can be read and written to by the Speex routines*/
static SpeexBits bits;

int initEncodeModule(void)
{
    if(state)
    {
        return 0;
    }   
    cbits=(char *)malloc(FRAME_SIZE);
    if(cbits==NULL)
    {
        printf("cbits is malloc failure\n");
        return -1;
    }
    inputFloat=(float *)malloc(sizeof(float)*FRAME_SIZE);
    if(inputFloat==NULL)
    {
        printf("inputFloat is malloc failure\n");
        return -1;
    }
    /*Create a new encoder state in narrowband mode*/
    state = speex_encoder_init(&speex_nb_mode);
    /*Set the quality to 8 (15 kbps)*/
    int tmp=8;
    int frame_size;
    speex_encoder_ctl(state, SPEEX_SET_QUALITY, &tmp);
    speex_encoder_ctl(state, SPEEX_GET_FRAME_SIZE, &frame_size);
    printf("SPEEX_GET_FRAME_SIZE frame_size=%d\n",frame_size);
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
    if(inputFloat)
    {
        free(inputFloat);
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
    int i=0;
    short int *in=(short int *)pcmBuf;
    int size=length/2;
    int nbBytes=0;
    int frameSize=FRAME_SIZE;
    int count=0;
    *outLength=0;
    while(size>0)
    {
        if(size<FRAME_SIZE){
            frameSize=size;
        }
        for (i=0;i<frameSize;i++)
            inputFloat[i]=in[i+count*FRAME_SIZE];
        /*Flush all the bits in the struct so we can encode a new frame*/
        speex_bits_reset(&bits);
        /*Encode the frame*/
        speex_encode(state, inputFloat, &bits);
        /*Copy the bits to an array of char that can be written*/
        nbBytes = speex_bits_write(&bits, cbits, FRAME_SIZE); 
        memcpy(speexBuf+*outLength,cbits,nbBytes);
        *outLength+=nbBytes;
        count++;
        size=size-frameSize;
    }
    return *outLength;
}

int speexmain(int argc, char **argv)
{
   char *inFile;
   char *outFile;
   FILE *fin;
   FILE *fstdout;
   inFile = argv[1];
   fin = fopen(inFile, "r");
   if(fin==NULL)
   {
        printf("error:fopen %s\n",inFile);
   }
   outFile=argv[2];
   fstdout=fopen(outFile, "w+");
   if(fstdout==NULL)
   {
        printf("error:fopen %s\n",outFile);
   }
   char *pcmBuffer=malloc(100*1024);
   int pcmLength=fread(pcmBuffer,1,100*1024,fin);
   fclose(fin);
   char *speexBuffer=malloc(100*1024);
   int outLength=0;
   initEncodeModule();
   encodePcmToSpeex(pcmBuffer,pcmLength,speexBuffer,100*1024,&outLength);
   if(outLength>0)
   {
        fwrite(speexBuffer,1,outLength,fstdout);
        fclose(fstdout);
   }
   destroyEncodeModule();
   return 0;
}
