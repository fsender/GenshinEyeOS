/**************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file eyeOS_GFX.cpp
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 基础图形驱动
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

#ifndef _Eyeos_gui_H_FILE
#define _Eyeos_gui_H_FILE
#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "Button2.h"

// **************** definations *****************

#define EYEOS_BTN_PIN_LIYUE 23
#define EYEOS_BTN_PIN_ROUND 16
#define MFONT_FILENAME "/vlwfonts/hywenhei20.vlw"

// ****************** includes ******************

#define _DEFINA_IPS_MISO_PIN 2
#define _DEFINA_IPS_MOSI_PIN 15
#define _DEFINA_IPS_SCK_PIN  14
#define _DEFINA_IPS_CS_PIN   5
#define _DEFINA_IPS_DC_PIN   27
#define _DEFINA_IPS_RST_PIN  33
#define _DEFINA_IPS_BL_PIN   22
#define _DEFINA_SD_CS_PIN    13

class LGFX : public lgfx::LGFX_Device
{
protected:
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Panel_ST7789    _panel_instance_liyue;
  lgfx::Panel_GC9A01    _panel_instance;    //用户可以自己添加屏幕型号操作
public:
  LGFX(void)
  {
    { // 设置总线控制。- SPI, I2C, I2S, FSMC, etc.
      auto cfg = _bus_instance.config();    // 获取总线配置的结构。
      cfg.spi_host = VSPI_HOST;             // 选择要使用的 SPI  (VSPI_HOST or HSPI_HOST)
      cfg.spi_mode = 0;                     // SPI设置通讯模式 (0 ~ 3)
      cfg.freq_write = 80000000;            // 传输时的SPI时钟（最高80MHz，四舍五入为80MHz除以整数得到的值）
      cfg.freq_read  = 16000000;            // 接收时的SPI时钟
      cfg.spi_3wire  =  true;               // 使用 MOSI 引脚接收时设置为 True
      cfg.use_lock   =  true;               // 使用锁时设置为 True, 在多任务处理模式下, SPI锁是必要的
      cfg.dma_channel = SPI_DMA_CH_AUTO;    // Set the DMA channel (1 or 2. 0=disable)   设置要打开的 DMA 通道 (0=DMA关闭)
      cfg.pin_sclk = _DEFINA_IPS_SCK_PIN;   // 设置 SCLK 引脚号
      cfg.pin_mosi = _DEFINA_IPS_MOSI_PIN;  // 设置 MOSI 引脚号
      cfg.pin_miso = _DEFINA_IPS_MISO_PIN;  // 设置 MISO 引脚号 (-1 = 禁用)
      cfg.pin_dc   = _DEFINA_IPS_DC_PIN;    // 设置 DC 引脚号  (-1 = 禁用)
      _bus_instance.config(cfg);            // 设定值反映在总线上。
     _panel_instance.setBus(&_bus_instance);// 在面板上设置总线。
     _panel_instance_liyue.setBus(&_bus_instance);// 在面板上设置总线。
    }

    { // 设置显示面板控件。- 显示宽度/高度, 颜色格式, 读写像素规则, etc.
      auto cfg = _panel_instance.config();    // 获取显示面板设置的结构。
      cfg.pin_cs           =    _DEFINA_IPS_CS_PIN;  // 设置 CS 引脚号   (-1 = 禁用)
      cfg.pin_rst          =    _DEFINA_IPS_RST_PIN;  // 设置 RST 引脚号  (-1 = 禁用)
      cfg.pin_busy         =    -1;  // 设置 BUSY 引脚号 (-1 = 禁用)
      cfg.offset_x         =     0;  // 面板的 X 方向偏移量
      cfg.offset_y         =     0;  // 面板的 Y 方向偏移量
      cfg.panel_width      =   240;  // 实际显示宽度
      cfg.panel_height     =   240;  // 实际显示高度
      cfg.memory_width     =   240;  // 实际缓冲区宽度
      cfg.memory_height    =   240;  // 实际缓冲区高度
      cfg.offset_rotation  =     0;  // 设置旋转方向的偏移, 可用的值介于0~7之间（4~7是镜像的）
      cfg.dummy_read_pixel =     0;  // 在读取像素之前读取的虚拟位数
      cfg.dummy_read_bits  =     0;  // 读取像素以外的数据之前的虚拟读取位数
      cfg.readable         =  true;  // 如果可以读取数据，则设置为 true
      cfg.invert           =  true;  // 如果面板的明暗反转，则设置为 true
      cfg.rgb_order        = false;  // 如果面板的红色和蓝色被交换，则设置为 true
      cfg.dlen_16bit       = false;  // 对于以 16 位单位发送数据长度的面板，设置为 true
      cfg.bus_shared       =  true;  // 如果总线与 SD 卡共享，则设置为 true（使用 drawJpgFile 等执行总线控制）
      _panel_instance.config(cfg);
      _panel_instance_liyue.config(cfg);
    }
/*
    { // 设置背光控制。 （不需要的话删掉）
    }
*/
    { // バックライト制御の設定を行います。（必要なければ削除）
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。

      cfg.pin_bl = _DEFINA_IPS_BL_PIN; // バックライトが接続されているピン番号
      cfg.invert = false;              // バックライトの輝度を反転させる場合 true
      cfg.freq   = 4000;              // バックライトのPWM周波数
      cfg.pwm_channel = 0;             // 使用するPWMのチャンネル番号

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
      _panel_instance_liyue.setLight(&_light_instance);
    }

    setPanel(&_panel_instance); // 初始化要使用的面板。默认为圆形
  }
  void setDisplayPanel(bool isSquare){
    setPanel(isSquare?(lgfx::v1::Panel_Device *)&_panel_instance_liyue:
        (lgfx::v1::Panel_Device *)&_panel_instance);
  }
};

#endif