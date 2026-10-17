#include "csocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    #include <luajit-2.1/lua.h>
    #include <luajit-2.1/lauxlib.h>
    #include <luajit-2.1/lualib.h>
}

// --- Implementación interna de csocket.h ---

int csocket_unix_server(const char *socket_path) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < int(0)) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path); // Limpiar socket previo si existía

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int csocket_unix_accept(int server_fd) {
    return accept(server_fd, NULL, NULL);
}

int csocket_unix_connect(const char *socket_path) {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

ssize_t csocket_send(int fd, const void *buf, size_t len) {
    return send(fd, buf, len, 0);
}

ssize_t csocket_recv(int fd, void *buf, size_t len, int flags) {
    return recv(fd, buf, len, flags);
}

int csocket_close(int fd) {
    return close(fd);
}

// --- Bindings para LuaJIT ---

static int l_unix_server(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int fd = csocket_unix_server(path);
    if (fd < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Fallo al crear el servidor Unix socket");
        return 2;
    }
    lua_pushinteger(L, fd);
    return 1;
}

static int l_unix_connect(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int fd = csocket_unix_connect(path);
    if (fd < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Fallo al conectar al Unix socket");
        return 2;
    }
    lua_pushinteger(L, fd);
    return 1;
}

static int l_send(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    ssize_t sent = csocket_send(fd, data, len);
    lua_pushinteger(L, sent);
    return 1;
}

static int l_recv(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    size_t max_len = luaL_optinteger(L, 2, 1024);
    
    char *buf = (char *)malloc(max_len);
    ssize_t received = csocket_recv(fd, buf, max_len, 0);
    
    if (received > 0) {
        lua_pushlstring(L, buf, received);
    } else {
        lua_pushnil(L);
    }
    free(buf);
    return 1;
}

static int l_close(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    int res = csocket_close(fd);
    lua_pushboolean(L, res == 0);
    return 1;
}

// Registro de funciones expuestas a LuaJIT
static const struct luaL_Reg csocket_funcs[] = {
    {"unix_server", l_unix_server},
    {"unix_connect", l_unix_connect},
    {"send", l_send},
    {"recv", l_recv},
    {"close", l_close},
    {NULL, NULL}
};

// se necesita de extern "C" por si se embebe hacia algún programa en C++.
extern "C" int luaopen_csocket(lua_State *L) {
    luaL_newlib(L, csocket_funcs);
    return 1;
}