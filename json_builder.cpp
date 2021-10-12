#include "json_builder.h"

namespace json {

	AfterKey Builder::Key(std::string&& key) {
		// Записываем переданный ключ
		ErrorsChecker errors_checher(*this, key_func_name_);

		keys_to_add_.push_back(std::move(key));

		return AfterKey(*this);
	}

	BuilderHelper Builder::Value(Node::Value&& value) {
		ErrorsChecker errors_checher(*this, value_func_name_);

		size_t unfinished_nodes_size = unfinished_nodes_.size();
		if (unfinished_nodes_size > 0 && std::holds_alternative<Array>(unfinished_nodes_.back())) {
			// Добавить в последний вектор
			std::get<Array>(unfinished_nodes_[unfinished_nodes_size - 1]).push_back(std::move(value));
		} else if (unfinished_nodes_size > 0 && std::holds_alternative<Dict>(unfinished_nodes_.back())) {
			// Добавить в последний словарь по последнему ключу
			std::get<Dict>(unfinished_nodes_[unfinished_nodes_size - 1])[keys_to_add_.back()] = std::move(value);
			keys_to_add_.pop_back();
		} else {
			// Создать результирующий узел
			root_ = std::move(value);
		}

		return *this;
	}

	// Кладем в вектор пустой словарь
	AfterStartDict Builder::StartDict() {
		ErrorsChecker errors_checher(*this, start_dict_func_name_);

		unfinished_nodes_.push_back(Dict());

		return *this;
	}

	// Кладем в вектор пустой вектор
	AfterStartArray Builder::StartArray() {
		ErrorsChecker errors_checher(*this, start_array_func_name_);

		unfinished_nodes_.push_back(Array());

		return *this;
	}

	// Заканчиваем словарь и добавляем его
	// в родительский узел
	BuilderHelper Builder::EndDict() {
		ErrorsChecker errors_checher(*this, end_dict_func_name_);

		size_t unfinished_nodes_size = unfinished_nodes_.size();
		Dict dict_to_push = std::get<Dict>(unfinished_nodes_.back());
		if (unfinished_nodes_size < 2) {
			// Создать результирующий узел
			root_ = std::move(dict_to_push);
			unfinished_nodes_.pop_back();
			return *this;
		}

		std::variant<Array, Dict>& containter_to_update = unfinished_nodes_[unfinished_nodes_size - 2];
		if (std::holds_alternative<Array>(containter_to_update)) {
			// Добавить словарь в последний вектор
			std::get<Array>(containter_to_update).push_back(std::move(dict_to_push));
		} else if (std::holds_alternative<Dict>(containter_to_update)) {
			// Добавить словарь в последний словарь по последнему ключу
			std::get<Dict>(containter_to_update)[keys_to_add_.back()] = std::move(dict_to_push);
			keys_to_add_.pop_back();
		}
		unfinished_nodes_.pop_back();

		return *this;
	}

	// Заканчиваем вектор и добавляем его
	// в родительский узел
	BuilderHelper Builder::EndArray() {
		ErrorsChecker errors_checher(*this, end_array_func_name_);

		size_t unfinished_nodes_size = unfinished_nodes_.size();
		Array array_to_push = std::get<Array>(unfinished_nodes_.back());
		if (unfinished_nodes_size < 2) {
			// Создать результирующий узел
			root_ = std::move(array_to_push);
			unfinished_nodes_.pop_back();
			return *this;
		}

		std::variant<Array, Dict>& containter_to_update = unfinished_nodes_[unfinished_nodes_size - 2];
		if (std::holds_alternative<Array>(containter_to_update)) {
			// Добавить вектор в последний вектор
			std::get<Array>(containter_to_update).push_back(std::move(array_to_push));
		} else if (std::holds_alternative<Dict>(containter_to_update)) {
			// Добавить вектор в последний словарь по последнему ключу
			std::get<Dict>(containter_to_update)[keys_to_add_.back()] = std::move(array_to_push);
			keys_to_add_.pop_back();
		}
		unfinished_nodes_.pop_back();

		return *this;
	}

	json::Node Builder::Build() {
		ErrorsChecker errors_checher(*this, build_func_name_);
		return root_;
	}

	// Обновляет флаг готовности root'a
	void Builder::UpdateIsReadyFlag_() {
		is_root_ready_ = (unfinished_nodes_.size() == 0) ? true : false;
	}

	// Метод для проверки на наличие ошибок
	// в порядке вызова методов
	void Builder::CheckErrors_(const std::string& func_name) {
		CheckIfRootReady_(func_name);
		IsKeyUsedOnMap_(func_name);
		AreValueStartArrayAndStartDictUsedProperly_(func_name);
		AreEndDictEndArrayUsedProperly_(func_name);
		IsReadyToBuild_(func_name);
		UpdateLastUsedFunctionInfo_(func_name);
	}

