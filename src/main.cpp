
#include <WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Fonts/Picopixel.h>
#include <PubSubClient.h>
//OWN LIBS
#include "base64.hpp" 
#include "upng.h"
#include "processor.h"
#include "config.h"
#include "commands.h"


MatrixPanel_I2S_DMA dma_display;
WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);
char SUBSCRIBE_CMND[255];
char STATUS_TOPIC[255];

void handle_dpx(const char* args)
{
  int pos_x,pos_y;
  int col_r,col_b,col_g;
  if(sscanf(args,"%d %d %d %d %d",&pos_x,&pos_y,&col_r,&col_b,&col_g) < 1){
    mqtt_client.publish(STATUS_TOPIC,"ERR PARAMS PARSING FAILED");
    return;
  }

  dma_display.drawPixelRGB888(pos_x,pos_y,col_r,col_g,col_b);
}

void handle_lpic(char* args){
  int slot,pos_x,pos_y;

   if(sscanf(args,"%d %d %d",&slot,&pos_x,&pos_y) < 1){
    mqtt_client.publish(STATUS_TOPIC,"ERR PARAMS PARSING FAILED");
    return;
  }

  if(slot < 0 || slot > MAX_SLOTS){
      mqtt_client.publish(STATUS_TOPIC,"ERR SLOT OUT OF RANGE");
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
    mqtt_client.publish(STATUS_TOPIC,"ERR FAILED TO DECODE");
  }
  if(upng != nullptr) {
    free(upng);
    upng = nullptr;
  }

}
void handle_dtxt(char* args){
  char* arg_ptr=nullptr;
  int pos_x,pos_y,fsize,blen;
  char fcol[8];
  char bcol[8];

  arg_ptr=strtok(args,"~");
  if(arg_ptr == NULL){
    mqtt_client.publish(STATUS_TOPIC,"ERR TEXT NOT PROVIDED");
    return;
  } 

  if(sscanf(arg_ptr,"%d %d %s %s %d %d",&pos_x,&pos_y,&fcol,&bcol,&fsize,&blen) < 1){
    mqtt_client.publish(STATUS_TOPIC,"ERR PARAMS PARSING FAILED");
    return;
  }


  if(fsize <= 1) dma_display.setFont(&Picopixel);
  arg_ptr = strtok(NULL,"~");
  if(arg_ptr == NULL){
    mqtt_client.publish(STATUS_TOPIC,"ERR TEXT PARSING FAILED");
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

void handle_upic(char* args){

  char* arg_ptr=nullptr;
  int slot,pic_len;

  arg_ptr=strtok(args,"~");
  if(arg_ptr == NULL){
    mqtt_client.publish(STATUS_TOPIC,"ERR PICTURE MISSING");
    return;
  } 

  if(sscanf(arg_ptr,"%d %d",&slot,&pic_len) < 1){
    mqtt_client.publish(STATUS_TOPIC,"ERR PARAMS PARSING FAILED");
    return;
  }

  if(slot < 0 || slot > MAX_SLOTS){
      mqtt_client.publish(STATUS_TOPIC,"ERR SLOT OUT OF RANGE");
      return;
  }
  //DATA HEADER IS MINIMUM 23 ASCII CHARS
  if(pic_len < 23) {
    mqtt_client.publish(STATUS_TOPIC,"ERR LENGTH INVALID");
    return;
  }

  arg_ptr=strtok(NULL,"~");
  if(arg_ptr == NULL){
     mqtt_client.publish(STATUS_TOPIC,"ERR PICTURE DATA PARSING");
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
    mqtt_client.publish(STATUS_TOPIC,"OK");
    return;
  }
  else{
    String slot_err = "ERR WRITING TO SLOT "+String(slot);
    mqtt_client.publish(STATUS_TOPIC,slot_err.c_str());
    return;
  }

}


void handle_dpic( char* args)
{
  int pos_x,pos_y, blen;

  char* arg_ptr=nullptr;

  arg_ptr=strtok(args,"~");
  if(arg_ptr == NULL){
    mqtt_client.publish(STATUS_TOPIC,"ERR PICTURE DATA MISSING");
    return;
  } 

  if(sscanf(arg_ptr,"%d %d %d",&pos_x,&pos_y,&blen) < 1){
    mqtt_client.publish(STATUS_TOPIC,"ERR PARAMS PARSING FAILED");
    return;
  }

  if(blen < 1){
    mqtt_client.publish(STATUS_TOPIC,"ERR PICTURE LENGTH IS SMALLER THAN 1");
    return;
  }

  arg_ptr=strtok(NULL,"~");
  if(arg_ptr == NULL){
    mqtt_client.publish(STATUS_TOPIC,"ERR PARSING PICTURE DATA");
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
    mqtt_client.publish(STATUS_TOPIC,"ERR PNG DECODING FAILED");
  }

  if(upng != nullptr) {
    free(upng);
    upng = nullptr;
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char* command = strrchr(topic, '/');
  command++;
  String str_cmd = String(command);
  str_cmd.toUpperCase();
  Serial.print("COMMAND: ");
  Serial.println(str_cmd);

  if(str_cmd == DPX_CMD) {
        handle_dpx((char *)payload);
  }

  if(str_cmd == DPIC_CMD) {
       handle_dpic((char *)payload);
  }

  if(str_cmd == UPIC_CMD) {
       handle_upic((char *)payload);
  }

  if(str_cmd == LPIC_CMD) {
       handle_lpic((char *)payload);
  }
  if(str_cmd == DTXT_CMD) {
       handle_dtxt((char *)payload);
  }

}




void wifi_setup()
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
}

void mqtt_setup(){
  mqtt_client.setServer(MQTT_HOST,MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  sprintf(SUBSCRIBE_CMND,"%s/cmnd/+",MQTT_PREFIX);
  sprintf(STATUS_TOPIC,"%s/status",MQTT_PREFIX);
}

void mqtt_reconnect(){

  bool connected=false;

  while(!connected)
  {
  Serial.println("Try to connect to MQTT");
  #ifndef MQTT_PASSWORD_AUTH
    connected = mqtt_client.connect(MQTT_IDENT)
  #else
    connected = mqtt_client.connect(MQTT_IDENT,MQTT_USER,MQTT_PASSWORD);
  #endif
    if(!connected) delay(1000);
  }
  Serial.println("Connected to MQTT");

  mqtt_client.subscribe(SUBSCRIBE_CMND);
  Serial.print("Subscribed: ");
  Serial.println(SUBSCRIBE_CMND);

}

void display_setup(){
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
  wifi_setup();

   if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  mqtt_setup();
  mqtt_reconnect();
  display_setup();
  
 
}


void loop() {
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();
}

