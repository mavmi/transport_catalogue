#include "map_renderer.h"

void MapRenderer::SetRenderSettings(RenderSettings render_settings) {
	render_settings_ = render_settings;
}

RenderSettings MapRenderer::GetRenderSettings(){
	return render_settings_;
}

void MapRenderer::RenderMap() {
	result_document_.Clear();
	CreateSphereProjector_();
	RenderBusesLines_();
	RenderNamesOfBuses_();
	RenderStopsCircles_();
	RenderNamesOfStops_();
}

// Вывод результата в виде строки
void MapRenderer::RenderMapAsString(std::ostream& output_stream) {
	RenderMap();
	result_document_.Render(output_stream);
}

// Вывод с эскейп-последовательностями
void MapRenderer::RenderMapAsJSON(std::ostream& output_stream) {
	RenderMap();
	// Временный поток для вывода svg документа
	std::ostringstream tmp_output_stream;
	// Выводим документ во временный поток
	result_document_.Render(tmp_output_stream);
	// Создаем JSON узел со строкой из временного потока
	json::Node node(tmp_output_stream.str());
	// Делаем документ из узла и выводим его через 
	// функцию json::Print(), она задаст нужный формат
	json::Document document(node);
	json::Print(document, output_stream);
}

void MapRenderer::CreateSphereProjector_() {
	const auto& buses = transport_catalogue_.GetBuses();
	std::vector<geo::Coordinates> points_of_all_stops = GetAllGeoPoints_(buses);
	sphere_projector_ = std::make_unique<SphereProjector>(points_of_all_stops.begin(), points_of_all_stops.end(), render_settings_.width, render_settings_.height, render_settings_.padding);
}

void MapRenderer::RenderBusesLines_() {
	size_t color_palette_counter = 0;

	// Метод transport_catalogue_.GetBuses() возвращает словарь
	// std::map<std::string, std::shared_ptr<BUS>>, где строка - это название маршрута,
	// а std::shared_ptr<BUS> - указатель на структуру маршрута
	for (const auto& [bus_name, bus] : transport_catalogue_.GetBuses()) {
		// Отрисовываем только непустые маршруты.
		// Дек bus->stops хранит в себе указетели
		// на остановки текущего маршрута
		if (bus.get()->stops.size() == 0) {
			continue;
		}
		// Ломаная для текущего маршрута
		svg::Polyline polyline;
		// Координаты остановок текущего маршрута
		std::vector<geo::Coordinates> current_bus_points_of_stops = GetGeoPointsOfBus_(bus);

		// Запись точек остановок маршрута в текущую ломаную
		for (const auto& geo_point : current_bus_points_of_stops) {
			svg::Point point = (*(sphere_projector_.get()))(geo_point);
			polyline.AddPoint(point);
		}

		polyline.SetStrokeColor(render_settings_.color_palette.at(color_palette_counter));
		// Двигаем счетчик по палитре цветов
		if (color_palette_counter + 1 == render_settings_.color_palette.size()) {
			color_palette_counter = 0;
		} else {
			color_palette_counter++;
		}
		polyline.SetFillColor(svg::NoneColor);
		polyline.SetStrokeWidth(render_settings_.line_width);
		polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		// Добавляем ломаную в итоговый документ SVG объектов
		result_document_.Add(std::move(polyline));
	}
}

void MapRenderer::RenderNamesOfBuses_() {
	using namespace std::literals;

	size_t color_palette_counter = 0;

	// Метод transport_catalogue_.GetBuses() возвращает словарь
	// std::map<std::string, std::shared_ptr<BUS>>, где строка - это название маршрута,
	// а std::shared_ptr<BUS> - указатель на структуру маршрута
	const auto& buses = transport_catalogue_.GetBuses();

	for (const auto& [bus_name, bus] : buses) {
		// Отрисовываем только непустые маршруты.
		// Дек bus->stops хранит в себе указетели
		// на остановки текущего маршрута
		if (bus.get()->stops.size() == 0) {
			continue;
		}

		for (int i = 0; i < 2; ++i) {
			std::shared_ptr<Stop> current_stop;
			if (i == 0) { // итерация для начальной остановки
				current_stop = bus.get()->stops.front();
			} else { // для конечной некольцевого маршрута
				current_stop = bus.get()->stops.at(bus.get()->stops.size() / 2);
			}

			svg::Text stop_name, background;
			stop_name.SetPosition((*sphere_projector_)({ current_stop.get()->coord_x, current_stop.get()->coord_y }));
			background.SetPosition((*sphere_projector_)({ current_stop.get()->coord_x, current_stop.get()->coord_y }));
			stop_name.SetOffset({ render_settings_.bus_label_offset[0], render_settings_.bus_label_offset[1] });
			background.SetOffset({ render_settings_.bus_label_offset[0], render_settings_.bus_label_offset[1] });
			stop_name.SetFontSize(render_settings_.bus_label_font_size);
			background.SetFontSize(render_settings_.bus_label_font_size);
			stop_name.SetFontFamily("Verdana"s);
			background.SetFontFamily("Verdana"s);
			stop_name.SetFontWeight("bold"s);
			background.SetFontWeight("bold"s);
			stop_name.SetData(bus.get()->name);
			background.SetData(bus.get()->name);

			background.SetFillColor(render_settings_.underlayer_color);
			background.SetStrokeColor(render_settings_.underlayer_color);
			background.SetStrokeWidth(render_settings_.underlayer_width);
			background.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
			background.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

			stop_name.SetFillColor(render_settings_.color_palette.at(color_palette_counter));

			// Добавляем тексты в итоговый документ SVG объектов
			result_document_.Add(std::move(background));
			result_document_.Add(std::move(stop_name));

			// Если конечная и начальная остановки одинаковые, то выходим.
			// Второе условие существует, потому что не кольцевой маршрут
			// хранится так: A - B - C - B - A. Здесь C - конечная.
			if (bus.get()->is_looped || bus.get()->stops.front() == bus.get()->stops.at(bus.get()->stops.size() / 2)) {
				break;
			}
		}

		// Двигаем счетчик по палитре цветов, как в рендере линии маршрута
		if (color_palette_counter + 1 == render_settings_.color_palette.size()) {
			color_palette_counter = 0;
		} else {
			color_palette_counter++;
		}
	}
}

