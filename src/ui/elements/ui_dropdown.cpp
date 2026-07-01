#include "ui_dropdown.h"

#include "overloaded.h"

#include <charconv>
#include <string>

namespace recompui {

    Dropdown::Dropdown(Element *parent) : Element(parent, Events(EventType::Text, EventType::Focus), "select") {
        // Reuse the dropdown styling already shipped in recomp.rcss (styles the
        // <select>, its selectvalue, the pop-up selectbox and its options).
        set_attribute("class", "config-option-dropdown__select");
        set_focusable(true);
        set_nav_auto(NavDirection::Up);
        set_nav_auto(NavDirection::Down);
        set_tab_index_auto();
    }

    Dropdown::~Dropdown() {

    }

    void Dropdown::set_index_internal(uint32_t new_index, bool trigger_callbacks) {
        index = new_index;
        // Reflect the selection in the underlying control. RmlUi's <select>
        // tracks its current option through the "value" attribute, matched
        // against each option's value (set in add_option below).
        set_input_text(std::to_string(new_index));

        if (trigger_callbacks) {
            for (const auto &function : index_changed_callbacks) {
                function(index);
            }
        }
    }

    void Dropdown::set_input_value(const ElementValue &val) {
        std::visit(overloaded {
            [this](uint32_t u) { set_index(u); },
            [this](float f) { set_index(uint32_t(f)); },
            [this](double d) { set_index(uint32_t(d)); },
            [](std::monostate) {}
        }, val);
    }

    void Dropdown::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Text: {
            // The Change handler delivers the select's "value" attribute, which
            // is the chosen option's index encoded as a string.
            const EventText &event = std::get<EventText>(e.variant);
            const std::string &text = event.text;
            uint32_t new_index = 0;
            auto result = std::from_chars(text.data(), text.data() + text.size(), new_index);
            if (result.ec == std::errc() && new_index != index) {
                index = new_index;
                for (const auto &function : index_changed_callbacks) {
                    function(index);
                }
            }
            break;
        }
        case EventType::Focus: {
            const EventFocus &event = std::get<EventFocus>(e.variant);
            if (focus_callback != nullptr) {
                focus_callback(event.active);
            }
            break;
        }
        default:
            break;
        }
    }

    void Dropdown::add_option(std::string_view name) {
        Element *option = get_current_context().create_element<Element>(this, 0u, std::string("option"), true);
        option->set_text(name);
        // Encode the option's index as its value so the change event reports it.
        option->set_input_text(std::to_string(options.size()));
        options.emplace_back(option);

        // The first option added is the default selection.
        if (options.size() == 1) {
            set_index_internal(0, false);
        }
    }

    void Dropdown::set_index(uint32_t index) {
        set_index_internal(index, false);
    }

    uint32_t Dropdown::get_index() const {
        return index;
    }

    void Dropdown::add_index_changed_callback(std::function<void(uint32_t)> callback) {
        index_changed_callbacks.emplace_back(callback);
    }

    void Dropdown::set_focus_callback(std::function<void(bool)> callback) {
        focus_callback = callback;
    }

};
