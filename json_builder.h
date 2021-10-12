#pragma once

#include <vector>
#include <string>
#include <stdexcept>

#include "json.h"

namespace json {

	class BuilderHelper;
	class AfterKey;
	class AfterStartArray;
	class AfterStartDict;
	class AfterKeyAfterValue;
	class AfterStartArrayAfterValue;


	class Builder {
	public:
		friend BuilderHelper;

		AfterKey Key(std::string&& key);

		BuilderHelper Value(Node::Value&& value);

		// Кладем в вектор пустой словарь
		AfterStartDict StartDict();

		// Кладем в вектор пустой вектор
		AfterStartArray StartArray();

		// Заканчиваем словарь и добавляем его
		// в родительский узел
		BuilderHelper EndDict();

		// Заканчиваем вектор и добавляем его
		// в родительский узел
		BuilderHelper EndArray();

		json::Node Build();

	private:
		const std::string constructor_name_ = "Builder";
		const std::string key_func_name_ = "Key";
		const std::string value_func_name_ = "Value";
		const std::string start_dict_func_name_ = "StartDict";
		const std::string start_array_func_name_ = "StartArray";
		const std::string end_dict_func_name_ = "EndDict";
		const std::string end_array_func_name_ = "EndArray";
		const std::string build_func_name_ = "Build";

		std::string last_function_ = "Builder";
		bool is_root_ready_ = false;

		Node root_;
		std::vector<std::string> keys_to_add_;
		std::vector<std::variant<Array, Dict>> unfinished_nodes_;

		class ErrorsChecker {
		public:
			ErrorsChecker(Builder& builder, const std::string& func_name)
				: builder_(builder), func_name_(func_name) {
				builder_.CheckErrors_(func_name_);
			}

			~ErrorsChecker() {
				builder_.UpdateIsReadyFlag_();
			}

		private:
			Builder& builder_;
			const std::string& func_name_;
		};

		// Обновляет флаг готовности root'a
		void UpdateIsReadyFlag_();

		// Метод для проверки на наличие ошибок
		// в порядке вызова методов
		void CheckErrors_(const std::string& func_name);

		// Выбрасывает исключение, если root готов, и был вызван не Build
		void CheckIfRootReady_(const std::string& func_name);

		// Проверяем, что метод Key вызван внутри словаря
		void IsKeyUsedOnMap_(const std::string& func_name);

		// Проверка, что методы Value, StartDict и StartArray 
		// были вызваны после конструктора, метода Key или в рамках вектора
		void AreValueStartArrayAndStartDictUsedProperly_(const std::string& func_name);

		// Провекра, что методы EndDict и EndArray не были вызваны
		// в контектсе другого контейнера
		void AreEndDictEndArrayUsedProperly_(const std::string& func_name);

		// Проверяем, что меод Key не был вызван два раза подряд
		void UpdateLastUsedFunctionInfo_(const std::string& func_name);

		void IsReadyToBuild_(const std::string& func_name);
	};


	class BuilderHelper {
	public:
		BuilderHelper(Builder& builder)
			: builder_(builder) {
		}
		AfterKey Key(std::string&& key);
		BuilderHelper Value(Node::Value&& value);
		AfterStartDict StartDict();
		AfterStartArray StartArray();
		BuilderHelper EndDict();
		BuilderHelper EndArray();
		json::Node Build();
		Builder& GetBuilder();

	private:
		Builder& builder_;

	};

	// Код работы не должен компилироваться в следующих ситуациях:
	// Непосредственно после Key вызван не Value, не StartDict и не StartArray
	class AfterKey
		: public BuilderHelper {
	public:
		AfterKey(Builder& builder)
			: BuilderHelper(builder) {
		}
		AfterKeyAfterValue Value(Node::Value&& value);
		AfterKey Key(std::string&& key) = delete;
		BuilderHelper EndDict() = delete;
		BuilderHelper EndArray() = delete;
		json::Node Build() = delete;
	};

	// За вызовом StartArray следует не Value, не StartDict, не StartArray и не EndArray
	class AfterStartArray
		: public BuilderHelper {
	public:
		AfterStartArray(Builder& builder)
			: BuilderHelper(builder) {
		}
		AfterStartArrayAfterValue Value(Node::Value&& value);
		AfterKey Key(std::string&& key) = delete;
		BuilderHelper EndDict() = delete;
		json::Node Build() = delete;
	};

	// За вызовом StartDict следует не Key и не EndDict
	class AfterStartDict
		: public BuilderHelper {
	public:
		AfterStartDict(Builder& builder)
			: BuilderHelper(builder) {
		}
		BuilderHelper Value(Node::Value&& value) = delete;
		BuilderHelper StartDict() = delete;
		BuilderHelper StartArray() = delete;
		BuilderHelper EndArray() = delete;
		json::Node Build() = delete;
	};

	// После вызова Value, последовавшего за вызовом Key, вызван не Key и не EndDict
	class AfterKeyAfterValue
		: public BuilderHelper {
	public:
		AfterKeyAfterValue(Builder& builder)
			: BuilderHelper(builder) {
		}
		BuilderHelper Value(Node::Value&& value) = delete;
		AfterStartDict StartDict() = delete;
		AfterStartArray StartArray() = delete;
		BuilderHelper EndArray() = delete;
		json::Node Build() = delete;
	};

	// После вызова StartArray и серии Value следует не Value, не StartDict, не StartArray и не EndArray
	class AfterStartArrayAfterValue
		: public BuilderHelper {
	public:
		AfterStartArrayAfterValue(Builder& builder)
			: BuilderHelper(builder) {
		}
		AfterKey Key(std::string&& key) = delete;
		AfterStartArrayAfterValue Value(Node::Value&& value);
		BuilderHelper EndDict() = delete;
		json::Node Build() = delete;
	};

}