#include "serialization.h"

void Serialization::Serialize(TransportCatalogue& transport_catalogue, MapRenderer& map_renderer, TransportRouter& transport_router){
	std::ofstream out(serialization_settings_.file_name, std::ios::binary);
	if (!out)
		return;

	transport_proto::Container container;

	*(container.mutable_transport_catalogue()) = ConvertTransportCatalogue_(transport_catalogue);
	*(container.mutable_render_settings()) = ConvertRenderSettings_(map_renderer.GetRenderSettings());
	*(container.mutable_route_settings()) = ConvertRouteSettings_(transport_router.GetRouteSettings());
	*(container.mutable_router()) = ConvertRouter_(transport_router);
	*(container.mutable_graph()) = ConvertGraph_(transport_router);

	container.SerializeToOstream(&out);
}

void Serialization::Deserialize(TransportCatalogue& transport_catalogue, MapRenderer& map_renderer, TransportRouter& transport_router){
	std::ifstream in(serialization_settings_.file_name, std::ios::binary);
	if (!in)
		return;

	transport_proto::Container container;
	if (!container.ParseFromIstream(&in))
		return;

	transport_catalogue = ConvertProtoTransportCatalogue_(*container.mutable_transport_catalogue());
	map_renderer.SetRenderSettings(ConvertProtoRenderSettings_(*container.mutable_render_settings()));
	ConvertProtoRouteSettings_(transport_router.GetRouteSettings(), *container.mutable_route_settings());
	ConvertProtoGraph_(*container.mutable_graph(), transport_router.GetGraph());
	ConvertProtoRouter_(*container.mutable_router(), transport_router.GetRouter());
}

void Serialization::SetSerializationSettings(SerializationSettings serialization_settings){
	serialization_settings_ = serialization_settings;
}

transport_proto::TransportCatalogue Serialization::ConvertTransportCatalogue_(TransportCatalogue& transport_catalogue){
	transport_proto::TransportCatalogue converted_catalogue;

	for (const auto& [stop_name, original_stop] : transport_catalogue.GetStops()){
		converted_catalogue.add_stops_list();            
	}
	for (const auto& [stop_name, original_stop] : transport_catalogue.GetStops()){
		transport_proto::Stop* new_converted_stop = converted_catalogue.mutable_stops_list(original_stop.get()->id);            
		
		new_converted_stop->set_name(original_stop.get()->name);
		new_converted_stop->set_coord_x(original_stop.get()->coord_x);
		new_converted_stop->set_coord_y(original_stop.get()->coord_y);
		new_converted_stop->set_id(original_stop.get()->id);
	}

	for (const auto& [bus_name, original_bus] : transport_catalogue.GetBuses()){
		transport_proto::Bus* new_converted_bus = converted_catalogue.add_buses_list();

		new_converted_bus->set_is_looped(original_bus.get()->is_looped);
		new_converted_bus->set_name(original_bus.get()->name);
		new_converted_bus->set_id(original_bus.get()->id);

		size_t count = (original_bus.get()->is_looped) ? original_bus.get()->stops.size() : original_bus.get()->stops.size() / 2 + 1; 
		for (size_t i = 0; i< count; ++i){
			new_converted_bus->add_stops(original_bus.get()->stops.at(i).get()->id);
		}
	}

	for (auto& [pair, distance] : transport_catalogue.GetStopsPairToDistance()){
		transport_proto::Pair* new_converted_pair = converted_catalogue.add_distances();

		new_converted_pair->set_id_from(pair.first.get()->id);
		new_converted_pair->set_id_to(pair.second.get()->id);
		new_converted_pair->set_distance(distance);
	}

	return converted_catalogue;
}

std::string Serialization::GetStopNameById_(transport_proto::TransportCatalogue& converted_catalogue, int id){
	for (int i = 0; i < converted_catalogue.stops_list_size(); ++i){
		const transport_proto::Stop& converted_stop = converted_catalogue.stops_list(i);
		if (converted_stop.id() == id)
			return converted_stop.name();
	}
	return "";
}

TransportCatalogue Serialization::ConvertProtoTransportCatalogue_(transport_proto::TransportCatalogue& converted_catalogue){
	TransportCatalogue transport_catalogue;

	for (int i = 0; i < converted_catalogue.stops_list_size(); ++i){
		const transport_proto::Stop& converted_stop = converted_catalogue.stops_list(i);
		transport_catalogue.AddStop({"Stop", converted_stop.name(),
			DoubleToString(converted_stop.coord_x()), 
			DoubleToString(converted_stop.coord_y())});
	}

	for (int i = 0; i < converted_catalogue.buses_list_size(); ++i){
		const transport_proto::Bus& converted_bus = converted_catalogue.buses_list(i);

		std::vector<std::string> stops;
		stops.push_back("Bus");
		stops.push_back(converted_bus.name());
		for (int j=0; j<converted_bus.stops_size(); ++j){
			stops.push_back(GetStopNameById_(converted_catalogue, converted_bus.stops(j)));
		}
		transport_catalogue.AddBus(stops, converted_bus.is_looped());
	}

	for (int i = 0; i < converted_catalogue.distances_size(); ++i){
		transport_catalogue.SetDistancesBetweenCurrentStopAndAnother(
			GetStopNameById_(converted_catalogue, converted_catalogue.distances(i).id_from()),
			GetStopNameById_(converted_catalogue, converted_catalogue.distances(i).id_to()),
			converted_catalogue.distances(i).distance()
		);
		
	}

	return transport_catalogue;
}



