/**************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file eyeOS.cpp
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 神之眼功能的核心代码, 含WiFi连接程序的图形化部分界面
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

#include "eyeOS.h"
#pragma GCC optimize ("O3")

void eyeOS_WiFiButtonServiceTask(void * _this);
void eyeOSesp32::init(uint8_t initBrightness){
  //此时, 占用系统资源的那些任务还没有被创建出来
  pinMode(_DEFINA_IPS_BL_PIN,OUTPUT); //初始化屏幕背光
  digitalWrite(_DEFINA_IPS_BL_PIN,LOW); //默认为低电平
  pinMode(EYEOS_BTN_PIN_ROUND,INPUT_PULLUP); //初始化圆形神之眼的按键引脚
  pinMode(EYEOS_BTN_PIN_LIYUE,INPUT_PULLUP); //初始化方形神之眼的按键引脚
  nvsData.begin(eyeOSconfig_Tag,false); //初始化NVS

  if(nvsData.isKey(eyeOSdeviceType_Tag)){//键名 device type 存储的其实是板载按键的引脚号
    wakeInfo = nvsData.getUChar(eyeOSdeviceType_Tag); //读取NVS, 获取方式为uint8_t即unsigned char
    if(wakeInfo == EYEOS_BTN_PIN_ROUND && digitalRead(EYEOS_BTN_PIN_LIYUE) == 0){
      //获取到信息为方形, 但是读取到其实是圆形
      wakeInfo = EYEOS_BTN_PIN_LIYUE; //更新数据为圆形
      nvsData.putUChar(eyeOSdeviceType_Tag,wakeInfo); //写入NVS
    }
    else if(wakeInfo == EYEOS_BTN_PIN_LIYUE && digitalRead(EYEOS_BTN_PIN_ROUND) == 0){
      //获取到信息为圆形, 但是读取到其实是方形
      wakeInfo = EYEOS_BTN_PIN_ROUND; //更新数据为方形
      nvsData.putUChar(eyeOSdeviceType_Tag,wakeInfo); //写入NVS
    }
  }
  while(!wakeInfo){  //未能识别到设备信息, 一直读取按键, 直到读到有按键按下
    if(digitalRead(EYEOS_BTN_PIN_LIYUE) == 0){ // 方形的开机键按下了
      wakeInfo= EYEOS_BTN_PIN_LIYUE; //更新数据为方形神之眼
      nvsData.putUChar(eyeOSdeviceType_Tag,wakeInfo); //写入NVS
    }
    //if(wakeInfo == EYEOS_BTN_PIN_ROUND || digitalRead(EYEOS_BTN_PIN_ROUND) == 0)
    else if(digitalRead(EYEOS_BTN_PIN_ROUND) == 0){ // 圆形的开机键按下了
      wakeInfo= EYEOS_BTN_PIN_ROUND; //更新数据为圆形神之眼
      nvsData.putUChar(eyeOSdeviceType_Tag,wakeInfo); //写入NVS
    }
  }
  //Serial.printf("wakeInfo: %d\n",wakeInfo);
  if(nvsData.isKey(eyeOSplayOrder_Tag)){
    nvsData.getString(eyeOSplayOrder_Tag,playOrder,8);
    playShowTime = nvsData.getUInt(eyeOSplayShowTime_Tag);
    playAlpha = nvsData.getUChar(eyeOSplayAlpha_Tag);
  }
  else{
    strcpy(playOrder,"hsflcby");
    playShowTime = 10000;
    playAlpha = 75;
  }
  nvsData.end(); //关闭NVS

  uint8_t fileErr = 0; //标记文件系统工作状态, 0 正常 非0 异常
  const char *reason_string[2]={
    "NO SD card detected!","NO Mjpeg or font file!"
  };
  SPI.begin(_DEFINA_IPS_SCK_PIN, _DEFINA_IPS_MISO_PIN, _DEFINA_IPS_MOSI_PIN); //开启SPI总线
  ips->setDisplayPanel(wakeInfo == EYEOS_BTN_PIN_LIYUE); //根据按键型号, 初始化显示
  ips->init(); //初始化显示屏, 默认方式就可以了
  ips->setBrightness(initBrightness); //设置在初始化阶段的屏幕亮度
  //ips->setColorDepth(24);  // RGB888の24ビットに設定 设置颜色深度, (但是在这里似乎用不到)
  ips->setColorDepth(16);
  ips->setTextColor(0xffff); //设置文本默认颜色
  ips->setFont(&fonts::Font0); //设置文本默认字体, 默认字体应该是original Adafruit Font 0
  //ips->drawString("EyeOS V0.1",100,80);
  //ips->drawString("For Inazuma",97,100);

  //-------------------------------------------------创建按键对象, 按键现在是共享资源
  while (btn_lock == nullptr) {//直到创建成功
    //Serial.println("btn_lock Mutex created");
    btn_lock = xSemaphoreCreateMutex(); //创建按键的共享锁
  }
  xSemaphoreGive(btn_lock); //初始可用
  //-------------------------------------------------创建WiFi对象, WiFi现在是共享资源
  while (server_lock == nullptr) {//直到创建成功
    server_lock = xSemaphoreCreateRecursiveMutex();
  }
  xSemaphoreGiveRecursive(server_lock); //初始可用

  //创建NVS资源访问锁, 防止多线程同时访问NVS flash导致崩溃
  while(nvs_lock == nullptr) {
    nvs_lock = xSemaphoreCreateMutex(); //初始状态是不可用的
  }
  xSemaphoreGiveRecursive(nvs_lock);

  //创建SD卡锁, 防止多线程同时访问SD卡导致崩溃
  while(SDCard_lock == nullptr) {
    SDCard_lock = xSemaphoreCreateRecursiveMutex(); //初始状态是不可用的
  }
  xSemaphoreGiveRecursive(SDCard_lock);

  for(;;){
    if( !SD.begin(_DEFINA_SD_CS_PIN,SPI,16000000) ){ //初始化SD卡, 未检测到
      fileErr=1;
    }
    else if( !CheckFilesDependency()){ //初始化文件存在性, 但是有的文件未找到
      fileErr=2;
    }
    else break;
    ips->setBrightness(255);
    ips->fillScreen(0x0000);
    //ips->setCursor(65,95);
    //ips->print("NO SD card detected!");
    ips->drawCentreString(reason_string[fileErr-1],120,95);
    while(!connectWifiGUI()) { //进入ota模式, 只能使用ArduinoOTA, 所以必须连接WiFi
      if(readButtonRaw() == 0) esp_restart();
      taskYIELD();
    }
      entryArduinoOTA_GUI(0,reason_string[fileErr-1]);
      /*
      initArduinoOTA();
      while(1){
        runArduinoOTA();
        delay(10);
        uint8_t nowRead = digitalRead(BTN_PIN);
        if(nowRead == 0 && lastRead) break;
        lastRead = nowRead;
      }
      */
      ips->fillScreen(0x0000);
      delay(400);
  }
  //程序运行至此, SD卡应当是可用的
  //初始化一些多任务的功能了吧...

  loopTaskHandler = xTaskGetCurrentTaskHandle(); //获取Arduino任务的Handler
  //while(btnCmdQueue == nullptr)
  //  btnCmdQueue = xSemaphoreCreateBinary();
  while(btnQueue == nullptr)
    btnQueue = xQueueCreate(1,sizeof(buttonResult_t));
  //创建任务, 用于为WiFi和按键事项提供服务
  xTaskCreatePinnedToCore(eyeOS_WiFiButtonServiceTask,"eyeOSWiFiButton",
    4096, this, 2, &eyeOS_WiFiButtonServiceTaskHandler, 0);
}
void eyeOSesp32::entryArduinoOTA_GUI(uint32_t maxTime, const char *msg){
  if(!WiFi.isConnected()) return; //WiFi未连接
  uint8_t lastRead = 0;
  initArduinoOTA();
  initWebServer(1);
  //for(;;){
    //if(digitalRead(BTN_PIN)==LOW){
    if(msg!=nullptr){
      //ips->setBrightness(255);
      lockSPI();
      ips->fillScreen(0);
      ips->setTextColor(0xffff);
      ips->setFont(&fonts::Font0);
      ips->drawCentreString(msg,120,95);
      ips->drawCentreString("WiFi Connected!",120,105);
      ips->drawCentreString("ArduinoOTA Start. Use \"espota\"",120,115);
      ips->setCursor(60,135);
      ips->print(WiFi.localIP());
      ips->print(":3232");
      ips->drawCentreString("or set host \"eyeos.local\" to upload",120,125);
      ips->drawCentreString("eyeOS " EYEOS_VERSION,120,12);
      ips->drawCentreString("By FriendshipEnder",120,22);
      ips->setTextColor(0xf800);
      ips->setTextColor(0xffff);
      unlockSPI();
    }
    Serial.print(WiFi.localIP());
    uint32_t entryMillis = millis();
    while(!maxTime || millis()-entryMillis<=maxTime){
      runArduinoOTA();
      loopWebServer();
      taskYIELD();
      uint8_t nowRead = readButtonRaw();
      if(nowRead == 0 && lastRead) break;
      lastRead = nowRead;
    }
  //}
  //if(lastRead) break;
  //delay(20);
  //}
  if(!maxTime) esp_restart(); //永远循环的时候, 按按键即可复位
  deinitArduinoOTA();
  deinitWebServer();
}

