#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <optional>

#include "map_renderer.h"
#include "json_reader.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "serialization.h"
#include "svg.h"

using namespace std;

void PrintUsage(std::ostream& stream = std::cerr) {
	stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		PrintUsage();
		return 1;
	}

	const std::string_view mode(argv[1]);

	TransportCatalogue transport_catalogue;
	MapRenderer map_renderer(transport_catalogue);
	TransportRouter transport_router(transport_catalogue);
	Serialization serialization;

	TransportManagers transport_managers(transport_catalogue, map_renderer, transport_router, serialization);

	if (mode == "make_base"sv) {

		ReadJSON(transport_managers);
		serialization.Serialize(transport_catalogue, map_renderer, transport_router);
		
	} else if (mode == "process_requests"sv) {

		json::Document input_document = json::Load(std::cin);
		json::Dict dict = input_document.GetRoot().AsDict();

		transport_managers.serialization.SetSerializationSettings(GetSerializationSettings(dict.at("serialization_settings").AsDict()));
		serialization.Deserialize(transport_catalogue, map_renderer, transport_router);
		PrintCatatlog(transport_managers, dict.at("stat_requests").AsArray(), std::cout);

	} else {
		PrintUsage();
		return 1;
	}
}