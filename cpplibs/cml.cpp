extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <vector>
#include <string>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <stdio.h>

// --- LUA COMPAT ---
#ifndef lua_rawlen
#define lua_rawlen(L, i) lua_objlen(L, i)
#endif

// --- NUM PARSER ---
static inline double parse_number(const char* s) {
    if (!s) return NAN;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == 0) return NAN;
    std::string tmp(s);
    for (char& c : tmp) if (c == ',') c = '.';
    char* end = nullptr;
    double v = strtod(tmp.c_str(), &end);
    if (end == tmp.c_str()) return NAN;
    while (*end && isspace((unsigned char)*end)) end++;
    return (*end == 0) ? v : NAN;
}

static inline double to_number(lua_State* L, int idx) {
    if (lua_isnumber(L, idx)) return lua_tonumber(L, idx);
    if (lua_isstring(L, idx)) return parse_number(lua_tostring(L, idx));
    return NAN;
}

// --- ESTRUCTURAS ---

struct LinearRegression {
    std::vector<std::string> features;
    std::string target;
    std::vector<double> weights;
    std::vector<std::vector<double>> X;
    std::vector<double> y;
    std::vector<double> mean_x, std_x;
    double mean_y = 0.0, std_y = 1.0;

    void build_matrix(lua_State* L, int idx) {
        X.clear(); y.clear();
        int n = (int)lua_rawlen(L, idx);
        int k = (int)features.size();
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, idx, i);
            std::vector<double> row(k);
            bool ok = true;
            for (int j = 0; j < k; j++) {
                lua_getfield(L, -1, features[j].c_str());
                double v = to_number(L, -1);
                lua_pop(L, 1);
                if (!std::isfinite(v)) ok = false;
                row[j] = v;
            }
            lua_getfield(L, -1, target.c_str());
            double yt = to_number(L, -1);
            lua_pop(L, 1); lua_pop(L, 1);
            if (!ok || !std::isfinite(yt)) continue;
            X.push_back(row);
            y.push_back(yt);
        }
    }

    void compute_stats(const std::vector<std::vector<double>>& X_train, const std::vector<double>& y_train) {
        size_t n = X_train.size();
        size_t k = features.size();
        mean_x.assign(k, 0.0); std_x.assign(k, 0.0);
        mean_y = 0.0; std_y = 0.0;
        for (size_t i = 0; i < n; i++) {
            mean_y += y_train[i];
            for (size_t j = 0; j < k; j++) mean_x[j] += X_train[i][j];
        }
        mean_y /= n;
        for (size_t j = 0; j < k; j++) mean_x[j] /= n;
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < k; j++) {
                double d = X_train[i][j] - mean_x[j];
                std_x[j] += d * d;
            }
            double dy = y_train[i] - mean_y;
            std_y += dy * dy;
        }
        for (size_t j = 0; j < k; j++) std_x[j] = std::sqrt(std_x[j] / n + 1e-12);
        std_y = std::sqrt(std_y / n + 1e-12);
    }

	void fit(double lr, int epochs, double lambda, const std::vector<std::vector<double>>& X_train, const std::vector<double>& y_train) {
		size_t n = X_train.size();
		size_t k = features.size();
		weights.assign(k + 1, 0.0);

		for (int e = 0; e < epochs; e++) {
			std::vector<double> grad(k + 1, 0.0);
			for (size_t i = 0; i < n; i++) {
				double pred = weights[0];
				for (size_t j = 0; j < k; j++) pred += weights[j + 1] * X_train[i][j];
				double err = pred - y_train[i];
				
				grad[0] += err;
				for (size_t j = 0; j < k; j++) grad[j + 1] += err * X_train[i][j];
			}

			// El bias (weights[0]) usualmente no se regulariza
			weights[0] -= lr * grad[0] / n;

			// Actualizacion de pesos con regularizacion L2
			for (size_t j = 0; j < k; j++) {
				// Se agrega (lambda * weights[j+1]) al gradiente
				double reg_term = lambda * weights[j + 1];
				weights[j + 1] -= lr * (grad[j + 1] / n + reg_term);
			}
		}
	}
};

