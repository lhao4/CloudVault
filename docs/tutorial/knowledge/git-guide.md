# Git 使用指南（初学者版）

## 1. 为什么需要 Git

1. 记录每次修改，支持回滚。
2. 支持多人并行开发。
3. 通过分支隔离风险改动。

## 2. 最小工作流

```bash
git status
git add <file>
git commit -m "feat: ..."
git pull --rebase
git push
```

## 3. 分支协作

```bash
git checkout -b feat/login
git push -u origin feat/login
```

合并前先 `rebase main`，解决冲突后再提交 PR。

## 4. Conventional Commits（本项目建议）

1. `feat:` 新功能
2. `fix:` 修复
3. `refactor:` 重构
4. `docs:` 文档
5. `test:` 测试

## 5. 常见问题

1. 冲突：`git status` 查看冲突文件，手动编辑后 `git add`。
2. 撤销最近一次提交但保留代码：`git reset --soft HEAD~1`。
3. 只挑一个提交：`git cherry-pick <commit>`。
