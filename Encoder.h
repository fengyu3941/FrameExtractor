#ifndef __ENCODER_H__
#define __ENCODER_H__

void* createEncoder(int id, unsigned int width, unsigned int height);
void EncodeVideo(void* p, uint8_t* src[4], int src_stride[4], const char* outfile_name);
void destroyEncoder(void*p);


#endif