struct LogisticRegression {
    std::vector<std::string> features;
    std::string target;
    std::vector<double> weights;
    std::vector<std::vector<double>> X;
    std::vector<double> y;
    std::vector<double> mean_x, std_x;

    static inline double sigmoid(double z) {
        if (z > 35.0) return 1.0;
        if (z < -35.0) return 0.0;
        return 1.0 / (1.0 + std::exp(-z));
    }

    void build_matrix(lua_State* L, int idx) {
        X.clear(); y.clear();
        int n = (int)lua_rawlen(L, idx);
        int k = (int)features.size();
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, idx, i);
            std::vector<double> row(k);
            bool ok = true;
            for (int j = 0; j < k; j++) {
                lua_getfield(L, -1, features[j].c_str());
                double v = to_number(L, -1);
                lua_pop(L, 1);
                if (!std::isfinite(v)) ok = false;
                row[j] = v;
            }
            lua_getfield(L, -1, target.c_str());
            double yt = to_number(L, -1);
            lua_pop(L, 1); lua_pop(L, 1);
            if (!ok || !std::isfinite(yt)) continue;
            X.push_back(row);
            y.push_back(yt >= 0.5 ? 1.0 : 0.0);
        }
    }

    void compute_stats(const std::vector<std::vector<double>>& X_train) {
        size_t n = X_train.size();
        size_t k = features.size();
        mean_x.assign(k, 0.0); std_x.assign(k, 0.0);
        for (size_t i = 0; i < n; i++)
            for (size_t j = 0; j < k; j++) mean_x[j] += X_train[i][j];
        for (size_t j = 0; j < k; j++) mean_x[j] /= n;
        for (size_t i = 0; i < n; i++)
            for (size_t j = 0; j < k; j++) {
                double d = X_train[i][j] - mean_x[j];
                std_x[j] += d * d;
            }
        for (size_t j = 0; j < k; j++) std_x[j] = std::sqrt(std_x[j] / n + 1e-12);
    }
};

// --- BINDINGS: LINEAR ---

static int lr_load(lua_State* L) {
    auto* m = *(LinearRegression**)luaL_checkudata(L, 1, "cml.LinearRegression");
    luaL_checktype(L, 2, LUA_TTABLE);
    m->build_matrix(L, 2);
    return 0;
}

static int lr_normalize(lua_State* L) {
    auto* m = *(LinearRegression**)lua_touserdata(L, 1);
    if (!m->X.empty()) m->compute_stats(m->X, m->y);
    return 0;
}

static int lr_predict(lua_State* L) {
    auto* m = *(LinearRegression**)luaL_checkudata(L, 1, "cml.LinearRegression");
    double pred = m->weights[0];
    for (size_t j = 0; j < m->features.size(); j++) {
        lua_getfield(L, 2, m->features[j].c_str());
        double v = (to_number(L, -1) - m->mean_x[j]) / (m->std_x[j] + 1e-12);
        lua_pop(L, 1);
        pred += m->weights[j + 1] * v;
    }
    lua_pushnumber(L, pred * m->std_y + m->mean_y);
    return 1;
}

// FUNCIONES ESTADISTICAS

static int lr_mse(lua_State* L) {
    auto* m = *(LinearRegression**)luaL_checkudata(L, 1, "cml.LinearRegression");
    luaL_checktype(L, 2, LUA_TTABLE);
    int n = (int)lua_rawlen(L, 2);
    double total_error = 0;
    int count = 0;

    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, 2, i); // [1] Fila (tabla)
        lua_getfield(L, -1, m->target.c_str()); // [2] Valor real
        double real = to_number(L, -1);
        lua_pop(L, 1); // Sacamos el valor real del stack, queda la fila en [1]

        if (std::isfinite(real)) {
            lua_pushcfunction(L, lr_predict); // [2] Funcion
            lua_pushvalue(L, 1);              // [3] Self (modelo)
            lua_pushvalue(L, -3);             // [4] Fila (estaba en el índice -3 ahora)
            
            if (lua_pcall(L, 2, 1, 0) == 0) { // Llamamos a predict(self, row)
                double pred = lua_tonumber(L, -1);
                total_error += std::pow(real - pred, 2);
                count++;
            }
            lua_pop(L, 1); // Sacamos el resultado del predict
        }
        lua_pop(L, 1); // Sacamos la fila del stack
    }
    lua_pushnumber(L, count > 0 ? total_error / count : 0.0);
    return 1;
}

