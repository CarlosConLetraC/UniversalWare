#pragma once
#include <litehtml.h>
#include <raylib.h>
#include <string>
#include <vector>

class HtmlContainer : public litehtml::document_container {
public:
    HtmlContainer();
    virtual ~HtmlContainer();

    // --- Métodos obligatorios de litehtml::document_container ---
    litehtml::tchar_t* resolve_color(const litehtml::tchar_t* color) const override;
    void draw_text(litehtml::uint_h hdc, const litehtml::tchar_t* text, litehtml::uint_h hFont, litehtml::web_color color, const litehtml::position& pos) override;
    int pt_to_px(int pt) const override;
    int get_default_font_size() const override;
    const litehtml::tchar_t* get_default_font_name() const override;
    void draw_background(litehtml::uint_h hdc, const litehtml::position& pos, const litehtml::background& bg) override;
    void draw_borders(litehtml::uint_h hdc, const litehtml::borders& borders, const litehtml::position& pos, bool root) override;

    // Métodos de gestión de fuentes y dimensiones (Stubs optimizados)
    litehtml::uint_h create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics& fm) override;
    void delete_font(litehtml::uint_h hFont) override;
    void text_width(const litehtml::tchar_t* text, litehtml::uint_h hFont, int& width) override;
    void draw_list_marker(litehtml::uint_h hdc, const litehtml::list_marker& marker) override;
    void load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_load) override;
    void get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz) override;
    void set_caption(const litehtml::tchar_t* caption) override;
    void set_base_url(const litehtml::tchar_t* base_url) override;
    void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
    void on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el) override;
    void set_cursor(const litehtml::tchar_t* cursor) override;
    void transform_text(litehtml::tchar_t* text, litehtml::text_transform tt) override;
    void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
    void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radiuses) override;
    void del_clip() override;
    std::shared_ptr<litehtml::element> create_element(const litehtml::tchar_t* tag_name, const litehtml::attributes_map& attributes, const std::shared_ptr<litehtml::document>& doc) override;
    
    // Función personalizada para pintar el documento renderizado usando Raylib
    void render(int mouse_x, int mouse_y, bool clicked);

    // Cargar archivo HTML y su CSS asociado
    bool load_html(const std::string& html_content, const std::string& css_content);

private:
    std::shared_ptr<litehtml::document> m_document;
    Font m_default_font;
    int m_viewport_width;
    int m_viewport_height;
};