
# 一、简介
本项目为RTSP流媒体服务器，对原项目进行改进，基于自己的理解优化了代码以及架构，并添加了PAUSE等功能。

---
# 构建

```c
mkdir build
cd build
cmake ..
make
```
## 运行
```c
./rtsp ../beyond.h264 ../beyond.aac
```
## 使用ffplay播放
### 1、UDP播放
```c
ffplay -i rtsp://127.0.0.1:8554/live
```
### 2、TCP播放
```c
ffplay -i rtsp://127.0.0.1:8554/live -rtsp_transport tcp
```