transport_proto::Color Serialization::ConvertColor_(svg::Color color){
	transport_proto::Color converted_color;

	if (std::holds_alternative<svg::Rgb>(color)){
		converted_color.set_index(1);
		converted_color.mutable_rgb()->set_red(std::get<svg::Rgb>(color).red);
		converted_color.mutable_rgb()->set_green(std::get<svg::Rgb>(color).green);
		converted_color.mutable_rgb()->set_blue(std::get<svg::Rgb>(color).blue);
	} else if (std::holds_alternative<svg::Rgba>(color)){
		converted_color.set_index(2);
		converted_color.mutable_rgba()->set_red(std::get<svg::Rgba>(color).red);
		converted_color.mutable_rgba()->set_green(std::get<svg::Rgba>(color).green);
		converted_color.mutable_rgba()->set_blue(std::get<svg::Rgba>(color).blue);
		converted_color.mutable_rgba()->set_opacity(std::get<svg::Rgba>(color).opacity);
	} else if (std::holds_alternative<std::string>(color)){
		converted_color.set_index(3);
		converted_color.set_text(std::get<std::string>(color));
	} else {
		converted_color.set_index(0);
	}

	return converted_color;
}

transport_proto::RenderSettings Serialization::ConvertRenderSettings_(RenderSettings render_settings){
	transport_proto::RenderSettings converted_settings;

	converted_settings.set_width(render_settings.width);
	converted_settings.set_height(render_settings.height);
	converted_settings.set_padding(render_settings.padding);
	converted_settings.set_line_width(render_settings.line_width);
	converted_settings.set_stop_radius(render_settings.stop_radius);
	converted_settings.set_bus_label_font_size(render_settings.bus_label_font_size);
	converted_settings.add_bus_label_offset(render_settings.bus_label_offset[0]);
	converted_settings.add_bus_label_offset(render_settings.bus_label_offset[1]);
	converted_settings.set_stop_label_font_size(render_settings.stop_label_font_size);
	converted_settings.add_stop_label_offset(render_settings.stop_label_offset[0]);
	converted_settings.add_stop_label_offset(render_settings.stop_label_offset[1]);
	converted_settings.set_underlayer_width(render_settings.underlayer_width);

	*(converted_settings.mutable_underlayer_color()) = ConvertColor_(render_settings.underlayer_color);

	for (size_t i = 0; i < render_settings.color_palette.size(); ++i){
		*(converted_settings.add_color_palette()) = ConvertColor_(render_settings.color_palette.at(i));
	}

	return converted_settings;
}

svg::Color Serialization::ConvertProtoColor_(transport_proto::Color converted_color){
	svg::Color color;

	if (converted_color.index() == 1){

		svg::Rgb rgb;
		rgb.red = static_cast<uint8_t>(converted_color.rgb().red());
		rgb.green = converted_color.rgb().green();
		rgb.blue = converted_color.rgb().blue();
		color = rgb;

	} else if (converted_color.index() == 2){
		
		svg::Rgba rgba;
		rgba.red = converted_color.rgba().red();
		rgba.green = converted_color.rgba().green();
		rgba.blue = converted_color.rgba().blue();
		rgba.opacity = converted_color.rgba().opacity();
		color = rgba;

	} else if (converted_color.index() == 3){
		
		color = converted_color.text();

	}

	return color;
}

RenderSettings Serialization::ConvertProtoRenderSettings_(transport_proto::RenderSettings converted_settings){
	RenderSettings render_settings;

	render_settings.width = converted_settings.width();
	render_settings.height = converted_settings.height();
	render_settings.padding = converted_settings.padding();
	render_settings.line_width = converted_settings.line_width();
	render_settings.stop_radius = converted_settings.stop_radius();
	render_settings.bus_label_font_size = converted_settings.bus_label_font_size();
	render_settings.bus_label_offset[0] = converted_settings.bus_label_offset(0);
	render_settings.bus_label_offset[1] = converted_settings.bus_label_offset(1);
	render_settings.stop_label_font_size = converted_settings.stop_label_font_size();
	render_settings.stop_label_offset[0] = converted_settings.stop_label_offset(0);
	render_settings.stop_label_offset[1] = converted_settings.stop_label_offset(1);
	render_settings.underlayer_width = converted_settings.underlayer_width();
	
	render_settings.underlayer_color = ConvertProtoColor_(converted_settings.underlayer_color());
	
	render_settings.color_palette.clear();
	for (int i = 0; i < converted_settings.color_palette_size(); ++i){
		render_settings.color_palette.push_back(ConvertProtoColor_(converted_settings.color_palette(i)));
	}

	return render_settings;
}



