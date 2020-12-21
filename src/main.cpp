
#include <WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Fonts/Picopixel.h>
//OWN LIBS
#include "base64.hpp" 
#include "upng.h"
#include "processor.h"
#include "config.h"
#include "commands.h"


MatrixPanel_I2S_DMA dma_display;
WiFiServer server(TCP_SERVER_PORT);




void setup_wifi()
{
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(SSID);
 
    WiFi.begin(SSID, PSK);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
}

void setup_display(){
    dma_display.begin(); 
    
  // fix the screen with green
    dma_display.fillRect(0, 0, dma_display.width(), dma_display.height(), dma_display.color444(255, 0, 0));
    delay(500);


    // draw a box in yellow
    dma_display.fillRect(0, 0, dma_display.width(), dma_display.height(), dma_display.color444(0, 255, 0));
    delay(500);

    dma_display.fillRect(0, 0, dma_display.width(), dma_display.height(), dma_display.color444(0, 0, 255));
    delay(500);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();

   if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  setup_display();
 
 
}

void handle_dpx(const char* args,WiFiClient* client)
{
  int pos_x,pos_y;
  int col_r,col_b,col_g;
  if(sscanf(args,"%d %d %d %d %d",&pos_x,&pos_y,&col_r,&col_b,&col_g) < 1){
    client->println("ERR PARAMS PARSING FAILED ");
    return;
  }

  if(pos_x < 0){
      client->println("ERR POS_X is negative");
      return;
  }

  if(pos_y < 0){
      client->println("ERR POS_Y is negative");
      return;
  }

  dma_display.drawPixelRGB888(pos_x,pos_y,col_r,col_g,col_b);
}

void handle_lpic(char* args,WiFiClient* client){
  int slot,pos_x,pos_y;

   if(sscanf(args,"%d %d %d",&slot,&pos_x,&pos_y) < 1){
    client->println("ERR PARAMS PARSING FAILED ");
    return;
  }

  if(slot < 0 || slot > MAX_SLOTS){
      client->println("ERR SLOT OUT OF RANGE");
      return;
  }

  if(pos_x < 0){
      client->println("ERR POS_X is negative");
      return;
  }

  if(pos_y < 0){
      client->println("ERR POS_Y is negative");
      return;
  }

  String path = "/slot"+String(slot)+".png";
  File file = SPIFFS.open(path, "r");
  char buffer[512];
  int bytes_read = file.readBytes(buffer,sizeof(buffer));
  file.close();
  upng_t* upng = upng_new_from_bytes((unsigned char*)buffer,bytes_read);
  if(upng_decode(upng) == UPNG_EOK)
  {
    unsigned int width = upng_get_width(upng);
    unsigned int height= upng_get_height(upng);
    uint16_t *rgb_bmp=process_to_rg566bmp(upng_get_buffer(upng),width,height);
    if (rgb_bmp != NULL)
    {
      dma_display.drawRGBBitmap(pos_x,pos_y,rgb_bmp,width,height);
      free(rgb_bmp);
    }
  }
  else{
    //FORCES CLEAR OF UPNG
    free(upng);
    upng = nullptr;
    client->println("ERR FAILED TO DECODE");
  }
  if(upng != nullptr) {
    free(upng);
    upng = nullptr;
  }

}
void handle_dtxt(char* args,WiFiClient* client){
  char* arg_ptr=nullptr;
  int pos_x,pos_y,fsize,blen;
  char fcol[8];
  char bcol[8];

  arg_ptr=strtok(args,"~");
  if(arg_ptr == NULL){
    client->println("ERR PARAMS INVALID");
    return;
  } 

  if(sscanf(arg_ptr,"%d %d %s %s %d %d",&pos_x,&pos_y,&fcol,&bcol,&fsize,&blen) < 1){
    client->println("ERR PARAMS PARSING FAILED ");
    return;
  }

  if(pos_x < 0){
      client->println("ERR POS_X is negative");
      return;
  }

  if(pos_x < 0){
        client->println("ERR POS_Y is negative");
        return;
  }



  if(fsize <= 1) dma_display.setFont(&Picopixel);
  arg_ptr = strtok(NULL,"~");
   if(arg_ptr == NULL){
    client->println("ERR PARAMS INVALID");
    return;
  }

  uint16_t text_color = hex2rgb565(fcol);
  uint16_t bg_color = hex2rgb565(bcol);

  char strbuf[blen+1];
  strncpy(strbuf,arg_ptr,blen+1);
  strbuf[blen]='\0';

  int16_t  x1, y1;
  uint16_t w, h;

  dma_display.setCursor(pos_x,pos_y);
  dma_display.getTextBounds(strbuf,pos_x,pos_y,&x1, &y1, &w, &h);
  dma_display.fillRect(x1,y1,w,h,bg_color);
  dma_display.setTextColor(text_color);
  dma_display.print(strbuf);

}

