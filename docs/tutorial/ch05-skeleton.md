# 第五章　客户端骨架——登录窗口

> **状态**：✅ 已完成
>
> **本章目标**：
>
> 1. 解决 CMake 配置报错，让 CLion 成功加载项目
> 2. 实现 `LoginWindow`：带样式的登录/注册双 Tab 窗口
> 3. 本地字段校验（不接网络，第六章再接入）
>
> **验收标准**：CLion 点击 Run → 弹出登录窗口，Tab 可切换，校验逻辑正常。

---

## 5.1 先解决构建问题

在写任何 UI 代码之前，先确认 CMake 能正常配置。

### 5.1.1 CLion 的构建目录

CLion 默认把构建目录命名为 `cmake-build-debug/` 和 `cmake-build-release/`，而不是通常的 `build/`。
如果 `.gitignore` 里只有 `build/`，构建产物就会被 git 追踪。

检查 `.gitignore`，确认包含：

```gitignore
cmake-build-debug/
cmake-build-release/
cmake-build-*/
```

### 5.1.2 common 库缺少源文件

如果你看到这个报错：

```
CMake Error: Cannot find source file: src/protocol_codec.cpp
```

原因是 `common/CMakeLists.txt` 列出了源文件，但文件还没创建。CMake 在**配置阶段**（不是编译阶段）就检查源文件是否存在。

解决方法：先创建空的桩文件（stub），让构建系统通过配置，第六章再填充实现。

**`common/src/protocol_codec.cpp`**（桩文件）：

```cpp
// 【第五章占位】本文件暂为空实现，第六章将在此实现：
//   - PDUBuilder：链式 API 构建二进制 PDU 数据包
//   - PDUParser ：接收字节流、检测粘包/半包、弹出完整 PDU
```

**`common/src/crypto_utils.cpp`**（桩文件）：

```cpp
// 【第五章占位】本文件暂为空实现，第六章将在此实现：
//   - crypto::hashPassword()     ：注册时生成带 salt 的存储哈希
//   - crypto::verifyPassword()   ：登录时校验密码
//   - crypto::hashForTransport() ：客户端发送前对密码做传输层哈希
```

创建完后，在 CLion 中点击菜单 **File → Reload CMake Project**，或点击工具栏的"重新加载"图标。

---

## 5.2 变量设计：先想清楚，再写代码

在写 `LoginWindow` 之前，先把需要哪些成员变量想清楚。

### 5.2.1 为什么控件要用指针？

Qt 窗口控件**必须**在堆上分配（用 `new`），原因是：

1. **parent-child 内存管理**：Qt 的 `QObject` 系统会在父对象析构时自动 `delete` 所有子对象。控件必须在堆上才能被父对象"收养"并管理生命周期。
2. **延迟初始化**：控件在 `setupUi()` 中构建，不能在构造函数初始化列表里初始化（初始化列表要求类型完整，而我们的头文件只有前向声明）。

所以成员变量全部是**裸指针**（raw pointer），但不需要手动 `delete`——Qt 帮我们管理。

### 5.2.2 命名规则

根据 `docs/reference/conventions.md`：

- 成员变量：`snake_case_`（尾部加下划线）
- 控件按 **所属区域 + 控件类型** 命名，一眼看出用途

### 5.2.3 完整成员变量清单

| 变量名 | 类型 | 所属 Tab | 作用 |
|--------|------|---------|------|
| `tab_widget_` | `QTabWidget*` | — | 登录/注册 Tab 容器 |
| `login_username_edit_` | `QLineEdit*` | 登录 | 用户名输入框 |
| `login_password_edit_` | `QLineEdit*` | 登录 | 密码输入框（Password 模式） |
| `login_btn_` | `QPushButton*` | 登录 | 「登录」按钮 |
| `login_status_label_` | `QLabel*` | 登录 | 错误提示（初始隐藏） |
| `reg_username_edit_` | `QLineEdit*` | 注册 | 注册用户名 |
| `reg_display_name_edit_` | `QLineEdit*` | 注册 | 昵称（显示名） |
| `reg_password_edit_` | `QLineEdit*` | 注册 | 密码（Password 模式） |
| `reg_confirm_edit_` | `QLineEdit*` | 注册 | 确认密码 |
| `reg_btn_` | `QPushButton*` | 注册 | 「创建账号」按钮 |
| `reg_status_label_` | `QLabel*` | 注册 | 注册结果提示（初始隐藏） |
| `server_dot_label_` | `QLabel*` | 底栏 | 彩色圆点 "●" |
| `server_addr_label_` | `QLabel*` | 底栏 | 服务器地址和状态文字 |

