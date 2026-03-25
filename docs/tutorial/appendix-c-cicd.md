# 附录 C　CI/CD 与部署

> **状态**：🔲 待写（随项目开发持续补充）

---

## 计划内容

### C.1 GitHub Actions CI

每次 push 自动构建服务端并运行测试：

```yaml
# .github/workflows/server-ci.yml（待完成）
name: Server CI
on:
  push:
    paths: [ "server/**", "common/**" ]
jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: sudo apt install libssl-dev libmysqlclient-dev
      - name: Build
        run: |
          cd server
          cmake -B build -DBUILD_TESTING=ON
          cmake --build build
      - name: Test
        run: ctest --test-dir server/build --output-on-failure
```

### C.2 Docker 部署

```yaml
# docker-compose.yml（待完成）
services:
  db:
    image: mysql:8.0
    environment:
      MYSQL_DATABASE: cloudvault
  server:
    build: ./server
    ports: ["5000:5000"]
    depends_on: [db]
```

### C.3 裸机部署步骤

1. 安装 MySQL，创建数据库和用户
2. 编译服务端：`cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
3. 复制配置文件：`cp config/server.json.example config/server.json`（填写真实配置）
4. 运行：`./build/cloudvault_server`

参考：[CI/CD 与 GitHub Actions 入门](knowledge/cicd-guide.md)