//此函数需要同时占用SD卡和WiFI
uint8_t eyeOSesp32::connectWifiGUI(){
  uint8_t _wifiConfig=getWiFiConfig();
  if((_wifiConfig&1)) return 2;
  //ips->drawCentreString("正在连接WiFi...",120,120);
  lockSPI();
  auto ifont = ips->getFont();
  unlockSPI();
  uint8_t lang = 0;
  const char * textdt[]={
    "connect to AP WiFi","手机连接WiFi",
    "SSID: " EYEOS_AP_SSID, "名称: " EYEOS_AP_SSID,
    "password: " EYEOS_AP_PASS,"密码: " EYEOS_AP_PASS,
    "Scan to connect.","使用手机WiFi扫码连接",
    "Open in broswer.","手机打开",
    "192.168.4.1","http://192.168.4.1",
    "SSID updated. Will connect.","SSID 已更新!正在连接...",
    EYEOS_VERSION "by FriendshipEnder.","eyeOS " EYEOS_VERSION
  };
  while(readButtonRaw() == 0) delay(1); //等待松开按键
  delay(20);
  while(!connectToWifi()){
    lockSPI();
    if(ifont->getType() == lgfx::v1::IFont::ft_vlw || ifont->getType() == lgfx::v1::IFont::ft_u8g2){
      lang = 1;
    }
    ips->fillScreen(0xffff);
    ips->setTextColor(TFT_RED,0xffff);
    ips->drawCentreString(textdt[lang],120,26);
    unlockSPI();
    initWiFiConfigServer();
    lockSPI();
    ips->setTextColor(TFT_DARKGREEN,0xffff);
    ips->drawCentreString(textdt[2+lang],120,46);
    ips->setTextColor(TFT_BLUE,0xffff);
    ips->drawCentreString(textdt[4+lang],120,66);
    ips->qrcode("WIFI:S:" EYEOS_AP_SSID ";T:WPA;P:" EYEOS_AP_PASS ";;",60,106,120);
    ips->setTextColor(TFT_OLIVE,0xffff);
    ips->drawCentreString(textdt[6+lang],120,89);
    unlockSPI();
    if(!lockWiFi(EYEOS_WIFI_WAIT_TICKOUT)) return 0;
    delay(3000);
    while(!WiFi.softAPgetStationNum()) {
      taskYIELD();
      if(readButtonRaw() == 0) {
        ips->setTextColor(0,0xffff);
        ips->setFont(ifont);
        WiFi.mode(WIFI_OFF);
        unlockWiFi();
        return 0;
      }
    }
    unlockWiFi();
    //ips->qrcode(String("http://")+textdt[4],60,108,120);
    lockSPI();
    ips->fillScreen(0xffff);
    ips->setTextColor(TFT_RED,0xffff);
    ips->drawCentreString(textdt[8+lang],120,30);
    ips->qrcode(textdt[11],60,90,120); //网址二维码
    ips->setTextColor(TFT_DARKCYAN,0xffff);
    ips->drawCentreString(textdt[10],120,51);
    ips->drawCentreString(textdt[14+lang],120,72);
    unlockSPI();
    Serial.println("Server Init.");
    while(!loopWiFiConfigServer()) {
      taskYIELD();
      if(readButtonRaw() == 0) {
        ips->setTextColor(0,0xffff);
        ips->setFont(ifont);
        lockWiFi();
        WiFi.mode(WIFI_OFF);
        unlockWiFi();
        return 0;
      }
    }
    lockSPI();
    ips->fillRect(60,90,120,120,0xffff);
    ips->setTextColor(TFT_DARKGREEN,0xffff);
    ips->drawCentreString(textdt[12+lang],120,120);
    unlockSPI();
    delay(300); //等待 "保存成功" 发送完
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //initArduinoOTA();
  ips->setTextColor(0,0xffff);
  ips->setFont(ifont);
  return 1;
}

uint8_t eyeOSesp32::readButtonRaw(){
  uint8_t rd = 1;
  if(xSemaphoreTake(btn_lock,5) == pdTRUE){ //此操作需要访问Button
    rd = digitalRead(wakeInfo);
    xSemaphoreGive(btn_lock);
  }
  return rd;
}
eyeOSesp32::buttonResult_t eyeOSesp32::readButton(uint32_t maxDelayTick){
  //此函数不会逝用互斥锁, 完全不会占用任何共享的资源
  eyeOSesp32::buttonResult_t rd=eyeOSesp32::buttonNoEvent;
  if(xQueueReceive(btnQueue,&rd,maxDelayTick) == pdFALSE){
    //队列为空, 返回空白事件
    rd = buttonNoEvent;
  }
  return rd;
}

//这样loadfont可能会有些慢速
bool eyeOSesp32::loadFont(const char* fn){
  bool fl_notOk=0;
  size_t fsize=0;
  fs::File fl;
  if(cn_font_vlw_SDfile != nullptr){//已经加载字体了, 需要卸载
    heap_caps_free(cn_font_vlw_SDfile);
  }
  if(xSemaphoreTakeRecursive(SDCard_lock, portMAX_DELAY)==pdTRUE){
    fl = SD.open(fn,"r");
    fsize = fl.size();
    fl_notOk = (!fl || fsize > 0x1ffff);
    xSemaphoreGiveRecursive(SDCard_lock);
  }
  if(fl_notOk) return false; //单个字体文件不能超过128KB
  if(fsize&0x03){
    fsize>>=2;
    fsize++;
    fsize<<=2;
  }
  Serial.printf("Need %d, available %d bytes\n",fsize,esp_get_free_heap_size());
  cn_font_vlw_SDfile = (uint8_t *)heap_caps_malloc(fsize,MALLOC_CAP_8BIT);
  if(cn_font_vlw_SDfile == nullptr) {
    Serial.println("no memory! cannot set font!");
    return false;
  }
  if(xSemaphoreTakeRecursive(SDCard_lock, portMAX_DELAY)==pdTRUE){
    fl.read(cn_font_vlw_SDfile,fsize);
    fl.close();
    fl_notOk = ips->loadFont(cn_font_vlw_SDfile); //若一切ok, 那么就直接加载进图形库内
    xSemaphoreGiveRecursive(SDCard_lock);
  }
  return fl_notOk;
}

void eyeOSesp32::unloadFont(){
  ips->unloadFont();
  heap_caps_free(cn_font_vlw_SDfile);
  cn_font_vlw_SDfile=nullptr;
}
static const char TEXT_HTML[] PROGMEM = "text/html";
const char* eyeOSesp32::webpage_html_start = "\
<!DOCTYPE html>\
<html lang=\'zh-cn\'>\
<head>\
<meta charset=\'UTF-8\'>\
<title>神之眼WiFi配网</title>\
</head>\
<body>\
<h1>欢迎进入神之眼WiFi配置页面</h1>\
<p><i>我是<b>烽瞳</b>, 叫我小烽就好。<br/>\
在提瓦特大陆就没有无线连接了呐。<br/>\
帮小烽找个WiFi好么, 可以么\? 可以么可以么\?\?<br/>\
求求你了求求你了邦邦小烽吧 (诚恳) !!</i></p>\
<form name=\'input\' action=\'/\' method=\'POST\'>\
wifi名称:<br/>\
<input type=\'text\' name=\'ssid\' maxlength=\"31\"/><br/>\
wifi密码:<br/>\
<input type=\'text\' name=\'password\' maxlength=\"31\"/><br/>\
<input type=\'submit\' value=\'保存'/>\
</form>\
<p>";
const char* eyeOSesp32::webpage_html_update = 
"戳一下<a href=\"/update\">介个链接</a>可以跳转至我的OTA更新页面哟~ ~ ~<br/>";
const char* eyeOSesp32::webpage_html_edit = 
"戳一下<a href=\"/edit\">介个链接</a>可以跳转至我的SD卡编辑器页面哟~ ~ ~<br/>";
const char* eyeOSesp32::webpage_html_order = 
"戳一下<a href=\"/order\">介个链接</a>可以跳转至神之眼显示顺序编辑器页面哟~ ~ ~<br/>";
const char* eyeOSesp32::webpage_html_mid = 
"基于 <a href=\"https://www.freertos.org\">FreeRTOS</a> 构建<br/>\
呵呵, 感谢固件作者: <a href=\"https://space.bilibili.com/180327370\">\
FriendshipEnder (Bilibili同名)</a> 赐予我灵魂<br/>\
以及神之眼硬件作者: <a href=\"https://space.bilibili.com/14958846\">\
渣渣一块钱4个 (Bilibili同名)</a> 赐予我\"肉身\"<br/><br/>\
软件版本: " EYEOS_VERSION " , 版权所有<br/>";
const char* eyeOSesp32::webpage_html_end =
"<br/><br/>ps:在SD卡根目录放一张start.jpg可以改启动图哦~~</p></body></html>";

const uint8_t eyeOSesp32::faviconData[1150]={
  0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x68, 0x04, 
  0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 
  0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xD1, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0x91, 0x00, 0x86, 0xFF, 0x91, 0x00, 0x86, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x91, 0x00, 
  0x86, 0xFF, 0x91, 0x00, 0x86, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xD8, 0x00, 0xC6, 0xFF, 0xD8, 0x00, 0xC6, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xD8, 0x00, 
  0xC6, 0xFF, 0xD8, 0x00, 0xC6, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xD8, 0x00, 0xC6, 0xFF, 0xEA, 0x5D, 0xD9, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xD8, 0x00, 
  0xC6, 0xFF, 0xEA, 0x5D, 0xD9, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xD8, 0x00, 0xC6, 0xFF, 0xE9, 0x9D, 0xDD, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xD8, 0x00, 
  0xC6, 0xFF, 0xE9, 0x9D, 0xDD, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0x78, 0x78, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0xE1, 0xE8, 
  0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xD1, 0xFF, 0xE1, 0xE8, 0xFE, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x78, 0x78, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xD1, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x78, 0x78, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x63, 0x63, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x63, 0x63, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 
  0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 
  0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00
};
/*
inline const char * getStaSsid(){return sta_ssid;}
inline const char * getStaPassword(){return sta_password;} */

uint8_t eyeOSesp32::getWiFiConfig(){
  lockWiFi();
  uint8_t _wifiConfig=wifiConfig;
  unlockWiFi();
  return _wifiConfig;
}

void eyeOSesp32::initWiFiConfigServer() {
  initApConfig();
  initWebServer();
}

/**
 * 初始化AP配置
 */
void eyeOSesp32::initApConfig(){
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    IPAddress local_IP(192,168,4,1);
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(EYEOS_AP_SSID, EYEOS_AP_PASS);
    sta_ssid[0]='\0';
    wifiConfig |= 2;
    MDNS.begin("eyeos");
    MDNS.addService("http","tcp",80);
    xSemaphoreGiveRecursive(server_lock);
  }
}

