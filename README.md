# QWaveBox

## 概述
QWaveBox是一款开源的跨平台音视频播放库，基于Qt和FFmpeg开发。它提供了强大的音视频解码和播放功能，可以轻松集成到其他应用程序中，也可作为独立的播放器使用。QWaveBox注重高性能和低延迟的媒体处理，同时提供简单易用的API接口。

## 功能

### 已实现功能
- [x] 本地音频和视频文件播放
- [x] 播放列表管理
- [x] 播放速度控制
- [x] 音量调节和静音功能
- [x] 进度条拖动实现快进快退
- [x] 全屏播放支持

### 待实现功能
- [ ] 网络媒体资源播放
- [ ] 更多音视频格式支持
- [ ] 视频滤镜效果
- [ ] 字幕支持
- [ ] 高级播放列表功能
- [ ] 硬件加速解码
- [ ] 流媒体协议扩展
- [ ] 直播功能支持

## 技术栈
- [ ] Qt6

- Qt 5.15.2+：用于UI界面构建
- C++17：编程语言标准


### 平台支持
- [x] Windows
- [ ] Linux
- [ ] macOS

### 第三方库依赖
- FFmpeg：用于音视频解码
- SDL2：用于音频、视频渲染

## 构建说明

QWaveBox使用CMake构建系统，并通过VCPKG管理第三方依赖。

### 前置条件
1. 安装CMake (3.14+)
2. 安装VCPKG并设置`VCPKG_ROOT`环境变量
3. 安装Qt 5.15.2+

## 许可证
本项目采用MIT许可证。

MIT License

Copyright (c) 2025 QWaveBox

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
