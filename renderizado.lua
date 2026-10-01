import("cjob", "Enum", "Math", "system")
import_as("cColor3", "Color3")
import_as("cEasingModes", "EasingModes")

local ffi = require("ffi")
ffi.cdef[[
    typedef struct _XDisplay Display;
    typedef unsigned long Window;
    typedef unsigned long Pixel;
    typedef unsigned long Drawable;
    typedef unsigned long Pixmap;
    typedef struct _XGC *GC;

    Display* XOpenDisplay(const char *display_name);
    Window XCreateSimpleWindow(Display *display, Window parent, int x, int y, 
                               unsigned int width, unsigned int height, 
                               unsigned int border_width, unsigned long border, 
                               unsigned long background);
    Pixmap XCreatePixmap(Display *display, Drawable d, unsigned int width, unsigned int height, unsigned int depth);
    int XMapWindow(Display *display, Window w);
    GC XCreateGC(Display *display, Drawable d, unsigned long valuemask, void *values);
    int XSetForeground(Display *display, GC gc, unsigned long foreground);
    int XFillRectangle(Display *display, Drawable d, GC gc, int x, int y, unsigned int width, unsigned int height);
    int XCopyArea(Display *display, Drawable src, Drawable dest, GC gc, 
                 int src_x, int src_y, unsigned int width, unsigned int height, int dest_x, int dest_y);
    int XFlush(Display *display);
    int XCloseDisplay(Display *display);
    
    int XDefaultScreen(Display *display);
    Window XDefaultRootWindow(Display *display);
    int XDefaultDepth(Display *display, int screen_number);
    int usleep(unsigned int usec);
]]

local x11 = ffi.load("X11")
local C = ffi.C

local dpy = x11.XOpenDisplay(nil)
if dpy == nil then error("No se pudo abrir X11 Display") end

local screen = x11.XDefaultScreen(dpy)
local root = x11.XDefaultRootWindow(dpy)
local depth = x11.XDefaultDepth(dpy, screen)

local COLOR_BLACK = 0x000000
local color3_cuadrado1 = Color3.fromRGB(255, 51, 68):transformAsHEX() -- 0xFF3344
local color3_cuadrado2 = Color3.fromRGB(0, 229, 255):transformAsHEX() -- 0x00E5FF

local width, height = 1200, 800
local win = x11.XCreateSimpleWindow(dpy, root, 10, 10, width, height, 1, COLOR_BLACK, COLOR_BLACK)

x11.XMapWindow(dpy, win)
x11.XFlush(dpy)

C.usleep(100000)

local buffer = x11.XCreatePixmap(dpy, win, width, height, depth)
local gc = x11.XCreateGC(dpy, win, 0, nil)

-- Estado de los objetos
local square1 = { x = 100, y = 150, size = 100 }
local square2 = { x = 100, y = 350, size = 100 }

-- Obtener funciones de easing directamente del Enum
local allEasingModes = {}

for Key_StyleMode, Value_StyleMode in pairs(EasingModes) do
    for Key_EaseMode, Value_EaseMode in pairs(Value_StyleMode) do
        table.insert(allEasingModes, {name = string.format("[%s][%s]", Key_StyleMode, Key_EaseMode), value = Value_EaseMode, index = #allEasingModes+1})
    end
end

allEasingModes.lenght = #allEasingModes

local function nextEasingMode(idx)
    return allEasingModes[Math.xclamp(idx, 1, allEasingModes.lenght)]
end

local f1 = nextEasingMode(1)
local f2 = nextEasingMode(2)

-- 1. Job Lógica: Cuadrado 1 (Modifica su posición y su color cíclico usando el Easing actual)
cjob.new(function()
    local start_x, target_x = 0, width - square1.size
    local duration = 1.0

    while true do
        local elapsed = 0
        print("f1: "..f1.name)
        while elapsed < duration do
            local dt = cjob.wait(0)
            elapsed = elapsed + dt/1000
            local alpha = math.min(elapsed / duration, 1.0)
            
            -- Posición con easing
            local eased_alpha = f1.value(alpha)
            square1.x = start_x + (target_x - start_x) * eased_alpha
            
            -- Color dinámico basado en el avance del easing (Cicatrizando tonos HSV de 0 a 360)
            local hue = (eased_alpha * 360 + (f1.index * 45)) % 360
            color3_cuadrado1 = Color3.hsvToHEX(hue, 1.0, 1.0)
        end

        -- Fijar valor destino exacto y sincronizar frame
        square1.x = target_x

        -- Invertir dirección y avanzar de easing
        start_x, target_x = target_x, start_x
        f1 = nextEasingMode(f1.index+1)
    end
end)

-- 2. Job Lógica: Cuadrado 2 (Modifica su posición y su color cíclico usando el Easing actual)
cjob.new(function()
    local start_x, target_x = 0, width - square2.size
    local duration = 1.0

    while true do
        local elapsed = 0
        print("f2: "..f2.name)
        while elapsed < duration do
            local dt = cjob.wait(0)
            elapsed = elapsed + dt/1000
            local alpha = math.min(elapsed / duration, 1.0)
            
            -- Posición con easing
            local eased_alpha = f2.value(alpha)
            square2.x = start_x + (target_x - start_x) * eased_alpha
            
            -- Color dinámico desfasado respecto al cuadrado 1
            local hue = ((1.0 - eased_alpha) * 360 + (f2.index * 45)) % 360
            color3_cuadrado2 = Color3.hsvToHEX(hue, 1.0, 1.0)
        end

        -- Fijar valor destino exacto y sincronizar frame
        square2.x = target_x

        -- Invertir dirección y avanzar de easing
        start_x, target_x = target_x, start_x
        f2 = nextEasingMode(f2.index+1)
    end
end)

-- 3. Job Principal: Render Loop
cjob.new(function()
    while true do--for frame = 1, 3e9, 1 do
        -- Clear
        x11.XSetForeground(dpy, gc, COLOR_BLACK)
        x11.XFillRectangle(dpy, buffer, gc, 0, 0, width, height)

        -- Draw Cuadrado 1 (Rojo)
        x11.XSetForeground(dpy, gc, color3_cuadrado1)
        x11.XFillRectangle(dpy, buffer, gc, math.floor(square1.x), math.floor(square1.y), square1.size, square1.size)

        -- Draw Cuadrado 2 (Cian)
        x11.XSetForeground(dpy, gc, color3_cuadrado2)
        x11.XFillRectangle(dpy, buffer, gc, math.floor(square2.x), math.floor(square2.y), square2.size, square2.size)

        -- Present
        x11.XCopyArea(dpy, buffer, win, gc, 0, 0, width, height, 0, 0)
        x11.XFlush(dpy)

        cjob.wait(1 / 120) -- 120 cuadros por segundo
    end

    x11.XCloseDisplay(dpy)
    os.exit()
end)

-- Toda la aplicación se orquesta desde aquí
cjob.async()