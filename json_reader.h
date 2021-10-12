#pragma once

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <utility>

#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "json.h"
#include "json_builder.h"
#include "transport_router.h"
#include "graph.h"
#include "router.h"
#include "domain.h"
#include "serialization.h"

struct TransportManagers {
	TransportManagers(TransportCatalogue& transoprt_catalogue, MapRenderer& map_renderer, TransportRouter& transport_router, Serialization& serialization)
		: transoprt_catalogue(transoprt_catalogue), map_renderer(map_renderer), transport_router(transport_router), serialization(serialization) {
	}

	TransportCatalogue& transoprt_catalogue;
	MapRenderer& map_renderer;
	TransportRouter& transport_router;
	Serialization& serialization;
};

std::string CreateAddStopQuery(const json::Dict& dict);

std::string CreateAddBusQuery(const json::Dict& dict);

void UpdateCatalog(TransportCatalogue& transoprt_catalogue, const json::Array& arr);

RenderSettings GetRenderSettings(const json::Dict& dict);

RouteSettings GetRouteSettings(const json::Dict& dict);

SerializationSettings GetSerializationSettings(const json::Dict& dict);

void PrintCatatlog(TransportManagers& transport_managers, const json::Array& requests_array, std::ostream& output_stream);

void ReadJSON(TransportManagers& transport_managers, std::istream& input_stream = std::cin, std::ostream& output_stream = std::cout);