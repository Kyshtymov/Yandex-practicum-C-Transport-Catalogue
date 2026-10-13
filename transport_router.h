#pragma once
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"
#include "domain.h"
#include <vector>
#include <optional>


class TransportRouter
{
public:
    TransportRouter(const TransportCatalogue& db);

    std::optional<std::vector<graph::Edge<Weight>>> GetNearestRoute(domain::Stop* stop_from, domain::Stop* stop_to);

private:
    void InitializeGraph(const TransportCatalogue& db);
    std::optional<graph::Router<Weight>> router_;
    graph::DirectedWeightedGraph<Weight> graph_;
};

