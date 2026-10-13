#pragma once
#include "transport_catalogue.h"
#include "json.h"
#include "request_handler.h"

class JSONReader{
public:

    struct DistancePairHasher {
        size_t operator()(const std::pair<std::string, std::string>& p) const {
            const auto hash1 = std::hash<std::string>{}(p.first);
            const auto hash2 = std::hash<std::string>{}(p.second);
            return hash1 * 13 + hash2;
        }
    };


    JSONReader(RequestHandler& request_handler);
    json::Document LoadJSON(const std::string&);
    json::Document LoadJSON(std::istream& input);
    json::Node RequestResponse(const json::Node& node);
    svg::Color IdentifyColorType(const json::Node& color_node);
    void WriteSvgSettings(const json::Node& node);
    void WriteStopsToCatalogue(const json::Node& node, std::unordered_map<std::pair<std::string, std::string>, Weight, DistancePairHasher>& stop_distances);
    void WriteRoutesToCatalogue(const json::Node& node);
    json::Document ReadAndProcessRequests(const json::Document& doc);
private:
    RequestHandler& request_handler_;
};

