#include "svg.h"

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(out);
    out << "/>"sv;
}


Polyline& Polyline::AddPoint(Point point){
    points_.emplace_back(point);
    return *this;
}


void Polyline::RenderObject(const RenderContext& context) const{
    auto& out = context.out;
    out << "<polyline points=\""sv;
    for (size_t i = 0; i < points_.size(); ++i) {
        if (i > 0) out << " "sv;
        out << points_[i].x << ","sv << points_[i].y;
    }
    out << "\" ";
    RenderAttrs(out);
    out << "/>"sv;
}


Text& Text::SetPosition(Point pos){
    pos_ = pos;
    return *this;
}

// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
Text& Text::SetOffset(Point offset){
    offset_ = offset;
    return *this;
}

// Задаёт размеры шрифта (атрибут font-size)
Text& Text::SetFontSize(uint32_t size){
    size_ = size;
    return *this;
}

// Задаёт название шрифта (атрибут font-family)
Text& Text::SetFontFamily(std::string font_family){
    font_family_ = font_family;
    return *this;
}

// Задаёт толщину шрифта (атрибут font-weight)
Text& Text::SetFontWeight(std::string font_weight){
    font_weight_ = font_weight;
    return *this;
}

// Задаёт текстовое содержимое объекта (отображается внутри тега text)
Text& Text::SetData(std::string data){
    data_ = data;
    return *this;
}


void Text::RenderObject(const RenderContext& context) const{
    auto& out = context.out;
    out << "<text"sv;
    out << " x=\""sv << pos_.x << "\""sv;
    out << " y=\""sv << pos_.y << "\""sv;
    //if (offset_.x != 0 || offset_.y != 0) {
    out << " dx=\""sv << offset_.x << "\""sv;
    out << " dy=\""sv << offset_.y << "\""sv;
    //}
    out << " font-size=\""sv << size_ << "\""sv;
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    RenderAttrs(out);
    out << ">"sv;

    for (char c : data_) {
        switch (c) {
        case '"': out << "&quot;"; break;
        case '\'': out << "&apos;"; break;
        case '<': out << "&lt;"; break;
        case '>': out << "&gt;"; break;
        case '&': out << "&amp;"; break;
        default: out << c;
        }
    }

    out << "</text>"sv;
}

void Document::AddPtr(std::unique_ptr<Object>&& obj){
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const{

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

    RenderContext ctx(out, 2, 2);
    for (auto& i:objects_)
        i->Render(ctx);

    out << "</svg>"sv;
}



}  // namespace svg