	// Выбрасывает исключение, если root готов, и был вызван не Build
	void Builder::CheckIfRootReady_(const std::string& func_name) {
		using namespace std::literals;

		if (is_root_ready_ && func_name != build_func_name_) {
			throw std::logic_error(func_name + " called when root is ready"s);
		}
	}

	// Проверяем, что метод Key вызван внутри словаря
	void Builder::IsKeyUsedOnMap_(const std::string& func_name) {
		using namespace std::literals;

		if (func_name != key_func_name_) {
			return;
		}

		if (unfinished_nodes_.size() == 0 || std::holds_alternative<Dict>(unfinished_nodes_.back()) == false) {
			throw std::logic_error(func_name + " was called not on Dict"s);
		}
	}

	// Проверка, что методы Value, StartDict и StartArray 
	// были вызваны после конструктора, метода Key или в рамках вектора
	void Builder::AreValueStartArrayAndStartDictUsedProperly_(const std::string& func_name) {
		using namespace std::literals;

		if (func_name != value_func_name_ && func_name != start_dict_func_name_ && func_name != start_array_func_name_) {
			return;
		}

		if (last_function_ != build_func_name_ && last_function_ != key_func_name_ &&
			(unfinished_nodes_.size() != 0 && std::holds_alternative<Array>(unfinished_nodes_.back()) == false)) {
			throw std::logic_error(func_name + " was called at the wrong time"s);
		}
	}

	// Провекра, что методы EndDict и EndArray не были вызваны
	// в контектсе другого контейнера
	void Builder::AreEndDictEndArrayUsedProperly_(const std::string& func_name) {
		using namespace std::literals;

		if (func_name != end_array_func_name_ && func_name != end_dict_func_name_) {
			return;
		}

		if (unfinished_nodes_.size() == 0 ||
			(func_name == end_dict_func_name_ && std::holds_alternative<Array>(unfinished_nodes_.back())) ||
			(func_name == end_array_func_name_ && std::holds_alternative<Dict>(unfinished_nodes_.back()))) {
			throw std::logic_error(func_name + " was called at the wrong time"s);
		}
	}

	// Проверяем, что меод Key не был вызван два раза подряд
	void Builder::UpdateLastUsedFunctionInfo_(const std::string& func_name) {
		using namespace std::literals;

		if (func_name == key_func_name_ && last_function_ == key_func_name_) {
			throw std::logic_error(func_name + " was called twice"s);
		}
		last_function_ = func_name;
	}

	void Builder::IsReadyToBuild_(const std::string& func_name) {
		using namespace std::literals;

		if (func_name == build_func_name_ && (last_function_ == constructor_name_ || unfinished_nodes_.size() != 0)) {
			throw std::logic_error(func_name + " was called to early"s);
		}
	}

	// ==================  BuilderHelper  ==================

	AfterKey BuilderHelper::Key(std::string&& key) {
		builder_.Key(std::move(key));
		return GetBuilder();
	}

	BuilderHelper BuilderHelper::Value(Node::Value&& value) {
		builder_.Value(std::move(value));
		return GetBuilder();
	}

	AfterStartDict BuilderHelper::StartDict() {
		builder_.StartDict();
		return GetBuilder();
	}

	AfterStartArray BuilderHelper::StartArray() {
		builder_.StartArray();
		return GetBuilder();
	}

	BuilderHelper BuilderHelper::EndDict() {
		builder_.EndDict();
		return GetBuilder();
	}

	BuilderHelper BuilderHelper::EndArray() {
		builder_.EndArray();
		return GetBuilder();
	}

	json::Node BuilderHelper::Build() {
		return builder_.Build();
	}

	Builder& BuilderHelper::GetBuilder() {
		return builder_;
	}

	// ==================  AfterKey  ==================

	AfterKeyAfterValue AfterKey::Value(Node::Value&& value) {
		GetBuilder().Value(std::move(value));
		return GetBuilder();
	}

	// ==================  AfterStartArray  ==================

	AfterStartArrayAfterValue AfterStartArray::Value(Node::Value&& value) {
		GetBuilder().Value(std::move(value));
		return GetBuilder();
	}

	// ==================  AfterStartArrayAfterValue  ==================

	AfterStartArrayAfterValue AfterStartArrayAfterValue::Value(Node::Value&& value) {
		GetBuilder().Value(std::move(value));
		return GetBuilder();
	}

}