#include "chttp.h"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
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

// chttp.accept() -> Devuelve objeto request o nil (con soporte universal para cualquier método HTTP)
int l_chttp_accept(lua_State *L) {
    if (server_fd < 0) return 0;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            lua_pushnil(L);
            return 1;
        }
        lua_pushnil(L);
        return 1;
    }

    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
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
    
    // Captura dinámica del método y la ruta
    if (sscanf(buffer, "%15s %255s", req->method, req->path) != 2) {
        close(client_sock);
        lua_pushnil(L);
        return 1;
    }

    // Normalizar el método HTTP a minúsculas (ej. GET -> get, POST -> post, OPTIONS -> options)
    for (int i = 0; req->method[i]; i++) {
        req->method[i] = tolower((unsigned char)req->method[i]);
    }

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

// Responder solicitudes estándar con JSON u otros formatos de texto
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

    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
    return 0;
}

// NUEVO: Método para servir archivos estáticos directamente desde el disco (ej. index.html)
int l_request_send_file(lua_State *L) {
    HttpRequest *req = (HttpRequest*)luaL_checkudata(L, 1, CHTTP_MT);
    if (req->client_sock < 0) return 0;

    int status = luaL_checkinteger(L, 2);
    const char *filepath = luaL_checkstring(L, 3);
    const char *content_type = luaL_optstring(L, 4, "text/html; charset=utf-8");

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        const char *not_found = "{\"error\":\"Archivo no encontrado en el servidor\"}";
        char header[256];
        snprintf(header, sizeof(header), 
            "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", 
            strlen(not_found));
        (void)write(req->client_sock, header, strlen(header));
        (void)write(req->client_sock, not_found, strlen(not_found));
        close(req->client_sock);
        req->client_sock = -1;
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char header[512];
    snprintf(header, sizeof(header), 
        "HTTP/1.1 %d OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", 
        status, content_type, file_size);
    (void)write(req->client_sock, header, strlen(header));

    char buffer[8192];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0)
        (void)write(req->client_sock, buffer, bytes_read);

    fclose(f);
    close(req->client_sock);
    req->client_sock = -1;

    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
    return 0;
}

// chttp.listen(host, port)
int l_chttp_listen(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return luaL_error(L, "No se pudo crear el socket HTTP");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

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

// __index para metatabla de request (expone propiedades y métodos al entorno Lua)
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
        lua_pushstring(L, req->body ? req->body : "");
        return 1;
    } else if (strcmp(key, "respond") == 0) {
        lua_pushcfunction(L, l_request_respond);
        return 1;
    } else if (strcmp(key, "send_file") == 0) {
        lua_pushcfunction(L, l_request_send_file);
        return 1;
    }
    return 0;
}