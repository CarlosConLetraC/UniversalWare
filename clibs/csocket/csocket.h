#pragma once
#define CSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

// Funciones principales del núcleo de sockets Unix
int csocket_unix_server(const char *socket_path);
int csocket_unix_accept(int server_fd);
int csocket_unix_connect(const char *socket_path);
ssize_t csocket_send(int fd, const void *buf, size_t len);
ssize_t csocket_recv(int fd, void *buf, size_t len, int flags);
int csocket_close(int fd);
