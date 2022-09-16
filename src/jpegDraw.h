/**************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file jpegDraw.h
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 为ESP32提供JPEG程序解码功能支持的头文件
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

#ifndef JPEGDRAW_H_FILE
#define JPEGDRAW_H_FILE

#include <Arduino.h>
#include "eyeos.h"
#include "eyeos_gfx.h"
#include "MJPEGDEC.h"

#define JPEG_BUFLEN_16BIT 1 // 允许使用 16bit 缓存, 这将会消耗相当大的内存

extern LGFX ips;
extern eyeOSesp32 eyeOS;
extern MJPEGClass mjpeg;

/** @brief 设置颜色覆盖层
 *  @param colormask 需要设置的颜色
 *  @param alphamask 图像的不透明度 单位 %
 */
void setDrawColorful(uint16_t colormask, uint16_t alphamask);

/** @brief Set the Draw JPG cover object
 *  @param jpgPath jpg文件路径
 *  @param alphamask 透明度
 *  @return uint8_t 1: 初始化正常 0: 错误
 */
bool setDrawJPGCover(const char *jpgPath, uint16_t alphamask);

#define setDrawNormal() freeJPGCoverBuffer()
void setJpegBuflen16Bit(bool j);
void freeJPGCoverBuffer();
//绘图回调函数
int jpegDrawCB(JPEGDRAW *pDraw);
int jpegDrawCB_colormask(JPEGDRAW *pDraw);
int jpegDrawCB_imagemask(JPEGDRAW *pDraw);
int jpegDrawToSpriteCB(JPEGDRAW *pDraw);
void setColormask(uint_fast16_t colormask);
void setAlphamask(uint_fast16_t alphamask);
uint_fast16_t getColormask();
uint_fast16_t getAlphamask();
#endif