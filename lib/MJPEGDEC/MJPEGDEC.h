/***********************************************************
 * 小影(FriendshipEnder) の MJPEG解码库(改编版)
 * 我觉得给人偶写程序和给无主的神之眼写程序, 原理上差不多的吧...
 * JPEGDEC Wrapper Class
 * 
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 * 
 * 
 **************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file eyeOS.h
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 神之眼JPEG图形功能解码头文件
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

#ifndef _MJPEGCLASS_H_
#define _MJPEGCLASS_H_

#define READ_BUFFER_SIZE 512
#define MBUF_SIZE 8192 //这样就能必须保证mpeg的每一帧都必须在此大小以内
//#define MAXOUTPUTSIZE (MAX_BUFFERED_PIXELS / 16 / 16)
#include <SD.h>
#include "JPEGDEC.h"

//#include <WiFi.h>
//extern WiFiClient dbg;

class MJPEGClass
{
public:
  typedef std::function<void(void)> f_t;

  /** @brief MJPEGClass的构造函数
   *  @param pfnDraw 绘图回调函数
   *  @param useBigEndian 绘图回调函数
   *  @param pfnDraw 绘图回调函数
   *  @param pfnDraw 绘图回调函数
   */
  MJPEGClass(JPEG_DRAW_CALLBACK *pfnDraw=nullptr, bool useBigEndian = false): 
    _pfnDraw(pfnDraw), _useBigEndian(useBigEndian){ _input_lock=nullptr; }

  /// @brief 显示静态的mJPEG图像
  bool drawJpg(const char *filename,int x=0,int y=0,int wLimit=240,int hLimit=240);

  /** @brief 初始化MJPG解码器
   *  @param input 输入数据流
   *  @param x,y,wLimit,hLimit 绘图位置和大小限制
   *  @return 绘制是否成功, 若绘制成功, 则返回1, 否则0
   */
  bool setupMJpeg(Stream *input, int x = 0, int y = 0, int wLimit = 240, int hLimit = 240,
    size_t bufferSize = MBUF_SIZE, f_t lk = nullptr, f_t ulk = nullptr );
  void endMJpeg();

  bool readMjpegBuf();
  bool drawMJpegFrame();
  void setJpegDrawCb(JPEG_DRAW_CALLBACK *f) { _pfnDraw = f; }
  void setUsingBigEndian(bool u) { _useBigEndian = u; }

private:
  Stream *_input;
  uint8_t *_mjpeg_buf = nullptr;
  uint8_t *_read_buf = nullptr;
  JPEG_DRAW_CALLBACK *_pfnDraw;
  f_t _input_lock;
  f_t _input_unlock;
  bool _useBigEndian;
  int _x;
  int _y;
  int _widthLimit;
  int _heightLimit;

  int32_t _mjpeg_buf_offset = 0;

  static JPEGDEC _jpeg;
  //static fs::File in_jpgFile;
  int _scale = -1;

  int32_t _inputindex = 0;
  int32_t _buf_read;
  int32_t _remain = 0;
};

#endif // _MJPEGCLASS_H_
