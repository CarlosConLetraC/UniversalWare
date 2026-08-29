#include "chttp.h"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

static int server_fd = -1;

typedef struct {
    int client_sock;
    char method[16];
    char path[256];
    char *body;
} HttpRequest;

// Destructor para liberar memoria automáticamente cuando Lua destruya el objeto request
int l_request_gc(lua_State *L) {
    HttpRequest *req = (HttpRequest*)luaL_checkudata(L, 1, CHTTP_MT);
    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
    return 0;
}

// chttp.accept() -> Devuelve objeto request o nil
int l_chttp_accept(lua_State *L) {
    if (server_fd < 0) return 0;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        // Manejo seguro de socket no bloqueante vacío
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            lua_pushnil(L);
            return 1;
        }
        lua_pushnil(L);
        return 1;
    }

    // Asegurar que el socket del cliente sea no bloqueante de forma segura
    int flags = fcntl(client_sock, F_GETFL, 0);
    if (flags != -1) {
        fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);
    }

    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // El cliente conectó pero aún no envió datos completos. 
            // Cerramos el socket temporalmente para evitar fugas y reintentamos luego.
            close(client_sock);
            lua_pushnil(L);
            return 1;
        } else {
            close(client_sock);
            lua_pushnil(L);
            return 1;
        }
    } else if (bytes_read == 0) {
        close(client_sock);
        lua_pushnil(L);
        return 1;
    }

    HttpRequest *req = (HttpRequest*)lua_newuserdata(L, sizeof(HttpRequest));
    if (!req) {
        close(client_sock);
        return 0;
    }
    
    req->client_sock = client_sock;
    req->body = NULL;
    
    sscanf(buffer, "%15s %255s", req->method, req->path);

    char *body_ptr = strstr(buffer, "\r\n\r\n");
    if (body_ptr) {
        body_ptr += 4;
        req->body = strdup(body_ptr);
    } else {
        req->body = strdup("");
    }

    luaL_getmetatable(L, CHTTP_MT);
    lua_setmetatable(L, -2);
    return 1;
}

// Modificar l_request_respond para prevenir doble liberación (double free)
int l_request_respond(lua_State *L) {
    HttpRequest *req = (HttpRequest*)luaL_checkudata(L, 1, CHTTP_MT);
    if (req->client_sock < 0) return 0; // Ya fue respondido o cerrado

    int status = luaL_checkinteger(L, 2);
    const char *body = luaL_checkstring(L, 3);

    char header[512];
    int body_len = strlen(body);
    snprintf(header, sizeof(header), 
        "HTTP/1.1 %d OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", 
        status, body_len);

    size_t w0 = write(req->client_sock, header, strlen(header));
    size_t w1 = write(req->client_sock, body, body_len);
    if (!(++w0) || !(++w1))
        luaL_error(L, "Error de red: el cliente corto la conexion prematuramente.");
    
    close(req->client_sock);
    req->client_sock = -1;

    // Liberamos el body aquí de forma segura y anulamos el puntero
    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
    return 0;
}

#include <fcntl.h> // Asegúrate de incluir esta librería al inicio de server.c

// chttp.listen(host, port)
int l_chttp_listen(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return luaL_error(L, "No se pudo crear el socket HTTP");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // --- NUEVO: Hacer el socket no bloqueante para evitar congelamientos ---
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
    // ------------------------------------------------------------------------

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_fd);
        return luaL_error(L, "Error en bind del puerto HTTP");
    }

    listen(server_fd, 128);
    return 0;
}

// __index para metatabla de request (exponer propiedades method, body, etc.)
int l_request_index(lua_State *L) {
    HttpRequest *req = (HttpRequest*)luaL_checkudata(L, 1, CHTTP_MT);
    const char *key = luaL_checkstring(L, 2);

    if (strcmp(key, "method") == 0) {
        lua_pushstring(L, req->method);
        return 1;
    } else if (strcmp(key, "path") == 0) {
        lua_pushstring(L, req->path);
        return 1;
    } else if (strcmp(key, "body") == 0) {
        lua_pushstring(L, req->body ? req->body : ""); // <-- Vital para POST/PUT/DELETE
        return 1;
    } else if (strcmp(key, "respond") == 0) {
        lua_pushcfunction(L, l_request_respond);
        return 1;
    }
    return 0;
}