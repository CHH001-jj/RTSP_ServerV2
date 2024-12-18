### RTSP_ServerV2 — 高性能 RTSP 推流服务器

`RTSP_ServerV2` 是一个跨平台（支持 **Linux** 和 **Windows**）的高性能 **RTSP 推流服务器**，用于实现视频点播或者直播等应用场景。

### 功能特点

1. **RTSP 协议核心信令交互**  
   实现了 RTSP 协议的以下信令：  
   - `OPTIONS`  
   - `DESCRIBE`  
   - `SETUP`  
   - `PLAY`  
   - `TEARDOWN`  

2. **音视频实时推流**  
   - 支持 **H.264 视频** 和 **AAC 音频** 文件的实时推流。  
   - 使用 **RTP 协议** 进行数据传输，封装 NALU 单元（H.264）和 ADTS 单元（AAC）。  
   - 音视频同步传输。

3. **高性能 I/O 模型**  
   实现了多种 I/O 模型，适配不同场景：  
   - **select**  
   - **poll**  
   - **epoll**  

4. **传输协议支持**  
   支持 **TCP** 和 **UDP** 两种传输协议，适应不同网络环境需求。

5. **跨平台支持**  
   - **Linux** 和 **Windows** 系统兼容。  
   - 使用 C++ 实现。

---

### 主要目录结构

```
RTSP_ServerV2/
│
├── build/                  # 构建目录
├── data/                   # 媒体文件存放目录
├── MediaServer/            # 核心服务器代码
│   ├── EventManager/       # 事件管理模块
│   │   ├── EPollPoller.cpp/.h
│   │   ├── EventScheduler.cpp/.h
│   │   ├── SelectPoller.cpp/.h
│   │   ├── PollPoller.cpp/.h
│   │   ├── SocketsOps.cpp/.h
│   │   ├── ThreadPool.cpp/.h
│   │   ├── Timer.cpp/.h
│   │   └── UsageEnvironment.cpp/.h
│   │
│   ├── Stream/             # 推流与 RTP 封装模块
│   │   ├── AACFileMediaSource.cpp/.h
│   │   ├── AACFileSink.cpp/.h
│   │   ├── H264FileMediaSource.cpp/.h
│   │   ├── H264FileSink.cpp/.h
│   │   ├── Buffer.cpp/.h
│   │   ├── Rtp.cpp/.h
│   │   ├── RtspConnection.cpp/.h
│   │   ├── MediaSession.cpp/.h
│   │   ├── MediaSource.cpp/.h
│   │   ├── Sink.cpp/.h
│   │   └── TcpConnection.cpp/.h
│   │
│   └── Utils/              # 工具类
│       ├── Common.h
│       └── Log.h
│
├── main.cpp                # 主程序入口
└── CMakeLists.txt          # CMake 构建配置
```

---

### 编译与运行

#### **依赖环境**
- **C++11** 或更高版本
- **CMake** 3.10+
- Linux 系统或 Windows 系统

#### **构建步骤**

1. **克隆项目**
   ```bash
   git clone <repo-url>
   cd RTSP_ServerV2
   ```

2. **使用 CMake 构建项目**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```
3. **运行服务器**
   ```bash
   ./RTSP_ServerV2
   ```
---

### 文件格式支持

- **H.264**：视频流文件（通过 NALU 单元传输）  
- **AAC**：音频流文件（通过 ADTS 单元传输）  

将 H.264 和 AAC 文件放入 `data/` 目录中，服务器即可加载和推送这些文件。

---

### 设计说明

1. **事件管理模块**  
   - 使用 `EventScheduler` 和多种 `Poller`（`select`、`poll`、`epoll`）实现事件调度。
   - 通过高效的多路复用技术管理大量连接。

2. **推流模块**  
   - `H264FileMediaSource` 和 `AACFileMediaSource` 负责读取 H264 和 AAC 文件。
   - `H264FileSink` 和 `AACFileMediaSink` 负责封装NALU 和 ADTS 单元并通过RTP进行推送。  

3. **RTSP 信令管理**  
   - `RtspConnection` 和 `RtspServer` 负责处理 RTSP 信令交互。
---

### 使用示例

1. 将你的 H.264 和 AAC 文件放入 `data/` 目录中。
2. 在main.cpp中设置网址和端口，启动服务器，默认监听 `8554` 端口。
3. 使用ffmpeg命令连接服务器并播放流：
   
   TCP：
      ```
      ffplay -rtsp_transport tcp rtsp://127.0.0.1:8554/video1
      ```
   UDP:
      ```
      ffplay rtsp://127.0.0.1:8554/video1
      ```
---

### 适用场景
- 视频点播/直播服务器  
- 实时音视频流传输  
- 基于 RTSP 协议的测试与开发  