void eyeOSesp32::initWebServer(uint8_t initOptions){
  uint8_t _wifiConfig=getWiFiConfig();
  if(!_wifiConfig) return;
  if((_wifiConfig&8)) deinitWebServer();
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    _initOptions = initOptions;
    if((_initOptions & 1)) httpUpdater.setup(&interface_server);
    interface_server.on("/", HTTP_GET, [&](){handleRoot();});
    interface_server.on("/", HTTP_POST, [&](){handleRootPost();});

    interface_server.on("/favicon.ico", HTTP_GET, [&](){
      interface_server.client().write("HTTP/1.1 200 OK\r\n"
                                      "Content-Type: image/x-icon\r\n"
                                      "Content-Length: 1150\r\n"
                                      "Connection: close\r\n\r\n");
      interface_server.client().write(faviconData,sizeof(faviconData));
    });
    interface_server.onNotFound( [&](){ handleNotFound(); });

    /*
    interface_server.on("/list", HTTP_GET, [&]() {handle_printDirectory();});
    interface_server.on("/edit", HTTP_DELETE, [&]() {handleDelete();});
    interface_server.on("/edit", HTTP_PUT, [&]() {handleCreate();});
    interface_server.on("/edit", HTTP_POST, [&]() {handle_returnOK();}, [&]() {handleFileUpload();});
    */
    if((_initOptions & 4)){
      interface_server.on("/order", HTTP_GET, [&](){handleOrder();});
      interface_server.on("/order", HTTP_POST, [&](){handleOrderPost();});
    }
    interface_server.begin();   
    wifiConfig |= 8;
    xSemaphoreGiveRecursive(server_lock);
  }
}
void eyeOSesp32::loopWebServer(){
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    if((wifiConfig & 8))
      interface_server.handleClient();
    xSemaphoreGiveRecursive(server_lock);
  }
}
void eyeOSesp32::deinitWebServer(){
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    if((wifiConfig&8)) interface_server.close();
    wifiConfig &= ~8;
    xSemaphoreGiveRecursive(server_lock);
  }
}
bool eyeOSesp32::connectToWifi(){
  uint8_t _wifiConfig=getWiFiConfig();
  if((_wifiConfig&1)) return 1;
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    //if((wifiConfig&8)) deinitWebServer(); //关闭服务器
    if((wifiConfig&8)){ //关闭服务器, 调用 deinitWebServer 会出现重复获取锁的问题
      interface_server.close();
      wifiConfig &= ~8;
    }
    wifiConfig &= ~2; //关闭AP
    uint32_t startConnectTime = millis();
    //Serial.println("connectToWifi");
    WiFi.mode(WIFI_STA);
    if(sta_ssid[0]&&sta_password[0]) WiFi.begin(sta_ssid,sta_password); //使用配网获取的数据
    else WiFi.begin(); //使用flash已存的数据
    while (!WiFi.isConnected()) {
      if(wifiConnectCB) wifiConnectCB();
      delay(1); //此时WiFi仍在使用
      if(millis()-startConnectTime>EYEOS_CONNECT_WIFI_TIMEOUT) break;
    }
    if((_wifiConfig = WiFi.isConnected())){
      sta_ssid[0] = 0;
      MDNS.begin("eyeos");
      MDNS.addService("http","tcp",80);
      wifiConfig |= 1; //WiFi已连接
    }
    xSemaphoreGiveRecursive(server_lock);
  }
  return _wifiConfig; //WiFi连接结果
  /*
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(sta_ssid, sta_password);
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          cnt++;
          //Serial.print(".");
          if(cnt>=40){
            cnt = 0;
            //重启系统
            //Serial.println("\r\nRestart now!");
            esp_restart();
          }
  }
  //Serial.println("connectToWifi Success!"); */
}
void eyeOSesp32::offWifi(){
  deinitArduinoOTA();
  deinitWebServer();
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    WiFi.mode(WIFI_OFF);
    wifiConfig = 0; //全部关闭了
    xSemaphoreGiveRecursive(server_lock);
  }
}

