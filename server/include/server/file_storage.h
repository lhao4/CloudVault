// =============================================================
// server/include/server/file_storage.h
// TODO（第八章）：服务端文件系统操作封装
//
// 职责：
//   - 用户注册时创建个人目录（storage_root/username/）
//   - 封装 mkdir、move、delete、rename、list、search 操作
//   - 路径合法性校验：防止路径穿越（../ 攻击）
//     所有路径先 weakly_canonical，再验证是否以 user_root 为前缀
//   - 分块上传临时文件管理（.tmp 后缀，完成后重命名）
// =============================================================

#pragma once

// TODO（第八章）：实现 FileStorage 类
