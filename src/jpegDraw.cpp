/**************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file jpegDraw.cpp
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 为ESP32提供JPEG程序解码功能支持
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

#include "jpegDraw.h"
static uint_fast16_t _colormask;
static uint_fast16_t _alp;       //alpha通道设置, 用于纯色和alpha通道互换
static uint_fast16_t _64_m_alp;  //alpha通道设置
static LGFX_Sprite *JPGCover = nullptr;
static bool jpegBuflen16Bit = JPEG_BUFLEN_16BIT;

void setJpegBuflen16Bit(bool j){
  jpegBuflen16Bit = j;
}
void setDrawColorful(uint16_t colormask, uint16_t alphamask){
  if(JPGCover != nullptr) freeJPGCoverBuffer();
  _colormask = colormask;
  setAlphamask(alphamask);
  mjpeg.setJpegDrawCb(jpegDrawCB_colormask);
  //mjpeg.drawJpg(filename,x,y,w,h);
  //mjpeg.setJpegDrawCb(jpegDrawCB);
}

bool setDrawJPGCover(const char *jpgPath, uint16_t alphamask){
  bool initOK = 0;
  mjpeg.setJpegDrawCb(jpegDrawCB);
  if(alphamask >=100) {
    freeJPGCoverBuffer();
    return 0; //不透明度 alpha 必须在 0 到 100 之间的开区间内
  }
  if(jpgPath[0]){
    eyeOS.lockSPI();
    initOK = SD.exists(jpgPath);
    eyeOS.unlockSPI();
    if(!initOK) return 0; //文件不存在, 无法继续
  }
  setAlphamask(alphamask);
  Serial.printf("Pre-init: Free %d Bytes:\n",esp_get_free_heap_size());

  //new分配的内存总是能成功, 否则会崩溃
  if(JPGCover == nullptr) {
    JPGCover = new LGFX_Sprite[15];
    for(int i=0;i<15;i++){
      JPGCover[i].setColorDepth(jpegBuflen16Bit?16:8);
      if(JPGCover[i].createSprite(240,16) == nullptr) return 0;
      Serial.printf("Block%d init. Free %d\n",i,esp_get_free_heap_size());
    }
  }
  mjpeg.setJpegDrawCb(jpegDrawToSpriteCB);
  eyeOS.lockSPI();
  if(jpgPath[0]) initOK = mjpeg.drawJpg(jpgPath);
  else initOK = mjpeg.drawMJpegFrame();
  eyeOS.unlockSPI();
  mjpeg.setJpegDrawCb(initOK?jpegDrawCB_imagemask:jpegDrawCB);
  return initOK;
}

void freeJPGCoverBuffer(){
  for(int i=0;i<15;i++) JPGCover[i].deleteSprite();
  delete []JPGCover;
  JPGCover = nullptr;
  mjpeg.setJpegDrawCb(jpegDrawCB); //设置jpg回调函数, 设置为默认
}

int jpegDrawCB(JPEGDRAW *pDraw){
  eyeOS.lockSPI();
  ips.startWrite(); //u16 数据类型始终为2字节
  size_t len=pDraw->iWidth*pDraw->iHeight;
  ips.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  ips.writePixels(pDraw->pPixels,len,true);
  //ips.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight,pDraw->pPixels);
  ips.endWrite();
  eyeOS.unlockSPI();
  return 1;
}
int jpegDrawToSpriteCB(JPEGDRAW *pDraw){
  size_t len=pDraw->iWidth*pDraw->iHeight;
  JPGCover[pDraw->y>>4].setAddrWindow(pDraw->x, 0, pDraw->iWidth, pDraw->iHeight);
  JPGCover[pDraw->y>>4].writePixels(pDraw->pPixels,len,!jpegBuflen16Bit);
  //ips.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight,pDraw->pPixels);
  return 1;
}
int jpegDrawCB_colormask(JPEGDRAW *pDraw){
  eyeOS.lockSPI();
  size_t len=pDraw->iWidth*pDraw->iHeight;
  ips.startWrite(); //u16 数据类型始终为2字节
  //ips.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  ips.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  for(size_t i=0;i<len;i++) ips.writeColor((uint16_t)
    ((((((pDraw->pPixels[i]&0xf800u)>>11u)*_alp + ((_colormask&0xf800u)>>11u)*_64_m_alp)<<5u)&0xf800u)
    |(((((pDraw->pPixels[i]&0x07e0u)>>5u )*_alp + ((_colormask&0x07e0u)>>5u)*_64_m_alp)>>1u)&0x07e0u)
    |(((((pDraw->pPixels[i]&0x001fu)     )*_alp + ((_colormask&0x001fu)    )*_64_m_alp)>>6u)&0x001fu))
    ,1);
  ips.endWrite();
  eyeOS.unlockSPI();
  return 1;
}

int jpegDrawCB_imagemask(JPEGDRAW *pDraw){
  eyeOS.lockSPI();
  size_t len=pDraw->iWidth*pDraw->iHeight;
  ips.startWrite(); //u16 数据类型始终为2字节
  //ips.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  ips.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  if (jpegBuflen16Bit){
    for(size_t i=0;i<len;i++){
      uint_fast16_t _icolormask =
      ((uint16_t *)(JPGCover[pDraw->y>>4].getBuffer()))[240u*(i/pDraw->iWidth)+pDraw->x+i%pDraw->iWidth];
      ips.writeColor((uint16_t)
      ((((((pDraw->pPixels[i]&0xf800u)>>11u)*_alp + ((_icolormask&0xf800u)>>11u)*_64_m_alp)<<5u)&0xf800u)
      |(((((pDraw->pPixels[i]&0x07e0u)>>5u )*_alp + ((_icolormask&0x07e0u)>>5u)*_64_m_alp)>>1u)&0x07e0u)
      |(((((pDraw->pPixels[i]&0x001fu)     )*_alp + ((_icolormask&0x001fu)    )*_64_m_alp)>>6u)&0x001fu)),1);
    }
  }
  else{
    for(size_t i=0;i<len;i++){
      uint_fast8_t _icolormask =
      ((uint8_t *)(JPGCover[pDraw->y>>4].getBuffer()))[240u*(i/pDraw->iWidth)+pDraw->x+i%pDraw->iWidth];
      ips.writeColor((uint16_t)
      ((((((pDraw->pPixels[i]&0xf800u)>>11u)*_alp + ((_icolormask&0xe0u)>>3u)*_64_m_alp)<<5u)&0xf800u)
      |(((((pDraw->pPixels[i]&0x07e0u)>>5u )*_alp + ((_icolormask&0x1cu)<<1u)*_64_m_alp)>>1u)&0x07e0u)
      |(((((pDraw->pPixels[i]&0x001fu)     )*_alp + ((_icolormask&0x03u)<<3u)*_64_m_alp)>>6u)&0x001fu)),1);
    }
  }
  ips.endWrite();
  eyeOS.unlockSPI();
  return 1;
}
void setColormask(uint_fast16_t colormask){  _colormask = colormask; }
void setAlphamask(uint_fast16_t alphamask){ 
  if(alphamask>=100) return;
  _alp = (alphamask<<4)/25;
  if(_alp>63) _alp=63;
  _64_m_alp = 63-_alp;
}
uint_fast16_t getColormask() { return _colormask; }
uint_fast16_t getAlphamask() { return _alp; }