---

## 5.3 头文件：声明与前向声明

### 5.3.1 为什么用前向声明？

头文件中我们只需要告诉编译器"这些类型存在"，不需要它的完整定义——因为我们只用了指针（指针的大小是固定的，不需要知道类的完整布局）。

用前向声明替代 `#include`：
- 加快编译速度（避免级联包含）
- 减少头文件间的耦合

完整的 `#include` 放在 `.cpp` 文件里，只在真正需要调用方法时包含。

### 5.3.2 Q_OBJECT 宏

```cpp
class LoginWindow : public QWidget {
    Q_OBJECT
    ...
};
```

`Q_OBJECT` 是 Qt 的"魔法"宏，它触发 **MOC（元对象编译器）** 自动生成信号槽的胶水代码。

**少了 Q_OBJECT 会怎样**？`connect()` 仍然能编译，但信号不会触发槽函数，或者出现 `undefined reference to vtable` 链接错误。

**`client/src/ui/login_window.h`** 完整代码：

```cpp
#pragma once

#include <QWidget>

// 前向声明：告诉编译器这些类存在，但不包含完整头文件
class QTabWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override = default;

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    void setupUi();
    void setupStyle();
    void connectSignals();

    // Tab 容器
    QTabWidget* tab_widget_;

    // 登录 Tab
    QLineEdit*   login_username_edit_;
    QLineEdit*   login_password_edit_;
    QPushButton* login_btn_;
    QLabel*      login_status_label_;

    // 注册 Tab
    QLineEdit*   reg_username_edit_;
    QLineEdit*   reg_display_name_edit_;
    QLineEdit*   reg_password_edit_;
    QLineEdit*   reg_confirm_edit_;
    QPushButton* reg_btn_;
    QLabel*      reg_status_label_;

    // 底部服务器状态栏
    QLabel* server_dot_label_;
    QLabel* server_addr_label_;
};
```

---

## 5.4 布局系统详解

Qt 布局系统是理解 UI 代码的关键。

### 5.4.1 三种核心布局

| 布局类 | 排列方向 | 典型用途 |
|--------|---------|---------|
| `QVBoxLayout` | 垂直（从上到下） | 表单字段、页面主体 |
| `QHBoxLayout` | 水平（从左到右） | 工具栏、并排按钮 |
| `QGridLayout` | 网格（行列） | 复杂表单 |

### 5.4.2 布局的嵌套关系

```
LoginWindow（QWidget，宽 400px，背景 #F4F6F8）
└── main_layout（QVBoxLayout，外边距 40px）
    └── card（QWidget，白色背景，圆角 16px）
        └── card_layout（QVBoxLayout，内边距 32px，间距 24px）
            ├── header_widget（QWidget）
            │   └── header_layout（QHBoxLayout）
            │       ├── icon_label（QLabel "💬"）
            │       └── title_widget（QWidget）
            │           └── title_layout（QVBoxLayout）
            │               ├── app_name_label（"CloudVault"）
            │               └── subtitle_label（"即时通讯 · 文件传输"）
            ├── tab_widget_（QTabWidget）
            │   ├── Tab「登录」（QVBoxLayout）
            │   │   ├── 用户名标签 + 输入框
            │   │   ├── 密码标签 + 输入框
            │   │   ├── login_status_label_（隐藏）
            │   │   └── login_btn_
            │   └── Tab「注册」（QVBoxLayout）
            │       ├── name_row（QHBoxLayout：用户名 + 昵称并排）
            │       ├── 密码标签 + 输入框
            │       ├── 确认密码标签 + 输入框
            │       ├── reg_status_label_（隐藏）
            │       └── reg_btn_
            └── status_frame（QWidget，底部状态栏）
                └── status_layout（QHBoxLayout）
                    ├── server_dot_label_（"●"）
                    └── server_addr_label_（"127.0.0.1:5000 · 已就绪"）
```

