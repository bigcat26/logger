# Logger库设计文档

## 1. 架构设计

### 1.1 整体架构

Logger库采用模块化、分层的架构设计，主要由以下核心组件构成：

- **Logger核心**：日志系统的核心组件，负责协调各模块工作
- **Appender**：日志输出目标组件，负责将日志发送到不同的目标
- **Layout**：日志格式化组件，负责将日志事件转换为特定格式的字符串
- **Filter**：日志过滤组件，负责根据规则过滤日志
- **OS适配层**：负责提供跨平台的操作系统接口支持

这种设计使得各组件之间松耦合，可以灵活组合和替换，便于扩展和定制。

### 1.2 数据流设计

日志记录的主要数据流如下：

1. 应用程序调用日志API生成日志事件
2. Logger核心接收日志事件并进行初步处理
3. 日志事件通过Layout组件进行格式化
4. 格式化后的日志通过Filter进行过滤
5. 过滤后的日志通过Appender输出到目标位置

### 1.3 线程安全设计

- 使用互斥锁（mutex）保护关键数据结构的访问
- 支持用户提供自定义的锁实现，以适应不同平台
- 单Appender模式下可选择禁用锁以提高性能

## 2. 模块设计

### 2.1 Logger核心模块

#### 2.1.1 主要功能

- 管理Appender、Layout和Filter组件
- 提供统一的日志记录API
- 维护全局默认Logger实例
- 处理日志级别过滤

#### 2.1.2 数据结构

```c
struct logger_t {
#ifndef LOGGER_SINGLE_APPENDER
  logger_lock_t *lock;  // 用于保护多Appender情况下的线程安全
#endif
  struct logger_appender_t *appenders;  // 日志输出目标链表
};
```

#### 2.1.3 关键API

- `logger_appender_create_stdout()`：创建控制台输出Appender
- `logger_appender_create_syslog()`：创建Syslog输出Appender
- `logger_appender_create_file()`：创建文件输出Appender

### 2.2 Appender模块

#### 2.2.1 功能概述
Appender负责将日志输出到不同的目标位置，如控制台、文件、网络等。

#### 2.2.2 主要实现
- **控制台Appender**：输出到标准输出设备
- **文件Appender**：写入文件系统
- **Syslog Appender**：发送到系统syslog服务
- **UDP Appender**：通过网络UDP协议发送

#### 2.2.3 数据结构
```c
struct logger_appender_t {
  logger_lock_t *lock;  // 保护Appender内部状态
  struct logger_appender_t *next;  // 指向下一个Appender
  struct logger_layout_t *layout;  // 关联的Layout
  struct logger_filter_t *filters;  // 关联的Filter链表
  int (*write)(struct logger_appender_t *, const struct logger_event_t *);  // 写入函数
  char *buf;  // 内部缓冲区
  size_t buf_size;  // 缓冲区大小
};
```

### 2.3 Layout模块

#### 2.3.1 功能概述
Layout负责将日志事件转换为特定格式的字符串，决定了日志的最终呈现形式。

#### 2.3.2 主要实现
- **完整格式（Full Layout）**：包含时间戳、日志级别、文件名、行号等完整信息
- **简单格式（Simple Layout）**：提供简洁的日志格式
- **Syslog格式（Syslog Layout）**：符合Syslog协议标准的格式

#### 2.3.3 数据结构
```c
struct logger_layout_t {
  int (*format)(struct logger_layout_t *, const struct logger_event_t *, char *, int);  // 格式化函数
};
```

### 2.4 Filter模块

#### 2.4.1 功能概述
Filter负责根据规则过滤日志，决定哪些日志需要被输出，哪些需要被丢弃。

#### 2.4.2 实现方式
- 支持级别过滤：根据日志级别进行过滤
- 支持链式过滤：多个Filter可以串联使用
- 支持自定义过滤函数：通过回调函数实现灵活的过滤逻辑

#### 2.4.3 数据结构
```c
typedef int (*logger_filter_fn)(struct logger_filter_t *filter, struct logger_event_t *, void *);

struct logger_filter_t {
  struct logger_filter_t *next;  // 指向下一个Filter
  logger_filter_fn accept;  // 过滤函数
  void *user;  // 用户数据
};
```

### 2.5 日志事件模块

#### 2.5.1 功能概述
日志事件包含了一条日志的所有相关信息，是Logger系统内部传递的基本数据单元。

#### 2.5.2 数据结构
```c
struct logger_event_t {
  log_level_t level;  // 日志级别
  uint32_t line;  // 源代码行号
  uint64_t timestamp_ns;  // 时间戳（纳秒）
  const char *file;  // 源文件路径
  const char *func;  // 函数名
  uint16_t tid;  // 线程ID
  uint16_t tag_id;  // 标签ID
  uint16_t msg_len;  // 消息长度
  uint16_t bin_len;  // 二进制数据长度
  const char *data;  // 日志数据
};
```

## 3. 接口规范

### 3.1 公共API

#### 3.1.1 初始化和配置
- `logger_init()`：初始化Logger系统
- `logger_deinit()`：销毁Logger系统
- `logger_config()`：配置Logger参数

#### 3.1.2 日志记录
- `logger_printf()`：格式化输出日志，包含文件和行号信息
- `logger_catf()`：简洁的日志输出函数
- `logger_printb()`：输出二进制数据日志

