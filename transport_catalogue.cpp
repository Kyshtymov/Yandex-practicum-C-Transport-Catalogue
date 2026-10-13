#include "transport_catalogue.h"
#include <unordered_set>


void TransportCatalogue::AddRoute(domain::Route route){
    route_deq_.push_back(std::move(route));
    domain::Route* last_route = &route_deq_.back();
    routes_name_[last_route->route_name] = last_route;

    for (const auto& last_route_stop:last_route->stops){
        auto* stop = FindStop(last_route_stop->stop_name);
        stop->routes.insert(last_route);
    }
}

void TransportCatalogue::AddStop(domain::Stop stop){
    stop_deq_.push_back(std::move(stop));
    domain::Stop* last_stop = &stop_deq_.back();
    last_stop->stop_index = stop_deq_.size() - 1;
    stops_name_[last_stop->stop_name] = last_stop;
}

const domain::Route* TransportCatalogue::FindRoute(std::string_view route_name) const {
    auto route_iter = routes_name_.find(route_name);
    return route_iter != routes_name_.end() ? (*route_iter).second : nullptr;
}

const domain::Stop* TransportCatalogue::FindStop(std::string_view stop_name) const {
    auto stop_iter = stops_name_.find(stop_name);
    return stop_iter != stops_name_.end() ? (*stop_iter).second : nullptr;
}

domain::Stop* TransportCatalogue::FindStop(std::string_view stop_name) {
    auto stop_iter = stops_name_.find(stop_name);
    return stop_iter != stops_name_.end() ? (*stop_iter).second : nullptr;
}


std::optional<Weight> TransportCatalogue::GetStopDistances(domain::Stop* stop1, domain::Stop* stop2) const{
    auto iter = stop_distances_.find({stop1, stop2});
    if (iter == stop_distances_.end())
        return std::nullopt;
    return iter->second;
}

void TransportCatalogue::SetStopDistances(domain::Stop* stop1, domain::Stop* stop2, int distance){
    stop_distances_[{stop1, stop2}] = distance;
}

const std::unordered_map<std::string_view,domain::Route*>& TransportCatalogue::GetRoutesName() const{
    return routes_name_;
}

const std::unordered_map<std::string_view,domain::Stop*>& TransportCatalogue::GetStopsName() const{
    return stops_name_;
}

struct StopPtrCompare {
    bool operator()(domain::Stop* stop1, domain::Stop* stop2) const {
        return stop1->stop_name < stop2->stop_name;
    }
};

const RouteStat TransportCatalogue::RouteStatCalc(const domain::Route& route) const {
    RouteStat route_stat;
    if (route.stops.empty()) return route_stat;

    std::unordered_set<domain::Stop*> unique_stops;
    double geographic_length = 0;

    for (size_t i = 1; i < route.stops.size(); i++) {
        domain::Stop* prev = route.stops[i-1];
        domain::Stop* curr = route.stops[i];

        auto dist_opt = GetStopDistances(prev, curr);
        if (!dist_opt.has_value())
            dist_opt = GetStopDistances(curr, prev);
        route_stat.route_lenght += dist_opt.value();
        geographic_length += geo::ComputeDistance(prev->coordinates, curr->coordinates);

        unique_stops.insert(prev);
        unique_stops.insert(curr);
    }

    route_stat.stops_amount = route.stops.size();
    route_stat.unique_stops_amount = unique_stops.size();

    if (geographic_length > 0) {
        route_stat.curvature = route_stat.route_lenght / geographic_length;
    }

    return route_stat;
}


domain::Stop* TransportCatalogue::GetStopByIndex(int index){
    if (index > -1 && index < stop_deq_.size())
        return &stop_deq_[index];
    else
        return nullptr;
}
