#include "html_container.hpp"

HtmlContainer::HtmlContainer() {
    // Usar fuente por defecto de Raylib o cargar una personalizada
    m_default_font = GetFontDefault();
    m_viewport_width = 800;
    m_viewport_height = 600;
}

HtmlContainer::~HtmlContainer() {
    // Limpieza de recursos si es necesario
}

bool HtmlContainer::load_html(const std::string& html_content, const std::string& css_content) {
    // Inicializar el contexto de litehtml con el CSS base y el HTML parseado
    litehtml::context html_context;
    // html_context.load_master_stylesheet(css_content.c_str()); // Dependiendo de la versión
    
    m_document = litehtml::document::createFromString(html_content.c_str(), this, &html_context);
    if (m_document) {
        m_document->render(m_viewport_width);
        return true;
    }
    return false;
}

void HtmlContainer::render(int mouse_x, int mouse_y, bool clicked) {
    if (m_document) {
        // Renderizar el documento a partir de las coordenadas (0,0)
        litehtml::position pos(0, 0, m_viewport_width, m_viewport_height);
        m_document->draw((litehtml::uint_h)0, 0, 0, &pos);
        
        // Manejo de eventos de clic en enlaces o elementos interactivos
        if (clicked) {
            m_document->on_mouse_click(mouse_x, mouse_y, mouse_x, mouse_y);
        }
    }
}

// --- Traducción de Primitivas a Raylib ---

void HtmlContainer::draw_text(litehtml::uint_h hdc, const litehtml::tchar_t* text, litehtml::uint_h hFont, litehtml::web_color color, const litehtml::position& pos) {
    Color raylib_color = { color.red, color.green, color.blue, color.alpha };
    Vector2 position = { (float)pos.x, (float)pos.y };
    
    // Dibujar texto usando Raylib
    DrawTextEx(m_default_font, text, position, (float)pos.height, 1.0f, raylib_color);
}

void HtmlContainer::draw_background(litehtml::uint_h hdc, const litehtml::position& pos, const litehtml::background& bg) {
    Color bg_color = { bg.m_color.red, bg.m_color.green, bg.m_color.blue, bg.m_color.alpha };
    DrawRectangle(pos.x, pos.y, pos.width, pos.height, bg_color);
}

void HtmlContainer::draw_borders(litehtml::uint_h hdc, const litehtml::borders& borders, const litehtml::position& pos, bool root) {
    // Dibujar bordes usando rectángulos de línea de Raylib si tienen grosor
    if (borders.left.width > 0) {
        DrawRectangle(pos.x, pos.y, borders.left.width, pos.height, 
            { borders.left.color.red, borders.left.color.green, borders.left.color.blue, borders.left.color.alpha });
    }
}

// Stubs requeridos por la interfaz abstracta (sin operaciones complejas adicionales por ahora)
litehtml::tchar_t* HtmlContainer::resolve_color(const litehtml::tchar_t* color) const { return (litehtml::tchar_t*)color; }
int HtmlContainer::pt_to_px(int pt) const { return pt; }
int HtmlContainer::get_default_font_size() const { return 16; }
const litehtml::tchar_t* HtmlContainer::get_default_font_name() const { return "Default"; }
litehtml::uint_h HtmlContainer::create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics& fm) { return 0; }
void HtmlContainer::delete_font(litehtml::uint_h hFont) {}
void HtmlContainer::text_width(const litehtml::tchar_t* text, litehtml::uint_h hFont, int& width) { width = strlen(text) * 8; }
void HtmlContainer::draw_list_marker(litehtml::uint_h hdc, const litehtml::list_marker& marker) {}
void HtmlContainer::load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_load) {}
void HtmlContainer::get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz) { sz.width = 0; sz.height = 0; }
void HtmlContainer::set_caption(const litehtml::tchar_t* caption) {}
void HtmlContainer::set_base_url(const litehtml::tchar_t* base_url) {}
void HtmlContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) {}
void HtmlContainer::on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el) {}
void HtmlContainer::set_cursor(const litehtml::tchar_t* cursor) {}
void HtmlContainer::transform_text(litehtml::tchar_t* text, litehtml::text_transform tt) {}
void HtmlContainer::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) {}
void HtmlContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radiuses) { BeginScissorMode(pos.x, pos.y, pos.width, pos.height); }
void HtmlContainer::del_clip() { EndScissorMode(); }
std::shared_ptr<litehtml::element> HtmlContainer::create_element(const litehtml::tchar_t* tag_name, const litehtml::attributes_map& attributes, const std::shared_ptr<litehtml::document>& doc) { return nullptr; }