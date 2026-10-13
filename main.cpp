#include <iostream>
#include <sstream>
#include <string>
#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include <fstream>
#include <iomanip>
#include <sstream>

std::string Print(const json::Node& node) {
    std::ostringstream out;
    json::Print(json::Document{node}, out);
    return out.str();
}


int main() {
    TransportCatalogue catalogue;
    MapRenderer map_renderrer;
    RequestHandler request_handler(catalogue, map_renderrer);
    JSONReader json_reader(request_handler);
    auto doc = json_reader.LoadJSON(std::cin);
    auto json_str = Print(doc.GetRoot());
    std::cout << json_str << std::endl;
}