void handle_upic(char* args,WiFiClient* client){

  char* arg_ptr=nullptr;
  int slot,pic_len;

  arg_ptr=strtok(args,"~");
  if(arg_ptr == NULL){
    client->println("ERR PARAMS INVALID");
    return;
  } 

  if(sscanf(arg_ptr,"%d %d",&slot,&pic_len) < 1){
    client->println("ERR PARAMS PARSING FAILED ");
    return;
  }

  if(slot < 0 || slot > MAX_SLOTS){
      client->println("ERR SLOT OUT OF RANGE");
      return;
  }
  //DATA HEADER IS MINIMUM 23 ASCII CHARS
  if(pic_len < 23) {
    client->println("ERR LENGTH INVALID");
    return;
  }

  arg_ptr=strtok(NULL,"~");
  if(arg_ptr == NULL){
     client->println("ERR No Picture Data present");
     return;
  }

  int header_size=sizeof("data:image/png;base64,");
  arg_ptr=arg_ptr+header_size-1;
  char b64txt[pic_len-header_size+2];
  strncpy(b64txt,arg_ptr,sizeof(b64txt));
  b64txt[pic_len-header_size+1]='\0';
  
  unsigned char buffer[512];
  unsigned int blength = decode_base64((unsigned char*)b64txt, buffer);
  String path = "/slot"+String(slot)+".png";
  File file = SPIFFS.open(path, "w");
  int bytes_written = file.write(buffer,blength);
  file.close();

  if(bytes_written > 0){
    client->println("OK");
    return;
  }
  else{
    client->println("ERR WRITING SLOT"+slot);
    return;
  }

}


void handle_dpic( char* args,WiFiClient* client)
{
  int pos_x,pos_y, blen;

  char* arg_ptr=nullptr;

  arg_ptr=strtok(args,"~");
  if(arg_ptr == NULL){
    client->println("ERR PARAMS INVALID");
    return;
  } 

  if(sscanf(arg_ptr,"%d %d %d",&pos_x,&pos_y,&blen) < 1){
    client->println("ERR PARAMS PARSING FAILED ");
    return;
  }

  if(pos_x < 0){
    client->println("ERR POS_X is negative");
    return;
  }

  if(pos_y < 0){
    client->println("ERR POS_Y is negative");
    return;
  }

  if(blen < 1){
    client->println("ERR BLEN is smaller than 1");
    return;
  }

  arg_ptr=strtok(NULL,"~");
  if(arg_ptr == NULL){
    client->println("ERR PARAMS INVALID");
    return;
  } 

  int header_size=sizeof("data:image/png;base64,");

  arg_ptr=arg_ptr+header_size-1;
  char b64txt[blen-header_size+2];
  strncpy(b64txt,arg_ptr,sizeof(b64txt));
  b64txt[blen-header_size+1]='\0';

  unsigned char buffer[512];
  unsigned int blength = decode_base64((unsigned char*)b64txt, buffer);

  upng_t* upng = upng_new_from_bytes(buffer,blength);
  if(upng_decode(upng) == UPNG_EOK)
  {
  
    unsigned int width = upng_get_width(upng);
    unsigned int height= upng_get_height(upng);
    uint16_t *rgb_bmp=process_to_rg566bmp(upng_get_buffer(upng),width,height);
    if (rgb_bmp != NULL)
    {
      dma_display.drawRGBBitmap(pos_x,pos_y,rgb_bmp,width,height);
      free(rgb_bmp);
    }
  }
  else{
    //FORCES CLEAR OF UPNG
    free(upng);
    upng = nullptr;
    client->println("ERR FAILED TO DECODE");
  }

  if(upng != nullptr) {
    free(upng);
    upng = nullptr;
  }
}

void handle_commands(WiFiClient* client){
    char buffer[1024];
    //int read = client->readBytesUntil('\n',buffer,sizeof(buffer));
    int read = client->readBytes(buffer,sizeof(buffer));
    if(read < 1) return;
    Serial.print("READ ");
    Serial.println(read);

    char* cmd_ptr = strtok(buffer," ");
    if(strcmp(DPX_CMD,cmd_ptr) == 0) {
        char* args_ptr=cmd_ptr+4;
        handle_dpx(args_ptr,client);
    }

    if(strcmp(DPIC_CMD,cmd_ptr) == 0) {
       char* args_ptr=cmd_ptr+5;
       handle_dpic(args_ptr,client);
    }

    if(strcmp(UPIC_CMD,cmd_ptr) == 0) {
       char* args_ptr=cmd_ptr+5;
       handle_upic(args_ptr,client);
    }

    if(strcmp(LPIC_CMD,cmd_ptr) == 0) {
       char* args_ptr=cmd_ptr+5;
       handle_lpic(args_ptr,client);
    }

    if(strcmp(DTXT_CMD,cmd_ptr) == 0) {
       char* args_ptr=cmd_ptr+5;
       handle_dtxt(args_ptr,client);
    }
}

void loop() {
  WiFiClient client = server.available();
  if (client){
    Serial.println("Client connected");
    while (client.connected()){
        if (client.available())
        {
           Serial.println("Client sending data");
              handle_commands(&client);
        }
      
    }
    client.stop();
    Serial.println("[Client disconnected]");
  }
  
}