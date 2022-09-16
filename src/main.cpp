/**************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file main.cpp
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 基于 ESP32 Arduino 的神之眼主程序
 *        依赖库文件: Button2, LovyanGFX
 * 当来自赛利康世界的烽瞳 穿越进了提瓦特, 
 * 还获得了一颗神之眼, 随即用"赛利康科技"改造这颗
 * 神之眼的故事...
 * @version 1.0
 * @date 2022-09-16
 * @copyright Copyright (c) 2022 FriendshipEnder
 * 
 * Apache License, Version 2.0
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "MJPEGDEC.h"
#include "eyeOS.h"
#include "jpegDraw.h"

#define START_PIC "/pictures/start.jpg"

LGFX ips;
eyeOSesp32 eyeOS(ips);

//extern WiFiServer dbg_server;
//extern WiFiClient dbg;

MJPEGClass mjpeg = MJPEGClass(jpegDrawCB);
void drawGradientJpg(const char *oldJpg, const char *newJpg);
String mkPath(uint8_t c);

uint8_t elLastPlay=0;  //上次正在播放的元素
uint8_t elPlay=0;      //正在播放的元素
uint8_t elLoops=0;     //参与循环的所有元素个数

void setup()
{
  Serial.begin(115200);
  eyeOS.init(0);
  ips.setTextColor(0x0000,0xffff);
  bool drawJpgStatus = mjpeg.drawJpg(START_PIC,0,0);
  if(!drawJpgStatus){
    eyeOS.loadFont(MFONT_FILENAME);
    ips.fillScreen(0xffff);
    ips.drawString("EyeOS V" EYEOS_VERSION,55,80);
    ips.setCursor(52,100);
    ips.print("神之眼 ");
    ips.println(eyeOS.getType()?"ST7789":"GC9A01");
  }
  for(int i=0;i<256;i++){
    ips.setBrightness(i);
    delay(5);
  }
  eyeOS.loadFont(MFONT_FILENAME);
  if(eyeOS.readButtonRaw() == 0){
    ips.drawCentreString("WiFi 正在连接...",120,130);
    if(eyeOS.connectWifiGUI()) {
      ips.fillRect(0,130,240,20,0xffff);
      ips.drawCentreString("WiFi 连接成功",120,130);
      ips.setCursor(40,150);
      ips.print(WiFi.localIP());
      //if(digitalRead(BTN_PIN) == 0) 
      //eyeOS.entryArduinoOTA_GUI(60000,"chip is in OTA download mode");

      //打开服务器和OTA服务器
      ips.drawCentreString("ArduinoOTA OK",120,170);
      eyeOS.initArduinoOTA();
      eyeOS.initWebServer();
      delay(1000);
    }
    setJpegBuflen16Bit(0);
  }
  else {
    eyeOS.offWifi();
    setJpegBuflen16Bit(1);
  } //不使用WiFi时就关掉咯

  eyeOS.unloadFont();
  //ips.fillSmoothCircle(120,220,16,TFT_MAROON);

  Serial.printf("gx: %c, %d\n\n",eyeOS.getPlayOrder(elPlay,&elLoops),eyeOS.getPlayTimeMs());
  while(eyeOS.readButtonRaw() == 0) delay(1);
  delay(20);
  if(drawJpgStatus) drawGradientJpg(START_PIC,mkPath(elPlay).c_str());
  setDrawNormal();
  eyeOS.readButton(); //清除上次按键的状态
}

void loop()
{
  uint32_t lastMicros;
  uint32_t stillingTime;
  uint32_t playTimeMs = eyeOS.getPlayTimeMs();
  File vFile;
  vFile = SD.open(mkPath(elPlay).c_str());
  mjpeg.setupMJpeg(&vFile, 0, 0, 240, 240, 8192, [](){eyeOS.lockSPI();}, [](){eyeOS.unlockSPI();});
  lastMicros = micros();
  stillingTime = millis();
  while (mjpeg.readMjpegBuf()){
    auto btnState = eyeOS.readButton();
    mjpeg.drawMJpegFrame();
    if(btnState == eyeOSesp32::buttonClicked || (playTimeMs && millis()-stillingTime>=playTimeMs)){
      Serial.printf("button clicked. Memory remain: %d\r\n",esp_get_free_heap_size());
      eyeOS.getPlayOrder(elPlay,&elLoops); //更新 elLoops
      if(elPlay<elLoops-1) elPlay++;
      else elPlay = 0;
      break;
    }
    else if(btnState == eyeOSesp32::buttonDoubleClicked){
      Serial.printf("button double clicked. Memory remain: %d\r\n",esp_get_free_heap_size());
      eyeOS.getPlayOrder(elPlay,&elLoops); //更新 elLoops
      if(elPlay) elPlay--;
      else elPlay = elLoops-1;
      break;
    }
    else if(btnState == eyeOSesp32::buttonLongClicked){
      Serial.println("button long clicked. Will restart.");
      for(int i=0;i<256;i++){
        ips.setBrightness(255-i);
        delay(1);
      }
      esp_restart(); //在单击时复位系统
    }
    //eyeOS.runArduinoOTA();
    //eyeOS.loopWebServer();
    uint32_t frameTime=micros()-lastMicros;
    Serial.println(frameTime);
    lastMicros+=frameTime;
    taskYIELD();
  }        
  Serial.println("mjpeg done!");
  if(elLastPlay != elPlay){
    drawGradientJpg("", mkPath(elPlay).c_str());
    if(elPlay>=elLoops) elPlay=elLoops-1;
    elLastPlay = elPlay;
  }
  mjpeg.endMJpeg();
  vFile.close();
}
void drawGradientJpg(const char *oldJpg, const char *newJpg){
  if(!setDrawJPGCover(oldJpg,99)) {
    freeJPGCoverBuffer();
    return;
  }
  for(int i=1;i<20;i++){
    setAlphamask(i*5);
    mjpeg.drawJpg(newJpg,0,0);
  }
  setDrawJPGCover("/pictures/alpha.jpg",eyeOS.getPlayAlpha());
}

String mkPath(uint8_t c){
  return String("/mjpeg/")+eyeOS.getPlayOrder(c,&elLoops)+".mjpeg";
}