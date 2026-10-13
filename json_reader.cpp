#include "json_reader.h"
#include "json.h"
#include "map_renderer.h"
#include <sstream>
#include <string_view>
#include "svg.h"
#include "json_builder.h"

JSONReader::JSONReader(RequestHandler& request_handler):request_handler_(request_handler){};

json::Document JSONReader::LoadJSON(const std::string& s) {
    std::istringstream strm(s);
    auto doc = json::Load(strm);
    return ReadAndProcessRequests(doc);
}

json::Document JSONReader::LoadJSON(std::istream& input) {
    auto doc = json::Load(input);
    return ReadAndProcessRequests(doc);
}

void JSONReader::WriteStopsToCatalogue(const json::Node& node, std::unordered_map<std::pair<std::string, std::string>, Weight, DistancePairHasher>& stop_distances){
    auto dict = node.AsMap();
    TransportCatalogue& catalogue = request_handler_.GetCatalogue();
    if (dict["type"].AsString() == "Stop"){
        domain::Stop stop;
        stop.coordinates = {dict["latitude"].AsDouble(), dict["longitude"].AsDouble()};
        stop.stop_name = dict["name"].AsString();
        if (dict.count("road_distances") > 0){
            auto road_dist_dict = dict["road_distances"].AsMap();
            for (auto& [key, value]:road_dist_dict){
                stop_distances[{stop.stop_name, key}] = value.AsDouble();
            }
        }
        catalogue.AddStop(stop);
    }
}

void JSONReader::WriteRoutesToCatalogue(const json::Node& node) {
    auto dict = node.AsMap();
    TransportCatalogue& catalogue = request_handler_.GetCatalogue();
    if (dict["type"].AsString() == "Bus") {
        domain::Route route;
        route.route_name = dict["name"].AsString();
        json::Array route_stops = dict["stops"].AsArray();

        std::vector<Weight> prefix_distances(route_stops.size(), 0);

        for (size_t i = 0; i < route_stops.size(); i++) {
            if (domain::Stop* catalogue_stop = catalogue.FindStop(route_stops[i].AsString())) {
                route.stops.push_back(catalogue_stop);
            }
        }

        for (size_t i = 1; i < route.stops.size(); i++) {
            auto dist = catalogue.GetStopDistances(route.stops[i-1], route.stops[i]);
            if (!dist.has_value() && !dict["is_roundtrip"].AsBool()) {
                dist = catalogue.GetStopDistances(route.stops[i], route.stops[i-1]);
            }
            if (dist.has_value()) {
                prefix_distances[i] = prefix_distances[i-1] + dist.value();
            }
        }

        if (!dict["is_roundtrip"].AsBool() && route.stops.size() > 1) {
            for (int i = route.stops.size() - 2; i >= 0; i--) {
                route.stops.push_back(route.stops[i]);
            }
        }

        route.roundtrip = dict["is_roundtrip"].AsBool();
        route.distances_between_stops = prefix_distances;
        catalogue.AddRoute(route);
    }
}
svg::Color JSONReader::IdentifyColorType(const json::Node& color_node){
    try {
        if (color_node.IsString()) {
            return color_node.AsString();
        } else if (color_node.IsArray()) {
            json::Array color_array = color_node.AsArray();
            if (color_array.size() == 3) {
                svg::Rgb rgb;
                rgb.red = color_array[0].AsDouble();
                rgb.green = color_array[1].AsDouble();
                rgb.blue = color_array[2].AsDouble();
                return rgb;
            } else if (color_array.size() == 4) {
                svg::Rgba rgba;
                rgba.red = color_array[0].AsDouble();
                rgba.green = color_array[1].AsDouble();
                rgba.blue = color_array[2].AsDouble();
                rgba.opacity = color_array[3].AsDouble();
                return rgba;
            }
        }
    } catch (const std::exception& e) {
        return svg::Rgb{0, 0, 0};
    }
    return svg::Rgb{0, 0, 0};
}

void JSONReader::WriteSvgSettings(const json::Node& node){
    json::Dict dict = node.AsMap();
    MapRenderer& renderer = request_handler_.GetMapRenderer();
    if (dict.count("width")) renderer.width = dict["width"].AsDouble();
    if (dict.count("height")) renderer.height = dict["height"].AsDouble();
    if (dict.count("padding")) renderer.padding = dict["padding"].AsDouble();
    if (dict.count("line_width")) renderer.line_width = dict["line_width"].AsDouble();
    if (dict.count("stop_radius")) renderer.stop_radius = dict["stop_radius"].AsDouble();
    if (dict.count("bus_label_font_size")) renderer.bus_label_font_size = dict["bus_label_font_size"].AsInt();
    if (dict.count("bus_label_offset")){
        json::Array offset_array = dict["bus_label_offset"].AsArray();
        renderer.bus_label_offset_dx = offset_array[0].AsDouble();
        renderer.bus_label_offset_dy = offset_array[1].AsDouble();
    }
    if (dict.count("stop_label_font_size")) renderer.stop_label_font_size = dict["stop_label_font_size"].AsInt();
    if (dict.count("stop_label_offset")) {
        json::Array offset_array = dict["stop_label_offset"].AsArray();
        renderer.stop_label_offset_dx = offset_array[0].AsDouble();
        renderer.stop_label_offset_dy = offset_array[1].AsDouble();
    }
    if (dict.count("underlayer_width")) renderer.underlayer_width = dict["underlayer_width"].AsDouble();

    if (dict.count("underlayer_color")) renderer.underlayer_color = IdentifyColorType(dict["underlayer_color"]);

    if (dict.count("color_palette")) {
        json::Array palette_array = dict["color_palette"].AsArray();
        for (auto& i:palette_array){
            renderer.color_palette.push_back(IdentifyColorType(i));
        }
    }
}