#### 3.1.3 Appender操作
- `logger_appender_create_stdout()`：创建标准输出Appender
- `logger_appender_create_file()`：创建文件Appender
- `logger_appender_create_syslog()`：创建Syslog Appender
- `logger_appender_create_udp()`：创建UDP Appender
- `logger_add_appender()`：向Logger添加Appender

#### 3.1.4 Layout操作
- `logger_layout_create_full()`：创建完整格式Layout
- `logger_layout_create_simple()`：创建简单格式Layout
- `logger_layout_create_syslog()`：创建Syslog格式Layout
- `logger_appender_set_layout()`：为Appender设置Layout

#### 3.1.5 Filter操作
- `logger_filter_create_level()`：创建级别过滤器
- `logger_filter_create_custom()`：创建自定义过滤器
- `logger_appender_add_filter()`：为Appender添加过滤器

### 3.2 内部接口

#### 3.2.1 操作系统适配层
- `logger_lock_acquire()`：获取锁
- `logger_lock_release()`：释放锁
- `logger_time()`：获取当前时间

#### 3.2.2 模块间交互接口
- `logger_event_create()`：创建日志事件
- `logger_event_destroy()`：销毁日志事件
- `logger_dispatch_event()`：分发日志事件

## 4. 配置和使用指南

### 4.1 基本配置
Logger库支持以下关键配置项：
- 缓冲区大小：控制日志输出的缓冲区大小
- 默认日志级别：设置默认的日志过滤级别
- 线程安全选项：启用或禁用线程安全机制

### 4.2 使用示例

#### 4.2.1 基本用法
```c
#include "logger/logger.h"

int main(void) {
    // 初始化Logger
    logger_init();
    
    // 创建并配置Appender
    struct logger_appender_t *console_appender = logger_appender_create_stdout();
    logger_add_appender(console_appender);
    
    // 记录不同级别的日志
    LOG_DEBUG("这是一条调试日志");
    LOG_INFO("这是一条信息日志");
    LOG_WARN("这是一条警告日志");
    LOG_ERROR("这是一条错误日志");
    
    // 清理资源
    logger_deinit();
    return 0;
}
```

#### 4.2.2 高级用法
```c
#include "logger/logger.h"
#include "logger/layout_full.h"
#include "logger/appender_file.h"

int main(void) {
    // 初始化Logger
    struct LOGGER_CFG cfg = {0};
    char log_buf[4096];
    cfg.buf = log_buf;
    cfg.buf_size = sizeof(log_buf);
    logger_init_with_cfg(&cfg);
    
    // 创建控制台Appender
    struct logger_appender_t *console_appender = logger_appender_create_stdout();
    
    // 创建文件Appender
    struct logger_appender_t *file_appender = logger_appender_create_file();
    struct LOGGER_APPENDER_FILE_CFG file_cfg = {0};
    file_cfg.filename = "app.log";
    file_cfg.max_size = 1024 * 1024;  // 1MB
    logger_appender_file_set_cfg(file_appender, &file_cfg);
    
    // 设置Layout
    struct logger_layout_t *full_layout = logger_layout_create_full();
    logger_appender_set_layout(console_appender, full_layout);
    logger_appender_set_layout(file_appender, full_layout);
    
    // 添加过滤器
    struct logger_filter_t *level_filter = logger_filter_create_level(LOG_LEVEL_INFO);
    logger_appender_add_filter(console_appender, level_filter);
    
    // 添加到Logger
    logger_add_appender(console_appender);
    logger_add_appender(file_appender);
    
    // 记录日志
    LOG_DEBUG("这条调试日志不会显示在控制台，但会写入文件");
    LOG_INFO("这条信息日志会同时显示在控制台和写入文件");
    
    // 清理资源
    logger_deinit();
    return 0;
}
```

## 5. 性能与优化

### 5.1 内存占用优化

- 使用固定大小的缓冲区，避免频繁的内存分配
- 支持静态配置，减少运行时内存消耗
- 模块化设计，只链接使用的功能组件

### 5.2 性能优化

- 支持异步日志记录模式，减少对主程序的阻塞
- 使用缓冲区批量写入，减少I/O操作次数
- 优化日志格式化算法，提高处理效率

### 5.3 资源限制考虑

- 支持日志文件轮转，避免单个文件过大
- 支持磁盘空间检查，避免因日志耗尽存储空间
- 支持紧急情况下的日志降级处理

## 6. 扩展性设计

### 6.1 自定义组件
Logger库设计为高度可扩展的架构，用户可以通过以下方式扩展系统：

#### 6.1.1 自定义Appender

1. 创建一个新的结构体，继承`logger_appender_t`
2. 实现`write`函数，定义日志输出逻辑
3. 提供创建和初始化函数

#### 6.1.2 自定义Layout

1. 创建一个新的结构体，继承`logger_layout_t`
2. 实现`format`函数，定义日志格式化逻辑
3. 提供创建和初始化函数

#### 6.1.3 自定义Filter

1. 实现`logger_filter_fn`类型的过滤函数
2. 使用`logger_filter_create_custom`创建过滤器
3. 添加到需要的Appender

## 7. 平台适配

### 7.1 操作系统支持

- Windows：使用Windows API实现底层功能
- Linux/macOS：使用POSIX标准API实现
- 嵌入式系统：支持自定义OS适配层

### 7.2 编译器兼容性

- 支持GCC、Clang、MSVC等主流编译器
- 兼容C89/C99/C11标准
- 支持不同的处理器架构

### 7.3 特殊环境适配

- 无标准C库环境：提供最小化实现
- 无操作系统环境：支持裸机运行
- 资源极度受限环境：提供精简配置选项