/**
 * 处理web get请求
 */
void eyeOSesp32::handleRoot() {
  String osStatus="<br/>神之眼形状: ";
  if(xSemaphoreTake(btn_lock,25) == pdTRUE){ //此操作需要访问Button, 不属于WiFi锁的管辖范围内
    osStatus+=(btn.getPin() == EYEOS_BTN_PIN_LIYUE)?"方":"圆";
    xSemaphoreGive(btn_lock);
  }
  osStatus+="形, 当前WiFi模式: ";
  osStatus+=(WiFi.getMode()==WIFI_AP)?"AP配网模式":"STA模式";
  osStatus+=", 当前IP地址: ";
  osStatus+=WiFi.localIP().toString();
  osStatus+="<br/>ESP32 芯片型号: ";
  osStatus+=ESP.getChipModel();
  osStatus+=" Rev";
  osStatus+=ESP.getChipRevision();
  osStatus+=", 芯片ID: ";
  uint64_t gotID;
  char cbuf[20]="";
  esp_flash_read_unique_chip_id(esp_flash_default_chip,&gotID);
  sprintf(cbuf, "%016llx", gotID);
  osStatus+=cbuf;
  osStatus+="<br/>闪存容量: ";
  osStatus+=ESP.getFlashChipSize();
  osStatus+=", MAC地址: ";
  osStatus+=WiFi.macAddress();
  interface_server.send(200, TEXT_HTML, 
    String(webpage_html_start)                                    //网页起始部分
    +((_initOptions&1)?webpage_html_update:emptyString)   //网页的http update部分
    +((_initOptions&2)?webpage_html_edit  :emptyString)   //网页的sd卡管理器部分
    +((_initOptions&4)?webpage_html_order :emptyString)   //网页的sd卡管理器部分
    +webpage_html_mid                                             //网页中间部分
    +osStatus+webpage_html_end);                                  //网页结束部分
}
void eyeOSesp32::handleRootPost() {
  //Serial.println("handleRootPost");
  if (interface_server.hasArg("ssid")) {
    //Serial.print("got ssid:");
    strncpy(sta_ssid, interface_server.arg("ssid").c_str(),31);
    sta_ssid[31]='\0';
    //Serial.println(sta_ssid);
  }
  else {
    //Serial.println("error, not found ssid");
    interface_server.send(200, TEXT_HTML, "<meta charset='UTF-8'>你WiFi名称没有写啊");
    return;
  }

  if (interface_server.hasArg("password") && interface_server.arg("password").length()>=8) {
    //Serial.print("got password:");
    strncpy(sta_password, interface_server.arg("password").c_str(),31);
    sta_password[31]='\0';
    //Serial.println(sta_password);
  }
  else {
    //Serial.println("error, not found password");
    interface_server.send(200, TEXT_HTML, "<meta charset='UTF-8'>你WiFi密码我似乎写不了");
    return;
  }

  interface_server.send(200, TEXT_HTML, "<meta charset='UTF-8'>保存成功");
  //连接wifi
  //connectToWifi();
}

