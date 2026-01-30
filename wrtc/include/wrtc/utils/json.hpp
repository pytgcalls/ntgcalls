//
// Created by laky64 on 24/01/26.
//

#pragma once
#include <any>
#include <string>
#include <vector>
#include <variant>
#include <type_traits>
#include <boost/json.hpp>

namespace wrtc {
    template<typename T>
    class iterable;
    class iterable_items;
    class iteration_proxy_value;

    template<typename>
    struct is_vector : std::false_type {};

    template<typename T, typename A>
    struct is_vector<std::vector<T, A>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_vector_v = is_vector<T>::value;

    template<typename>
    inline constexpr bool always_false_v = false;

    using obj_it = boost::json::object::const_iterator;
    using arr_it = boost::json::array::const_iterator;

    class json {
        boost::json::value* ref_ = nullptr;
        boost::json::value storage_;

        boost::json::value& value() {
            return ref_ ? *ref_ : storage_;
        }

        [[nodiscard]] const boost::json::value& value() const {
            return ref_ ? *ref_ : storage_;
        }

        static json view(boost::json::value& v) {
            json j;
            j.ref_ = &v;
            return j;
        }

    public:
        class exception final : public std::exception {
        public:
            explicit exception(std::string msg): _msg(std::move(msg)) {}

            [[nodiscard]] const char* what() const noexcept override {
                return _msg.c_str();
            }

        private:
            std::string _msg;
        };

        json() = default;

        explicit json(boost::json::value v)
            : storage_(std::move(v)) {}

        json(const std::initializer_list<json> list) {
            bool maybe_object = list.size() != 0;
            for (const auto& el : list) {
                if (!el.value().is_array()) {
                    maybe_object = false;
                    break;
                }
                if (const auto& arr = el.value().as_array(); arr.size() != 2 || !arr[0].is_string()) {
                    maybe_object = false;
                    break;
                }
            }

            if (maybe_object) {
                boost::json::object obj;
                for (const auto& el : list) {
                    const auto& arr = el.value().as_array();
                    obj[arr[0].as_string()] = arr[1];
                }
                storage_ = std::move(obj);
            } else {
                boost::json::array arr;
                for (const auto& el : list) {
                    arr.emplace_back(el.value());
                }
                storage_ = std::move(arr);
            }
        }

        template<
            typename T,
            typename D = std::decay_t<T>,
            std::enable_if_t<
                std::is_same_v<D, uint8_t> ||
                std::is_integral_v<D> ||
                std::is_floating_point_v<D> ||
                std::is_same_v<D, bool> ||
                std::is_same_v<D, std::string> ||
                std::is_same_v<D, const char*> ||
                std::is_enum_v<D> ||
                is_vector_v<D>,
                int
            > = 0
        >
        json(T v) { // NOLINT
            if constexpr (is_vector_v<D>) {
                boost::json::array arr;
                for (const auto& el : v) {
                    arr.emplace_back(el);
                }
                storage_ = std::move(arr);
            } else if constexpr (std::is_enum_v<D>) {
                using U = std::underlying_type_t<D>;
                storage_ = static_cast<U>(v);
            } else if constexpr (std::is_same_v<D, const char*>) {
                storage_ = std::string(v);
            } else {
                storage_ = static_cast<D>(v);
            }
        }

        [[nodiscard]] bool is_null() const {
            return value().is_null();
        }

        [[nodiscard]] bool is_boolean() const {
            return value().is_bool();
        }

        [[nodiscard]] bool is_string() const {
            return value().is_string();
        }

        [[nodiscard]] bool is_number() const {
            return value().is_number();
        }

        [[nodiscard]] bool is_object() const {
            return value().is_object();
        }

        [[nodiscard]] bool empty() const {
            if(value().is_object()) return value().as_object().empty();
            if(value().is_array()) return value().as_array().empty();
            return false;
        }

        [[nodiscard]] std::string dump() const {
            return boost::json::serialize(value());
        }

        template<
            typename T,
            std::enable_if_t<
                std::conjunction_v<
                    std::negation<std::is_pointer<T>>,
                    std::negation<std::is_same<T, std::nullptr_t>>,
                    std::negation<std::is_same<T, std::string::value_type>>,
                    std::negation<std::is_same<T, json>>,
                    std::negation<std::is_same<T, iteration_proxy_value>>,
                    std::negation<std::is_same<T, std::any>>,
                    std::negation<std::is_same<T, std::initializer_list<std::string::value_type>>>,
                    std::negation<std::is_same<T, std::string_view>>
                >,
                int
            > = 0
        >
        operator T() const { // NOLINT
            return get<T>();
        }

        template<typename T, typename D = std::decay_t<T>>
        T get() const { // NOLINT
            const auto& v = value();
            if constexpr (std::is_same_v<D, std::string> || std::is_convertible_v<T, std::string_view>) {
                if (!v.is_string()) throw exception("JSON value is not a string");
                return static_cast<T>(v.as_string());
            } else if constexpr (std::is_same_v<D, bool>) {
                if (!v.is_bool()) throw exception("JSON value is not a bool");
                return static_cast<T>(v.as_bool());
            } else if constexpr (std::is_integral_v<D>) {
                if (v.is_int64()) {
                    return static_cast<T>(v.as_int64());
                }
                if (v.is_uint64()) {
                    return static_cast<T>(v.as_uint64());
                }
                throw exception("JSON value is not an integer");
            } else if constexpr (std::is_floating_point_v<D>) {
                if (!v.is_double()) throw exception("JSON value is not a double");
                return static_cast<T>(v.as_double());
            } else if constexpr (std::is_enum_v<D>) {
                if (!v.is_number()) throw exception("JSON value is not a number");
                using U = std::underlying_type_t<D>;
                U val;
                if (v.is_int64()) val = static_cast<U>(v.as_int64());
                else if (v.is_uint64()) val = static_cast<U>(v.as_uint64());
                else throw exception("JSON enum value not integral");
                return static_cast<T>(val);
            } else {
                static_assert(always_false_v<T>, "get<T>() type not supported");
            }
        }


