# 用 MkDocs + GitHub Pages 搭建项目文档网站

> 本文记录 CloudVault 文档站的完整搭建过程，从本地预览到自动部署一步步来。

---

## 1. 为什么选 MkDocs Material

真实项目中文档有两种常见选择：

| 工具 | 语言 | 适合场景 |
|------|------|----------|
| **MkDocs + Material** | Python | 技术文档、教程，配置简单，Markdown 写完即用 |
| Docusaurus | Node.js | 需要自定义组件、文档+博客一体化 |
| VitePress | Node.js | Vue 生态项目 |

CloudVault 是纯技术项目，MkDocs Material 开箱即用，不需要前端知识，是最轻量的选择。

---

## 2. 核心概念

在动手之前，先搞清楚三件事的关系：

```
你的仓库（master 分支）
  └── docs/            ← Markdown 源文件
  └── mkdocs.yml       ← 网站配置

GitHub Actions          ← 触发器：每次 push master
  └── mkdocs gh-deploy ← 把 docs/ 编译成 HTML

gh-pages 分支           ← 存放编译后的 HTML（自动创建）
  └── index.html ...

GitHub Pages            ← 把 gh-pages 分支发布成网站
  └── https://你的用户名.github.io/CloudVault/
```

**关键点**：你只需维护 Markdown，HTML 由工具生成，完全不需要手写。

---

## 3. 本地环境搭建

### 3.1 安装 MkDocs Material

```bash
pip install mkdocs-material
```

验证安装：

```bash
mkdocs --version
# mkdocs, version 1.x.x
```

### 3.2 本地预览

在项目根目录（`mkdocs.yml` 所在目录）运行：

```bash
mkdocs serve
```

浏览器打开 `http://127.0.0.1:8000`，修改 Markdown 文件后页面自动刷新。

---

## 4. mkdocs.yml 配置详解

CloudVault 的 `mkdocs.yml` 每项配置的含义：

```yaml
site_name: CloudVault 云巢          # 网站标题，显示在浏览器 tab 和导航栏
site_url: https://xxx.github.io/CloudVault/  # 正式网站地址，影响 sitemap
repo_url: https://github.com/xxx/CloudVault  # 右上角显示 GitHub 链接
docs_dir: docs                      # Markdown 源文件目录

theme:
  name: material                    # 使用 Material 主题
  language: zh                      # 界面语言（搜索、提示文字等）
  palette:                          # 明暗模式切换
    - scheme: default               # 浅色模式
      toggle:
        icon: material/brightness-7
        name: 切换到深色模式
    - scheme: slate                 # 深色模式
      toggle:
        icon: material/brightness-4
        name: 切换到浅色模式
  features:
    - navigation.tabs               # 顶部 tab 导航（对应 nav 第一层）
    - navigation.sections           # 左侧栏分组标题
    - navigation.top                # 滚动时显示"回到顶部"按钮
    - search.highlight              # 搜索结果高亮关键词
    - content.code.copy             # 代码块右上角复制按钮

plugins:
  - search:
      lang: zh                      # 支持中文搜索分词

markdown_extensions:
  - admonition                      # 支持 !!! note 这样的提示框
  - pymdownx.superfences            # 支持代码块嵌套、mermaid 图表
  - pymdownx.highlight:
      anchor_linenums: true         # 代码行号可锚链接
  - tables                          # 支持 GFM 表格
  - toc:
      permalink: true               # 标题旁显示锚链接 #

nav:                                # 网站导航结构，决定左侧栏和顶部 tab
  - 首页: index.md
  - 项目实战教程:
    - 教程概述与目录: tutorial/index.md
    - 知识补充:
      - Git 使用指南: tutorial/knowledge/git-guide.md
      # ...
  - 工程参考:
    - PRD 产品需求: reference/prd.md
    # ...
```

### nav 层级与导航的对应关系

```
nav 第一层  →  顶部 Tab（需开启 navigation.tabs）
nav 第二层  →  左侧栏分组标题
nav 第三层  →  左侧栏页面链接
```

---

## 5. 文档目录结构

CloudVault 按**读者目的**划分目录，而不是按文件类型：

```
docs/
├── index.md                    # 首页：是什么、怎么导航
├── tutorial/                   # 教程：怎么从 0 做出来（给学习者）
│   ├── index.md                # 教程目录与章节概览
│   └── knowledge/              # 知识补充：教程中涉及的独立概念
│       ├── git-guide.md
│       ├── cmake-guide.md
│       ├── epoll-guide.md
│       ├── qt-signals-slots.md
│       ├── cicd-guide.md
│       └── mkdocs-github-pages.md   ← 本文
└── reference/                  # 工程参考：系统是什么样的（给开发者查阅）
    ├── prd.md
    ├── architecture.md
    ├── api.md
    ├── database.md
    ├── modules.md
    ├── ui-flow.md
    └── testing.md
```

---

## 6. GitHub Actions 自动部署

### 6.1 工作流文件

`.github/workflows/docs.yml`：

```yaml
name: Deploy Docs

on:
  push:
    branches:
      - master
    paths:                        # 只有改了文档才触发，不浪费 CI 时间
      - "docs/**"
      - "mkdocs.yml"

permissions:
  contents: read
  pages: write
  id-token: write

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.x"

      - name: Install MkDocs Material
        run: pip install mkdocs-material

      - name: Build and deploy
        run: mkdocs gh-deploy --force
```

### 6.2 `mkdocs gh-deploy` 做了什么

```
mkdocs gh-deploy --force
  ├── 执行 mkdocs build（将 docs/ 编译成 site/ 目录的 HTML）
  ├── 切换到 gh-pages 分支（不存在则自动创建）
  ├── 将 site/ 内容推送到 gh-pages 分支
  └── 切换回原分支
```

你不需要手动管理 `gh-pages` 分支，它完全由工具自动维护。

### 6.3 触发条件说明

```yaml
paths:
  - "docs/**"
  - "mkdocs.yml"
```

这个配置表示：只有修改了 `docs/` 下的文件或 `mkdocs.yml` 才会触发部署。修改代码（`Server/`、`Client/`）不会触发，节省 CI 资源。

---

## 7. 在 GitHub 上开启 Pages

第一次部署前需要手动开启一次：

1. 进入仓库页面 → **Settings**
2. 左侧菜单找到 **Pages**
3. Source 选择 **Deploy from a branch**
4. Branch 选择 **`gh-pages`**，目录选 **`/ (root)`**
5. 点击 Save

之后每次 push master（且包含文档改动），网站会在约 1 分钟内自动更新。

---

## 8. 常用命令速查

```bash
# 本地预览（热重载）
mkdocs serve

# 手动构建（生成 site/ 目录，不部署）
mkdocs build

# 手动部署到 GitHub Pages（平时不需要，Actions 会自动做）
mkdocs gh-deploy --force

# 清除构建缓存
mkdocs build --clean
```

---

## 9. 常见问题

**Q: push 后 Actions 没有触发？**

检查改动的文件是否在 `docs/` 或 `mkdocs.yml`，其他路径的改动不触发文档部署。

**Q: 网站显示 404？**

确认 GitHub Pages 的 Branch 设置为 `gh-pages`，而不是 `master`。

**Q: 中文搜索不好用？**

已在 `mkdocs.yml` 配置 `lang: zh`，MkDocs Material 内置了中文分词支持。

**Q: 如何在文档里写提示框？**

```markdown
!!! note "注意"
    这是一个提示框。

!!! warning "警告"
    这是一个警告框。

!!! tip "技巧"
    这是一个技巧提示。
```

效果需要 `admonition` 扩展（已在 `mkdocs.yml` 配置）。