void eyeOSesp32::handleOrder(){
  File f;
  lockSPI();
  if(!(f = SD.open("/edit/order.html"))) handleNotFound();
  else interface_server.streamFile(f,TEXT_HTML);
  unlockSPI();
}
void eyeOSesp32::handleOrderPost(){
  const String ostart="<html><meta charset='UTF-8'><body><h2>数据更新成功</h2><p>当前顺序: ";
  const String oend="%<br/><a href=\"/\">回到主页</a></p></body></html>";
  lockNVS();
  if (interface_server.hasArg("showTime")) {
    playShowTime = uint32_t(interface_server.arg("showTime").toFloat()*1000.f);
    Serial.printf("playShowTime is updated to %f\n",playShowTime);
  }
  if (interface_server.hasArg("alpha")) {
    playAlpha = interface_server.arg("alpha").toInt();
    Serial.printf("playAlpha is updated to %d\n",playAlpha);
  }
  if (interface_server.hasArg("elmOrder")) {
    String argElmOrder = interface_server.arg("elmOrder");
    const char chartable[8] = "hsflcby";
    if(argElmOrder!=emptyString){
      int i2=0;
      //strncpy(playOrder,interface_server.arg("elmOrder").c_str(),8);
      for(int i=0;i<16 && argElmOrder.c_str()[i];i++){
        for(int j=0;j<7;j++){
          if(argElmOrder.c_str()[i]==chartable[j]){
            playOrder[i2] = argElmOrder.c_str()[i];
            i2++;
            break;
          }
        }
      }
      playOrder[i2]='\0';
    }
    Serial.printf("playOrder is updated to %s\n",playOrder);
  }
  nvsData.begin(eyeOSconfig_Tag,false); //初始化NVS
  nvsData.putString(eyeOSplayOrder_Tag,playOrder);
  nvsData.putUInt(eyeOSplayShowTime_Tag,playShowTime);
  nvsData.putUChar(eyeOSplayAlpha_Tag,playAlpha);
  nvsData.end();
  interface_server.send(200,TEXT_HTML,ostart+playOrder
  +"<br/>当前间隔时间: "+playShowTime/1000.f
  +"秒<br/>当前覆盖图层透明度: "+playAlpha+oend);
  unlockNVS();
}
char eyeOSesp32::getPlayOrder(uint8_t idx, uint8_t *itotal){
  char ch = 0;
  lockNVS();
  if(itotal != nullptr) *itotal = strlen(playOrder);
  if(idx<strlen(playOrder))
    ch = playOrder[idx];
  else ch = playOrder[strlen(playOrder)-1];
  unlockNVS();
  return ch;
}

