/**************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file eyeOS.h
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 神之眼功能的核心代码头文件
 *        依赖库文件: Button2, LovyanGFX
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

/* --------------- 定义 * definations --------------- */
#ifndef _EYEOS_H_FILE
#define _EYEOS_H_FILE

/** @brief 检测必要文件存在性 
 *  @example CheckFilesDependency() (SD.exists("/a/b.txt") && SD.exists("/c/d.json"))
 */
#define CheckFilesDependency() (SD.exists(MFONT_FILENAME))
//#define ENABLE_DEBUGSERVER
#define EYEOS_CONNECT_WIFI_TIMEOUT 8000
#define EYEOS_WIFI_WAIT_TICKOUT 1000
#define SDCARD_LOCK_TICKOUT 10000
#define EYEOS_AP_SSID "eyeOS"
#define EYEOS_AP_PASS "yuanshen"

/* --------------- 包含 * includings --------------- */
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include <pgmspace.h>
#include <SD.h>
#include "esp_heap_caps.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <Preferences.h> //读写NVS实现很多数据的非易失性存储, 比如型号等信息

#include "eyeos_version.h"
#include "eyeos_gfx.h"

class eyeOSesp32{
public:

/* --------------------------- 表明功能状态的枚举体 --------------------------- */

  /* @brief 设置指令, 用于任务间通信. WiFi设置等功能均移动到新的任务内
  enum {
    enableWiFi      , disableWiFi      ,
    enableWebServer , disableWebServer ,
    enableArduinoOTA, disableArduinoOTA,
    enableButtonRead, disableButtonRead
  } eyeOSSetting_t; */
  
  /// @brief 设置指令, 用于任务间通信. WiFi设置等功能均移动到新的任务内
  typedef enum {
    buttonNoEvent=0, buttonPressed, buttonReleased,
    buttonClicked, buttonDoubleClicked, buttonTripleClicked,
    buttonLongClicked, buttonComboLongClicked
  } buttonResult_t;

  /* @brief 表明当前的状态
  enum{
    statusWiFiOK      , statusWiFiNotOK      ,
    statusArduinoOTAOK, statusArduinoOTANotOK,
    statusServerOK    , statusServerNotOK    ,
    statusStartArduinoOTA
  } eyeOSStatus_t; */

/* ------------------------------ 基础功能函数 ------------------------------ */

  /// @brief 构造函数
  eyeOSesp32(LGFX &_lgfx) : ips(&_lgfx){}

  /** @brief 初始化神之眼系统
   *  @note 无卡将会强行打开WiFi, 进入OTA升级模式
   */
  void init(uint8_t initBrightness = 255);

  /** @brief 获取神之眼到底是璃月的方形还是其他地区的圆形
   *  @return true: 方形, false: 圆 */
  bool getType(){ return wakeInfo == EYEOS_BTN_PIN_LIYUE; }

  /// @brief 安全的读取按键电平
  uint8_t readButtonRaw();
  
  /// @brief 安全的读取按键实际状态
  buttonResult_t readButton(uint32_t maxDelayTick = 0);


/* ------------------------------ 字体处理函数 ------------------------------ */

  /** @brief 从SD卡加载VLW格式的字体, 加载后消耗RAM, 存储在内部的数组中,
   *  @attention 使用字体时, 需要调用 getFontPtr 函数返回字体指针
   *  @param fn 字体文件名
   *  @return 加载是否成功
   *  @note 这样loadfont可能会有些慢速, 但是渲染字体将会很快
   */
  bool loadFont(const char* fn);

  /// @brief 返回指向字体数组的指针
  inline const uint8_t * getFontPtr(){
    return cn_font_vlw_SDfile;
  }

  /// @brief 卸载VLW字体, 释放字体占用的RAM空间
  void unloadFont();


/* ---------------------------- WiFi相关处理函数 ---------------------------- */

  /// @brief 获取WiFiConfig的值, 可以根据此值判断WiFi各个相关组件的工作状态
  uint8_t getWiFiConfig();

  /** @brief 连接到WiFi, 不带图形界面
    * @return true 连接成功了 
    */
  bool connectToWifi();

  /// @brief 关闭WiFi
  void offWifi();

  /// @brief 初始化arduinoOTA升级服务
  void initArduinoOTA();

