#include "transport_router.h"

TransportRouter::TransportRouter(TransportCatalogue& transoprt_catalogue)
	: transoprt_catalogue_(transoprt_catalogue) {
		graph_ = std::make_shared<graph::DirectedWeightedGraph<double>>();
		router_ = std::make_shared<graph::Router<double>>(*graph_.get());
}

void TransportRouter::SetRouteSettings(const RouteSettings& route_settings) {
	route_settings_ = route_settings;
}

// Обработка запроса
void TransportRouter::ParseQuery(const std::string& stop_from, const std::string& stop_to, int request_id, json::Builder& json_builder) {
	using namespace std::literals;

	const auto& stops = transoprt_catalogue_.GetStops();

	if (!stops.count(stop_from) || !stops.count(stop_to)) {
		ExitWithEmptyResult_(request_id, json_builder);
		return;
	}

	graph::VertexId from = stops.at(stop_from).get()->id;
	graph::VertexId to = stops.at(stop_to).get()->id;

	auto result = router_.get()->BuildRoute(from, to);
	if (result == std::nullopt) {
		ExitWithEmptyResult_(request_id, json_builder);
		return;
	}

	json_builder.StartDict().Key("items"s).StartArray();

	const auto& id_to_stop = transoprt_catalogue_.GetStopIdToStops();
	for (int edge_num : (*result).edges) {
		const auto& edge = graph_.get()->GetEdge(edge_num);

		graph::VertexId id_from = edge.from;
		graph::VertexId id_to = edge.to;

		std::shared_ptr<Stop> stop_from = id_to_stop.at(id_from);
		std::shared_ptr<Stop> stop_to = id_to_stop.at(id_to);

		double waiting_time = edge.weight;
		double hash = Hash_(stop_from.get()->id, stop_to.get()->id, waiting_time);
		std::shared_ptr<Route> route = hash_to_route_.at(hash);

		json_builder.StartDict()
			.Key("type"s).Value("Wait"s)
			.Key("stop_name"s).Value(stop_from.get()->name)
			.Key("time"s).Value(route_settings_.bus_wait_time)
			.EndDict()

			.StartDict()
			.Key("bus"s).Value(route.get()->bus_name)
			.Key("span_count"s).Value(route.get()->span_count)
			.Key("time"s).Value(waiting_time - route_settings_.bus_wait_time)
			.Key("type"s).Value("Bus"s)
			.EndDict();

	}

	json_builder.EndArray()
		.Key("request_id"s).Value(request_id)
		.Key("total_time"s).Value((*result).weight)
		.EndDict();
}

std::deque<std::shared_ptr<Route>> TransportRouter::SetHash(){
	const std::map<std::string, std::shared_ptr<Bus>>& buses = transoprt_catalogue_.GetBuses();
	std::deque<std::shared_ptr<Route>> all_possible_ways;

	for (const auto& [bus_name, bus_ptr] : buses) {

		size_t subvector_first_pos = all_possible_ways.size();
		const std::deque<std::shared_ptr<Stop>>& stops = bus_ptr.get()->stops;

		for (int i = 0; i < static_cast<int>(stops.size()) - 1; ++i) {

			std::shared_ptr<Stop> stop_from = stops.at(i);
			std::shared_ptr<Stop> stop_to = stops.at(i + 1);

			graph::VertexId id_from = stop_from.get()->id;
			graph::VertexId id_to = stop_to.get()->id;

			double waiting_time = CalculateTimeWithWaiting_(stop_from, stop_to);
			double hash = Hash_(id_from, id_to, waiting_time);
			hash_to_route_[hash] = std::make_shared<Route>(stop_from, stop_to,
				CalculateTimeWithWaiting_(stop_from, stop_to), bus_name, 1);

			all_possible_ways.push_back(hash_to_route_.at(hash));

		}
		for (int i = 0; i < static_cast<int>(stops.size()) - 2; ++i) {
			for (int j = i + 1; j < static_cast<int>(stops.size()) - 1; ++j) {

				double hash;
				std::shared_ptr<Route> route;

				if (j == i + 1) {
					route = SumRoutes_(all_possible_ways.at(subvector_first_pos + i), all_possible_ways.at(subvector_first_pos + i + 1), bus_name);
				} else {
					route = SumRoutes_(all_possible_ways.back(), all_possible_ways.at(subvector_first_pos + j), bus_name);
				}

				hash = Hash_(route.get()->stop_from.get()->id, route.get()->stop_to.get()->id, route.get()->waiting_time);
				hash_to_route_[hash] = route;
				all_possible_ways.push_back(route);

			}
		}
	}

	return all_possible_ways;
}

// Создание графа на основе всевозможных путей в рамках каждого маршрута
void TransportRouter::SetGraph() {
	std::deque<std::shared_ptr<Route>> all_possible_ways = SetHash();

	graph_ = std::make_shared<graph::DirectedWeightedGraph<double>>(transoprt_catalogue_.GetStopsCount());
	while (!all_possible_ways.empty()) {
		std::shared_ptr<Route> route_struct = all_possible_ways.front();
		graph_.get()->AddEdge({ route_struct.get()->stop_from.get()->id, route_struct.get()->stop_to.get()->id, route_struct.get()->waiting_time });
		all_possible_ways.pop_front();
	}

	router_ = std::make_shared<graph::Router<double>>(*graph_.get());

}

RouteSettings& TransportRouter::GetRouteSettings(){
	return route_settings_;
}

std::shared_ptr<graph::DirectedWeightedGraph<double>> TransportRouter::GetGraph(){
	return graph_;
}

std::shared_ptr<graph::Router<double>> TransportRouter::GetRouter(){
	return router_;
}

// Расчет времени пути между остановками с учетом времени ожидания
double TransportRouter::CalculateTimeWithWaiting_(const std::shared_ptr<Stop> stop_from, const std::shared_ptr<Stop> stop_to) {
	return CalculateTime_(stop_from, stop_to) + route_settings_.bus_wait_time;
}

// Расчет времени пути между остановками
double TransportRouter::CalculateTime_(const std::shared_ptr<Stop> stop_from, const std::shared_ptr<Stop> stop_to) {
	return transoprt_catalogue_.GetDistanceBetweenTwoStops(stop_from.get()->name, stop_to.get()->name) / route_settings_.bus_velocity;
}

// Сложение маршрутов с общими остановками
std::shared_ptr<Route> TransportRouter::SumRoutes_(const std::shared_ptr<Route> left, const std::shared_ptr<Route> right, const std::string& bus_name) {
	return std::make_shared<Route>(left.get()->stop_from, right.get()->stop_to,
		left.get()->waiting_time + right.get()->waiting_time - route_settings_.bus_wait_time,
		bus_name, left.get()->span_count + right.get()->span_count);
}

// Расчет хэша структуры маршрута
double TransportRouter::Hash_(graph::VertexId id_from, graph::VertexId id_to, double waiting_time) {
	return 12.0 * id_from + 250.0 * id_to +
		static_cast<double>(id_from) * static_cast<double>(id_to) +
		123.0 * waiting_time + static_cast<double>(id_to) * waiting_time;
}

void TransportRouter::ExitWithEmptyResult_(int request_id, json::Builder& json_builder) {
	using namespace std::literals;

	json_builder.StartDict()
		.Key("request_id"s).Value(request_id)
		.Key("error_message"s).Value("not found"s)
		.EndDict();
}