uint32_t eyeOSesp32::getPlayTimeMs(){
  lockNVS();
  uint32_t v=playShowTime;
  unlockNVS();
  return v;
}
uint8_t eyeOSesp32::getPlayAlpha(){
  lockNVS();
  uint8_t v=playAlpha;
  unlockNVS();
  return v;
}




void eyeOSesp32::handleNotFound() {
  interface_server.send(404, "text/plain", "File Not Found\n\n");
}    //与 FSBrowser 里面的 not found 冲突


//------------------------------------------------------------------//
void eyeOSesp32::initArduinoOTA(){
  uint8_t _wifiConfig = getWiFiConfig();
  if((_wifiConfig & 4)) return; //已经开启了ArduinoOTA服务
  if(!(_wifiConfig & 1)){
    connectToWifi();
    if(!(_wifiConfig & 1)) return; //尝试连接两次都没连接上
  }
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        //Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        //Serial.println("\nEnd");
        //esp_restart();
      })
      .onProgress(otaOnProgressCB)
      /*.onProgress(
        [](unsigned int progress, unsigned int total) {
        static uint8_t in_progress = 120;
        if(in_progress!=(progress / (total / 100))){
          //Serial.printf("Progress: %u%%\n", in_progress);
          in_progress = (progress / (total / 100));
        }
      })*/
      .onError([](ota_error_t error) {
        /*Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed"); */
        esp_restart();
      })
      .setHostname("eyeos");

    ArduinoOTA.begin();
    wifiConfig |= 4;
    xSemaphoreGiveRecursive(server_lock);
  }
}