### 5.4.3 关键方法说明

```cpp
// 设置布局与边缘的距离（左, 上, 右, 下）
layout->setContentsMargins(32, 32, 32, 32);

// 设置子控件之间的间距
layout->setSpacing(24);

// 添加弹性空间：把内容推向一侧，另一侧留空
layout->addStretch();

// 添加固定间距（px）
layout->addSpacing(8);
```

---

## 5.5 QSS 样式表

### 5.5.1 QSS vs CSS

QSS 语法与 CSS 非常相似，但有区别：

| 特性 | CSS | QSS |
|------|-----|-----|
| CSS 变量 | `var(--color)` | ❌ 不支持，需硬编码 |
| 类选择器 | `.className` | `ClassName`（类名本身） |
| ID 选择器 | `#id` | `#objectName`（需 `setObjectName()`）|
| 伪类 | `:hover` `:focus` | ✅ 支持（语法相同） |
| box-shadow | ✅ 支持 | ❌ 不支持 |

### 5.5.2 objectName 的作用

```cpp
// 代码中设置
card->setObjectName("card");

// QSS 中精确匹配
QWidget#card {
    background: white;
    border-radius: 16px;
}
```

如果不用 `objectName`，`QWidget { ... }` 会匹配**所有** QWidget，导致样式污染。

### 5.5.3 色彩系统（来自 ui-flow.md）

| 变量名（含义） | 色值 | 用途 |
|-------------|------|------|
| accent | `#3B82F6` | 按钮、选中 Tab、聚焦边框 |
| bg | `#F4F6F8` | 窗口背景 |
| surface | `#FFFFFF` | 卡片背景 |
| surface2 | `#F0F2F5` | 输入框背景、状态栏背景 |
| border | `#E2E6EA` | 输入框边框 |
| text | `#111827` | 主要文字 |
| text2 | `#6B7280` | 次级文字、标签 |
| danger | `#EF4444` | 错误提示 |
| success | `#22C55E` | 服务器在线圆点 |

---

## 5.6 信号槽机制

### 5.6.1 基本概念

- **信号（Signal）**：事件发生时发出的通知，如 `QPushButton::clicked`
- **槽（Slot）**：响应信号的函数，可以是任何普通函数或成员函数
- **connect()**：把信号和槽连接起来

```cpp
// 新式语法（Qt5+）：编译期类型检查，推荐使用
connect(发送者, &发送者类::信号,
        接收者, &接收者类::槽);

// 示例
connect(login_btn_, &QPushButton::clicked,
        this, &LoginWindow::onLoginClicked);
```

### 5.6.2 为什么需要 Q_OBJECT？

Qt 的信号槽不是标准 C++ 的一部分，需要 **MOC**（Meta-Object Compiler）在编译前生成额外的 C++ 代码。`Q_OBJECT` 宏告诉 MOC "这个类需要处理"。

CMake 的 `AUTOMOC ON`（在 `client/CMakeLists.txt` 中设置）会自动检测 `Q_OBJECT` 并运行 MOC。

---

## 5.7 登录/注册校验逻辑

### 5.7.1 onLoginClicked() 流程

```
读取输入（trimmed 去空白）
      │
      ▼
  任一为空？ ──是──▶ 显示 "用户名和密码不能为空" → return
      │否
      ▼
  禁用按钮，改文字为 "登录中…"
      │
      ▼
  qDebug() 输出（第六章替换为 AuthService::login()）
```

### 5.7.2 onRegisterClicked() 流程

```
读取四个字段
      │
      ▼
  任一为空？ ──是──▶ "所有字段均为必填项" → return
      │否
      ▼
  用户名长度 3–20？ ──否──▶ 提示 → return
      │是
      ▼
  密码 >= 8 位？ ──否──▶ "密码至少需要 8 位字符" → return
      │是
      ▼
  两次密码一致？ ──否──▶ "两次输入的密码不一致" → return
      │是
      ▼
  禁用按钮，改文字为 "注册中…"
      │
      ▼
  qDebug() 输出（第六章替换为 AuthService::registerUser()）
```