transport_proto::RouteSettings Serialization::ConvertRouteSettings_(RouteSettings& route_settings){
	transport_proto::RouteSettings converted_settings;

	converted_settings.set_bus_wait_time(route_settings.bus_wait_time);
	converted_settings.set_bus_velocity(route_settings.bus_velocity);

	return converted_settings;
}

transport_proto::Graph Serialization::ConvertGraph_(TransportRouter& transport_router){
	transport_proto::Graph converted_graph;

	std::shared_ptr<graph::DirectedWeightedGraph<double>> orig_graph = transport_router.GetGraph();

	auto& edges = orig_graph.get()->GetEdges();
	for (size_t i = 0; i < edges.size(); ++i){
		transport_proto::Edge* new_edge = converted_graph.add_edges();
	
		new_edge->set_from(edges.at(i).from);
		new_edge->set_to(edges.at(i).to);
		new_edge->set_weight(edges.at(i).weight);
	}

	auto& incidence_lists = orig_graph.get()->GetIncidenceLists();
	for (size_t i = 0; i < incidence_lists.size(); ++i){
		transport_proto::IncidenceList* new_list = converted_graph.add_incidence_lists();
		for (size_t j = 0; j < incidence_lists.at(i).size(); ++j){
			new_list->add_incidence_list(incidence_lists.at(i).at(j));
		}
	}

	return converted_graph;
}

transport_proto::Router Serialization::ConvertRouter_(TransportRouter& transport_router){
	transport_proto::Router converted_router;

	std::shared_ptr<graph::Router<double>> orig_router = transport_router.GetRouter();
	auto& routes_internal_data = orig_router.get()->GetInternalData();
	for (size_t i = 0; i < routes_internal_data.size(); ++i){
		transport_proto::RouteInternalDataArr* new_array = converted_router.add_data();
		for (size_t j = 0; j < routes_internal_data.at(i).size(); ++j){
			transport_proto::RouteInternalData* new_struct = new_array->add_data();

			if (routes_internal_data.at(i).at(j).has_value()){
				new_struct->set_has_value(1);
				new_struct->set_weight(routes_internal_data.at(i).at(j).value().weight);
				if (routes_internal_data.at(i).at(j).value().prev_edge.has_value()){
					new_struct->mutable_edge_id()->set_has_value(1);
					new_struct->mutable_edge_id()->set_edge_id(routes_internal_data.at(i).at(j).value().prev_edge.value());
				} else {
					new_struct->mutable_edge_id()->set_has_value(0);
				}
			} else {
				new_struct->set_has_value(0);
			}
		}
	}

	return converted_router;
}

void Serialization::ConvertProtoRouteSettings_(RouteSettings& route_settings, 
	transport_proto::RouteSettings converted_settings){
	route_settings.bus_wait_time = converted_settings.bus_wait_time();
	route_settings.bus_velocity = converted_settings.bus_velocity();
}

void Serialization::ConvertProtoGraph_(transport_proto::Graph& converted_graph, 
	std::shared_ptr<graph::DirectedWeightedGraph<double>> graph){
	
	auto& edges = graph.get()->GetEdges();
	for (int i = 0; i < converted_graph.edges_size(); ++i){
		edges.push_back({
			converted_graph.edges(i).from(),
			converted_graph.edges(i).to(),
			converted_graph.edges(i).weight()
		});
	}

	auto& incidence_lists = graph.get()->GetIncidenceLists();
	for (int i = 0; i < converted_graph.incidence_lists_size(); ++i){
		std::vector<graph::EdgeId> new_list;
		for (int j = 0; j < converted_graph.incidence_lists(i).incidence_list_size(); ++j){
			new_list.push_back(converted_graph.incidence_lists(i).incidence_list(j));
		}
		incidence_lists.push_back(new_list);
	}
}

void Serialization::ConvertProtoRouter_(transport_proto::Router& converted_router,
	std::shared_ptr<graph::Router<double>> router){
	auto& routes_internal_data = router.get()->GetInternalData();

	for (int i = 0; i < converted_router.data_size(); ++i){
		std::vector<std::optional<graph::Router<double>::RouteInternalData>> new_arr;

		for (int j = 0; j < converted_router.data(i).data_size(); ++j){
			
			if (converted_router.data(i).data(j).has_value()){
				graph::Router<double>::RouteInternalData str;
				if (converted_router.data(i).data(j).edge_id().has_value()){
					str = {
						converted_router.data(i).data(j).weight(),
						converted_router.data(i).data(j).edge_id().edge_id()
					};
				} else {
					str = {
						converted_router.data(i).data(j).weight(),
						std::nullopt
					};
				}
				new_arr.push_back(str);
			} else {
				new_arr.push_back(std::nullopt);
			}

		}

		routes_internal_data.push_back(new_arr);
	}
}