void MapRenderer::RenderStopsCircles_() {
	// transport_catalogue_.GetStopsToBuses() вовзращает 
	// словарь std::map<std::string, std::shared_ptr<std::set<std::string>>>,
	// где ключ - это название маршрута, а значение - 
	// указатель на сет названий маршуртов, которые проходят через эту остановку
	const auto& stops_to_buses = transport_catalogue_.GetStopsToBuses();

	// transport_catalogue_.GetStops() возвращает
	// словарь std::map<std::string, std::shared_ptr<STOP>>.
	// Здесь ключ - это название остановки, а значение - 
	// указатель на нее
	const auto& stops = transport_catalogue_.GetStops();

	for (const auto& [stop_name, buses_set_ptr] : stops_to_buses) {
		// Отбрасываем остановки, через которые не ходят автобусы
		if (buses_set_ptr.get()->size() == 0) {
			continue;
		}

		std::shared_ptr<Stop> current_stop = stops.at(stop_name);
		svg::Circle stop_circle;
		stop_circle.SetCenter((*sphere_projector_)({ current_stop.get()->coord_x, current_stop.get()->coord_y }));
		stop_circle.SetRadius(render_settings_.stop_radius);
		stop_circle.SetFillColor("white");

		result_document_.Add(std::move(stop_circle));
	}
}

void MapRenderer::RenderNamesOfStops_() {
	using namespace std::literals;

	// transport_catalogue_.GetStopsToBuses() вовзращает 
	// словарь std::map<std::string, std::shared_ptr<std::set<std::string>>>,
	// где ключ - это название маршрута, а значение - 
	// указатель на сет названий маршуртов, которые проходят через эту остановку
	const auto& stops_to_buses = transport_catalogue_.GetStopsToBuses();

	// transport_catalogue_.GetStops() возвращает
	// словарь std::map<std::string, std::shared_ptr<STOP>>.
	// Здесь ключ - это название остановки, а значение - 
	// указатель на нее
	const auto& stops = transport_catalogue_.GetStops();

	for (const auto& [name_of_stop, buses_set_ptr] : stops_to_buses) {
		// Отбрасываем остановки, через которые не ходят автобусы
		if (buses_set_ptr.get()->size() == 0) {
			continue;
		}

		std::shared_ptr<Stop> current_stop = stops.at(name_of_stop);
		svg::Text stop_name, background;

		stop_name.SetPosition((*sphere_projector_)({ current_stop.get()->coord_x, current_stop.get()->coord_y }));
		background.SetPosition((*sphere_projector_)({ current_stop.get()->coord_x, current_stop.get()->coord_y }));
		stop_name.SetOffset({ render_settings_.stop_label_offset[0], render_settings_.stop_label_offset[1] });
		background.SetOffset({ render_settings_.stop_label_offset[0], render_settings_.stop_label_offset[1] });
		stop_name.SetFontSize(render_settings_.stop_label_font_size);
		background.SetFontSize(render_settings_.stop_label_font_size);
		stop_name.SetFontFamily("Verdana"s);
		background.SetFontFamily("Verdana"s);
		stop_name.SetData(current_stop.get()->name);
		background.SetData(current_stop.get()->name);

		background.SetFillColor(render_settings_.underlayer_color);
		background.SetStrokeColor(render_settings_.underlayer_color);
		background.SetStrokeWidth(render_settings_.underlayer_width);
		background.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		background.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		stop_name.SetFillColor("black");

		result_document_.Add(std::move(background));
		result_document_.Add(std::move(stop_name));
	}
}

// Возвращает вектор точек всех остановок по всем маршрутам, обработанных классом SphereProjector
std::vector<geo::Coordinates> MapRenderer::GetAllGeoPoints_(const std::map<std::string, std::shared_ptr<Bus>>& buses) {
	std::vector<geo::Coordinates> result_vector;

	for (const auto& [name, bus_ptr] : buses) {
		std::vector<geo::Coordinates> stop_points = GetGeoPointsOfBus_(bus_ptr);
		result_vector.insert(result_vector.end(), stop_points.begin(), stop_points.end());
	}

	return result_vector;
}

// Вовзращает вектор точек всех остановок, обработанных классом SphereProjector, по текущему маршруту
std::vector<geo::Coordinates> MapRenderer::GetGeoPointsOfBus_(const std::shared_ptr<Bus> bus) {
	std::vector<geo::Coordinates> result_vector;
	std::deque<std::shared_ptr<Stop>>& stops = bus.get()->stops;
	const auto& stops_to_buses = transport_catalogue_.GetStopsToBuses();

	for (size_t i = 0; i < stops.size(); ++i) {
		std::shared_ptr<Stop> stop = stops.at(i);
		// Берем координаты остановок, по которым ходят автобусы
		if (stops_to_buses.count(stop.get()->name) && stops_to_buses.at(stop.get()->name)->size() != 0) {
			result_vector.push_back({ stop.get()->coord_x, stop.get()->coord_y });
		}
	}

	return result_vector;
}