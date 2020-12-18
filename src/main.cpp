#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "base64.hpp" 
#include "upng.h"
#include "processor.h"

#define CONFIG_ARDUINO_LOOP_STACK_SIZE 16384

MatrixPanel_I2S_DMA dma_display;
WiFiServer server(80);
const char* SSID = "XXX";
const char* PSK = "XXX";

//COMMANDS
const char* DPX_CMD = "DPX";
const char* DPIC_CMD = "DPIC";
//END

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
    Serial.println("HELLO WORLD");
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


  setup_display();
 
}

void handle_dpx(const char* args)
{
  Serial.println("FOUND");
  int pos_x,pos_y;
  int col_r,col_b,col_g;
  if(sscanf(args,"%d %d %d %d %d",&pos_x,&pos_y,&col_r,&col_b,&col_g) > 0){
    dma_display.drawPixelRGB888(pos_x,pos_y,col_r,col_g,col_b);
  }
}

void handle_dpicx( char* args)
{
  int pos_x,pos_y, blen;

  char* arg_ptr=nullptr;

  arg_ptr=strtok(args," ");
  if(arg_ptr == NULL) return;
  Serial.println(arg_ptr);
  pos_x = atoi(arg_ptr);

  arg_ptr=strtok(NULL," ");
  if(arg_ptr == NULL) return;
  Serial.println(arg_ptr);
  pos_y = atoi(arg_ptr);

  arg_ptr=strtok(NULL," ");
  if(arg_ptr == NULL) return;
  Serial.println(arg_ptr);
  blen = atoi(arg_ptr);

  arg_ptr=strtok(NULL," ");
  if(arg_ptr == NULL) return;
 
  int header_size=sizeof("data:image/png;base64,");
  arg_ptr=arg_ptr+(header_size-1);
  char b64txt[blen-header_size+2];
  //memcpy(b64txt,arg_ptr,sizeof(b64txt));
  strncpy(b64txt,arg_ptr,sizeof(b64txt));
  Serial.println(String(b64txt));

  unsigned char buffer[512];
  unsigned int blength = decode_base64((unsigned char*)b64txt, buffer);
  Serial.print("DECODE LEN:");
  Serial.println(blength);

  upng_t* upng = upng_new_from_bytes(buffer,blength);
  if(upng_decode(upng) == UPNG_EOK)
  {
    Serial.println("PNG Could be decoded");
    unsigned int width = upng_get_width(upng);
    unsigned int height= upng_get_height(upng);
    uint16_t *rgb_bmp=process_to_rg566bmp(upng_get_buffer(upng),width,height);
    if (rgb_bmp != NULL)
    {
      dma_display.drawRGBBitmap(pos_x,pos_y,rgb_bmp,width,height);
      free(rgb_bmp);
    }
  }
  free(upng);
  arg_ptr=nullptr;

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
        handle_dpx(args_ptr);
    }

    if(strcmp(DPIC_CMD,cmd_ptr) == 0) {
       char* args_ptr=cmd_ptr+5;
       handle_dpicx(args_ptr);
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