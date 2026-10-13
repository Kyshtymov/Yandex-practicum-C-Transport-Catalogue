#include "transport_router.h"
#include <iostream>

TransportRouter::TransportRouter(const TransportCatalogue& db){
    InitializeGraph(db);
}

void TransportRouter::InitializeGraph(const TransportCatalogue& db) {
    const auto& stops_name = db.GetStopsName();
    const auto& routes_name = db.GetRoutesName();

    graph::DirectedWeightedGraph<Weight> route_graph(stops_name.size());
    for (auto& [route_name, route_ptr] : routes_name) {
        auto& stops = route_ptr->stops;
        if (stops.size() < 2) continue;

        for (size_t i = 0; i < stops.size() - 1; ++i) {
            Weight accumulated_distance = 0;
            int span_count = 0;

            for (size_t j = i; j < stops.size() - 1; ++j) {
                auto dist = db.GetStopDistances(stops[j], stops[j+1]);
                if (!dist.has_value() && !route_ptr->roundtrip) {
                    dist = db.GetStopDistances(stops[j+1], stops[j]);
                }
                if (!dist.has_value()) break;

                accumulated_distance += dist.value();
                span_count++;

                Weight factor_hour_to_minutes = 60;
                Weight factor_km_to_m = 1000;

                Weight travel_time = accumulated_distance * factor_hour_to_minutes / (domain::Route::bus_velocity * factor_km_to_m);

                Weight total_time = static_cast<Weight>(domain::Stop::bus_wait_time) + travel_time;

                graph::Edge<Weight> edge;
                edge.from = stops[i]->stop_index;
                edge.to = stops[j+1]->stop_index;
                edge.weight = total_time;
                edge.route_name = route_name;
                edge.span = span_count;
                route_graph.AddEdge(edge);
            }
        }
    }

    graph_ = std::move(route_graph);
    router_.emplace(graph_);
}


std::optional<std::vector<graph::Edge<Weight>>> TransportRouter::GetNearestRoute(domain::Stop* stop_from, domain::Stop* stop_to){
    if (!router_.has_value())
        return std::nullopt;

    auto route_info = router_->BuildRoute(stop_from->stop_index, stop_to->stop_index);

    std::vector<graph::Edge<Weight>> edges_info;
    for (auto& i:route_info->edges)
        edges_info.push_back(graph_.GetEdge(i));

    return edges_info;
}
