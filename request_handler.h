#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include <optional>
#include <unordered_set>
#include "transport_router.h"

class RequestHandler {
public:
    RequestHandler(TransportCatalogue& db, MapRenderer& renderer);

    std::optional<RouteStat> GetRouteStat(const std::string_view bus_name) const;
    std::optional<std::set<std::string_view>> GetBusesByStop(std::string_view stop_name) const;
    TransportCatalogue& GetCatalogue();
    MapRenderer& GetMapRenderer();
    svg::Document RenderMap() const;
    std::optional<std::vector<domain::Item>> GetRoute(const std::string_view from, const std::string_view to);
    void MakeRouter();
private:
    TransportCatalogue& db_;
    MapRenderer& renderer_;
    std::optional<TransportRouter> transport_router_;
};
