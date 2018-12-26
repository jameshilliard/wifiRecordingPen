#ifndef __SPEEX_DECODE_H_
#define __SPEEX_DECODE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int initDecodeModule(void);
    int destroyDecodeModule(void);
    int decodeSpeexToPcm(const char *pcmBuf,int length,char *speexBuf,int bufSize,int *outLength);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
