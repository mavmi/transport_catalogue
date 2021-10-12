#pragma once

#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <memory>

#include "svg.h"
#include "geo.h"
#include "json.h"
#include "transport_catalogue.h"

inline const double EPSILON = 1e-6;

struct RenderSettings {
	RenderSettings() = default;

	double width = 1200.0;
	double height = 1200.0;
	double padding = 50.0;
	double line_width = 14.0;
	double stop_radius = 5.0;
	int bus_label_font_size = 20;
	double bus_label_offset[2] = { 7.0, 15.0 };
	int stop_label_font_size = 20;
	double stop_label_offset[2] = { 7.0, -3.0 };
	svg::Color underlayer_color = svg::Rgba(255, 255, 255, 0.85);
	double underlayer_width = 3.0;
	std::vector<svg::Color> color_palette{ "green", svg::Rgb(255, 160, 0), "red" };
};

inline bool IsZero(double value) {
	return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
	template <typename PointInputIt>
	SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
		double max_height, double padding)
		: padding_(padding) {
		if (points_begin == points_end) {
			return;
		}

		const auto [left_it, right_it]
			= std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
			return lhs.lng < rhs.lng;
				});
		min_lon_ = left_it->lng;
		const double max_lon = right_it->lng;

		const auto [bottom_it, top_it]
			= std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
			return lhs.lat < rhs.lat;
				});
		const double min_lat = bottom_it->lat;
		max_lat_ = top_it->lat;

		std::optional<double> width_zoom;
		if (!IsZero(max_lon - min_lon_)) {
			width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
		}

		std::optional<double> height_zoom;
		if (!IsZero(max_lat_ - min_lat)) {
			height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
		}

		if (width_zoom && height_zoom) {
			zoom_coeff_ = std::min(*width_zoom, *height_zoom);
		} else if (width_zoom) {
			zoom_coeff_ = *width_zoom;
		} else if (height_zoom) {
			zoom_coeff_ = *height_zoom;
		}
	}

	svg::Point operator()(geo::Coordinates coords) const {
		return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
			(max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
	}

private:
	double padding_;
	double min_lon_ = 0;
	double max_lat_ = 0;
	double zoom_coeff_ = 0;
};

class MapRenderer {
public:
	MapRenderer(TransportCatalogue& transport_catalogue)
		: transport_catalogue_(transport_catalogue) {

	}

	void SetRenderSettings(RenderSettings render_settings);

	RenderSettings GetRenderSettings();

	void RenderMap();

	void RenderMapAsString(std::ostream& output_stream = std::cout);

	void RenderMapAsJSON(std::ostream& output_stream = std::cout);

private:
	svg::Document result_document_;
	TransportCatalogue& transport_catalogue_;
	RenderSettings render_settings_;
	std::unique_ptr<SphereProjector> sphere_projector_;

	void CreateSphereProjector_();

	void RenderBusesLines_();

	void RenderNamesOfBuses_();

	void RenderStopsCircles_();

	void RenderNamesOfStops_();

	// Получает вектор всех точек остановок по всем маршрутам
	std::vector<geo::Coordinates> GetAllGeoPoints_(const std::map<std::string, std::shared_ptr<Bus>>& buses);

	// Получает вектор точек остановок по текущему маршруту
	std::vector<geo::Coordinates> GetGeoPointsOfBus_(const std::shared_ptr<Bus> bus);
};