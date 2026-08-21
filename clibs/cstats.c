#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if LUA_VERSION_NUM < 502
#define lua_rawlen lua_objlen
#endif

// Estructura optimizada: el array de datos va al final (Flexible Array Member)
typedef struct {
    size_t n;
    double *data;
} Array;

// Helper para validar y obtener el objeto Array
static Array* check_array(lua_State *L, int idx) {
    Array *arr = (Array*)luaL_checkudata(L, idx, "ArrayMT");
    if (arr->data == NULL && arr->n > 0) {
        luaL_error(L, "error: array already freed manually.");
    }
    return arr;
}

// CONSTRUCTOR: cstats.array({1, 2, 3})
static int l_array(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    size_t n = lua_rawlen(L, 1);

    // El struct vive en el GC de Lua (poca memoria)
    Array *arr = (Array*)lua_newuserdata(L, sizeof(Array));
    arr->n = n;
    
    // El array de datos vive en el Heap del sistema (mucha memoria)
    arr->data = (double*)malloc(n * sizeof(double));
    if (!arr->data && n > 0) return luaL_error(L, "no hay memoria suficiente en el sistema");

    for (size_t i = 0; i < n; i++) {
        lua_rawgeti(L, 1, (lua_Integer)i + 1);
        arr->data[i] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    luaL_getmetatable(L, "ArrayMT");
    lua_setmetatable(L, -2);
    return 1;
}

// METODOS ESTADISTICOS (Mucho mas rapidos al ser punteros directos).
static int l_mean(lua_State *L) {
    Array *arr = check_array(L, 1);
    if (arr->n == 0) { lua_pushnumber(L, 0); return 1; }

    double sum = 0.0;
    for (size_t i = 0; i < arr->n; i++) sum += arr->data[i];
    
    lua_pushnumber(L, sum / (double)arr->n);
    return 1;
}

static int l_mse(lua_State *L) {
    Array *a = check_array(L, 1);
    Array *b = check_array(L, 2);
    
    size_t n = (a->n < b->n) ? a->n : b->n;
    if (n == 0) { lua_pushnumber(L, 0); return 1; }

    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        double d = a->data[i] - b->data[i];
        sum += d * d;
    }
    lua_pushnumber(L, sum / (double)n);
    return 1;
}

static int l_r2(lua_State *L) {
    Array *y_true = check_array(L, 1);
    Array *y_pred = check_array(L, 2);
    size_t n = (y_true->n < y_pred->n) ? y_true->n : y_pred->n;
    if (n == 0) { lua_pushnumber(L, 0); return 1; }

    double sum_y = 0.0;
    for (size_t i = 0; i < n; i++) sum_y += y_true->data[i];
    double mean_y = sum_y / (double)n;

    double ss_res = 0.0, ss_tot = 0.0;
    for (size_t i = 0; i < n; i++) {
        double res = y_true->data[i] - y_pred->data[i];
        double tot = y_true->data[i] - mean_y;
        ss_res += res * res;
        ss_tot += tot * tot;
    }

    if (ss_tot < 1e-12) lua_pushnumber(L, 0.0);
    else lua_pushnumber(L, 1.0 - (ss_res / ss_tot));
    return 1;
}

// VARIANCE
static int l_var(lua_State *L) {
    Array *arr = check_array(L, 1);
    size_t n = arr->n;
    if (n == 0) { lua_pushnumber(L, 0); return 1; }

    double sum = 0.0, sq_sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += arr->data[i];
        sq_sum += arr->data[i] * arr->data[i];
    }
    
    double mean = sum / (double)n;
    // Formula eficiente: E[X^2] - (E[X])^2
    double variance = (sq_sum / (double)n) - (mean * mean);
    
    lua_pushnumber(L, variance);
    return 1;
}

// STD
static int l_std(lua_State *L) {
    // Reutilizamos l_var para evitar duplicar logica
    l_var(L); 
    double var = lua_tonumber(L, -1);
    lua_pushnumber(L, sqrt(var));
    return 1;
}

// MAE
static int l_mae(lua_State *L) {
    Array *a = check_array(L, 1);
    Array *b = check_array(L, 2);
    size_t n = (a->n < b->n) ? a->n : b->n;
    if (n == 0) { lua_pushnumber(L, 0); return 1; }

    double sum_abs_diff = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum_abs_diff += fabs(a->data[i] - b->data[i]);
    }

    lua_pushnumber(L, sum_abs_diff / (double)n);
    return 1;
}

// CORR
static int l_corr(lua_State *L) {
    Array *x = check_array(L, 1);
    Array *y = check_array(L, 2);
    size_t n = (x->n < y->n) ? x->n : y->n;
    if (n == 0) { lua_pushnumber(L, 0); return 1; }

    double sum_x = 0.0, sum_y = 0.0;
    double sum_xy = 0.0;
    double sum_x2 = 0.0, sum_y2 = 0.0;

    for (size_t i = 0; i < n; i++) {
        double xi = x->data[i];
        double yi = y->data[i];
        sum_x += xi;
        sum_y += yi;
        sum_xy += xi * yi;
        sum_x2 += xi * xi;
        sum_y2 += yi * yi;
    }

    double num = ((double)n * sum_xy) - (sum_x * sum_y);
    double den = sqrt(((double)n * sum_x2 - (sum_x * sum_x)) * ((double)n * sum_y2 - (sum_y * sum_y)));

    if (fabs(den) < 1e-12) lua_pushnumber(L, 0);
    else lua_pushnumber(L, num / den);

    return 1;
}

