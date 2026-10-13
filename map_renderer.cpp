#include "geo.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include "map_renderer.h"

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};


std::pair<svg::Text, svg::Text> MapRenderer::CreateRouteText(domain::Route& route, int color_index, svg::Point stop_coordinates){

    svg::Text route_text;
    route_text.SetPosition(stop_coordinates);
    route_text.SetOffset({this->bus_label_offset_dx, this->bus_label_offset_dy});
    route_text.SetFontSize(this->bus_label_font_size);
    route_text.SetFontFamily("Verdana");
    route_text.SetFontWeight("bold");
    route_text.SetData(route.route_name);

    svg::Text route_text_back = route_text;
    route_text_back.SetFillColor(this->underlayer_color);
    route_text_back.SetStrokeColor(this->underlayer_color);
    route_text_back.SetStrokeWidth(this->underlayer_width);
    route_text_back.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    route_text_back.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    route_text.SetFillColor(this->color_palette[color_index]);

    return {route_text_back, route_text};
}

std::pair<svg::Text, svg::Text> MapRenderer::CreateStopText(domain::Stop& stop, svg::Point stop_coordinates){

    svg::Text stop_text;
    stop_text.SetPosition(stop_coordinates);
    stop_text.SetOffset({this->stop_label_offset_dx, this->stop_label_offset_dy});
    stop_text.SetFontSize(this->stop_label_font_size);
    stop_text.SetFontFamily("Verdana");
    stop_text.SetData(stop.stop_name);

    svg::Text stop_text_back = stop_text;
    stop_text_back.SetFillColor(this->underlayer_color);
    stop_text_back.SetStrokeColor(this->underlayer_color);
    stop_text_back.SetStrokeWidth(this->underlayer_width);
    stop_text_back.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    stop_text_back.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    stop_text.SetFillColor("black");

    return {stop_text_back, stop_text};
}


void MapRenderer::AddRoutesToMap(svg::Document& map_objects, std::set<domain::Route*, domain::RoutePtrCompare>& routes_sorted, const SphereProjector& proj){
    int color_index = 0;
    for (auto routes_ptr:routes_sorted){
        svg::Polyline polyline;
        for (auto route_stops:routes_ptr->stops){
            const svg::Point screen_coord = proj(route_stops->coordinates);
            polyline.AddPoint(screen_coord);
        }
        polyline.SetStrokeWidth(this->line_width);
        polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        polyline.SetFillColor(svg::NoneColor);
        polyline.SetStrokeColor(this->color_palette[color_index]);

        map_objects.Add(polyline);
        color_index++;
        if (color_index == static_cast<int>(this->color_palette.size()))
            color_index = 0;
    }

}

void MapRenderer::AddRoutesTextToMap(svg::Document& map_objects, std::set<domain::Route*, domain::RoutePtrCompare>& routes_sorted, const SphereProjector& proj){
    int color_index = 0;
    for (auto routes_ptr:routes_sorted){
        std::pair<svg::Text, svg::Text> route_texts = CreateRouteText(*routes_ptr, color_index, proj(routes_ptr->stops[0]->coordinates));
        map_objects.Add(route_texts.first);
        map_objects.Add(route_texts.second);
        if (*routes_ptr->stops[0] != *routes_ptr->stops[routes_ptr->stops.size()/2] && (!routes_ptr->roundtrip)){
            std::pair<svg::Text, svg::Text> route_texts_last_stop = CreateRouteText(*routes_ptr, color_index, proj(routes_ptr->stops[routes_ptr->stops.size()/2]->coordinates));
            map_objects.Add(route_texts_last_stop.first);
            map_objects.Add(route_texts_last_stop.second);
        }

        color_index++;
        if (color_index == static_cast<int>(this->color_palette.size()))
            color_index = 0;
    }
}

void MapRenderer::AddStopsToMap(svg::Document& map_objects, std::set<domain::Stop*, domain::StopPtrCompare>& stops_sorted, const SphereProjector& proj){
    for (auto stops_ptr:stops_sorted){
        svg::Circle circle;
        circle.SetCenter(proj(stops_ptr->coordinates));
        circle.SetRadius(this->stop_radius);
        circle.SetFillColor("white");
        map_objects.Add(circle);
    }
}

void MapRenderer::AddStopsTextToMap(svg::Document& map_objects, std::set<domain::Stop*, domain::StopPtrCompare>& stops_sorted, const SphereProjector& proj){
    for (auto stops_ptr:stops_sorted){
        std::pair<svg::Text, svg::Text> stop_texts = CreateStopText(*stops_ptr, proj(stops_ptr->coordinates));
        map_objects.Add(stop_texts.first);
        map_objects.Add(stop_texts.second);
    }
}

void MapRenderer::AddObjectsToMap(svg::Document& map_objects, std::unordered_map<std::string_view, domain::Route*> routes_name){
    std::vector<geo::Coordinates> geo_coords;
    std::set<domain::Route*, domain::RoutePtrCompare> routes_sorted;
    std::set<domain::Stop*, domain::StopPtrCompare> stops_sorted;
    for (auto& [routes_key, routes_ptr]:routes_name){
        if (!routes_ptr->stops.empty()){
            routes_sorted.insert(routes_ptr);
            for (auto route_stops:routes_ptr->stops){
                stops_sorted.insert(route_stops);
                geo_coords.push_back(route_stops->coordinates);
            }
        }
    }

    const SphereProjector proj{
        geo_coords.begin(), geo_coords.end(), this->width, this->height, this->padding
    };

    AddRoutesToMap(map_objects, routes_sorted, proj);
    AddRoutesTextToMap(map_objects, routes_sorted, proj);
    AddStopsToMap(map_objects, stops_sorted, proj);
    AddStopsTextToMap(map_objects, stops_sorted, proj);

}


