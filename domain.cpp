#include "domain.h"

using namespace std::literals;

const std::string BusRequest = "Bus"s;
const std::string StopRequest = "Stop"s;
const std::string MapRequest = "Map"s;
const std::string RouteRequest = "Route"s;

std::string DoubleToString(double value) {
	std::ostringstream output;
	output << std::setprecision(20) << value;
	return output.str();
}