static int lr_r2(lua_State* L) {
    auto* m = *(LinearRegression**)luaL_checkudata(L, 1, "cml.LinearRegression");
    luaL_checktype(L, 2, LUA_TTABLE);
    int n = (int)lua_rawlen(L, 2);
    
    std::vector<double> actuals;
    double sum_y = 0;
    
    // Primero recolectamos valores reales y media
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, 2, i);
        lua_getfield(L, -1, m->target.c_str());
        double val = to_number(L, -1);
        if (std::isfinite(val)) {
            actuals.push_back(val);
            sum_y += val;
        }
        lua_pop(L, 2);
    }
    
    if (actuals.empty()) { lua_pushnumber(L, 0); return 1; }
    double mean_y = sum_y / actuals.size();
    double ss_res = 0, ss_tot = 0;

    // Segundo pase: residuos
    for (size_t i = 0; i < actuals.size(); i++) {
        lua_rawgeti(L, 2, (int)i + 1);
        
        lua_pushcfunction(L, lr_predict);
        lua_pushvalue(L, 1);  // self
        lua_pushvalue(L, -3); // row
        
        if (lua_pcall(L, 2, 1, 0) == 0) {
            double pred = lua_tonumber(L, -1);
            ss_res += std::pow(actuals[i] - pred, 2);
            ss_tot += std::pow(actuals[i] - mean_y, 2);
        }
        lua_pop(L, 2); // resultado y fila
    }

    lua_pushnumber(L, 1.0 - (ss_res / (ss_tot + 1e-12)));
    return 1;
}

static int lr_export(lua_State* L) {
    auto* m = *(LinearRegression**)luaL_checkudata(L, 1, "cml.LinearRegression");
    lua_newtable(L);
    
    // Features
    lua_newtable(L);
    for (size_t i = 0; i < m->features.size(); i++) {
        lua_pushstring(L, m->features[i].c_str());
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "features");

    // Pesos des-normalizados para uso directo: 
    // w_real = w_norm * (std_y / std_x)
    lua_newtable(L);
    double bias = m->weights[0] * m->std_y + m->mean_y;
    for (size_t j = 0; j < m->features.size(); j++) {
        double w_real = m->weights[j + 1] * m->std_y / (m->std_x[j] + 1e-12);
        lua_pushnumber(L, w_real);
        lua_rawseti(L, -2, j + 1);
        bias -= w_real * m->mean_x[j];
    }
    lua_setfield(L, -2, "weights");
    lua_pushnumber(L, bias);
    lua_setfield(L, -2, "bias");

    return 1;
}

static int lr_fit(lua_State* L) {
    auto* m = *(LinearRegression**)luaL_checkudata(L, 1, "cml.LinearRegression");
    
    // Captura de argumentos desde Lua
    double lr     = luaL_optnumber(L, 3, 0.01);
    int epochs    = (int)luaL_optinteger(L, 4, 1000);
    double ratio  = luaL_optnumber(L, 5, 0.8);
    double lambda = luaL_optnumber(L, 6, 0.01); // <--- Nuevo parametro (índice 6)

    m->build_matrix(L, 2);
    if (m->X.size() < 2) return luaL_error(L, "not enough data");

    int split = (int)(m->X.size() * ratio);
    std::vector<std::vector<double>> X_tr(m->X.begin(), m->X.begin() + split);
    std::vector<double> y_tr(m->y.begin(), m->y.begin() + split);

    m->compute_stats(X_tr, y_tr);
    for(size_t i=0; i<X_tr.size(); ++i) {
        for(size_t j=0; j<m->features.size(); ++j) X_tr[i][j] = (X_tr[i][j] - m->mean_x[j]) / (m->std_x[j] + 1e-12);
        y_tr[i] = (y_tr[i] - m->mean_y) / (m->std_y + 1e-12);
    }

    // Pasar lambda al metodo fit
    m->fit(lr, epochs, lambda, X_tr, y_tr);
    
    lua_pushboolean(L, true);
    return 1;
}

