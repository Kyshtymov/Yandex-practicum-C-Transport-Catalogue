#pragma once
#include <unordered_map>
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include "domain.h"
#include <optional>

using RouteStat = domain::RouteStat;

class TransportCatalogue {
public:

    void AddRoute(domain::Route route);
    void AddStop(domain::Stop stop);
    const domain::Route* FindRoute(std::string_view route_name) const;
    const domain::Stop* FindStop(std::string_view stop_name) const;
    domain::Stop* FindStop(std::string_view stop_name);
    const RouteStat RouteStatCalc(const domain::Route& route) const;
    std::optional<Weight> GetStopDistances(domain::Stop* stop1, domain::Stop* stop2) const;
    void SetStopDistances(domain::Stop* stop1, domain::Stop* stop2, int distance);
    const std::unordered_map<std::string_view, domain::Route*>& GetRoutesName() const;
    const std::unordered_map<std::string_view, domain::Stop*>& GetStopsName() const;
    domain::Stop* GetStopByIndex(int index);
private:
    std::deque<domain::Stop> stop_deq_;
    std::deque<domain::Route> route_deq_;
    std::unordered_map<std::string_view, domain::Stop*> stops_name_;
    std::unordered_map<std::string_view, domain::Route*> routes_name_;
    std::unordered_map<std::pair<domain::Stop*, domain::Stop*>, Weight, domain::DistancePtrHasher> stop_distances_;
};
