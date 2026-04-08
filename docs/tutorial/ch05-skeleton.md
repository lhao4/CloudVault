# 第五章　项目骨架——客户端登录窗口与服务端最小运行

> **状态**：✅ 已完成
>
> **本章目标**：
>
> 1. 解决 CMake 配置报错，让 CLion 成功加载客户端项目
> 2. 用 Qt Designer `.ui` 文件实现 `LoginWindow`：带样式的登录/注册双 Tab 窗口
> 3. 本地字段校验、密码显示/隐藏、应用图标（不接网络，第六章再接入）
> 4. 搭建服务端骨架：读取 JSON 配置、初始化 spdlog、优雅处理 SIGINT/SIGTERM
>
> **验收标准（客户端）**：CLion 点击 Run → 弹出登录窗口，Tab 可切换，校验逻辑正常。
>
> **验收标准（服务端）**：WSL 终端运行 `./cloudvault_server` → 打印启动横幅 → `Ctrl+C` 优雅退出。

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

## 5.2 AUTOUIC：用 .ui 文件替代手写布局

### 5.2.1 两种写 UI 的方式

Qt 有两种组织界面的方式：

| 方式 | 优点 | 缺点 |
|------|------|------|
| 纯代码（手写 `new QLabel()`） | 完全掌控，适合动态生成 | 几百行布局代码，难维护 |
| `.ui` 文件（Qt Designer XML） | 布局与逻辑分离，所见即所得 | 需要 AUTOUIC 工具链 |

本项目采用 **`.ui` 文件** 方式。CMake 中已开启：

```cmake
set(CMAKE_AUTOUIC ON)   # 自动运行 UIC，将 .ui 转换为 ui_*.h
set(CMAKE_AUTOMOC ON)   # 自动运行 MOC，处理 Q_OBJECT 宏
set(CMAKE_AUTORCC ON)   # 自动运行 RCC，将 .qrc 资源编译进二进制
```

### 5.2.2 AUTOUIC 的工作原理

```
login_window.ui  ──UIC──▶  ui_login_window.h（自动生成）
                                │
                                │ 包含 Ui::LoginWindow 结构体
                                │ 包含 setupUi(QWidget*) 方法
                                ▼
login_window.cpp  ──▶  ui_->setupUi(this)  ──▶  所有控件被创建并挂载
```

`setupUi(this)` 一行代码等价于以前几百行的 `new QLabel / new QLineEdit / layout->addWidget(...)` 。

### 5.2.3 成员变量：只需一个 unique_ptr

旧的手写方式需要声明十几个成员指针：

```cpp
// ❌ 手写方式：臃肿
QTabWidget*  tab_widget_;
QLineEdit*   login_username_edit_;
QPushButton* login_btn_;
// ... 还有十几个
```

AUTOUIC 方式只需要**一个智能指针**：

```cpp
// ✅ AUTOUIC 方式：简洁
std::unique_ptr<Ui::LoginWindow> ui_;
```

所有控件通过 `ui_->控件名` 访问，控件名来自 `.ui` 文件中的 `objectName`。

---

## 5.3 头文件：声明与前向声明

### 5.3.1 为什么用前向声明？

`Ui::LoginWindow` 的完整定义在 AUTOUIC 生成的 `ui_login_window.h` 中（位于构建目录）。
头文件只用前向声明，不 `#include "ui_login_window.h"`，原因：

- 加快编译速度（避免级联包含）
- 减少头文件间的耦合

完整的 `#include "ui_login_window.h"` 放在 `.cpp` 文件里。

### 5.3.2 unique_ptr 的析构函数陷阱

```cpp
// login_window.h
std::unique_ptr<Ui::LoginWindow> ui_;   // 前向声明，Ui::LoginWindow 是不完整类型
```

`unique_ptr` 析构时需要知道 `Ui::LoginWindow` 的完整大小，而头文件里只有前向声明——**编译器无法生成默认析构函数**。