// --- BINDINGS: LOGISTIC ---

static int logr_load(lua_State* L) {
    auto* m = *(LogisticRegression**)luaL_checkudata(L, 1, "cml.LogisticRegression");
    m->build_matrix(L, 2);
    return 0;
}

static int logr_normalize(lua_State* L) {
    auto* m = *(LogisticRegression**)luaL_checkudata(L, 1, "cml.LogisticRegression");
    if (!m->X.empty()) m->compute_stats(m->X);
    return 0;
}

static int logr_fit(lua_State* L) {
    auto* m = *(LogisticRegression**)luaL_checkudata(L, 1, "cml.LogisticRegression");
    double lr = luaL_optnumber(L, 3, 0.01);
    int epochs = (int)luaL_optinteger(L, 4, 1000);
    m->build_matrix(L, 2);
    if (m->X.empty()) return 0;
    m->compute_stats(m->X);
    size_t k = m->features.size();
    m->weights.assign(k + 1, 0.0);
    for (int e = 0; e < epochs; e++) {
        std::vector<double> grad(k + 1, 0.0);
        for (size_t i = 0; i < m->X.size(); i++) {
            double z = m->weights[0];
            for (size_t j = 0; j < k; j++) 
                z += m->weights[j + 1] * ((m->X[i][j] - m->mean_x[j]) / (m->std_x[j] + 1e-12));
            double err = LogisticRegression::sigmoid(z) - m->y[i];
            grad[0] += err;
            for (size_t j = 0; j < k; j++) 
                grad[j+1] += err * ((m->X[i][j] - m->mean_x[j]) / (m->std_x[j] + 1e-12));
        }
        for (size_t j = 0; j <= k; j++) m->weights[j] -= lr * grad[j] / m->X.size();
    }
    lua_pushboolean(L, true);
    return 1;
}

static int logr_probability(lua_State* L) {
    auto* m = *(LogisticRegression**)luaL_checkudata(L, 1, "cml.LogisticRegression");
    double z = m->weights[0];
    for (size_t j = 0; j < m->features.size(); j++) {
        lua_getfield(L, 2, m->features[j].c_str());
        double v = to_number(L, -1);
        lua_pop(L, 1);
        if (!m->mean_x.empty()) v = (v - m->mean_x[j]) / (m->std_x[j] + 1e-12);
        z += m->weights[j + 1] * v;
    }
    lua_pushnumber(L, LogisticRegression::sigmoid(z));
    return 1;
}

static int logr_predict(lua_State* L) {
    lua_pushcfunction(L, logr_probability);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1);
    lua_pushnumber(L, lua_tonumber(L, -1) >= 0.5 ? 1 : 0);
    return 1;
}

static int logr_accuracy(lua_State* L) {
    auto* m = *(LogisticRegression**)luaL_checkudata(L, 1, "cml.LogisticRegression");
    int n = (int)lua_rawlen(L, 2);
    int correct = 0, total = 0;
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, 2, i);
        lua_getfield(L, -1, m->target.c_str());
        double real = to_number(L, -1);
        lua_pop(L, 1);
        if (std::isfinite(real)) {
            lua_pushcfunction(L, logr_probability);
            lua_pushvalue(L, 1); lua_pushvalue(L, -3);
            lua_call(L, 2, 1);
            double prob = lua_tonumber(L, -1); lua_pop(L, 1);
            if ((real >= 0.5 && prob >= 0.5) || (real < 0.5 && prob < 0.5)) correct++;
            total++;
        }
        lua_pop(L, 1);
    }
    lua_pushnumber(L, total > 0 ? (double)correct / total : 0.0);
    return 1;
}

