#include <iostream>

#include "transport_catalogue.h"

using namespace std;

// vector<string>& words - это результат работы
// функции GetTypeNameAndContent.
// Функция создает указатели на новую останоку
// в мэпе остановок и на новый сет в словаре stops_to_buses_,
// если таковых не сущетсвует.
void TransportCatalogue::AddStop(const vector<string>& words) {
	string stop_name = words.at(1);

	std::shared_ptr<Stop> current_stop;
	if (!stops_.count(stop_name)) {
		current_stop = GetPointerToStopByName_(stop_name);
	} else {
		current_stop = stops_.at(stop_name);
	}
	current_stop.get()->coord_x = stod(words.at(2));
	current_stop.get()->coord_y = stod(words.at(3));

	if (!stops_to_buses_.count(stop_name)) {
		stops_to_buses_[stop_name] = make_shared<set<string>>();
	}

	SetDistancesBetweenCurrentStopAndOtherOnes_(current_stop, words);
}

// vector<string>& words - это результат работы
// функции GetTypeNameAndContent.
// is_loop - это return функции GetTypeNameAndContent.
// Функция создает указатель на новый маршрут и
// закидывает в его дек остановок указатели на
// остановки.
// Если добавляемая остановка не существует в 
// мэпе остановок, то сперва создается указатель на нее,
// и он попадает в мэпы оставнок и stops_to_buses_,
// а потом уже в дек маршрута.
void TransportCatalogue::AddBus(const vector<string>& words, bool is_loop) {
	size_t words_index = 2;
	string stop_name;
	string bus_name = words[1];
	std::shared_ptr<Bus> bus = make_shared<Bus>();

	bus.get()->name = bus_name;
	bus.get()->is_looped = is_loop;
	bus.get()->id = buses_count_++;
	for (; words_index < words.size(); ++words_index) {
		stop_name = words.at(words_index);
		if (!stops_.count(stop_name)) {
			GetPointerToStopByName_(stop_name);
		}
		stops_[stop_name].get()->buses.insert(bus_name);
		bus.get()->stops.push_back(stops_[stop_name]);

		if (!stops_to_buses_.count(stop_name)) {
			stops_to_buses_[stop_name] = make_shared<set<string>>();
		}
		stops_to_buses_[stop_name].get()->insert(bus.get()->name);
	}

	// если в запросе на обновление каталога " - " - это сепаратор
	// (GetTypeNameAndContent вернет false),
	// то нужно отдельно прописать, что автобус после конечной поедет обратно.
	if (!is_loop) {
		words_index -= 2;
		for (; words_index >= 2; --words_index) {
			stop_name = words[words_index];
			bus.get()->stops.push_back(stops_[stop_name]);
		}
	}

	buses_[bus.get()->name] = bus;
}

std::shared_ptr<Bus> TransportCatalogue::GetBus(const string& bus_name) const {
	if (!buses_.count(bus_name)) {
		return nullptr;
	}
	return buses_.at(bus_name);
}

int TransportCatalogue::GetDistanceBetweenTwoStops(const string& first_stop_name, const string& second_stop_name) {
	std::shared_ptr<Stop> first_stop = GetPointerToStopByName_(first_stop_name);
	std::shared_ptr<Stop> second_stop = GetPointerToStopByName_(second_stop_name);

	for (const auto& [left, right] : stops_pair_to_distance_) {
		if (left.first.get() == first_stop.get() && left.second.get() == second_stop.get()) {
			return right;
		}
	}
	for (const auto& [left, right] : stops_pair_to_distance_) {
		if (left.second.get() == first_stop.get() && left.first.get() == second_stop.get()) {
			return right;
		}
	}
	return 0;
}

std::shared_ptr<std::set<std::string>> TransportCatalogue::GetStopToBuses(const string& stop_name) const {
	if (!stops_to_buses_.count(stop_name)) {
		return nullptr;
	}
	return stops_to_buses_.at(stop_name);
}

size_t TransportCatalogue::GetStopsCount() {
	return stops_count_;
}

std::unordered_map<std::pair<std::shared_ptr<Stop>, std::shared_ptr<Stop>>, double, Hasher>& TransportCatalogue::GetStopsPairToDistance(){
	return stops_pair_to_distance_;
}

const std::map<std::string, std::shared_ptr<Stop>>& TransportCatalogue::GetStops() {
	return stops_;
}

const std::map<std::string, std::shared_ptr<Bus>>& TransportCatalogue::GetBuses() {
	return buses_;
}

const std::map<std::string, std::shared_ptr<std::set<std::string>>>& TransportCatalogue::GetStopsToBuses() {
	return stops_to_buses_;
}

const std::map<graph::VertexId, std::shared_ptr<Stop>>& TransportCatalogue::GetStopIdToStops() {
	return stop_id_to_stops_;
}

std::shared_ptr<Stop> TransportCatalogue::GetStopByName(const std::string& name){
	return stops_.at(name);
}

void TransportCatalogue::SetDistancesBetweenCurrentStopAndAnother(const std::string& current_stop, const std::string another_name, double distance) {
	std::shared_ptr<Stop> left = GetPointerToStopByName_(current_stop);
	std::shared_ptr<Stop> right = GetPointerToStopByName_(another_name);
	stops_pair_to_distance_[{ left, right }] = distance;
}

// Записывает расстояния между текущей остановкой и всеми другими в словарь stops_pair_to_distance_
// на основании разбитого на слова, которые лежат в векторе vector<string>& words, запроса.
void TransportCatalogue::SetDistancesBetweenCurrentStopAndOtherOnes_(std::shared_ptr<Stop> current_stop, const vector<string>& words) {
	std::shared_ptr<Stop> another_stop;
	pair<int, string> distance_and_name;

	for (size_t i = 4; i < words.size(); ++i) {
		ReadDistanceAndStopNameFromString_(distance_and_name, words.at(i));

		another_stop = GetPointerToStopByName_(distance_and_name.second);
		another_stop.get()->name = distance_and_name.second;

		if (!stops_pair_to_distance_.count({ current_stop, another_stop }) && !stops_pair_to_distance_.count({ another_stop, current_stop })) {
			stops_pair_to_distance_[{ current_stop, another_stop }] = distance_and_name.first;
		}
	}
}

// Вспомогательный метод для разбиения строки типа " 'расстояние'm to 'название остановки' " на pair<int, string>
void TransportCatalogue::ReadDistanceAndStopNameFromString_(pair<int, string>& distance_and_name, const string& line) {
	size_t line_index = 0;
	string text;

	for (;; ++line_index) {
		if (line[line_index] != 'm') {
			text += line[line_index];
		} else {
			line_index += 5;
			distance_and_name.first = stoi(text);
			text = "";
			break;
		}
	}

	for (; line_index < line.length(); ++line_index) {
		text += line[line_index];
	}
	distance_and_name.second = text;
}

// Берем указатель на остановку, если она существует,
// или создаем новый, ставим ему имя и возвращаем.
std::shared_ptr<Stop> TransportCatalogue::GetPointerToStopByName_(const string& stop_name) {
	if (!stops_.count(stop_name)) {
		stops_[stop_name] = make_shared<Stop>();
		stops_[stop_name].get()->name = stop_name;
		stops_[stop_name].get()->id = stops_count_;
		stop_id_to_stops_[stops_count_++] = stops_[stop_name];
	}
	return stops_.at(stop_name);
}