解决方案：在头文件**声明** `~LoginWindow()`，在 `.cpp`（已包含完整定义）**定义**为 `= default`：

```cpp
// login_window.h
~LoginWindow() override;       // 声明，阻止编译器 inline 生成

// login_window.cpp
LoginWindow::~LoginWindow() = default;   // 此处 Ui::LoginWindow 已完整
```

### 5.3.3 完整头文件

**`client/src/ui/login_window.h`**：

```cpp
#pragma once

#include <QWidget>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }   // 前向声明
QT_END_NAMESPACE

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override;

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void toggleLoginPwdVisibility();
    void toggleRegPwdVisibility();
    void toggleRegConfirmVisibility();

private:
    void setupStyle();
    void connectSignals();
    void resetLoginBtn();
    void resetRegBtn();

    std::unique_ptr<Ui::LoginWindow> ui_;
};
```

---

## 5.4 布局结构

`.ui` 文件描述以下控件树：

```
LoginWindow（QWidget，最小 400×560）
└── mainLayout（QVBoxLayout，外边距 40/40/40/32）
    └── card（QWidget，透明）
        └── cardLayout（QVBoxLayout，间距 20）
            ├── headerWidget（QHBoxLayout）
            │   ├── iconLabel（QLabel，40×40，显示应用图标）
            │   └── titleWidget
            │       ├── appNameLabel（"CloudVault"）
            │       └── appSubtitleLabel（"即时通讯 · 文件传输"）
            ├── tabWidget（QTabWidget）
            │   ├── Tab「登录」（QVBoxLayout）
            │   │   ├── 用户名标签 + 输入框
            │   │   ├── 密码标签 + [输入框 | 显示按钮]（QHBoxLayout）
            │   │   ├── loginStatusLabel（错误提示，初始隐藏）
            │   │   └── loginBtn
            │   └── Tab「注册」（QVBoxLayout）
            │       ├── [用户名输入框 | 昵称输入框]（QHBoxLayout）
            │       ├── 密码标签 + [输入框 | 显示按钮]
            │       ├── 确认密码标签 + [输入框 | 显示按钮]
            │       ├── regStatusLabel（初始隐藏）
            │       └── regBtn
            └── statusFrame（QWidget，底部服务器状态栏）
                ├── serverDotLabel（"●"）
                └── serverAddrLabel（"127.0.0.1:5000 · 已就绪"）
```

---

## 5.5 QSS 主题系统

### 5.5.1 QSS vs CSS

QSS 语法与 CSS 非常相似，但有区别：

| 特性 | CSS | QSS |
|------|-----|-----|
| CSS 变量 | `var(--color)` | ❌ 不支持 |
| 类选择器 | `.className` | `ClassName`（类名本身） |
| ID 选择器 | `#id` | `#objectName` |
| 伪类 | `:hover` `:focus` | ✅ 支持 |
| box-shadow | ✅ | ❌ |

### 5.5.2 objectName 的作用

```cpp
// QSS 中精确匹配某个控件
QWidget#card { background: white; }    // 只匹配 objectName="card" 的 QWidget
QWidget { background: white; }         // ❌ 匹配所有 QWidget，样式污染
```

### 5.5.3 @token 主题常量系统

QSS 不支持 CSS 变量，但我们可以用 **C++ 常量 + 字符串替换** 模拟：

```cpp
// login_window.cpp 顶部，单一可信来源
namespace {
const QPair<const char*, const char*> kTheme[] = {
    {"@bgPage",         "#F4F6F8"},  // 页面背景
    {"@bgField",        "#F0F2F5"},  // 输入框背景
    {"@accent",         "#3B82F6"},  // 品牌蓝
    {"@accentHover",    "#2563EB"},  // 品牌蓝悬停
    {"@error",          "#EF4444"},  // 错误红
    // ...
};
}
```

