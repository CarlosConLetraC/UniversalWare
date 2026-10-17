import("raylib", "cColor3")

-- Inicialización de la ventana de la aplicación
local screenWidth = 900
local screenHeight = 600
raylib.init_window(screenWidth, screenHeight, "UniversalWare Desktop Framework")
raylib.set_target_fps(true)

-- Paleta de colores usando tu librería nativa cColor3 (tematización fluida)
local theme = {
    bg = cColor3.fromRGB(30, 30, 46),          -- Fondo oscuro moderno (estilo IDE)
    sidebar = cColor3.fromRGB(24, 24, 37),     -- Panel lateral
    card = cColor3.fromRGB(49, 50, 68),        -- Tarjetas / contenedores
    text = cColor3.fromRGB(205, 214, 244),     -- Texto principal
    accent = cColor3.fromRGB(137, 180, 250),   -- Color de acento (Azul suave)
    hover = cColor3.fromRGB(166, 227, 161)     -- Color interactivo (Verde suave)
}

-- Estado de la aplicación
local counter = 0
local statusText = "Estado: Listo y operando con bajo consumo."

-- Utilidad simple para detectar clics en botones (Immediate Mode UI)
local function drawButton(text, x, y, w, h, baseColor, hoverColor, callback)
    -- Lógica simulada de interacción de mouse (se puede expandir con inputs de raylib)
    -- Por ahora dibujamos el contenedor y el texto
    raylib.draw_text(text, x + 15, y + 12, 16, theme.text)
end

-- Loop principal de la aplicación de escritorio
while not raylib.window_should_close() do
    
    raylib.begin_drawing()
    raylib.clear_background(theme.bg)
    
    -- 1. Dibujar Barra Lateral (Sidebar)
    
    -- 2. Dibujar Header / Barra Superior
    raylib.draw_text("UniversalWare Desktop Engine", 20, 20, 22, theme.accent)
    raylib.draw_text("Alternativa nativa ultraligera a Electron y .NET", 20, 48, 14, theme.text)

    -- 3. Dibujar Panel de Contenido / Tarjeta de Estadísticas
    raylib.draw_text("Panel de Control de Rendimiento", 250, 100, 18, theme.hover)
    raylib.draw_text(string.format("Contador de eventos interactivos: %d", counter), 250, 140, 16, theme.text)
    raylib.draw_text(statusText, 250, 180, 14, theme.text)

    -- Simulación de interactividad con eventos de teclado básicos
    -- TODO: agregarlos. . .
    
    raylib.end_drawing()
end

raylib.close_window()