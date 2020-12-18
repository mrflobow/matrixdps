#include "processor.h"

uint16_t* process_to_rg566bmp(const uint8_t* rgba,unsigned int height,unsigned int width){
    uint16_t *b565= (uint16_t*)malloc(sizeof(uint16_t)*height*width);
    // Allocation failed
    if(b565 == NULL) return NULL;
    int index=0;

    int len = height*width*4;
    for (int i=0; i < len; i=i+4)
    {
            uint8_t red   = *(rgba+i);
            uint8_t green = *(rgba+i+1);
            uint8_t blue  = *(rgba+i+2);
           // uint8_t alpha = *(rgba+index+3);
            uint16_t b = (blue >> 3) & 0x1f;
            uint16_t g = ((green >> 2) & 0x3f) << 5;
            uint16_t r = ((red >> 3) & 0x1f) << 11;

           // *b565 = (r | g | b);
            b565[index] =  (r | g | b);
            index++;
    }


      return b565;
}