```cpp
void LoginWindow::setupStyle() {
    QString qss = QStringLiteral(R"(
        LoginWindow { background-color: @bgPage; }
        QPushButton#loginBtn { background: @accent; }
        QPushButton#loginBtn:hover { background: @accentHover; }
        ...
    )");

    // 将 @token 替换为实际颜色值
    for (const auto& [token, value] : kTheme) {
        qss.replace(QLatin1String(token), QLatin1String(value));
    }
    setStyleSheet(qss);
}
```

**好处**：修改主题只需改 `kTheme` 表，QSS 模板不动。

### 5.5.4 色彩语义表

| Token | 色值 | 语义 |
|-------|------|------|
| `@bgPage` | `#F4F6F8` | 窗口背景 |
| `@bgField` | `#F0F2F5` | 输入框背景 |
| `@bgFieldFocus` | `#FFFFFF` | 输入框聚焦背景 |
| `@border` | `#E2E6EA` | 默认边框 |
| `@txtPrimary` | `#111827` | 主文字 |
| `@txtSecondary` | `#6B7280` | 次要文字 |
| `@accent` | `#3B82F6` | 品牌蓝（按钮/高亮） |
| `@accentHover` | `#2563EB` | 品牌蓝悬停 |
| `@accentDisabled` | `#93C5FD` | 品牌蓝禁用 |
| `@error` | `#EF4444` | 错误红 |
| `@statusBg` | `#F0FDF4` | 状态栏背景（绿） |
| `@statusDot` | `#16A34A` | 在线状态点 |

---

## 5.6 应用图标与资源文件

### 5.6.1 Qt 资源系统（.qrc）

图片、图标等静态资源通过 `.qrc` 文件编译进可执行文件，不依赖外部路径：

**`client/resources/resources.qrc`**：

```xml
<!DOCTYPE RCC>
<RCC version="1.0">
    <qresource prefix="/icons">
        <file>icons/app_icon.png</file>
    </qresource>
</RCC>
```

运行时通过 `:/icons/app_icon.png` 访问（`:/` 是 Qt 资源系统的虚拟根路径）。

`CMAKE_AUTORCC ON` 会自动检测 `.qrc` 并编译。**添加新的 `.qrc` 文件后需要 Reload CMake Project**。

### 5.6.2 在代码中使用图标

```cpp
// main.cpp：设置任务栏/标题栏图标
app.setWindowIcon(QIcon(":/icons/app_icon.png"));

// login_window.cpp：设置 header 里的图标标签
ui_->iconLabel->setPixmap(QPixmap(":/icons/app_icon.png"));
```

> ⚠️ 不要在 `.ui` 文件的 `<pixmap>` 属性里引用资源，设计时方便但运行时不可靠。改在代码里显式加载。

---

## 5.7 信号槽机制

### 5.7.1 基本概念

- **信号（Signal）**：事件发生时发出的通知，如 `QPushButton::clicked`
- **槽（Slot）**：响应信号的函数
- **connect()**：把信号和槽连接起来

```cpp
// 新式语法（Qt5+）：编译期类型检查
connect(ui_->loginBtn, &QPushButton::clicked,
        this, &LoginWindow::onLoginClicked);

// 信号参数可以多于槽参数，多余的参数被丢弃
connect(ui_->loginPwdToggleBtn, &QPushButton::toggled,   // toggled(bool)
        this, &LoginWindow::toggleLoginPwdVisibility);    // 无参数槽，bool 被丢弃
```

### 5.7.2 为什么需要 Q_OBJECT？

Qt 的信号槽不是标准 C++，需要 **MOC**（Meta-Object Compiler）在编译前生成额外代码。`Q_OBJECT` 宏告诉 MOC "这个类需要处理"。

`CMAKE_AUTOMOC ON` 会自动检测 `Q_OBJECT` 并运行 MOC。

---

## 5.8 密码显示/隐藏

每个密码框右侧有一个可勾选按钮，点击切换明文/密文：

