//
// Created by laky64 on 24/01/26.
//

#include <wrtc/utils/json.hpp>

namespace wrtc {
    iterable_items json::items() const {
        auto &self = const_cast<json&>(*this);
        return self.items();
    }

    iterable_items json::items() {
        if(value().is_array()) {
            return iterable_items(value().as_array());
        }
        if(value().is_object()) {
            return iterable_items(value().as_object());
        }
        throw exception("Not an array or object");
    }

    void json::push_back(const json &item) {
        if(!value().is_array()) throw exception("Not an array");
        value().as_array().push_back(item.value());
    }

    bool json::contains(const std::string &key) const {
        if (!value().is_object())
            return false;
        return value().as_object().contains(key);
    }

    iterable<json> json::begin() const {
        if (value().is_array())
            return iterable<json>(value().as_array().begin());
        if (value().is_object())
            return iterable<json>(value().as_object().begin());

        throw exception("Not iterable");
    }

    iterable<json> json::end() const {
        if (value().is_array())
            return iterable<json>(value().as_array().end());
        if (value().is_object())
            return iterable<json>(value().as_object().end());

        throw exception("Not iterable");
    }
} // wrtc