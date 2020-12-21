#ifndef PROCESSOR_H
	#define PROCESSOR_H
    #include <Arduino.h>

    uint16_t* process_to_rg566bmp(const uint8_t* rgba,unsigned int height,unsigned int width);
    uint16_t hex2rgb565(const char* str);
#endif