```cpp
// .ui 文件：密码框行布局（QHBoxLayout）
// [QLineEdit loginPasswordEdit] [QPushButton loginPwdToggleBtn checkable=true]

void LoginWindow::toggleLoginPwdVisibility() {
    const bool show = ui_->loginPwdToggleBtn->isChecked();
    ui_->loginPasswordEdit->setEchoMode(
        show ? QLineEdit::Normal : QLineEdit::Password);
    ui_->loginPwdToggleBtn->setText(show ? "隐藏" : "显示");
}
```

注册 Tab 的两个密码框同理（`regPwdToggleBtn`、`regConfirmToggleBtn`）。

---

## 5.9 登录/注册校验逻辑

### 5.9.1 onLoginClicked() 流程

```
读取输入（username.trimmed()，password 原值保留）
      │
      ▼
  username 为空 或 password.trimmed() 为空？
  ──是──▶ 显示 "用户名和密码不能为空" → return
      │否
      ▼
  禁用按钮，改文字为 "登录中…"
      │
      ▼
  qCDebug(lcLogin) 输出
  TODO（第六章）：AuthService::login(username, password)
  当前占位：直接调用 resetLoginBtn() 恢复按钮
```

> **密码为何不做 `trimmed()`？**
> 密码前后的空格可能是有意设置的，不应裁剪。但用 `password.trimmed().isEmpty()` 检测"仅含空格"的无效输入。

### 5.9.2 onRegisterClicked() 流程

```
读取四个字段
      │
      ▼
  任一字段 trimmed() 为空？ ──是──▶ "所有字段均为必填项" → return
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
  TODO（第六章）：AuthService::registerUser(username, display_name, password)
```

### 5.9.3 按钮状态恢复

第六章接入 `AuthService` 后，`resetLoginBtn()` / `resetRegBtn()` 会在服务回调（`onLoginFailed` / `onRegisterFailed`）中调用。本章暂时在 TODO 后直接调用作为占位。

---

## 5.10 结构化日志

不用 `qDebug()`，改用分类日志，可按需开关：

```cpp
// login_window.cpp 顶部
Q_LOGGING_CATEGORY(lcLogin, "ui.login")

// 使用
qCDebug(lcLogin) << "准备登录：用户名 =" << username;
```

运行时启用：

```bash
# 环境变量方式
QT_LOGGING_RULES="ui.login=true" ./cloudvault_client

# 或在 main.cpp 中代码控制
QLoggingCategory::setFilterRules("ui.login=true");
```

---

## 5.11 在 CLion 中运行

### 5.11.1 首次配置

1. 打开 CLion，选择 **File → Open**，打开 `client/` 目录
2. CLion 自动检测 `client/CMakeLists.txt` 并开始配置
3. 若配置失败，检查 **Settings → Build → CMake** 中 Qt6_DIR 是否正确

### 5.11.2 设置 Qt6 路径

CLion → Settings → Build → CMake → CMake options，添加：

```
-DQt6_DIR=C:/Qt/6.x.x/msvc2022_64/lib/cmake/Qt6
```

### 5.11.3 添加新资源文件后

每次新增 `.qrc` 或 `.ui` 文件，需要：

**File → Reload CMake Project**（让 `GLOB_RECURSE` 重新扫描）

### 5.11.4 选择运行目标

工具栏下拉菜单选择 `cloudvault_client`，点击绿色 Run 按钮（或 `Shift+F10`）。

正常结果：
```
CloudVault 登录窗口弹出，宽约 400px
Tab 可切换，密码框右侧有"显示"按钮
应用图标显示在标题栏和任务栏
控制台无报错（qCDebug 默认不输出）
```

---

## 5.12 踩坑记录

### 坑 1：`undefined reference to vtable for LoginWindow`

**原因**：`Q_OBJECT` 宏存在但 MOC 没有运行。

**解决**：确认 `CMakeLists.txt` 中有 `set(CMAKE_AUTOMOC ON)`，清空 `cmake-build-debug/` 重新配置。

### 坑 2：窗口样式全部丢失