json::Node JSONReader::RequestResponse(const json::Node& node){
    json::Dict dict = node.AsMap();
    if (dict["type"].AsString() == "Bus"){
        auto route_stats = request_handler_.GetRouteStat(dict["name"].AsString());
        if (!route_stats.has_value()){
            return json::Builder{}
                        .StartDict()
                            .Key("request_id").Value(dict["id"].AsInt())
                            .Key("error_message").Value(std::string("not found"))
                        .EndDict()
                        .Build();
        }

        return json::Builder{}
                    .StartDict()
                        .Key("curvature").Value(route_stats->curvature)
                        .Key("request_id").Value(dict["id"].AsInt())
                        .Key("route_length").Value(route_stats->route_lenght)
                        .Key("stop_count").Value(static_cast<int>(route_stats->stops_amount))
                        .Key("unique_stop_count").Value(static_cast<int>(route_stats->unique_stops_amount))
                    .EndDict()
                    .Build();
    }

    if (dict["type"].AsString() == "Stop"){
        std::optional<std::set<std::string_view>> stop_routes = request_handler_.GetBusesByStop(dict["name"].AsString());
        if (!stop_routes.has_value()){
            return json::Builder{}
                        .StartDict()
                            .Key("request_id").Value(dict["id"].AsInt())
                            .Key("error_message").Value(std::string("not found"))
                        .EndDict()
                        .Build();
        }

        json::Builder builder{};

        builder.StartDict().Key("request_id").Value(dict["id"].AsInt()).Key("buses").StartArray();

        for (const auto& i:stop_routes.value()){
            builder.Value(std::string(i));
        }
        return builder.EndArray().EndDict().Build();
    }

    if (dict["type"].AsString() == "Map"){
        std::ostringstream map_responce;
        svg::Document doc = request_handler_.RenderMap();
        doc.Render(map_responce);
        return json::Builder{}
                    .StartDict()
                        .Key("request_id").Value(dict["id"].AsInt())
                        .Key("map").Value(map_responce.str())
                    .EndDict()
                    .Build();
    }

    if (dict["type"].AsString() == "Route"){
        std::optional<std::vector<domain::Item>> near_route_info_opt = request_handler_.GetRoute(dict["from"].AsString(), dict["to"].AsString());

        if (!near_route_info_opt.has_value()){
            return json::Builder{}
                .StartDict()
                .Key("request_id").Value(dict["id"].AsInt())
                .Key("error_message").Value(std::string("not found"))
                .EndDict()
                .Build();
       }
        std::vector<domain::Item>& near_route_info = near_route_info_opt.value();
        json::Array json_item;
        Weight total_time = 0;
        for (auto& i : near_route_info){
            json::Dict dict_wait;
            dict_wait["stop_name"] = std::string(i.stop_name);
            dict_wait["time"] = domain::Stop::bus_wait_time;
            dict_wait["type"] = std::string("Wait");

            json::Dict dict_route;
            dict_route["bus"] = std::string(i.route_name);
            dict_route["time"] = i.time - static_cast<Weight>(domain::Stop::bus_wait_time);
            dict_route["type"] = std::string("Bus");
            dict_route["span_count"] = i.span;

            json_item.push_back(dict_wait);
            json_item.push_back(dict_route);
            total_time += i.time;
        }

        return json::Builder{}.StartDict()
            .Key("request_id").Value(dict["id"].AsInt())
            .Key("total_time").Value(total_time)
            .Key("items")
            .Value(json::Node(json_item).GetValue())
            .EndDict()
            .Build();
    }
    return json::Node();
}


json::Document JSONReader::ReadAndProcessRequests(const json::Document& doc){
    TransportCatalogue& catalogue = request_handler_.GetCatalogue();
    json::Dict json_dict = doc.GetRoot().AsMap();
    json::Array responce_array;
    json::Array array_json;
    if (json_dict.count("base_requests") > 0){
        array_json = json_dict["base_requests"].AsArray();
        std::unordered_map<std::pair<std::string, std::string>, Weight, DistancePairHasher> stop_distances;
        for (auto& i:array_json){
            WriteStopsToCatalogue(i, stop_distances);
        }

        for (const auto& [elem, dist]:stop_distances){
            domain::Stop* stop1 = catalogue.FindStop(elem.first);
            domain::Stop* stop2 = catalogue.FindStop(elem.second);
            catalogue.SetStopDistances(stop1, stop2, dist);
        }

        for (auto& i:array_json){
            WriteRoutesToCatalogue(i);
        }

    }

    if (json_dict.count("render_settings") > 0){
        WriteSvgSettings(json_dict["render_settings"]);
    }

    if (json_dict.count("routing_settings") > 0){
        json::Dict dict = json_dict["routing_settings"].AsMap();
        domain::Stop::bus_wait_time = dict["bus_wait_time"].AsInt();
        domain::Route::bus_velocity = dict["bus_velocity"].AsDouble();
        request_handler_.MakeRouter();
    }

    if (json_dict.count("stat_requests") > 0){
        array_json = json_dict["stat_requests"].AsArray();
        for (auto& i:array_json){
            responce_array.push_back(RequestResponse(i));
        }
    }

    json::Document doc_responce{responce_array};
    return doc_responce;
}

