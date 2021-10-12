#pragma once

#include <string>
#include <iostream>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <memory>
#include <algorithm>
#include <deque>

#include "transport_catalogue.h"
#include "json_builder.h"
#include "router.h"

struct RouteSettings {
	int bus_wait_time; // мин
	double bus_velocity; // м/мин
};

struct Route {
	Route() = default;

	Route(std::shared_ptr<Stop> stop_from, std::shared_ptr<Stop> stop_to, double waiting_time,
		const std::string& bus_name, int span_count)
		: stop_from(stop_from), stop_to(stop_to), waiting_time(waiting_time),
		bus_name(bus_name), span_count(span_count) {

	}

	std::shared_ptr<Stop> stop_from;
	std::shared_ptr<Stop> stop_to;
	double waiting_time;
	const std::string& bus_name;
	int span_count;
};


class TransportRouter {
public:
	TransportRouter(TransportCatalogue& transoprt_catalogue);

	void SetRouteSettings(const RouteSettings& route_settings);

	// Обработка запроса
	void ParseQuery(const std::string& stop_from, const std::string& stop_to, int request_id, json::Builder& json_builder);

	std::deque<std::shared_ptr<Route>> SetHash();

	// Создание графа на основе всевозможных путей в рамках каждого маршрута
	void SetGraph();

	RouteSettings& GetRouteSettings();

	std::shared_ptr<graph::DirectedWeightedGraph<double>> GetGraph();

	std::shared_ptr<graph::Router<double>> GetRouter();

private:
	RouteSettings route_settings_;
	std::shared_ptr<graph::DirectedWeightedGraph<double>> graph_;
	std::shared_ptr<graph::Router<double>> router_;
	TransportCatalogue& transoprt_catalogue_;
	std::map<double, std::shared_ptr<Route>> hash_to_route_;

	// Расчет времени пути между остановками с учетом времени ожидания
	double CalculateTimeWithWaiting_(const std::shared_ptr<Stop> stop_from, const std::shared_ptr<Stop> stop_to);

	// Расчет времени пути между остановками
	double CalculateTime_(const std::shared_ptr<Stop> stop_from, const std::shared_ptr<Stop> stop_to);

	// Сложение маршрутов с общими остановками
	std::shared_ptr<Route> SumRoutes_(const std::shared_ptr<Route> left, const std::shared_ptr<Route> right, const std::string& bus_name);

	// Расчет хэша структуры маршрута
	double Hash_(graph::VertexId id_from, graph::VertexId id_to, double waiting_time);

	void ExitWithEmptyResult_(int request_id, json::Builder& json_builder);
};