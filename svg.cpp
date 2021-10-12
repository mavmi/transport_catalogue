#include "svg.h"

namespace svg {

	using namespace std::literals;

	void Object::Render(const RenderContext& context) const {
		context.RenderIndent();

		// Делегируем вывод тега своим подклассам
		RenderObject(context);

		context.out << std::endl;
	}

	// ---------- Circle ------------------

	Circle& Circle::SetCenter(Point center) {
		center_ = center;
		return *this;
	}

	Circle& Circle::SetRadius(double radius) {
		radius_ = radius;
		return *this;
	}

	void Circle::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
		out << "r=\""sv << radius_ << "\" "sv;
		RenderAttrs(out);
		out << "/>"sv;
	}

	// ---------- Polyline ------------------

	Polyline& Polyline::AddPoint(Point point) {
		points_.push_back(std::move(point));
		return *this;
	}

	void Polyline::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<polyline points=\""sv;
		for (size_t i = 0; i < points_.size(); ++i) {
			if (i != 0) {
				out << " "sv;
			}
			const Point& point = points_.at(i);
			out << point.x << ","sv << point.y;
		}
		out << "\""sv;
		RenderAttrs(out);
		out << "/>"sv;
	}

	// ---------- Text ------------------

	Text& Text::SetPosition(Point pos) {
		pos_ = pos;
		return *this;
	}

	Text& Text::SetOffset(Point offset) {
		offset_ = offset;
		return *this;
	}

	Text& Text::SetFontSize(uint32_t size) {
		font_size_ = size;
		return *this;
	}

	Text& Text::SetFontFamily(const std::string& font_family) {
		font_family_ = font_family;
		return *this;
	}

	Text& Text::SetFontWeight(const std::string& font_weight) {
		font_weight_ = font_weight;
		return *this;
	}

	Text& Text::SetData(const std::string& data) {
		data_ = data;
		return *this;
	}

	void Text::PrintText(const RenderContext& context) const {
		auto& out = context.out;
		for (char c : data_) {
			switch (c) {
			case '\"':
				out << "&quot;";
				break;
			case '\'':
				out << "&apos;";
				break;
			case '<':
				out << "&lt;";
				break;
			case '>':
				out << "&gt;";
				break;
			case '&':
				out << "&amp;";
				break;
			default:
				out << c;
			}
		}
	}

	void Text::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<text";
		RenderAttrs(out);
		out << " x=\"" << pos_.x << "\"";
		out << " y=\"" << pos_.y << "\"";
		out << " dx=\"" << offset_.x << "\"";
		out << " dy=\"" << offset_.y << "\"";
		out << " font-size=\"" << font_size_ << "\"";
		if (font_family_ != "") {
			out << " font-family=\"" << font_family_ << "\"";
		}
		if (font_weight_ != "") {
			out << " font-weight=\"" << font_weight_ << "\"";
		}
		out << ">";
		Text::PrintText(context);
		out << "</text>";
	}

	// ---------- Document ------------------

	void Document::AddPtr(std::unique_ptr<Object>&& obj) {
		items_.push_back(std::move(obj));
	}

	void Document::Render(std::ostream& out) const {
		RenderContext render_context(out);

		out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
		out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

		for (const auto& ptr : items_) {
			out << "  ";
			ptr.get()->Render(render_context);
		}

		out << "</svg>"sv;
	}

	void Document::Clear() {
		items_.clear();
	}

}  // namespace svg


namespace shapes {

	// ---------- Triangle ------------------

	Triangle::Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
		: p1_(p1)
		, p2_(p2)
		, p3_(p3) {
	}

	// Реализует метод Draw интерфейса svg::Drawable
	void Triangle::Draw(svg::ObjectContainer& container) const {
		container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
	}

	// ---------- Star ------------------

	Star::Star(svg::Point center, double outer_rad, double inner_rad, int num_rays) {
		polyline_ = CreateStar(center, outer_rad, inner_rad, num_rays);
	}


	void Star::Draw(svg::ObjectContainer& container) const {
		container.Add(polyline_);
	}

	svg::Polyline Star::CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) {
		using namespace svg;
		Polyline polyline;
		for (int i = 0; i <= num_rays; ++i) {
			double angle = 2 * M_PI * (i % num_rays) / num_rays;
			polyline.AddPoint({ center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle) });
			if (i == num_rays) {
				break;
			}
			angle += M_PI / num_rays;
			polyline.AddPoint({ center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle) });
		}
		polyline.SetFillColor("red").SetStrokeColor("black");
		return polyline;
	}

	// ---------- Snowman ------------------

	Snowman::Snowman(svg::Point head_center, double head_radius) {
		head_center_ = head_center;
		head_radius_ = head_radius;
	}

	void Snowman::Draw(svg::ObjectContainer& container) const {
		using namespace svg;

		Circle circle1;
		circle1.SetCenter(head_center_);
		circle1.SetRadius(head_radius_);
		circle1.SetFillColor("rgb(240,240,240)").SetStrokeColor("black");

		Circle circle2;
		circle2.SetCenter({ head_center_.x, head_center_.y + head_radius_ * 2 });
		circle2.SetRadius(head_radius_ * 1.5);
		circle2.SetFillColor("rgb(240,240,240)").SetStrokeColor("black");

		Circle circle3;
		circle3.SetCenter({ head_center_.x, head_center_.y + head_radius_ * 5 });
		circle3.SetRadius(head_radius_ * 2);
		circle3.SetFillColor("rgb(240,240,240)").SetStrokeColor("black");

		container.Add(circle3);
		container.Add(circle2);
		container.Add(circle1);
	}


} //namespace shapes