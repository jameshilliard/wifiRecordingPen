#include <speex/speex.h>
#include <string.h>
#include <stdio.h>


/*The frame size in hardcoded for this sample code but it doesn't have to be*/
#define 	 DECODE_FRAME_SIZE 	 		160
#define 	 DECODE_SPEEX_FRAME_SIZE 	38 

static int   decodeFrameSize=DECODE_FRAME_SIZE;
/*Holds the state of the decoder*/
static void  *state=NULL;
/*Holds bits so they can be read and written to by the Speex routines*/
static SpeexBits bits;

int initDecodeModule(void)
{
    if(state)
    {
        return 0;
    }   
    /*Create a new encoder state in narrowband mode*/
    state = speex_decoder_init(&speex_nb_mode);
    int enh = 1;
	speex_decoder_ctl(state, SPEEX_GET_FRAME_SIZE, &decodeFrameSize);
	speex_decoder_ctl(state, SPEEX_SET_ENH, &enh);
    printf("SPEEX_GET_FRAME_SIZE dec_frame_size=%d\n",decodeFrameSize);
    speex_bits_init(&bits); 
    return 0;
}

int destroyDecodeModule(void)
{
    if(state)
    {
        /*Destroy the encoder state*/
       speex_decoder_destroy(state);
       /*Destroy the bit-packing struct*/
       speex_bits_destroy(&bits);  
       state=NULL;
    }
    return 0;
}

int decodeSpeexToPcm(const char *speexBuf,int length,char *pcmBuf,int bufSize,int *outLength)
{
    if(pcmBuf==NULL || speexBuf==NULL)
    {
        printf("encodePcmToSpeex error\n");
        return -1;
    }
    int i=0;
    char  *in=(char *)speexBuf;
	short output_buffer[decodeFrameSize];
    int   size=length;
    int   nbBytes=0;
    int   frameSize=DECODE_SPEEX_FRAME_SIZE;//why
    int   count=0;
    *outLength=0;
    while(size>0)
    {
        if(size<38){
            frameSize=size;
        }
        speex_bits_reset(&bits);
		//printf("encodePcmToSpeex 0\n");
		speex_bits_read_from(&bits, (char *)in+count*DECODE_SPEEX_FRAME_SIZE,DECODE_SPEEX_FRAME_SIZE);
		//printf("encodePcmToSpeex 1\n");
		nbBytes=speex_decode_int(state, &bits, output_buffer);
		if(nbBytes>=0){
			/*Flush all the bits in the struct so we can encode a new frame*/
			memcpy(pcmBuf+*outLength,output_buffer,decodeFrameSize*sizeof(short));
			*outLength+=decodeFrameSize*sizeof(short);
			count++;
			size=size-frameSize;
		}
		else{
			printf("encodePcmToSpeex 2 %d\n",nbBytes);
			*outLength=0;
			break;
		}
    }
    return *outLength;
}

int speexDecondeMain(int argc, char **argv)
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
   
   char *speexBuffer=malloc(100*1024);
   int speexLength=fread(speexBuffer,1,100*1024,fin);
   fclose(fin);
   char *pcmBuffer=malloc(100*1024);
   int outLength=0;
   initDecodeModule();
   decodeSpeexToPcm(speexBuffer,speexLength,pcmBuffer,100*1024,&outLength);
   if(outLength>0)
   {
        fwrite(pcmBuffer,1,outLength,fstdout);
        fclose(fstdout);
   }
   destroyDecodeModule();
   return 0;
}

int main(int argc, char **argv)
{
	speexDecondeMain(argc,argv);
	return 0;
}

