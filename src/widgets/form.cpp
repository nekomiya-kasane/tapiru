/**
 * @file form.cpp
 * @brief Form component implementation with field validation.
 */

#include "tapiru/widgets/form.h"

#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/widgets/builders.h"

#include <algorithm>

namespace tapiru {

    namespace {

        class form_component : public component_base {
          public:
            explicit form_component(form_option opt) : opt_(std::move(opt)) {
                int n = static_cast<int>(opt_.fields.size());
                errors_.resize(static_cast<size_t>(n), false);
                cursor_positions_.resize(static_cast<size_t>(n), 0);
                // Initialize cursor positions from existing values
                for (int i = 0; i < n; ++i) {
                    if (opt_.fields[static_cast<size_t>(i)].value) {
                        cursor_positions_[static_cast<size_t>(i)] =
                            static_cast<int>(opt_.fields[static_cast<size_t>(i)].value->size());
                    }
                }
            }

            element render() override {
                int n = static_cast<int>(opt_.fields.size());
                if (n == 0) {
                    return text_builder("[dim]Empty form[/]");
                }

                rows_builder rows;
                rows.gap(0);

                for (int i = 0; i < n; ++i) {
                    const auto &f = opt_.fields[static_cast<size_t>(i)];
                    bool focused = (i == focused_field_);
                    bool has_error = errors_[static_cast<size_t>(i)];

                    // Label row
                    std::string label_markup = f.label + ":";
                    if (focused) {
                        label_markup = "[bold]" + f.label + ":[/]";
                    }
                    rows.add(text_builder(label_markup));

                    // Input row: show value with cursor indicator
                    std::string val = f.value ? *f.value : "";
                    std::string input_display;
                    if (focused) {
                        // Show cursor position with brackets
                        int cp = cursor_positions_[static_cast<size_t>(i)];
                        if (cp > static_cast<int>(val.size())) {
                            cp = static_cast<int>(val.size());
                        }
                        input_display =
                            "  > " + val.substr(0, static_cast<size_t>(cp)) + "|" + val.substr(static_cast<size_t>(cp));
                    } else {
                        input_display = "    " + val;
                    }

                    // Pad to input_width
                    while (static_cast<int>(input_display.size()) < static_cast<int>(opt_.input_width) + 4) {
                        input_display += ' ';
                    }

                    auto input_tb = text_builder(input_display);
                    if (has_error) {
                        input_tb.style_override(opt_.error_sty.fg.kind != color_kind::default_color
                                                    ? opt_.error_sty
                                                    : style{color::from_rgb(255, 80, 80), color::default_color()});
                    } else if (focused) {
                        input_tb.style_override(opt_.focused_sty.fg.kind != color_kind::default_color
                                                    ? opt_.focused_sty
                                                    : style{color::from_rgb(100, 200, 255), color::default_color()});
                    } else {
                        if (opt_.input_sty.fg.kind != color_kind::default_color) {
                            input_tb.style_override(opt_.input_sty);
                        }
                    }
                    rows.add(std::move(input_tb));

                    // Error message row
                    if (has_error && !f.error_message.empty()) {
                        std::string err = "    [red]" + f.error_message + "[/]";
                        rows.add(text_builder(err));
                    }
                }

                // Submit hint
                rows.add(text_builder(""));
                if (submit_blocked_) {
                    rows.add(text_builder("[red]Fix errors before submitting[/]"));
                } else {
                    rows.add(text_builder("[dim]Enter: submit | Tab: next field | Shift+Tab: prev[/]"));
                }

                return std::move(rows);
            }

            bool on_event(const input_event &ev) override {
                auto *ke = std::get_if<key_event>(&ev);
                if (!ke) {
                    return false;
                }

                int n = static_cast<int>(opt_.fields.size());
                if (n == 0) {
                    return false;
                }

                // Tab navigation
                if (ke->key == special_key::tab) {
                    validate_current();
                    if (has_mod(*ke, key_mod::shift)) {
                        focused_field_ = (focused_field_ - 1 + n) % n;
                    } else {
                        focused_field_ = (focused_field_ + 1) % n;
                    }
                    return true;
                }

                // Enter: attempt submit
                if (ke->key == special_key::enter) {
                    return try_submit();
                }

                // Editing the current field
                auto &f = opt_.fields[static_cast<size_t>(focused_field_)];
                if (!f.value) {
                    return false;
                }
                auto &val = *f.value;
                int &cp = cursor_positions_[static_cast<size_t>(focused_field_)];
                if (cp > static_cast<int>(val.size())) {
                    cp = static_cast<int>(val.size());
                }

                if (ke->key == special_key::backspace) {
                    if (cp > 0) {
                        val.erase(static_cast<size_t>(cp - 1), 1);
                        --cp;
                        errors_[static_cast<size_t>(focused_field_)] = false;
                        submit_blocked_ = false;
                    }
                    return true;
                }
                if (ke->key == special_key::delete_) {
                    if (cp < static_cast<int>(val.size())) {
                        val.erase(static_cast<size_t>(cp), 1);
                        errors_[static_cast<size_t>(focused_field_)] = false;
                        submit_blocked_ = false;
                    }
                    return true;
                }
                if (ke->key == special_key::left) {
                    if (cp > 0) {
                        --cp;
                    }
                    return true;
                }
                if (ke->key == special_key::right) {
                    if (cp < static_cast<int>(val.size())) {
                        ++cp;
                    }
                    return true;
                }
                if (ke->key == special_key::home) {
                    cp = 0;
                    return true;
                }
                if (ke->key == special_key::end) {
                    cp = static_cast<int>(val.size());
                    return true;
                }

                // Printable character
                if (ke->codepoint >= 32 && ke->codepoint < 127) {
                    val.insert(val.begin() + cp, static_cast<char>(ke->codepoint));
                    ++cp;
                    errors_[static_cast<size_t>(focused_field_)] = false;
                    submit_blocked_ = false;
                    return true;
                }

                return false;
            }

          private:
            static bool has_mod(const key_event &ke, key_mod m) {
                return (static_cast<uint8_t>(ke.mods) & static_cast<uint8_t>(m)) != 0;
            }

            void validate_current() {
                int i = focused_field_;
                const auto &f = opt_.fields[static_cast<size_t>(i)];
                if (f.validator && f.value) {
                    errors_[static_cast<size_t>(i)] = !f.validator(*f.value);
                }
            }

            bool try_submit() {
                int n = static_cast<int>(opt_.fields.size());
                bool all_valid = true;
                for (int i = 0; i < n; ++i) {
                    const auto &f = opt_.fields[static_cast<size_t>(i)];
                    if (f.validator && f.value) {
                        bool ok = f.validator(*f.value);
                        errors_[static_cast<size_t>(i)] = !ok;
                        if (!ok) {
                            all_valid = false;
                        }
                    }
                }
                if (all_valid) {
                    submit_blocked_ = false;
                    if (opt_.on_submit) {
                        opt_.on_submit();
                    }
                    return true;
                }
                submit_blocked_ = true;
                // Focus first error field
                for (int i = 0; i < n; ++i) {
                    if (errors_[static_cast<size_t>(i)]) {
                        focused_field_ = i;
                        break;
                    }
                }
                return true;
            }

            form_option opt_;
            int focused_field_ = 0;
            std::vector<bool> errors_;
            std::vector<int> cursor_positions_;
            bool submit_blocked_ = false;
        };

    } // namespace

    component make_form(form_option opt) {
        return std::make_shared<form_component>(std::move(opt));
    }

} // namespace tapiru
