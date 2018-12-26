#ifndef __SPEEX_ENCODE_H_
#define __SPEEX_ENCODE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int initEncodeModule(void);
    int destroyEncodeModule(void);
    int encodePcmToSpeex(const char *pcmBuf,int length,char *speexBuf,int bufSize,int *outLength);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
