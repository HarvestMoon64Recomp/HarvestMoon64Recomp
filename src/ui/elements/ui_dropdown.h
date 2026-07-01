#pragma once

#include "ui_element.h"

namespace recompui {

    class Dropdown : public Element {
    private:
        std::vector<Element *> options;
        uint32_t index = 0;
        std::vector<std::function<void(uint32_t)>> index_changed_callbacks;
        std::function<void(bool)> focus_callback = nullptr;

        void set_index_internal(uint32_t index, bool trigger_callbacks);
        void set_input_value(const ElementValue &val) override;
        ElementValue get_element_value() override { return get_index(); }
    protected:
        virtual void process_event(const Event &e) override;
        std::string_view get_type_name() override { return "Dropdown"; }
    public:
        Dropdown(Element *parent);
        virtual ~Dropdown();
        void add_option(std::string_view name);
        void set_index(uint32_t index);
        uint32_t get_index() const;
        void add_index_changed_callback(std::function<void(uint32_t)> callback);
        void set_focus_callback(std::function<void(bool)> callback);
        size_t num_options() const { return options.size(); }
    };

} // namespace recompui