        bool operator==(const std::string_view rhs) const {
            const auto& v = value();
            return v.is_string() && v.as_string() == rhs;
        }

        template<
            typename T,
            std::enable_if_t<
                std::negation_v<std::is_pointer<T>>
            > = 0
        >
        bool operator==(T rhs) const {
            return get<T>() == rhs;
        }

        json& operator=(const json& other) {
            if (this == &other)
                return *this;

            if (ref_) {
                *ref_ = other.value();
            } else {
                storage_ = other.value();
            }
            return *this;
        }

        template<typename T, typename D = std::decay_t<T>>
        json operator[](const T& key) {
            auto& v = value();
            if (v.is_null())
                return view(v);
            if constexpr (
                std::is_same_v<D, std::string> ||
                std::is_same_v<D, std::string_view> ||
                std::is_convertible_v<T, std::string_view>
            ) {
                if (!v.is_object())
                    throw exception("Not an object");
                return view(v.as_object()[key]);
            } else if constexpr (std::is_integral_v<D>) {
                if (!v.is_array())
                    throw exception("Not an array");
                return view(v.as_array()[key]);
            } else {
                static_assert(always_false_v<D>, "Unsupported key type");
                throw exception("Unsupported key type");
            }
        }

        template<typename T>
        json operator[](const T& key) const {
            auto &self = const_cast<json&>(*this);
            return self.operator[](key);
        }

        static json object() {
            return json(boost::json::object());
        }

        static json array() {
            return json(boost::json::array());
        }

        [[nodiscard]] iterable_items items() const;

        [[nodiscard]] iterable_items items();

        void push_back(const json &item);

        [[nodiscard]] bool contains(const std::string& key) const;

        [[nodiscard]] iterable<json> begin() const;

        [[nodiscard]] iterable<json> end() const;

        static json parse(const std::string& data) {
            json j;
            j.storage_ = boost::json::parse(data);
            return j;
        }

        template<typename It>
        static json parse(It first, It last) {
            return parse(std::string(first, last));
        }
    };

    class iteration_proxy_value {
        std::string key_;
        json value_;

    public:
        explicit iteration_proxy_value(std::string key, const json& v)
            : key_(std::move(key)), value_(v) {}

        explicit iteration_proxy_value(const json& v) : iteration_proxy_value("", v) {}

        [[nodiscard]] std::string key() const {
            if (key_.empty()) throw json::exception("No key for this value");
            return key_;
        }

        [[nodiscard]] json value() const { return value_; }
    };

    template<typename Mode>
    struct iterable_policy;

    template<>
    struct iterable_policy<json> {
        static json deref(
            std::variant<obj_it, arr_it> it
        ) {
            return std::visit([]<typename T0>(T0 ptr) {
                using P = std::decay_t<T0>;
                if constexpr (std::is_same_v<P, obj_it>) {
                    return json(ptr->value());
                } else {
                    return json(*ptr);
                }
            }, it);
        }
    };

    template<>
    struct iterable_policy<iteration_proxy_value> {
        static iteration_proxy_value deref(
            std::variant<obj_it, arr_it> it
        ) {
            return std::visit([]<typename T0>(T0 ptr) {
                using P = std::decay_t<T0>;
                if constexpr (std::is_same_v<P, obj_it>) {
                    return iteration_proxy_value(
                        ptr->key_c_str(),
                        json(ptr->value())
                    );
                } else {
                    return iteration_proxy_value(
                        json(*ptr)
                    );
                }
            }, it);
        }
    };

    template<typename T>
    class iterable {
        static_assert(
            std::is_same_v<T, json> ||
            std::is_same_v<T, iteration_proxy_value>,
            "Unsupported iteration mode"
        );
        std::variant<obj_it, arr_it> it_;

    public:
        explicit iterable(obj_it it) : it_(it) {}
        explicit iterable(arr_it it) : it_(it) {}

        using value_type = decltype(iterable_policy<T>::deref(std::declval<obj_it>()));

        value_type operator*() const {
            return iterable_policy<T>::deref(it_);
        }

        iterable& operator++() {
            std::visit([]<typename T0>(T0& ptr) {
                ++ptr;
            }, it_);
            return *this;
        }

        bool operator!=(const iterable& other) const {
            return it_ != other.it_;
        }
    };

    class iterable_items {
        std::variant<
            const boost::json::array*,
            const boost::json::object*
        > data_;

    public:
        explicit iterable_items(const boost::json::array& a) : data_(&a) {}

        explicit iterable_items(const boost::json::object& o) : data_(&o) {}

        [[nodiscard]] iterable<iteration_proxy_value> begin() const {
            return std::visit([](auto ptr) {
                return iterable<iteration_proxy_value>(ptr->begin());
            }, data_);
        }

        [[nodiscard]] iterable<iteration_proxy_value> end() const {
            return std::visit([](auto ptr) {
                return iterable<iteration_proxy_value>(ptr->end());
            }, data_);
        }
    };
} // wrtc