static int l_array_append(lua_State *L) {
    Array *arr = check_array(L, 1);
    double val = luaL_checknumber(L, 2);
    size_t new_n = arr->n + 1;

    // 1. Crear el objeto userdata (asa)
    Array *new_arr = (Array*)lua_newuserdata(L, sizeof(Array));
    new_arr->n = new_n;
    
    // 2. Reservar memoria nueva en el Heap
    new_arr->data = (double*)malloc(new_n * sizeof(double));
    if (!new_arr->data) return luaL_error(L, "Out of memory");

    // 3. Copiar y agregar
    memcpy(new_arr->data, arr->data, arr->n * sizeof(double));
    new_arr->data[arr->n] = val;

    luaL_getmetatable(L, "ArrayMT");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_array_slice(lua_State *L) {
    Array *arr = check_array(L, 1);
    lua_Integer start = luaL_checkinteger(L, 2);
    lua_Integer end = luaL_optinteger(L, 3, (lua_Integer)arr->n);

    // Ajuste de indices (Lua usa indices desde 1)
    if (start < 1) start = 1;
    if (end > (lua_Integer)arr->n) end = (lua_Integer)arr->n;
    if (start > end) {
        // Devolver array vacio si el rango es invalido
        Array *empty = (Array*)lua_newuserdata(L, sizeof(Array));
        empty->n = 0;
        luaL_getmetatable(L, "ArrayMT");
        lua_setmetatable(L, -2);
        return 1;
    }

    size_t slice_n = (size_t)(end - start + 1);
    Array *new_arr = (Array*)lua_newuserdata(L, sizeof(Array) + (slice_n * sizeof(double)));
    new_arr->n = slice_n;

    // Copiar la seccion especifica
    memcpy(new_arr->data, &arr->data[start - 1], slice_n * sizeof(double));

    luaL_getmetatable(L, "ArrayMT");
    lua_setmetatable(L, -2);
    return 1;
}

// Soporta: print(#arr)
static int l_array_len(lua_State *L) {
    Array *arr = check_array(L, 1);
    lua_pushinteger(L, (lua_Integer)arr->n);
    return 1;
}

static int l_array_gc(lua_State *L) {
    Array *arr = (Array*)lua_touserdata(L, 1);
    if (arr->data != NULL) {
        free(arr->data);
        arr->data = NULL;
        arr->n = 0;
    }
    return 0;
}

// Liberar array de memoria manualmente desde Lua, o mejor dicho: un alias para el garbage collector. . .
static int l_array_free(lua_State *L) {
    // Simplemente llamamos al mismo destructor
    return l_array_gc(L);
}

// Soporta: print(arr[1]) y arr:mean()
static int l_array_index(lua_State *L) {
    Array *arr = check_array(L, 1);

    if (lua_isnumber(L, 2)) {
        lua_Integer idx = lua_tointeger(L, 2);
        if (idx < 1 || idx > (lua_Integer)arr->n) {
            lua_pushnil(L); // Fuera de rango
        } else {
            lua_pushnumber(L, arr->data[idx - 1]);
        }
        return 1;
    }

    // Si no es un numero, buscamos el metodo en la metatabla
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, "__methods"); // Tabla donde moveremos los metodos
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    return 1;
}

static int l_array_tostring(lua_State *L) {
    // Usamos luaL_checkudata para asegurarnos de que es un Array
    Array *arr = (Array*)luaL_checkudata(L, 1, "ArrayMT");
    
    // lua_touserdata devuelve el puntero genérico (la direccion de memoria)
    // El especificador %p en lua_pushfstring formatea automaticamente el puntero a hexadecimal
    lua_pushfstring(L, "cstats.array: %p (%d elements)", lua_touserdata(L, 1), (int)arr->n);
    return 1;
}

// REGISTRO Y METATABLA
static const struct luaL_Reg array_methods[] = {
    {"corr",        l_corr},
    {"mae",         l_mae},
    {"mean",        l_mean},
    {"mse",         l_mse},
    {"r2",          l_r2},
    {"std",         l_std},
    {"var",         l_var},
    {"append",      l_array_append},
    {"slice",       l_array_slice},
    {"free_memory", l_array_free},
    {NULL, NULL}
};

int luaopen_cstats(lua_State *L) {
    luaL_newmetatable(L, "ArrayMT");

    // 1. Tabla de metodos para :mean(), :var(), etc.
    lua_newtable(L);
    luaL_setfuncs(L, array_methods, 0);
    lua_setfield(L, -2, "__methods");

    // 2. Metametodos principales
    lua_pushcfunction(L, l_array_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, l_array_len);
    lua_setfield(L, -2, "__len");

    lua_pushcfunction(L, l_array_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, l_array_gc);
    lua_setfield(L, -2, "__gc");

    // 3. Libreria principal
    luaL_Reg funcs[] = {
        {"array", l_array},
        {NULL, NULL}
    };
    luaL_newlib(L, funcs);
    return 1;
}