---

## 5.8 完整代码

### `client/src/main.cpp`

```cpp
#include <QApplication>
#include <QScreen>
#include "ui/login_window.h"

int main(int argc, char* argv[]) {
    // QApplication 必须是第一个创建的 Qt 对象
    QApplication app(argc, argv);

    app.setApplicationName("CloudVault");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("CloudVault");

    LoginWindow window;

    // 居中显示：availableGeometry() 返回去掉任务栏的可用区域
    const QRect screen_geo = QGuiApplication::primaryScreen()->availableGeometry();
    window.move(screen_geo.center() - window.rect().center());

    window.show();
    return app.exec();  // 启动事件循环，阻塞直到窗口关闭
}
```

### `client/src/ui/login_window.h`

见 §5.3.2 的完整代码。

### `client/src/ui/login_window.cpp`

见项目文件 `client/src/ui/login_window.cpp`（带逐行注释）。

---

## 5.9 在 CLion 中运行

### 5.9.1 首次配置

1. 打开 CLion，选择 **File → Open**，打开项目根目录（包含顶层 `CMakeLists.txt`）
2. CLion 会自动检测 CMake 并开始配置
3. 若配置失败，检查：
   - **Settings → Build, Execution, Deployment → CMake** 中确认 Generator 为 `Ninja` 或 `Unix Makefiles`
   - `cmake-build-debug/` 目录已被 `.gitignore` 排除

### 5.9.2 设置 Qt6 路径（若找不到 Qt）

CLion → Settings → Build → CMake → CMake options，添加：

```
-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
```

WSL 中 Qt 通常安装在 `~/Qt/6.x.x/gcc_64`。

### 5.9.3 选择运行目标

工具栏下拉菜单选择 `cloudvault_client`，点击绿色 Run 按钮（或 `Shift+F10`）。

正常输出：
```
CloudVault 登录窗口应弹出，宽约 400px
切换 Tab、输入内容、点击按钮均正常响应
控制台输出：[LoginWindow] 准备登录：用户名 = xxx
```

---

## 5.10 踩坑记录

### 坑 1：`undefined reference to vtable for LoginWindow`

**原因**：`Q_OBJECT` 宏存在但 MOC 没有运行，生成的 `moc_login_window.cpp` 缺失。

**解决**：
1. 确认 `client/CMakeLists.txt` 中有 `set(CMAKE_AUTOMOC ON)`
2. 清空 `cmake-build-debug/` 目录，重新配置

### 坑 2：窗口样式全部丢失（QSS 不生效）

**原因**：`setObjectName()` 和 QSS 中的 `#objectName` 拼写不一致。

**解决**：对照检查代码中 `setObjectName("xxx")` 和 QSS 中 `#xxx` 是否完全匹配（大小写敏感）。

### 坑 3：密码框内容可见

**原因**：忘记设置 `setEchoMode(QLineEdit::Password)`。

**解决**：在 `setupUi()` 中对所有密码输入框调用此方法。

### 坑 4：`QGuiApplication::primaryScreen()` 返回 nullptr（WSL2）

**原因**：WSL2 没有连接显示器时 Qt 无法获取屏幕信息。

**解决**：在 `window.move()` 前加空指针检查：

```cpp
if (auto* screen = QGuiApplication::primaryScreen()) {
    const QRect geo = screen->availableGeometry();
    window.move(geo.center() - window.rect().center());
}
```

---

## 本章新知识点

- **QWidget 布局系统**：QVBoxLayout / QHBoxLayout 嵌套，setContentsMargins / setSpacing / addStretch
- **Qt parent-child 内存管理**：为什么控件用指针，为什么不需要手动 delete
- **QSS 样式表**：objectName 选择器、伪类（:focus / :hover / :disabled）、与 CSS 的区别
- **信号槽新式语法**：`connect(&Sender::signal, &Receiver::slot)`，编译期类型检查
- **Q_OBJECT 与 AUTOMOC**：MOC 如何被 CMake 自动触发

---

上一章：[第四章 — 工程初始化与 CMake 配置](ch04-cmake.md) ｜ 下一章：[第六章 — 协议基础](ch06-protocol.md)
