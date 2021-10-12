#include "request_handler.h"

using namespace std;

// Подсчет количества остановок
int GetStopsCount(const shared_ptr<Bus> bus) {
	return bus.get()->stops.size();
}

// Подсчет количества уникальных остановок
int GetUniqueStop(const shared_ptr<Bus> bus) {
	int counter = 0;
	auto it = bus.get()->stops.begin();
	auto end = bus.get()->stops.end();

	for (; it != end; it++) {
		if (find(next(it), end, *it) == end) {
			counter++;
		}
	}

	return counter;
}

// Подсчет длины пути по координатам
double CalculateGeographicLength(const shared_ptr<Bus> bus) {
	double result = 0;

	for (int i = 0; i < static_cast<int>(bus.get()->stops.size()) - 1; ++i) {
		result += geo::ComputeDistance({ bus.get()->stops[i]->coord_x, bus.get()->stops[i]->coord_y }, { bus.get()->stops[i + 1]->coord_x, bus.get()->stops[i + 1]->coord_y });
	}

	return result;
}

// Подсчет фактической длины пути
double CalculateActualLength(const shared_ptr<Bus> bus, TransportCatalogue& transport_catalogue) {
	double result = 0;

	for (int i = 0; i < static_cast<int>(bus.get()->stops.size()) - 1; ++i) {
		result += transport_catalogue.GetDistanceBetweenTwoStops(bus.get()->stops[i]->name, bus.get()->stops[i + 1]->name);
	}

	return result;
}

// Подсчет кривизны пути
double CalculateCurvature(double geo_length, double actual_length) {
	return (actual_length / geo_length);
}

// Сохранение информации о маршруте в вектор bus_info_vector
void GetBusInfo(TransportCatalogue& transport_catalogue, const vector<string>& words, vector<pair<string, json::Node>>& bus_info_vector) {
	string bus_name = words.at(1);
	shared_ptr<Bus> bus = transport_catalogue.GetBus(bus_name);

	if (bus.get() == nullptr) {
		bus_info_vector.push_back({ "error_message"s, json::Node("not found") });
	} else {
		double geo_length = CalculateGeographicLength(bus);
		double actual_length = CalculateActualLength(bus, transport_catalogue);
		double curvature = CalculateCurvature(geo_length, actual_length);
		int stops_count = GetStopsCount(bus);
		int unique_stops = GetUniqueStop(bus);

		bus_info_vector.push_back({ "curvature"s, json::Node(curvature) });
		bus_info_vector.push_back({ "route_length"s, json::Node(actual_length) });
		bus_info_vector.push_back({ "stop_count"s, json::Node(stops_count) });
		bus_info_vector.push_back({ "unique_stop_count"s, json::Node(unique_stops) });
	}
}

// Сохранение информации об остановке в вектор stop_info_vector
void GetStopInfo(TransportCatalogue& transport_catalogue, const vector<string>& words, vector<pair<string, json::Node>>& stop_info_vector) {
	string stop_name = words[1];
	shared_ptr<set<string>> stop_to_buses = transport_catalogue.GetStopToBuses(stop_name);

	if (stop_to_buses.get() == nullptr) {
		stop_info_vector.push_back({ "error_message"s, json::Node("not found") });
	} else if (stop_to_buses.get()->size() == 0) {
		return;
	} else {
		for (const string& stop : *(stop_to_buses.get())) {
			stop_info_vector.push_back({ "bus"s, json::Node(stop) });
		}
	}
}

// Чтение запроса на вывод, разбиение его по словам и обработка
void ParseOutputQuery(TransportCatalogue& transport_catalogue, vector<pair<string, json::Node>>& vector_to_store_output_info, istream& input_stream) {
	size_t query_index = 0;
	string word;
	string query;
	vector<string> words;

	getline(input_stream, query);

	for (;; ++query_index) {
		if (query[query_index] != ' ') {
			word += query[query_index];
		} else {
			query_index++;
			break;
		}
	}
	words.push_back(word);

	word = "";
	for (; query_index < query.size(); ++query_index) {
		word += query[query_index];
	}
	words.push_back(word);

	if (words.front() == BusRequest) {
		GetBusInfo(transport_catalogue, words, vector_to_store_output_info);
	} else if (words.front() == StopRequest) {
		GetStopInfo(transport_catalogue, words, vector_to_store_output_info);
	}
}

// Функция разбивает пришедший на вход запрос в виде строки
// на вектор слов.
// Фукнция вернет false при условии, что " - " - это
// сепаратор между словами. В таком случае маршрут
// нужно зациклить, прописав ручками движение
// автобуса от последней остановки к начальной
// (это делает метод AddBus класса транспортного каталога).
bool GetTypeNameAndContent(const string& query, vector<string>& words) {
	string word;
	size_t query_index = 0;
	bool is_loop = true;

	// записываем тип запроса (Bus, Stop)
	for (;; ++query_index) {
		if (query[query_index] != ' ') {
			word += query[query_index];
		} else {
			query_index++;
			break;
		}
	}
	words.push_back(word);

	// записываем название
	word = "";
	for (;; ++query_index) {
		if (query[query_index] != ':') {
			word += query[query_index];
		} else {
			query_index += 2;
			break;
		}
	}
	words.push_back(word);

	// На случай пустого маршрута или остановки
	if (query_index >= query.size()) {
		return true;
	}

	// записываем все остальные слова
	word = "";
	for (;; ++query_index) {
		if (query_index == query.size()) {
			words.push_back(word);
			break;
		} else if (query[query_index] == '\n') {
			words.push_back(word);
			word = "";
		} else if (query[query_index] == ' ' && query[query_index + 1] == '>' && query[query_index + 2] == ' ') {
			words.push_back(word);
			word = "";
			query_index += 2;
		} else if (query[query_index] == ' ' && query[query_index + 1] == '-' && query[query_index + 2] == ' ') {
			is_loop = false;
			words.push_back(word);
			word = "";
			query_index += 2;
		} else if (query[query_index] == ',' && query[query_index + 1] == '_') {
			words.push_back(word);
			word = "";
			query_index += 1;
		} else {
			word += query[query_index];
		}
	}

	return is_loop;
}

// Функция делает вызов GetTypeNameAndContent,
// получает разбитый на отдельные слова запрос
// и, исходя из типа запроса, вызывает нужный
// метод по обновлнию транспортного каталога.
void ParseInputQuery(TransportCatalogue& transport_catalogue, istream& input_stream) {
	string query;
	getline(input_stream, query);
	vector<string> words;
	bool is_loop;

	is_loop = GetTypeNameAndContent(query, words);
	if (words.front() == BusRequest) {
		transport_catalogue.AddBus(words, is_loop);
	} else if (words.front() == StopRequest) {
		transport_catalogue.AddStop(words);
	}
}