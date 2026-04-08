# CloudVault MySQL Container

## 启动

```bash
docker compose -f docker-compose.mysql.yml up -d
```

## 连接参数

- Host: `127.0.0.1`
- Port: `3308`
- Database: `cloudvault`
- App User: `cloudvault_app`
- App Password: `cloudvault_dev_123`
- Root Password: `cloudvault_root_123`

## 常用命令

```bash
docker compose -f docker-compose.mysql.yml ps
docker compose -f docker-compose.mysql.yml logs -f cloudvault-mysql
docker compose -f docker-compose.mysql.yml down
```

## 登录

```bash
mysql -h127.0.0.1 -P3308 -ucloudvault_app -p cloudvault
mysql -h127.0.0.1 -P3308 -uroot -p
```
