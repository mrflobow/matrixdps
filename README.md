# MATRIX Display Server

## Setup

1. Rename config.h.dist to config.h
2. Compile

## Commands

Provides a TCP Server to Control the Matrix Display 

NAME |COMMAND | EXAMPLE |  
 -- | --- | ---
DRAW PIXEL | DPX {POSX} {POSY} {R} {G} {B} | DPX 0 0 255 0 0 | Draws Pixel directly to the screen
DRAW PICTURE | DPIC {POSX} {POSY} {LEN}~{base64data} | DPIC 0 0 512~data:image/png;base64,... | Draws Picture directly from Network Stream
UPLOAD PICTURE | UPIC {SLOT} {LEN}~{base64data} | UPIC 0 512~data:image/png;base64 | Uploads Picture to SPIFF of ESP
LOAD PICTURE FROM SPIFFS | LPIC {SLOT} {POSX} {POSY} | LPIC 1 0 0
DRAW TEXT | DTXT {POSX} {POSY} {R} {G} {B} {SIZE} {LEN}~{PAYLOAD} | DTXT 0 0 255 0 0 1 10~Hello World | Draws a text to the screen