**原因**：`objectName` 和 QSS 中的 `#objectName` 拼写不一致（大小写敏感）。

**解决**：对照 `.ui` 文件中的 `objectName` 属性和 QSS 选择器。

### 坑 3：图标加载不出来

**原因**：新增 `.qrc` 后没有 Reload CMake Project，资源未编译进可执行文件。

**解决**：File → Reload CMake Project → Rebuild。另外，图标应在代码中通过 `QPixmap(":/icons/app_icon.png")` 加载，不要依赖 `.ui` 文件的 `<pixmap>` 属性。

### 坑 4：`unique_ptr` 报 incomplete type 错误

**原因**：`~LoginWindow()` 的默认实现被编译器 inline 生成在头文件，此时 `Ui::LoginWindow` 还是前向声明（不完整类型）。

**解决**：在头文件声明 `~LoginWindow() override;`，在 `.cpp`（已包含 `ui_login_window.h`）定义为 `= default`。

### 坑 5：`QGuiApplication::primaryScreen()` 返回 nullptr（WSL2）

**原因**：WSL2 没有连接显示器时 Qt 无法获取屏幕信息。

**解决**：

```cpp
if (auto* screen = QGuiApplication::primaryScreen()) {
    window.move(screen->availableGeometry().center() - window.rect().center());
}
```

---

---

## 5.13 服务端骨架（WSL 编译运行）

### 5.13.1 设计目标

服务端骨架和客户端骨架遵循同一原则：**能编译、能运行、结构完整，业务逻辑留 TODO**。

本章服务端骨架实现：
- 读取 JSON 配置文件（nlohmann/json）
- 初始化 spdlog（控制台 + 轮转文件双 sink）
- 打印启动横幅
- 捕获 SIGINT / SIGTERM，优雅关闭

### 5.13.2 项目结构

```
server/
├── CMakeLists.txt
├── config/
│   └── server.example.json   # 配置模板（server.json 被 gitignore）
├── include/server/
│   ├── server_app.h          # 本章实现
│   ├── event_loop.h          # TODO（第七章）
│   ├── tcp_server.h          # TODO（第七章）
│   ├── thread_pool.h         # TODO（第七章）
│   ├── session_manager.h     # TODO（第八章）
│   ├── message_dispatcher.h  # TODO（第八章）
│   ├── file_storage.h        # TODO（第十一章）
│   ├── db/
│   │   ├── database.h        # TODO（第八章）
│   │   ├── user_repository.h # TODO（第八章）
│   │   └── friend_repository.h
│   └── handler/
│       ├── auth_handler.h    # TODO（第九章）
│       ├── friend_handler.h  # TODO（第九章）
│       ├── chat_handler.h    # TODO（第十章）
│       └── file_handler.h    # TODO（第十一章）
└── src/
    ├── main.cpp
    └── server_app.cpp        # 本章实现
```

所有 `TODO` 头文件目前只有 `#pragma once`，作用是让 IDE 的代码补全能感知到整体架构。

### 5.13.3 CMake：系统包替代 FetchContent

WSL 下 `FetchContent` 需要联网，在 `/mnt/d/`（NTFS 挂载）上还会遇到权限问题。改用系统包：

```bash
sudo apt install -y libspdlog-dev nlohmann-json3-dev libssl-dev libgtest-dev
```

`server/CMakeLists.txt` 关键配置：

```cmake
# 系统包，无需联网
find_package(spdlog        REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL       REQUIRED)

# MySQL 骨架阶段可选，第八章改回 REQUIRED
find_package(MySQL)
if(MySQL_FOUND)
    target_link_libraries(cloudvault_server PRIVATE MySQL::MySQL)
    target_compile_definitions(cloudvault_server PRIVATE HAVE_MYSQL)
endif()
```

测试子目录同理，`find_package(GTest REQUIRED)` + 空源文件守卫：

```cmake
find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
if(NOT TEST_SOURCES)
    message(STATUS "cloudvault_server_tests: 暂无测试文件，跳过构建")
    return()
endif()
```

