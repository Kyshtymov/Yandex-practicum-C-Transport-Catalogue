#pragma once
#include "svg.h"
#include "domain.h"
#include <vector>
#include "unordered_map"

class SphereProjector;

struct MapRenderer{
    double width = 1200;
    double height = 800;
    double padding = 10;
    double line_width = 0;
    double stop_radius = 0;
    int bus_label_font_size = 1;
    double bus_label_offset_dx = 0;
    double bus_label_offset_dy = 0;
    int stop_label_font_size = 0;
    double stop_label_offset_dx = 0;
    double stop_label_offset_dy = 0;
    svg::Color underlayer_color;
    double underlayer_width = 0;
    std::vector<svg::Color> color_palette;
    void AddObjectsToMap(svg::Document& map_objects, std::unordered_map<std::string_view, domain::Route*> routes_name);
    void AddRoutesToMap(svg::Document& map_objects, std::set<domain::Route*, domain::RoutePtrCompare>& routes_sorted, const SphereProjector& proj);
    void AddRoutesTextToMap(svg::Document& map_objects, std::set<domain::Route*, domain::RoutePtrCompare>& routes_sorted, const SphereProjector& proj);
    void AddStopsToMap(svg::Document& map_objects, std::set<domain::Stop*, domain::StopPtrCompare>& stops_sorted, const SphereProjector& proj);
    void AddStopsTextToMap(svg::Document& map_objects, std::set<domain::Stop*, domain::StopPtrCompare>& stops_sorted, const SphereProjector& proj);
    std::pair<svg::Text, svg::Text> CreateRouteText(domain::Route& route, int color_index, svg::Point stop_coordinates);
    std::pair<svg::Text, svg::Text> CreateStopText(domain::Stop& stop, svg::Point stop_coordinates);
};