  /// @brief 保持arduinoOTA升级服务可用(需要连续调用)
  void runArduinoOTA();

  /// @brief 关闭arduinoOTA升级服务
  void deinitArduinoOTA();

  /// @brief 初始化配网服务器, 调用后, WiFi将会自动进入AP模式
  void initWiFiConfigServer();

  /** @brief 服务函数需要 循环运行(需要连续调用)
   *  @return 是否接收到了有效的WiFi信息, 接收到返回True
   */
  bool loopWiFiConfigServer();

  // @brief 返回从网页中获取到的WiFi名称
  //inline const char * gotStaSsid(){ return (const char *)sta_ssid; }

  // @brief 返回从网页中获取到的WiFi密码
  //inline const char * gotStaPassword(){ return (const char *)sta_password; }

  /// @brief 设置正在连接WiFi时调用的函数
  inline void setWifiConnectCB(void (*callfx)(void)) {wifiConnectCB = callfx;}

  /// @brief 设置正在OTA更新时调用的函数
  inline void setOtaOnProgressCB(void (*callfx)(unsigned int, unsigned int))
    {otaOnProgressCB = callfx;}

  /** @brief 初始化服务器, 在ap模式和sta模式现在均可用于初始化
   *  @param initOptions 初始化选项:
   * bit 0: 1: 初始化 httpUpdater; 0: 不初始化httpUpdater. 强烈建议初始化, 否则可能变砖
   * bit 1: 1: 正常, 可使用所有功能; 0: 不允许网页服务器使用sd卡, 只能wifi配网和固件更新
   * bit 2: 是否允许设置七元素顺序 1: 允许 0: 不允许
   */
  void initWebServer(uint8_t initOptions = 7);

  /// @brief 初始化AP(用于配网)
  void initApConfig();

  /// @brief 维持服务器正常服务
  void loopWebServer();

  /// @brief 关闭服务器
  void deinitWebServer();

/* ------------------------------ 元素播放器相关函数 ------------------------------ */

  /// @brief 获取下一个应该播放的元素
  char getPlayOrder(uint8_t idx, uint8_t *itotal = nullptr);
  
  /// @brief 获取元素应该播放的毫秒数
  uint32_t getPlayTimeMs();

  /// @brief 获取元素应该播放时增加的半透明图层的透明度
  uint8_t getPlayAlpha();

/* ------------------------------ GUI图形界面相关函数 ------------------------------ */

  void entryArduinoOTA_GUI(uint32_t maxTime = 0, const char *msg = nullptr);

  /** @brief GUI方式连接WiFi
   *  @return 0:未连接, 1 已经连接 , 2 之前就连接上了
   */
  uint8_t connectWifiGUI();
  
  /// @brief 开始使用SPI(包括使用SD卡, 或者开始显示), 不要占用太长时间
  inline bool lockSPI(TickType_t pDelay = portMAX_DELAY){ 
    return (xSemaphoreTakeRecursive(SDCard_lock, pDelay))==pdTRUE; }

  /// @brief 结束使用SPI(包括使用SD卡, 或者开始显示)
  inline void unlockSPI(){ xSemaphoreGiveRecursive(SDCard_lock); }
  
  /// @brief 开始使WiFi, 不要占用太长时间
  inline bool lockWiFi(TickType_t pDelay = portMAX_DELAY){ 
    return (xSemaphoreTakeRecursive(server_lock, pDelay))==pdTRUE; }

  /// @brief 结束使用WiFi
  inline void unlockWiFi(){ xSemaphoreGiveRecursive(server_lock); }

  /// @brief 开始读写NVS
  inline bool lockNVS(TickType_t pDelay = portMAX_DELAY){ 
    return (xSemaphoreTakeRecursive(nvs_lock, pDelay))==pdTRUE; }

  /// @brief 结束读写NVS
  inline void unlockNVS(){ xSemaphoreGiveRecursive(nvs_lock); }

protected:
  LGFX *ips;
  Button2 btn;
  
  /// @brief Button按键作为共享资源使用
  SemaphoreHandle_t btn_lock = nullptr;