### 5.13.4 ServerApp 头文件

**`server/include/server/server_app.h`**：

```cpp
#pragma once
#include <string>

class ServerApp {
public:
    ServerApp();
    ~ServerApp();

    // 加载配置、初始化日志；失败返回 false
    bool init(const std::string& config_path);

    // 启动服务，阻塞直到收到 SIGINT / SIGTERM
    void run();

private:
    void shutdown();
    bool initialized_ = false;
};
```

设计要点：
- `init()` 和 `run()` 分离——方便后续在 `init()` 后插入健康检查再调用 `run()`
- `initialized_` 标志让 `shutdown()` 做到**幂等**（析构和信号都可能触发）

### 5.13.5 配置文件（nlohmann/json）

**`server/config/server.example.json`**：

```json
{
    "server":   { "host": "0.0.0.0", "port": 5000, "thread_count": 8 },
    "database": { "host": "127.0.0.1", "port": 3308, "name": "cloudvault",
                  "user": "cloudvault_app", "password_env": "CLOUDVAULT_DB_PASSWORD" },
    "storage":  { "root": "/data/cloudvault/filesys" },
    "tls":      { "cert": "/etc/cloudvault/cert.pem", "key": "/etc/cloudvault/key.pem" },
    "log":      { "level": "info", "file": "logs/server.log" }
}
```

> `server.json` 写入 `.gitignore`，避免把本地路径和密码提交进仓库。仓库只保留 `server.example.json`。

读取时用 `json::json_pointer` 安全访问嵌套键——键不存在时返回默认值，不会抛异常：

```cpp
using jp = json::json_pointer;
const int port = cfg.value(jp("/server/port"), 5000);
```

### 5.13.6 spdlog：控制台 + 轮转文件双 sink

```cpp
// 确保日志目录存在（运行时创建，不提交 logs/ 到 git）
std::filesystem::create_directories(
    std::filesystem::path(log_file).parent_path());

auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
auto file_sink    = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    log_file,
    10 * 1024 * 1024,   // 单文件上限 10 MB
    3                   // 最多保留 3 个轮转文件
);

auto logger = std::make_shared<spdlog::logger>(
    "server",
    spdlog::sinks_init_list{console_sink, file_sink}
);
logger->set_level(spdlog::level::from_str(log_level));
logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-5l%$] [tid %t] %v");
spdlog::set_default_logger(logger);
```

`set_default_logger()` 之后，代码里直接用 `spdlog::info(...)` 即可，不需要传 logger 指针。

### 5.13.7 信号处理与优雅关闭

```cpp
// 全局 shutdown 标志，信号处理函数只做最简单的操作
static std::atomic<bool> g_shutdown{false};

static void onSignal(int sig) {
    spdlog::info("收到信号 {}，准备关闭...", sig);
    g_shutdown.store(true);
}

// init() 末尾注册
std::signal(SIGINT,  onSignal);
std::signal(SIGTERM, onSignal);
```

`run()` 的主循环（骨架阶段，第七章替换为事件循环）：

```cpp
void ServerApp::run() {
    while (!g_shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    shutdown();
}
```

`shutdown()` 用 `initialized_` 保证幂等——析构函数和信号都可能触发它：

```cpp
void ServerApp::shutdown() {
    if (!initialized_) return;
    initialized_ = false;

    spdlog::info("正在关闭服务...");
    // TODO（第七章）：event_loop_.stop()、tcp_server_.stop()
    // TODO（第八章）：db_pool_.close()
    spdlog::info("CloudVault Server 已关闭，Bye.");
    spdlog::shutdown();
}
```

### 5.13.8 编译与运行

