/**************** 枫丹科技(误) 我敢直面天空岛的威光 ****************
 * @file MJPEGDEC.h
 * @author FriendshipEnder (f_ender@163.com)
 * Bilibili : FriendshipEnder
 * 
 * @brief 神之眼JPEG图形功能解码文件
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

#include "MJPEGDEC.h"

JPEGDEC MJPEGClass::_jpeg;

bool MJPEGClass::setupMJpeg(Stream *input, int x, int y, int wLimit, int hLimit,size_t bufferSize,f_t lk,f_t ulk)
{
  _input = input;
  _input_lock = lk;
  _input_unlock = ulk;
  if(_mjpeg_buf == nullptr) _mjpeg_buf = (uint8_t *)malloc(bufferSize);
  //else _mjpeg_buf = (uint8_t *)realloc(_mjpeg_buf, bufferSize); //避免重新分配内存
  //if(mjpeg_buf == nullptr) dbg.printf("mjpeg_buf malloc failed!\n");

  //_mjpeg_buf = mjpeg_buf;
  //_pfnDraw = pfnDraw;
  //_useBigEndian = useBigEndian;
  _x = x;
  _y = y;
  _widthLimit = wLimit;
  _heightLimit = hLimit;
  _inputindex = 0;
  if(_read_buf == nullptr) _read_buf = (uint8_t *)malloc(READ_BUFFER_SIZE);

  return (_mjpeg_buf != nullptr && _read_buf != nullptr);
}

void MJPEGClass::endMJpeg(){
  if(_read_buf != nullptr){
    free(_read_buf);
    _read_buf = nullptr;
  }
  if(_mjpeg_buf != nullptr){
    free(_mjpeg_buf);
    _mjpeg_buf=nullptr;
  }
}

bool MJPEGClass::readMjpegBuf(){
  if (_inputindex == 0){
    if(_input_lock)_input_lock();
    _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
    _inputindex += _buf_read;
    if(_input_unlock)_input_unlock();
  }
  _mjpeg_buf_offset = 0;
  int i = 3;
  bool found_FFD9 = false;
  if (_buf_read > 0){
    i = 3;
    while ((_buf_read > 0) && (!found_FFD9)){
      if ((_mjpeg_buf_offset > 0) && (_mjpeg_buf[_mjpeg_buf_offset - 1] == 0xFF) && (_read_buf[0] == 0xD9)) // JPEG trailer
        found_FFD9 = true;
      else{
        while ((i < _buf_read) && (!found_FFD9)){
          if ((_read_buf[i] == 0xFF) && (_read_buf[i + 1] == 0xD9)){ // JPEG trailer
            found_FFD9 = true;
            ++i;
          }
          ++i;
        }
      }

      // Serial.printf("i: %d\n", i);
      memcpy(_mjpeg_buf + _mjpeg_buf_offset, _read_buf, i);
      _mjpeg_buf_offset += i;
      size_t o = _buf_read - i;
      if (o > 0){
        // Serial.printf("o: %d\n", o);
        memcpy(_read_buf, _read_buf + i, o);
        if(_input_lock)_input_lock();
        _buf_read = _input->readBytes(_read_buf + o, READ_BUFFER_SIZE - o);
        if(_input_unlock)_input_unlock();
        _inputindex += _buf_read;
        _buf_read += o;
        // Serial.printf("_buf_read: %d\n", _buf_read);
      }
      else{
        if(_input_lock)_input_lock();
        _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
        if(_input_unlock)_input_unlock();
        _inputindex += _buf_read;
      }
      i = 0;
    }
    if (found_FFD9) return true;
  }

  return false;
}

bool MJPEGClass::drawMJpegFrame(){
  _remain = _mjpeg_buf_offset;
  //dbg.print("remain:");
  //dbg.println(_remain);
  _jpeg.openRAM(_mjpeg_buf, _remain, _pfnDraw);
  if (_scale == -1){
    int iMaxMCUs; // scale to fit height
    int w = _jpeg.getWidth();
    int h = _jpeg.getHeight();
    float ratio = (float)h / _heightLimit;
    if (ratio <= 1) {
      _scale = 0;
      iMaxMCUs = _widthLimit / 16;
    }
    else if (ratio <= 2) {
      _scale = JPEG_SCALE_HALF;
      iMaxMCUs = _widthLimit / 8;
      w /= 2; h /= 2;
    }
    else if (ratio <= 4) {
      _scale = JPEG_SCALE_QUARTER;
      iMaxMCUs = _widthLimit / 4;
      w /= 4; h /= 4;
    }
    else {
      _scale = JPEG_SCALE_EIGHTH;
      iMaxMCUs = _widthLimit / 2;
      w /= 8; h /= 8;
    }
    _jpeg.setMaxOutputSize(iMaxMCUs);
    _x = (w > _widthLimit) ? 0 : ((_widthLimit - w) / 2);
    _y = (_heightLimit - h) / 2;
  }
  if (_useBigEndian) _jpeg.setPixelType(RGB565_BIG_ENDIAN);
  _jpeg.decode(_x, _y, _scale);
  _jpeg.close();

  return true;
}

bool MJPEGClass::drawJpg(const char *filename,int x,int y,int wLimit,int hLimit){
  static fs::File in_jpgFile;
  //static JPEGDEC _jpeg;
  //if(!SD.exists(filename)) return;
  if(!_jpeg.open(filename, 
  [](const char *szFilename, int32_t *pFileSize) ->void *{
    in_jpgFile = SD.open(szFilename, "r");
    *pFileSize = in_jpgFile.size();
    return &in_jpgFile;
  },
  [](void *pHandle){
    // Serial.println("jpegCloseFile");
    //File *f = static_cast<File *>(pHandle);
    static_cast<File *>(pHandle)->close();
  },
  [](JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen) ->int32_t{
    // Serial.printf("jpegReadFile, iLen: %d\n", iLen);
    return (int32_t)static_cast<File *>(pFile->fHandle)->read(pBuf, iLen);
  },
  [](JPEGFILE *pFile, int32_t iPosition) ->int32_t{
    // Serial.printf("jpegSeekFile, pFile->iPos: %d, iPosition: %d\n", pFile->iPos, iPosition);
    return static_cast<File *>(pFile->fHandle)->seek(iPosition)?iPosition:0;
  },
  _pfnDraw
  )) return 0; //JPEG初始化失败了

  // scale to fit height
  int _scale;
  int iMaxMCUs;
  float ratio = (float)_jpeg.getHeight() / hLimit;
  if (ratio <= 1) {
      _scale = 0;
      iMaxMCUs = wLimit / 16;
  }
  else if (ratio <= 2) {
      _scale = JPEG_SCALE_HALF;
      iMaxMCUs = wLimit / 8;
  }
  else if (ratio <= 4) {
      _scale = JPEG_SCALE_QUARTER;
      iMaxMCUs = wLimit / 4;
  }
  else {
      _scale = JPEG_SCALE_EIGHTH;
      iMaxMCUs = wLimit / 2;
  }
  _jpeg.setMaxOutputSize(iMaxMCUs);
  if (_useBigEndian) _jpeg.setPixelType(RGB565_BIG_ENDIAN);
  bool willturn = _jpeg.decode(x, y, _scale);
  _jpeg.close();
  if(in_jpgFile) in_jpgFile.close();
  return willturn;
}