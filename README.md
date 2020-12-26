# MATRIX Display Service

## Setup

1. Rename config.h.dist to config.h
2. Compile

## Commands

Provides a MQTT Client to Control the Matrix Display. 

Prefix is the one you define at MQTT_PREFIX (config.h). So full topic will be e.g. /office/matrixdps/cmnd/dpx .


NAME |COMMAND | ARGS |  EXAMPLE | Comment 
 -- | --- | --- | --- | ---
DRAW PIXEL | {PREFIX}/cmnd/dpx | {POSX} {POSY} {R} {G} {B} | 0 0 255 0 0 | Draws Pixel directly to the screen
DRAW PICTURE | {PREFIX}/cmnd/dpic | {POSX} {POSY} {LEN}~{base64data} | 0 0 512~data:image/png;base64,... | Draws Picture directly from Network Stream
UPLOAD PICTURE | {PREFIX}/cmnd/upic | {SLOT} {LEN}~{base64data} | 0 512~data:image/png;base64 | Uploads Picture to SPIFF of ESP
LOAD PICTURE FROM SPIFFS | {PREFIX}/cmnd/lpic | {SLOT} {POSX} {POSY} | 1 0 0
DRAW TEXT | {PREFIX}/cmnd/dtxt | {POSX} {POSY} {FORGROUND_COLOR} {BACKGROUND_COLOR} {SIZE} {LEN}~{PAYLOAD} | 0 10 #00FF00 #000000 1 5~Hello | Draws a text to the screen


