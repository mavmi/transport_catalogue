#pragma once

#include <string>
#include <memory>
#include <deque>
#include <set>
#include <iomanip>

#include "graph.h"

extern const std::string BusRequest;
extern const std::string StopRequest;
extern const std::string MapRequest;
extern const std::string RouteRequest;

std::string DoubleToString(double value);

struct Stop {
	Stop() = default;

	std::string name;
	double coord_x = 0;
	double coord_y = 0;
	graph::VertexId id;
	std::set<std::string> buses;
};

struct Bus {
	bool is_looped;
	std::string name;
	graph::VertexId id;
	std::deque<std::shared_ptr<Stop>> stops;
};