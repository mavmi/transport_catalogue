#include "json_reader.h"

// Переводит информацию об остановке в JSON документе
// в строку
std::string CreateAddStopQuery(const json::Dict& dict) {
	using namespace std::literals;

	std::string result_query = StopRequest + " "s + dict.at("name"s).AsString() + ": "s +
		DoubleToString(dict.at("latitude"s).AsDouble()) + ",_"s +
		DoubleToString(dict.at("longitude"s).AsDouble());

	// Информации о расстоянии до ближайших
	// остановок может не быть
	try {
		json::Dict distances = dict.at("road_distances"s).AsDict();

		for (const auto& [stop_name, distance] : distances) {
			result_query += ",_"s + DoubleToString(distance.AsDouble()) + "m to "s + stop_name;
		}
	} catch (std::out_of_range&) {}

	return result_query;
}

// Переводит информацию об маршруте в JSON документе
// в строку
std::string CreateAddBusQuery(const json::Dict& dict) {
	using namespace std::literals;

	std::string is_roundtrip_separator = " > "s;
	std::string not_roundtrip_separator = " - "s;

	std::string result_query = BusRequest + " "s + dict.at("name"s).AsString() + ": "s;
	bool is_roundtrip = dict.at("is_roundtrip"s).AsBool();

	json::Array bus_stops = dict.at("stops"s).AsArray();
	for (size_t i = 0; i < bus_stops.size(); ++i) {
		result_query += bus_stops.at(i).AsString();
		// Чтоб не ставить сепаратор после последнего элемента
		if (i != bus_stops.size() - 1) {
			result_query += (is_roundtrip) ? is_roundtrip_separator : not_roundtrip_separator;
		}
	}

	return result_query;
}

// Переводит запросы на обновление транспортного каталога из JSON в 
// std::istringstream и обновляет каталог
void UpdateCatalog(TransportCatalogue& transoprt_catalogue, const json::Array& arr) {
	for (const json::Node& node : arr) {
		const json::Dict dict = node.AsDict();
		if (dict.at("type").AsString() == BusRequest) {
			std::istringstream strm(CreateAddBusQuery(dict));
			ParseInputQuery(transoprt_catalogue, strm);
		} else if (dict.at("type").AsString() == StopRequest) {
			std::istringstream strm(CreateAddStopQuery(dict));
			ParseInputQuery(transoprt_catalogue, strm);
		}
	}
}

RenderSettings GetRenderSettings(const json::Dict& dict) {
	using namespace std::literals;

	RenderSettings render_settings;

	render_settings.width = dict.at("width"s).AsDouble();
	render_settings.height = dict.at("height"s).AsDouble();
	render_settings.padding = dict.at("padding"s).AsDouble();
	render_settings.line_width = dict.at("line_width"s).AsDouble();
	render_settings.stop_radius = dict.at("stop_radius"s).AsDouble();
	render_settings.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();

	render_settings.bus_label_offset[0] = dict.at("bus_label_offset"s).AsArray().at(0).AsDouble();
	render_settings.bus_label_offset[1] = dict.at("bus_label_offset"s).AsArray().at(1).AsDouble();

	render_settings.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();

	render_settings.stop_label_offset[0] = dict.at("stop_label_offset"s).AsArray().at(0).AsDouble();
	render_settings.stop_label_offset[1] = dict.at("stop_label_offset"s).AsArray().at(1).AsDouble();

	json::Node color_node = dict.at("underlayer_color"s);
	if (color_node.IsString()) {
		render_settings.underlayer_color = color_node.AsString();
	} else if (color_node.IsArray()) {
		json::Array color_array = color_node.AsArray();
		if (color_array.size() == 3) {
			render_settings.underlayer_color = svg::Rgb(color_array.at(0).AsInt(), color_array.at(1).AsInt(), color_array.at(2).AsInt());
		} else if (color_array.size() == 4) {
			render_settings.underlayer_color = svg::Rgba(color_array.at(0).AsInt(), color_array.at(1).AsInt(), color_array.at(2).AsInt(), color_array.at(3).AsDouble());
		}
	}

	render_settings.underlayer_width = dict.at("underlayer_width"s).AsDouble();

	json::Array colors = dict.at("color_palette"s).AsArray();
	render_settings.color_palette.clear();
	for (size_t i = 0; i < colors.size(); ++i) {
		json::Node color = colors.at(i);
		if (color.IsString()) {
			render_settings.color_palette.push_back(color.AsString());
			continue;
		}
		json::Array color_array = color.AsArray();
		if (color_array.size() == 3) {
			render_settings.color_palette.push_back(svg::Rgb(color_array.at(0).AsInt(), color_array.at(1).AsInt(), color_array.at(2).AsInt()));
		} else if (color_array.size() == 4) {
			render_settings.color_palette.push_back(svg::Rgba(color_array.at(0).AsInt(), color_array.at(1).AsInt(), color_array.at(2).AsInt(), color_array.at(3).AsDouble()));
		}
	}

	return render_settings;
}

