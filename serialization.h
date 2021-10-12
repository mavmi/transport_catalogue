#pragma once

#include <filesystem>
#include <fstream>  
#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <variant>

#include "transport_catalogue.pb.h"
#include "map_renderer.pb.h"
#include "svg.pb.h"
#include "transport_router.pb.h"
#include "graph.pb.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "svg.h"
#include "domain.h"

using Path = std::filesystem::path;

struct SerializationSettings{
	Path file_name;
};

class Serialization{
public:
	Serialization() = default;

	void Serialize(TransportCatalogue& transport_catalogue, MapRenderer& map_renderer, TransportRouter& transport_router);

	void Deserialize(TransportCatalogue& transport_catalogue, MapRenderer& map_renderer, TransportRouter& transport_router);

	void SetSerializationSettings(SerializationSettings serialization_settings);

private:
	SerializationSettings serialization_settings_;

	transport_proto::TransportCatalogue ConvertTransportCatalogue_(TransportCatalogue& transport_catalogue);

	std::string GetStopNameById_(transport_proto::TransportCatalogue& converted_catalogue, int id);
	TransportCatalogue ConvertProtoTransportCatalogue_(transport_proto::TransportCatalogue& converted_catalogue);



	transport_proto::Color ConvertColor_(svg::Color color);
	transport_proto::RenderSettings ConvertRenderSettings_(RenderSettings render_settings);

	svg::Color ConvertProtoColor_(transport_proto::Color converted_color);
	RenderSettings ConvertProtoRenderSettings_(transport_proto::RenderSettings converted_settings);



	transport_proto::RouteSettings ConvertRouteSettings_(RouteSettings& route_settings);
	transport_proto::Graph ConvertGraph_(TransportRouter& transport_router);
	transport_proto::Router ConvertRouter_(TransportRouter& transport_router);

	void ConvertProtoRouteSettings_(RouteSettings& route_settings, 
		transport_proto::RouteSettings converted_settings);
	void ConvertProtoGraph_(transport_proto::Graph& converted_graph, 
		std::shared_ptr<graph::DirectedWeightedGraph<double>> graph);
	void ConvertProtoRouter_(transport_proto::Router& converted_router,
		std::shared_ptr<graph::Router<double>> router);
};