void eyeOSesp32::runArduinoOTA(){ 
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    if((wifiConfig&4)) ArduinoOTA.handle();
    xSemaphoreGiveRecursive(server_lock);
  }
}

void eyeOSesp32::deinitArduinoOTA(){
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    if((wifiConfig & 4)) ArduinoOTA.end();
    wifiConfig &= ~4;
    xSemaphoreGiveRecursive(server_lock);
  }
}

bool eyeOSesp32::loopWiFiConfigServer(){
  bool stastat = 0;
  loopWebServer();
  if(xSemaphoreTakeRecursive(server_lock,EYEOS_WIFI_WAIT_TICKOUT) == pdTRUE){
    stastat = sta_ssid[0]&&sta_password[0];
    xSemaphoreGiveRecursive(server_lock);
  }
  return stastat;
}

void eyeOS_WiFiButtonServiceTask(void * _this){
  //守护的资源: 一个按键
  if(xSemaphoreTake(((eyeOSesp32 *)_this)->btn_lock,portMAX_DELAY)==pdTRUE){
    /******** 旧设     需要一个额外的变量 btnCmdQueue(QueueHandle_t)...
    ((eyeOSesp32 *)_this)->btn.setClickHandler([&](Button2& btn){
      //预先定义 buttonClicked 事件
      eyeOSesp32::buttonResult_t bResult = eyeOSesp32::buttonClicked;
      //当收到来自其他程序的请求时, (没有受到请求的话, xSemaphoreTake 会返回pdFALSE)
      if(xSemaphoreTake(((eyeOSesp32 *)_this)->btnCmdQueue,0)==pdTRUE){
        //向按键状态缓冲区发送按键被点按的状态: buttonClicked (按键被点按)
        xQueueSend(((eyeOSesp32 *)_this)->btnQueue,&bResult,portMAX_DELAY);
      }
    });
    */

    //设置按钮被点按的callback函数:
    ((eyeOSesp32 *)_this)->btn.setClickHandler([&](Button2& btn){
      //预先定义 buttonClicked 事件
      eyeOSesp32::buttonResult_t bResult = eyeOSesp32::buttonClicked;
      //无论队列状态如何, 总是更新当前按键状态值
      xQueueOverwrite(((eyeOSesp32 *)_this)->btnQueue,&bResult);
    });
    //设置按钮被长按的callback函数:
    ((eyeOSesp32 *)_this)->btn.setLongClickHandler([&](Button2& btn){
      //预先定义 buttonLongClicked 事件
      eyeOSesp32::buttonResult_t bResult = eyeOSesp32::buttonLongClicked;
      xQueueOverwrite(((eyeOSesp32 *)_this)->btnQueue,&bResult);
    });
    //设置按钮被双击的callback函数:
    ((eyeOSesp32 *)_this)->btn.setDoubleClickHandler([&](Button2& btn){
      eyeOSesp32::buttonResult_t bResult = eyeOSesp32::buttonDoubleClicked;
      xQueueOverwrite(((eyeOSesp32 *)_this)->btnQueue,&bResult);
    });
    //初始化按钮, 
    ((eyeOSesp32 *)_this)->btn.begin(((eyeOSesp32 *)_this)->wakeInfo); 
    //初始化按钮的长按识别时间, 
    ((eyeOSesp32 *)_this)->btn.setLongClickTime(350); 
    //结束对资源的守护
    xSemaphoreGive(((eyeOSesp32 *)_this)->btn_lock);
  }
  int i;
  for(i=0;;i++){
    ((eyeOSesp32 *)_this)->runArduinoOTA();
    ((eyeOSesp32 *)_this)->loopWebServer();
    if(xSemaphoreTake(((eyeOSesp32 *)_this)->btn_lock,10)==pdTRUE){
      ((eyeOSesp32 *)_this)->btn.loop();
      xSemaphoreGive(((eyeOSesp32 *)_this)->btn_lock);
    }
    vTaskDelay(2);
    if(i>=100){
      Serial.printf("Task stk: %d\n",(int)uxTaskGetStackHighWaterMark(NULL));
      i=0;
    }
  }
}