  /// @brief SD卡 ( SPI? ) 作为共享资源使用
  SemaphoreHandle_t SDCard_lock = nullptr;
  /// @brief 存储按键指令, 当需要请求按键信号时, 就往 btnQueue 发送信息
  SemaphoreHandle_t nvs_lock = nullptr;
  /// @brief 存储按键状态, 当按键信号有需要的时候就有用了
  QueueHandle_t btnQueue = nullptr;

  uint8_t *cn_font_vlw_SDfile;                              //指向字库的指针, 存放在RAM内哦
  const char *const eyeOSconfig_Tag = "eyeOS config";       //NVS Tag: 标记这段数据
  const char *const eyeOSdeviceType_Tag = "device type";    //用于区分神之眼的形状信息, 此部分数据存入NVS
  const char *const eyeOSplayOrder_Tag = "playOrder";       //用于区分神之眼的播放顺序
  const char *const eyeOSplayShowTime_Tag = "playShowTime"; //用于区分神之眼的切换时间
  const char *const eyeOSplayAlpha_Tag = "playAlpha";       //用于设置透明图层的透明度
  const char *const eyeOSuniqueKey_Tag = "uniqueKey";       //用于设置透明图层的透明度
  Preferences nvsData;
  /// @brief 管理WiFi和按键的任务函数, 一直执行
  friend void eyeOS_WiFiButtonServiceTask(void * _this);
  TaskHandle_t loopTaskHandler;
  TaskHandle_t eyeOS_WiFiButtonServiceTaskHandler;


  uint8_t wakeInfo=0;          //标记神之眼是圆的还是方的 
  uint8_t wifiConfig=0;        //标记WiFi运行状态
  uint8_t _initOptions=0;      //服务器功能的运行模式
  uint8_t playAlpha=0;         //覆盖层的透明度(如果有覆盖层)
  String lastPlayingFile;      //上次播放的文件
  char playOrder[8];
  uint32_t playShowTime=0x7fffffff;

  /* ------------------------------ WiFi相关处理函数 ------------------------------ */
  /** @note 以下资源均为 eyeOS_loopTask 任务独占函数, 访问这些函数必须使用互斥锁!!! 
   * vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv  */

  static const char* webpage_html_start;  //离线 html 文档开头部分
  static const char* webpage_html_update; //离线 html 的 update 链接 (可能不封装它们)
  static const char* webpage_html_edit;   //离线 html 的 edit 链接 (可能不封装它们)
  static const char* webpage_html_order;  //离线 html 的 order 链接 (可能不封装它们)
  static const char* webpage_html_mid;    //离线 html 文档中间部分 (然后会封装系统状态数据)
  static const char* webpage_html_end;    //离线 html 文档结尾部分
  static const char* webpage_chk_start;   //离线 html 文档检查激活状态开头部分
  static const char* webpage_chk_end;     //离线 html 文档检查激活状态结尾部分
  static const uint8_t faviconData[1150]; //图标数据, 此数据甚至不需要sd卡
  WebServer interface_server;
  SemaphoreHandle_t server_lock = nullptr;
  HTTPUpdateServer httpUpdater;
  File uploadFile; //用于 FSBrowser 的文件, 通常为关闭状态


  void(* wifiConnectCB)(void) = nullptr;
  void(* otaOnProgressCB)(unsigned int, unsigned int) = nullptr;
  char sta_ssid[32] ;
  char sta_password[32] ;
    
  void handleRoot();
  void handleRootPost();
  void handleOrder();
  void handleOrderPost();
  //void handle_returnOK();
  //void returnFail(String msg);
  //bool loadFromSdCard(String path);
  //void handleFileUpload();
  //void deleteRecursive(String path);
  //void handleDelete();
  //void handleCreate();
  //void handle_printDirectory();
  void handleNotFound();

  void replyOK();
  void replyOKWithMsg(String msg);
  void replyNotFound(String msg);
  void replyBadRequest(String msg);
  void replyServerError(String msg);
  void handleStatus();
  void handleFileList();
  bool handleFileRead(String path);
  String lastExistingParent(String path);
  void handleFileCreate();
  void deleteRecursive(String path);
  void handleFileDelete();
  void handleFileUpload();
  void handleGetEdit();
  /** ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   * @note 以上资源均为 eyeOS_loopTask 任务独占函数, 访问这些函数必须使用互斥锁!!!  */
};
#endif