#ifndef _SERIAL_LIGHT_H_
#define _SERIAL_LIGHT_H_

#ifdef __cplusplus
extern "C" {
#endif
    void  uart_rec_ack(void *rec_buffer, unsigned int rec_length);
    void  ctrlLegs(void);
    int   initSerialLight(void);
    
#ifdef __cplusplus
}
#endif

#endif /* _SERIAL_H_ */