RouteSettings GetRouteSettings(const json::Dict& dict) {
	using namespace std::literals;

	RouteSettings route_settings;
	route_settings.bus_wait_time = dict.at("bus_wait_time"s).AsInt();
	route_settings.bus_velocity = dict.at("bus_velocity"s).AsDouble() * 50.0 / 3.0; // перевод из км/ч в м/мин

	return route_settings;
}

SerializationSettings GetSerializationSettings(const json::Dict& dict){
	SerializationSettings serialization_settings;

	serialization_settings.file_name = Path(dict.at("file").AsString());

	return serialization_settings;
}

std::string FindStopName(TransportCatalogue& transoprt_catalogue, size_t index) {
	auto& stops = transoprt_catalogue.GetStops();

	for (const auto stop : stops) {
		if (stop.second.get()->id == index) {
			return stop.second.get()->name;
		}
	}
	return "";
}

void PrintCatatlog(TransportManagers& transport_managers, const json::Array& requests_array, std::ostream& output_stream) {
	using namespace std::literals;

	transport_managers.transport_router.SetHash();

	json::Builder json_builder;
	json_builder.StartArray();
	for (size_t i = 0; i < requests_array.size(); ++i) {
		const json::Dict current_request = requests_array.at(i).AsDict();

		int request_id = current_request.at("id"s).AsInt();
		std::string type = current_request.at("type"s).AsString();

		if (type == MapRequest) {  // запрос на рисовку карты
			std::ostringstream tmp_output_stream;
			transport_managers.map_renderer.RenderMapAsString(tmp_output_stream);
			json_builder.StartDict().Key("request_id").Value(request_id)
				.Key("map"s).Value(tmp_output_stream.str()).EndDict();
		} else if (type == BusRequest || type == StopRequest) {  //запрос на информацию об остановке или маршруте
			std::string name = current_request.at("name"s).AsString();
			std::vector<std::pair<std::string, json::Node>> tmp_vector;
			// Создаем запрос к транспортному каталогу в виде потока
			std::istringstream query_input_str_stream(type + " "s + name);
			// Просим у транспортного каталога записать информацию о запросе в вектор tmp_vector
			ParseOutputQuery(transport_managers.transoprt_catalogue, tmp_vector, query_input_str_stream);

			json_builder.StartDict()
				.Key("request_id"s).Value(request_id);
			if (tmp_vector.size() == 0) { // Когда нет автобусов для текущей остановки
				json_builder.Key("buses"s).StartArray().EndArray();
			} else if (tmp_vector.front().first == "error_message") { // Если нет информации об остановке или маршруте
				json_builder.Key("error_message"s).Value("not found"s);
			} else if (type == BusRequest) { // Вывод информации о маршруте
				json_builder
					.Key(std::move(tmp_vector.at(0).first)).Value(tmp_vector.at(0).second.AsDouble())
					.Key(std::move(tmp_vector.at(1).first)).Value(tmp_vector.at(1).second.AsDouble())
					.Key(std::move(tmp_vector.at(2).first)).Value(tmp_vector.at(2).second.AsInt())
					.Key(std::move(tmp_vector.at(3).first)).Value(tmp_vector.at(3).second.AsInt());
			} else if (type == StopRequest) { // Вывод информации об остановке
				json_builder.Key("buses").StartArray();
				for (size_t i = 0; i < tmp_vector.size(); ++i) {
					json_builder.Value(tmp_vector.at(i).second.AsString());
				}
				json_builder.EndArray();
			}
			json_builder.EndDict();
		} else if (type == RouteRequest) { // запрос на построение маршрута
			const auto& stops = transport_managers.transoprt_catalogue.GetStops();
			std::string stop_from = current_request.at("from"s).AsString();
			std::string stop_to = current_request.at("to"s).AsString();
			transport_managers.transport_router.ParseQuery(stop_from, stop_to, request_id, json_builder);
		}
	}

	json::Document result_document(json_builder.EndArray().Build());
	json::Print(result_document, output_stream);
}

void ReadJSON(TransportManagers& transport_managers, std::istream& input_stream, std::ostream& output_stream) {
	using namespace std::literals;

	json::Document input_document = json::Load(input_stream);
	json::Dict dict = input_document.GetRoot().AsDict();

	if (dict.count("base_requests"))
		UpdateCatalog(transport_managers.transoprt_catalogue, dict.at("base_requests"s).AsArray());
	if (dict.count("render_settings"))
		transport_managers.map_renderer.SetRenderSettings(GetRenderSettings(dict.at("render_settings"s).AsDict()));
	if (dict.count("routing_settings"))
		transport_managers.transport_router.SetRouteSettings(GetRouteSettings(dict.at("routing_settings"s).AsDict()));
	if (dict.count("serialization_settings"))
		transport_managers.serialization.SetSerializationSettings(GetSerializationSettings(dict.at("serialization_settings").AsDict()));

	transport_managers.transport_router.SetGraph();
}