static int logr_export(lua_State* L) {
    auto* m = *(LogisticRegression**)luaL_checkudata(L, 1, "cml.LogisticRegression");
    lua_newtable(L);
    lua_newtable(L);
    for (size_t i = 0; i < m->features.size(); i++) {
        lua_pushstring(L, m->features[i].c_str()); lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "features");
    lua_newtable(L);
    for (size_t i = 0; i < m->weights.size(); i++) {
        lua_pushnumber(L, m->weights[i]); lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "weights");
    lua_pushnumber(L, m->weights.empty() ? 0.0 : m->weights[0]);
    lua_setfield(L, -2, "bias");
    return 1;
}

static int logr_get_weights(lua_State* L) {
    auto* m = *(LogisticRegression**)luaL_checkudata(L, 1, "cml.LogisticRegression");
    lua_newtable(L);
    for (size_t i = 0; i < m->weights.size(); i++) {
        lua_pushnumber(L, m->weights[i]); lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// --- GC ---
static int lr_gc(lua_State* L) { delete *(LinearRegression**)lua_touserdata(L, 1); return 0; }
static int logr_gc(lua_State* L) { delete *(LogisticRegression**)lua_touserdata(L, 1); return 0; }

// --- CONSTRUCTORES ---

static int lr_new(lua_State* L) {
    auto** obj = (LinearRegression**)lua_newuserdata(L, sizeof(LinearRegression*));
    *obj = new LinearRegression();
    luaL_getmetatable(L, "cml.LinearRegression");
    lua_setmetatable(L, -2);
    lua_getfield(L, 1, "features");
    for (int i = 1; i <= (int)lua_rawlen(L, -1); i++) {
        lua_rawgeti(L, -1, i); (*obj)->features.push_back(lua_tostring(L, -1)); lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 1, "target"); (*obj)->target = lua_tostring(L, -1); lua_pop(L, 1);
    return 1;
}

static int logr_new(lua_State* L) {
    auto** obj = (LogisticRegression**)lua_newuserdata(L, sizeof(LogisticRegression*));
    *obj = new LogisticRegression();
    luaL_getmetatable(L, "cml.LogisticRegression");
    lua_setmetatable(L, -2);
    lua_getfield(L, 1, "features");
    for (int i = 1; i <= (int)lua_rawlen(L, -1); i++) {
        lua_rawgeti(L, -1, i); (*obj)->features.push_back(lua_tostring(L, -1)); lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 1, "target"); (*obj)->target = lua_tostring(L, -1); lua_pop(L, 1);
    return 1;
}

// --- REGISTRO ---

static const luaL_Reg logistic_methods[] = {
    {"load",        logr_load},
	{"normalize",   logr_normalize},
	{"fit",         logr_fit},
	{"predict",     logr_predict},
    {"probability", logr_probability},
	{"accuracy",    logr_accuracy},
    {"get_weights", logr_get_weights},
	{"export",      logr_export},
	{NULL, NULL}
};

static const luaL_Reg linear_methods[] = {
    {"load",      lr_load},
    {"normalize", lr_normalize},
    {"fit",       lr_fit},
    {"predict",   lr_predict},
    {"mse",       lr_mse},
    {"export",    lr_export},
	{"r2",        lr_r2},
    {NULL, NULL}
};

extern "C" int luaopen_cml(lua_State* L) {
    // --- Metatabla Linear ---
    luaL_newmetatable(L, "cml.LinearRegression");
    lua_pushvalue(L, -1); 
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, linear_methods, 0); // REGISTRAMOS LA TABLA COMPLETA
    lua_pushcfunction(L, lr_gc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);

    // --- Metatabla Logistic (ya corregida antes) ---
    luaL_newmetatable(L, "cml.LogisticRegression");
    lua_pushvalue(L, -1); 
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, logistic_methods, 0);
    lua_pushcfunction(L, logr_gc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);

    // --- Tabla del Modulo ---
    lua_newtable(L);
    lua_pushcfunction(L, lr_new); lua_setfield(L, -2, "LinearRegression");
    lua_pushcfunction(L, logr_new); lua_setfield(L, -2, "LogisticRegression");
    
    return 1;
}
