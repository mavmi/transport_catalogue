#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <iomanip>

#include "geo.h"
#include "json.h"
#include "transport_catalogue.h"
#include "domain.h"

int GetStopsCount(const std::shared_ptr<Bus> bus);

int GetUniqueStop(const std::shared_ptr<Bus> bus);

double CalculateGeographicLength(const std::shared_ptr<Bus> bus);

double CalculateActualLength(const std::shared_ptr<Bus> bus, TransportCatalogue& transport_catalogue);

double CalculateCurvature(double geo_length, double actual_length);

void GetBusInfo(TransportCatalogue& transport_catalogue, const std::vector<std::string>& words, std::vector<std::pair<std::string, json::Node>>& bus_info_vector);

void GetStopInfo(TransportCatalogue& transport_catalogue, const std::vector<std::string>& words, std::vector<std::pair<std::string, json::Node>>& stop_info_vector);

void ParseOutputQuery(TransportCatalogue& transport_catalogue, std::vector<std::pair<std::string, json::Node>>& vector_to_store_output_info, std::istream& input_stream = std::cin);

bool GetTypeNameAndContent(const std::string& query, std::vector<std::string>& words);

void ParseInputQuery(TransportCatalogue& transport_catalogue, std::istream& input_stream = std::cin);