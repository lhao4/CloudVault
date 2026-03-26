// =============================================================
// server/tests/test_helpers.h
// 服务端测试辅助工具
// =============================================================

#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace test_helpers {

inline void setNonBlocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("fcntl(O_NONBLOCK) failed");
    }
}

struct SocketPair {
    int peer_fd = -1;
    int conn_fd = -1;

    SocketPair() {
        const int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
            throw std::runtime_error("listen socket failed");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            ::close(listen_fd);
            throw std::runtime_error("bind failed");
        }
        if (::listen(listen_fd, 1) != 0) {
            ::close(listen_fd);
            throw std::runtime_error("listen failed");
        }

        socklen_t addr_len = sizeof(addr);
        if (::getsockname(listen_fd, reinterpret_cast<sockaddr*>(&addr), &addr_len) != 0) {
            ::close(listen_fd);
            throw std::runtime_error("getsockname failed");
        }

        peer_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (peer_fd < 0) {
            ::close(listen_fd);
            throw std::runtime_error("peer socket failed");
        }
        if (::connect(peer_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            ::close(peer_fd);
            peer_fd = -1;
            ::close(listen_fd);
            throw std::runtime_error("connect failed");
        }

        conn_fd = ::accept(listen_fd, nullptr, nullptr);
        ::close(listen_fd);
        if (conn_fd < 0) {
            ::close(peer_fd);
            peer_fd = -1;
            throw std::runtime_error("accept failed");
        }

        setNonBlocking(peer_fd);
        setNonBlocking(conn_fd);
    }

    ~SocketPair() {
        if (peer_fd >= 0) {
            ::close(peer_fd);
        }
        if (conn_fd >= 0) {
            ::close(conn_fd);
        }
    }
};

struct TempDir {
    std::filesystem::path path;

    TempDir() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path()
             / ("cloudvault_server_test_" + std::to_string(::getpid()) + "_" + std::to_string(stamp));
        std::filesystem::create_directories(path);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

}  // namespace test_helpers
