/**
 * @file component_factories.cpp
 * @brief Component factory implementations wrapping interactive builders.
 */

#include "tapiru/widgets/component_factories.h"

#include "tapiru/widgets/interactive.h"

#include <algorithm>

namespace tapiru {

// ── button_component ────────────────────────────────────────────────────

namespace {

class button_component final : public component_base {
  public:
    explicit button_component(button_option opt) : opt_(std::move(opt)) {}

    element render() override {
        auto bb = button_builder(opt_.label);
        if (opt_.on_click) {
            bb.on_click(opt_.on_click);
        }
        bb.focused(focused());
        if (!opt_.sty.is_default()) {
            bb.style_override(opt_.sty);
        }
        return element(std::move(bb));
    }

    bool on_event(const input_event &ev) override {
        if (auto *ke = std::get_if<key_event>(&ev)) {
            if (ke->key == special_key::enter && focused()) {
                if (opt_.on_click) {
                    opt_.on_click();
                }
                return true;
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

  private:
    button_option opt_;
};

// ── checkbox_component ──────────────────────────────────────────────────

class checkbox_component final : public component_base {
  public:
    explicit checkbox_component(checkbox_option opt) : opt_(std::move(opt)) {}

    element render() override {
        auto cb = checkbox_builder(opt_.label, opt_.value);
        cb.focused(focused());
        if (!opt_.sty.is_default()) {
            cb.style_override(opt_.sty);
        }
        return element(std::move(cb));
    }

    bool on_event(const input_event &ev) override {
        if (auto *ke = std::get_if<key_event>(&ev)) {
            if ((ke->key == special_key::enter || ke->codepoint == U' ') && focused()) {
                if (opt_.value) {
                    *opt_.value = !*opt_.value;
                }
                return true;
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

  private:
    checkbox_option opt_;
};

// ── radio_group_component ───────────────────────────────────────────────

class radio_group_component final : public component_base {
  public:
    explicit radio_group_component(radio_group_option opt) : opt_(std::move(opt)) {}

    element render() override {
        int sel = opt_.selected ? *opt_.selected : 0;
        auto rb = radio_group_builder(opt_.options, opt_.selected);
        rb.focused_index(focused() ? sel : -1);
        if (!opt_.sty.is_default()) {
            rb.style_override(opt_.sty);
        }
        return element(std::move(rb));
    }

    bool on_event(const input_event &ev) override {
        if (!opt_.selected) {
            return false;
        }
        if (auto *ke = std::get_if<key_event>(&ev)) {
            int n = static_cast<int>(opt_.options.size());
            if (n == 0) {
                return false;
            }
            if (ke->key == special_key::up || ke->key == special_key::left) {
                *opt_.selected = (*opt_.selected - 1 + n) % n;
                return true;
            }
            if (ke->key == special_key::down || ke->key == special_key::right) {
                *opt_.selected = (*opt_.selected + 1) % n;
                return true;
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

  private:
    radio_group_option opt_;
};

// ── selectable_list_component ───────────────────────────────────────────

class selectable_list_component final : public component_base {
  public:
    explicit selectable_list_component(selectable_list_option opt) : opt_(std::move(opt)) {}

    element render() override {
        auto lb = selectable_list_builder(opt_.items, opt_.cursor);
        if (opt_.visible_count > 0) {
            lb.visible_count(opt_.visible_count);
        }
        if (!opt_.sty.is_default()) {
            lb.style_override(opt_.sty);
        }
        if (!opt_.highlight_sty.is_default()) {
            lb.highlight_style(opt_.highlight_sty);
        }
        return element(std::move(lb));
    }

    bool on_event(const input_event &ev) override {
        if (!opt_.cursor) {
            return false;
        }
        if (auto *ke = std::get_if<key_event>(&ev)) {
            int n = static_cast<int>(opt_.items.size());
            if (n == 0) {
                return false;
            }
            if (ke->key == special_key::up) {
                *opt_.cursor = std::max(0, *opt_.cursor - 1);
                return true;
            }
            if (ke->key == special_key::down) {
                *opt_.cursor = std::min(n - 1, *opt_.cursor + 1);
                return true;
            }
            if (ke->key == special_key::enter) {
                if (opt_.on_select) {
                    opt_.on_select(*opt_.cursor);
                }
                return true;
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

  private:
    selectable_list_option opt_;
};

// ── text_input_component ────────────────────────────────────────────────

class text_input_component final : public component_base {
  public:
    explicit text_input_component(text_input_option opt) : opt_(std::move(opt)), cursor_pos_(0) {}

    element render() override {
        auto tb = text_input_builder(opt_.buffer);
        if (!opt_.placeholder.empty()) {
            tb.placeholder(opt_.placeholder);
        }
        tb.width(opt_.width);
        tb.focused(focused());
        tb.cursor_pos(cursor_pos_);
        if (!opt_.sty.is_default()) {
            tb.style_override(opt_.sty);
        }
        return element(std::move(tb));
    }

    bool on_event(const input_event &ev) override {
        if (!opt_.buffer || !focused()) {
            return false;
        }
        if (auto *ke = std::get_if<key_event>(&ev)) {
            auto &buf = *opt_.buffer;
            if (ke->key == special_key::backspace && cursor_pos_ > 0) {
                buf.erase(cursor_pos_ - 1, 1);
                cursor_pos_--;
                if (opt_.on_change) {
                    opt_.on_change(buf);
                }
                return true;
            }
            if (ke->key == special_key::delete_ && cursor_pos_ < buf.size()) {
                buf.erase(cursor_pos_, 1);
                if (opt_.on_change) {
                    opt_.on_change(buf);
                }
                return true;
            }
            if (ke->key == special_key::left && cursor_pos_ > 0) {
                cursor_pos_--;
                return true;
            }
            if (ke->key == special_key::right && cursor_pos_ < buf.size()) {
                cursor_pos_++;
                return true;
            }
            if (ke->key == special_key::home) {
                cursor_pos_ = 0;
                return true;
            }
            if (ke->key == special_key::end) {
                cursor_pos_ = static_cast<uint32_t>(buf.size());
                return true;
            }
            if (ke->codepoint >= 0x20 && ke->key == special_key::none) {
                // Insert printable character
                char utf8[4];
                int len = 0;
                char32_t cp = ke->codepoint;
                if (cp < 0x80) {
                    utf8[0] = static_cast<char>(cp);
                    len = 1;
                } else if (cp < 0x800) {
                    utf8[0] = static_cast<char>(0xC0 | (cp >> 6));
                    utf8[1] = static_cast<char>(0x80 | (cp & 0x3F));
                    len = 2;
                } else if (cp < 0x10000) {
                    utf8[0] = static_cast<char>(0xE0 | (cp >> 12));
                    utf8[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    utf8[2] = static_cast<char>(0x80 | (cp & 0x3F));
                    len = 3;
                } else {
                    utf8[0] = static_cast<char>(0xF0 | (cp >> 18));
                    utf8[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                    utf8[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    utf8[3] = static_cast<char>(0x80 | (cp & 0x3F));
                    len = 4;
                }
                buf.insert(cursor_pos_, utf8, static_cast<size_t>(len));
                cursor_pos_ += static_cast<uint32_t>(len);
                if (opt_.on_change) {
                    opt_.on_change(buf);
                }
                return true;
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

  private:
    text_input_option opt_;
    uint32_t cursor_pos_;
};

// ── slider_component ────────────────────────────────────────────────────

class slider_component final : public component_base {
  public:
    explicit slider_component(slider_option opt) : opt_(std::move(opt)) {}

    element render() override {
        auto sb = slider_builder(opt_.value, opt_.min_val, opt_.max_val);
        sb.width(opt_.width);
        sb.focused(focused());
        if (!opt_.sty.is_default()) {
            sb.style_override(opt_.sty);
        }
        return element(std::move(sb));
    }

    bool on_event(const input_event &ev) override {
        if (!opt_.value || !focused()) {
            return false;
        }
        if (auto *ke = std::get_if<key_event>(&ev)) {
            if (ke->key == special_key::right || ke->key == special_key::up) {
                *opt_.value = std::min(opt_.max_val, *opt_.value + opt_.step);
                if (opt_.on_change) {
                    opt_.on_change(*opt_.value);
                }
                return true;
            }
            if (ke->key == special_key::left || ke->key == special_key::down) {
                *opt_.value = std::max(opt_.min_val, *opt_.value - opt_.step);
                if (opt_.on_change) {
                    opt_.on_change(*opt_.value);
                }
                return true;
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

  private:
    slider_option opt_;
};

} // anonymous namespace

// ── Factory functions ───────────────────────────────────────────────────

component make_button(button_option opt) {
    return std::make_shared<button_component>(std::move(opt));
}

component make_checkbox(checkbox_option opt) {
    return std::make_shared<checkbox_component>(std::move(opt));
}

component make_radio_group(radio_group_option opt) {
    return std::make_shared<radio_group_component>(std::move(opt));
}

component make_selectable_list(selectable_list_option opt) {
    return std::make_shared<selectable_list_component>(std::move(opt));
}

component make_text_input(text_input_option opt) {
    return std::make_shared<text_input_component>(std::move(opt));
}

component make_slider(slider_option opt) {
    return std::make_shared<slider_component>(std::move(opt));
}

} // namespace tapiru
