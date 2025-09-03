# C++ 实用工具库

一个包含常用 C++ 工具、模式和组件的集合，适用于学习和实际使用。

> **注意**: 本项目目前**正在开发和学习中**。代码作为我 C++ 学习过程的一部分正在不断改进和完善。欢迎贡献和建议！

## 📦 包含组件

### 核心工具
- **NonCopyable**: 禁止对象拷贝的基类
- **Singleton**: 线程安全的单例模式实现
- **BlockingQueue**: 采用生产者-消费者模式的线程安全队列
- **ThreadPool**: 高效的任务队列线程池实现

### 构建系统
- 基于 CMake 的构建系统
- 库和测试的分离编译
- 易于集成到其他项目中

## 🚀 开始使用

### 环境要求
- C++17 兼容编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- Google Test (用于运行测试)

### 构建项目

```bash
# 克隆仓库
git clone https://github.com/yzyJuanW/CppCoreUtils.git
cd CppCoreUtils

# 创建构建目录
bash run.sh
```

### 简单示例

```cpp
#include "utils/thread_pool.hpp"

// 创建包含4个工作线程的线程池
ywh::utils::ThreadPool pool(4);

// 向池中提交任务
for (int i = 0; i < 10; i++) {
    pool.Enqueue([i] {
        std::cout << "任务 " << i << " 已执行\n";
    });
}
```

## 📁 项目结构

```
cpp-utilities/
├── include/utils/          # 公共头文件
│   ├── blocking_queue.hpp  # 线程安全队列实现
│   ├── nocopyable.h        # 禁止拷贝基类
│   ├── singleton.h         # 单例模式模板
│   └── thread_pool.hpp     # 线程池接口
├── src/utils/              # 实现文件
│   └── thread_pool.cc      # 线程池实现
├── test/                   # 测试文件
│   ├── test.cxx            # 通用测试
│   └── test_thread_pool.cxx # 线程池专项测试
├── CMakeLists.txt          # 根 CMake 配置
└── run.sh                  # 构建和测试脚本
```

## 🛠️ 使用方法

### 集成到您的项目

您可以轻松地将此库集成到您的 CMake 项目中：

```cmake
# 添加为子目录
add_subdirectory(path/to/cpp-utilities)

# 链接库
target_link_libraries(您的目标 PUBLIC utils)
```

### 仅头文件组件

某些组件是仅头文件的，可以直接使用：

```cpp
#include "utils/singleton.h"
#include "utils/nocopyable.h"
```

## 🔧 开发状态

本项目正作为学习练习积极开发中。当前重点领域：

- [x] 基本线程池实现
- [x] 阻塞队列与生产者-消费者模式
- [x] 单例和禁止拷贝模式
- [ ] 数据库连接池（计划中）
- [ ] 额外的同步原语
- [ ] 性能基准测试
- [ ] 更全面的测试覆盖

## 🤝 贡献方式

由于这是一个学习项目，我欢迎：

- 代码审查和建议
- 错误报告和修复
- 实现改进
- 文档增强
- 额外工具的功能请求

请随时提出问题或提交拉取请求！

## 📝 许可证

本项目采用 MIT 许可证 - 详见 LICENSE 文件。

## 🎯 学习目标

通过这个项目，我专注于：

- 现代 C++ 特性和惯用法
- 线程安全和并发模式
- 内存管理和 RAII
- 模板元编程
- CMake 构建系统最佳实践
- 使用 Google Test 进行测试驱动开发
- 代码组织和架构

## 📚 参考资源

本实现借鉴了：

- 《C++ Concurrency in Action》by Anthony Williams
- 《Effective Modern C++》by Scott Meyers
- 各种开源 C++ 项目
- C++ 标准库实现

---

*本项目是我持续深入学习 C++ 和软件设计模式旅程的一部分。随着我学习和提高技能，代码可能会有重大变化。*