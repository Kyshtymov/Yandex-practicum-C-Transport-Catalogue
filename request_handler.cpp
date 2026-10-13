#include "request_handler.h"
#include "map_renderer.h"
#include "router.h"
#include "graph.h"
#include <algorithm>

RequestHandler::RequestHandler(TransportCatalogue& db, MapRenderer& renderer):db_(db), renderer_(renderer){}

std::optional<RouteStat> RequestHandler::GetRouteStat(const std::string_view bus_name) const{
    const auto* route_ptr = db_.FindRoute(bus_name);
    if (route_ptr == nullptr){
        return std::nullopt;
    }
    const auto& route = *route_ptr;
    return db_.RouteStatCalc(route);
}

std::optional<std::set<std::string_view>> RequestHandler::GetBusesByStop(const std::string_view stop_name) const {
    std::set<std::string_view> result;
    if (const auto* stop_ptr = db_.FindStop(stop_name)) {
        for (const auto& route : stop_ptr->routes) {
            result.insert(route->route_name);
        }
    }
    else
        return std::nullopt;
    return result;
}


std::optional<std::vector<domain::Item>> RequestHandler::GetRoute(const std::string_view from, const std::string_view to){

    if (!transport_router_.has_value())
        return std::nullopt;

    domain::Stop* stop_from = db_.FindStop(from);
    domain::Stop* stop_to = db_.FindStop(to);
    auto routes_edges = transport_router_->GetNearestRoute(stop_from, stop_to);

    std::vector<domain::Item> vect_item;
    if (routes_edges.has_value()) {
        for (auto& edge : routes_edges.value()) {
            domain::Item item;
            item.stop_name = db_.GetStopByIndex(edge.from)->stop_name;
            item.route_name = edge.route_name;
            item.span = edge.span;
            item.time = edge.weight;
            vect_item.push_back(item);
        }
        return vect_item;
    }
    return std::nullopt;
}

TransportCatalogue& RequestHandler::GetCatalogue(){
    return db_;
}

MapRenderer& RequestHandler::GetMapRenderer(){
    return renderer_;
}

svg::Document RequestHandler::RenderMap() const{
    svg::Document map_objects;
    renderer_.AddObjectsToMap(map_objects, db_.GetRoutesName());
    return map_objects;
}



void RequestHandler::MakeRouter(){
    transport_router_.emplace(db_);
}
