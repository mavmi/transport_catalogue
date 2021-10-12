#pragma once

#include <memory>
#include <utility>
#include <string>
#include <deque>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <iomanip>

#include "domain.h"

struct Hasher {
	size_t operator()(const std::pair<std::shared_ptr<Stop>, std::shared_ptr<Stop>>& stops) const {
		std::hash<std::string> hasher;
		size_t result = 0;

		result += hasher(stops.first.get()->name) + 12 * hasher(stops.second.get()->name);
		result += stops.first.get()->coord_x + 24 * stops.second.get()->coord_x;
		result += stops.first.get()->coord_y + 48 * stops.second.get()->coord_y;

		return result;
	}
};

class TransportCatalogue {
public:
	TransportCatalogue() = default;

	void AddStop(const std::vector<std::string>& words);

	void AddBus(const std::vector<std::string>& words, bool is_loop);

	std::shared_ptr<Bus> GetBus(const std::string& bus_name) const;

	int GetDistanceBetweenTwoStops(const std::string& first_stop_name, const std::string& second_stop_name);

	std::shared_ptr<std::set<std::string>> GetStopToBuses(const std::string& stop_name) const;

	const std::map<std::string, std::shared_ptr<Stop>>& GetStops();

	const std::map<std::string, std::shared_ptr<Bus>>& GetBuses();

	const std::map<std::string, std::shared_ptr<std::set<std::string>>>& GetStopsToBuses();

	const std::map<graph::VertexId, std::shared_ptr<Stop>>& GetStopIdToStops();

	size_t GetStopsCount();

	std::unordered_map<std::pair<std::shared_ptr<Stop>, std::shared_ptr<Stop>>, double, Hasher>& GetStopsPairToDistance();

	std::shared_ptr<Stop> GetStopByName(const std::string& name);

	void SetDistancesBetweenCurrentStopAndAnother(const std::string& current_stop, const std::string another_name, double distance);

private:
	graph::VertexId stops_count_ = 0;
	graph::VertexId buses_count_ = 0;

	// stops_[название остановки] = указатель на остановку
	std::map<std::string, std::shared_ptr<Stop>> stops_;

	// buses_[название маршрута] = указатель на маршрут
	std::map<std::string, std::shared_ptr<Bus>> buses_;

	// stops_to_buses_[название остановки] = список названий маршрутов
	std::map<std::string, std::shared_ptr<std::set<std::string>>> stops_to_buses_;

	std::map<graph::VertexId, std::shared_ptr<Stop>> stop_id_to_stops_;

	// stops_pair_to_distance_[две остановки] = расстояние между ними
	std::unordered_map<std::pair<std::shared_ptr<Stop>, std::shared_ptr<Stop>>, double, Hasher> stops_pair_to_distance_;

	void SetDistancesBetweenCurrentStopAndOtherOnes_(std::shared_ptr<Stop> current_stop, const std::vector<std::string>& words);

	void ReadDistanceAndStopNameFromString_(std::pair<int, std::string>& distance_and_name, const std::string& line);

	std::shared_ptr<Stop> GetPointerToStopByName_(const std::string& stop_name);
};