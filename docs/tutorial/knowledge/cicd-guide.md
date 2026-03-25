# CI/CD 与 GitHub Actions 入门

## 1. 基础概念

1. CI：每次提交自动构建和测试。
2. CD：通过测试后自动部署或发布。

## 2. GitHub Actions 结构

`workflow` -> `jobs` -> `steps`

```yaml
name: ci
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: cmake -S Server -B build/server -DBUILD_TESTING=ON
      - run: cmake --build build/server -j
      - run: ctest --test-dir build/server --output-on-failure
```

## 3. 本项目 CI 建议

1. 分别验证 `Server` 与 `Client` 构建。
2. 缓存 FetchContent 依赖。
3. PR 必须通过 CI 才可合并。