```bash
# 1. 安装依赖（仅首次）
sudo apt install -y libspdlog-dev nlohmann-json3-dev libssl-dev libgtest-dev

# 2. 配置（build 目录必须在 Linux 文件系统，见坑 6）
cmake -B ~/cv-server-build -S /mnt/d/CloudVault/server -G Ninja

# 3. 编译
cmake --build ~/cv-server-build

# 4. 准备配置文件
cp /mnt/d/CloudVault/server/config/server.example.json \
   /mnt/d/CloudVault/server/config/server.json

# 5. 运行
~/cv-server-build/cloudvault_server /mnt/d/CloudVault/server/config/server.json
```

预期输出：

```
[2026-03-25 16:30:58.601] [info ] ╔══════════════════════════════════════╗
[2026-03-25 16:30:58.601] [info ] ║      CloudVault Server  v2.0         ║
[2026-03-25 16:30:58.601] [info ] ╚══════════════════════════════════════╝
[2026-03-25 16:30:58.601] [info ] 监听地址  : 0.0.0.0:5000
[2026-03-25 16:30:58.601] [info ] 工作线程  : 8
[2026-03-25 16:30:58.601] [warn ] 骨架阶段：网络层与数据库层尚未初始化
[2026-03-25 16:30:58.601] [info ] 初始化完成，按 Ctrl+C 关闭服务
^C
[2026-03-25 16:31:21.604] [info ] 收到信号 2，准备关闭...
[2026-03-25 16:31:21.628] [info ] 正在关闭服务...
[2026-03-25 16:31:21.628] [info ] CloudVault Server 已关闭，Bye.
```

---

## 5.14 踩坑补充（服务端）

### 坑 6：build 目录在 NTFS 上报 `Operation not permitted`

**现象**：

```
CMake Error at .../CMakeDetermineSystem.cmake:246 (configure_file):
  Operation not permitted
```

**原因**：`cmake -B build` 把构建目录建在 `/mnt/d/`（Windows NTFS 挂载），CMake 的 `configure_file` 在 WSL 挂载的 NTFS 上没有创建某些文件的权限。

**解决**：build 目录放在 Linux 本地文件系统（ext4），源码可以留在 Windows：

```bash
cmake -B ~/cv-server-build -S /mnt/d/CloudVault/server -G Ninja
#         ^^^^^^^^^^^^^^^^       ^^^^^^^^^^^^^^^^^^^^^^^^
#         build 在 Linux          源码在 Windows（只读，没问题）
```

### 坑 7：`std::cerr` / `std::cout` 报 not a member of 'std'

**原因**：`<iostream>` 没有被直接或间接 `#include`。`<fstream>` 不会自动包含 `<iostream>`。

**解决**：在 `.cpp` 文件的 include 列表中显式加 `#include <iostream>`。

---

## 本章新知识点

**客户端**

- **AUTOUIC**：`.ui` 文件转 `ui_*.h`，`setupUi(this)` 替代手写布局
- **unique_ptr + 前向声明**：析构函数必须在完整类型可见处定义
- **Qt 资源系统**：`.qrc` + `AUTORCC`，`:/prefix/file` 虚拟路径
- **@token 主题系统**：C++ 常量 + 字符串替换模拟 CSS 变量
- **分类日志**：`Q_LOGGING_CATEGORY` / `qCDebug()`，按需开关
- **信号槽参数兼容**：槽的参数可少于信号，多余参数被丢弃
- **QSS 伪类**：`:hover` / `:focus` / `:disabled` / `:checked`

**服务端**

- **spdlog 多 sink**：console + rotating_file，`set_default_logger` 全局生效
- **nlohmann/json_pointer**：安全读取嵌套键，键不存在时返回默认值
- **`std::atomic<bool>` + `std::signal`**：跨线程可见的 shutdown 标志
- **幂等 shutdown**：`initialized_` 标志防止析构和信号重复关闭资源
- **WSL build 目录**：build 产物必须写到 Linux 文件系统，源码可在 Windows

---

上一章：[第四章 — 工程初始化与 CMake 配置](ch04-cmake.md) ｜ 下一章：[第六章 — 协议基础](ch06-protocol.md)
