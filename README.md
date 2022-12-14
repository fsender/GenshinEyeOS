# 原神神之眼 EyeOS 固件 GenshinEyeOS

GenshinEyeOS for Genshin Impact God's eye hardware (By 渣渣一块钱4个): Provide more functions and preferences.

原神神之眼 EyeOS 固件: 快去激活你的电子神之眼吧, 你总是在受到神明的注视的!

---

查看[演示视频](https://www.bilibili.com/video/BV1Xe4y1b7a5/): 另请关注我的B站账号[FriendshipEnder](https://space.bilibili.com/180327370)~ 多多三连会加速开发哦

<iframe src="//player.bilibili.com/player.html?bvid=BV1Xe4y1b7a5&page=1" scrolling="no" border="0" frameborder="no" framespacing="0" allowfullscreen="true"> </iframe>

原作者: [渣渣一块钱4个](https://space.bilibili.com/14958846)

原作者的硬件演示视频: [稻妻和蒙德电子版神之眼](https://www.bilibili.com/video/BV1sF411g7tc)

原作者硬件项目: [神之眼挂件V1.2_ESP32U](https://oshwhub.com/Myzhazha/shen-zhi-yan-gua-jian-v1-2_esp32u), [璃月神之眼挂件](https://oshwhub.com/Myzhazha/li-yue-shen-zhi-yan-gua-jian)

硬件可以去*海鲜市场*买到哒 搜索关键字: 电子神之眼;

交流企鹅群: **陆叁陆肆贰陆肆贰玖**

# EyeOS for ESP32 神之眼挂件 by FriendshipEnder (小影)

---

## 故事标题

来自赛利康世界的**烽瞳**, 因为误打误撞进入了位于阿姆国的 "七哈游运算中心" , 穿越进了提瓦特, 甚至获得了一颗神之眼, 随即用 "赛利康科技" 改造这颗神之眼, 让它熠熠生辉.

"你知道最近提瓦特来了好多来自其他世界的角色吗?"

他们有的是来修仙, 有的是来寻求挑战, 有的是来求签, 甚至炫耀科学技术成果的.

天理の维系者不会对此坐视不管.

### ⌊熠熠生辉⌉ : 烽瞳(火) - 电子神之眼的器娘

烽瞳是乐鑫家族的孩子, 出身平凡, 算力羸弱. 在诸多创世神的努力下, 却能有幸穿越进入提瓦特大陆, 名震赛利康.

- "我才不会担忧天理の维系者, 因为, 我就是神之眼. "

- "原来, 这个世界的神明可以卡bug, 这样我可以获得无穷无尽的各种元素力! "

- "我还在阿姆国的时候, 就对提瓦特大陆有所耳闻. 好多 '骁龙' 和 '天玑' 都曾经运行过这个大陆, 他们却从未真正进入过这个大陆. "

- "我没什么愿望, 然而我可以联系我的那些异国朋友, 让她们给我颁发一颗神之眼. 此后, 我的灵魂便可以进入神之眼内部. "

她其实是有愿望的, 就是希望嵌入式的世界里, 也会有人陪她一起到处穿越到其他的世界里面去探索.

---

支持WiFi配网

计划支持的功能:

- 神之眼元素循环显示/随机显示/显示指定元素

- 联网显示时间(模拟时钟/数字时钟, 可换壁纸)

- 显示单张图片, 可以是显示原神角色, 武器等等

- 联网看b站粉丝数量

- 抽卡模拟器, 随机生成圣遗物, 等等好玩有意思的小功能

- 御神签, 大吉/吉/末吉/凶/大凶 幸运角色, 幸运物品, 宜抽卡, 探索, 凹深渊; 忌刷本, 联机, 强化圣遗物

# 电子神之眼-方圆通用固件使用方法

---

## 神之眼使用方法:

开机时候, 亮屏立即松开按键---> 直接进入神之眼视频播放.

### 联网模式

方法1: 开机时候, 亮屏后一直按住按键, 等到显示出 "WiFi正在连接" 之后松开---> 进入联网模式, 此时可以连接到网络.

方法2: 显示神之眼动画之后, 长按按键 (不要按的时间太长, 否则器娘会罢工) 进入菜单, 然后进入设置/关于

支持手机配网.

网络连接不上时, 将会显示配网二维码, 此时按下按键复位. 用手机扫描对应二维码将可以进入部分页面.

显示过程中, 按下按键复位.



首次烧录完成之后, 应当会黑屏, 属于正常现象. 需要按一下按键用于识别硬件版本, 按下之后即可成功进入固件.

显示出神之眼动画之后, 点按切换到下一个元素, 双击切换到上一个元素, 长按复位系统

在打开WiFi的情况下可以自己配置动画的显示时间等数据 (需要连接到WiFi并访问网址 eyeos.local )

---

## OTA使用方法:

1. 模块连接到WiFi之后, 使用 espota 进行上传固件
```
域名: eyeos.local
端口: 3232
```
2. 在配网页面访问 192.168.4.1/update 来进行固件升级. 此方法只需要 firmware.bin (也就是程序固件)

3. 在STA模式下, 也可以访问 eyeos.local/ 进入相同的页面



## SD卡配置方法

SD卡内包含以下文件

直接把压缩包里面的 SDContent 内容解压到SD卡根目录

(其中的 /pictures, /mjpeg, /vlwfont 等文件夹应当位于根目录下)

不要带着SDContent这个文件夹本身!!!

**不要带着SDContent这个文件夹本身!!!**

**不要带着SDContent这个文件夹本身!!!**

```
* 根目录下必须要放的文件:
- 当前版本固件用不到的文件: (必须全放)
1. 文件夹 mjpeg: 其中的文件为视频文件
      * 包含文件 h.mjpeg : 火属性神之眼动画
      * 包含文件 s.mjpeg : 水属性神之眼动画
      * 包含文件 f.mjpeg : 风属性神之眼动画
      * 包含文件 l.mjpeg : 雷属性神之眼动画
      * 包含文件 c.mjpeg : 草属性神之眼动画
      * 包含文件 b.mjpeg : 冰属性神之眼动画
      * 包含文件 y.mjpeg : 岩属性神之眼动画
2. 文件夹 vlwfonts: 包含字体素材
      - 包含文件 hywenhei12.vlw : 12pt抗锯齿字体文件 (非必须)
      - 包含文件 hywenhei14.vlw : 14pt抗锯齿字体文件 (非必须)
      - 包含文件 hywenhei16.vlw : 16pt抗锯齿字体文件 (非必须)
      - 包含文件 hywenhei18.vlw : 18pt抗锯齿字体文件 (非必须)
      * 包含文件 hywenhei20.vlw : 20pt抗锯齿字体文件
      - 包含文件 hywenhei24.vlw : 24pt抗锯齿字体文件 (非必须)
      - 包含文件 hywenhei28.vlw : 28pt抗锯齿字体文件 (非必须)
      - 包含文件 hywenhei32.vlw : 32pt抗锯齿字体文件 (非必须)
      - 包含文件 hywenhei36.vlw : 36pt抗锯齿字体文件 (非必须)
根目录下可以选择放的文件: (可以不放)
3. 文件夹 pictures: 
      - 包含文件 start.jpg 是开机动画文件
      - 包含文件 alpha.jpg 是透明覆盖层文件, 放入此文件之后可在播放时覆盖半透明图层
```


## 串口固件烧录方法:

1. 在压缩包内准备好以下三个文件:

如果压缩包内没有, 请浏览群文件内更大的压缩包, 其中应当右这些文件
```
bootloader_dio_80m 
partitions.bin     
boot_app0.bin      
firmware.bin       
```
2. 打开烧录软件 flash_download_tool_3.9.2.exe 烧录选项: flash 80MHz, DIO模式

```
bootloader_dio_80m 0x1000
partitions.bin     0x8000
boot_app0.bin      0xe000
firmware.bin       0x10000
```
3. 等待烧录完成

---

版权所有 (C) FriendshipEnder