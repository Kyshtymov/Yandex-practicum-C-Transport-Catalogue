#pragma once
#include <string>
#include <vector>
#include <set>
#include "geo.h"

using Weight = double;

namespace domain {

struct Stop;

struct Route {

    std::string route_name;
    std::vector<Stop*> stops;
    std::vector<Weight> distances_between_stops;
    bool roundtrip;
    bool operator==(const Route& other) const {
        return route_name == other.route_name;
    }
    bool operator!=(const Route& other) const {
        return route_name != other.route_name;
    }

    inline static Weight bus_velocity = 0;
};

struct RoutePtrCompare {
    bool operator()(Route* route1, Route* route2) const {
        return route1->route_name < route2->route_name;
    }
};

struct DistancePtrHasher {
    size_t operator()(const std::pair<Stop*, Stop*>& p) const {
        const auto hash1 = std::hash<const void*>{}(p.first);
        const auto hash2 = std::hash<const void*>{}(p.second);
        return hash1 * 19 + hash2;
    }
};

struct Stop {

    std::string stop_name;
    geo::Coordinates coordinates;
    std::set<Route*, RoutePtrCompare> routes;
    int stop_index;

    bool operator==(const Stop& other) const {
        return stop_name == other.stop_name;
    }

    bool operator!=(const Stop& other) const {
        return stop_name != other.stop_name;
    }

    inline static int bus_wait_time = 0;
};


struct StopPtrCompare {
    bool operator()(Stop* stop1, Stop* stop2) const {
        return stop1->stop_name < stop2->stop_name;
    }
};


struct RouteStat{
    size_t stops_amount = 0;
    size_t unique_stops_amount = 0;
    int route_lenght = 0;
    double curvature = 0;
};

struct Item{
    std::string_view stop_name;
    std::string_view route_name;
    Weight time;
    int span;
    bool wait;
};

}
