/**
 * @file theme.cpp
 * @brief Theme/style sheet implementation with preset themes.
 */

#include "tapiru/core/theme.h"

namespace tapiru {

    void theme::define(std::string name, const style &s) {
        styles_[std::move(name)] = s;
    }

    const style *theme::lookup(std::string_view name) const {
        auto it = styles_.find(std::string(name));
        return (it != styles_.end()) ? &it->second : nullptr;
    }

    bool theme::has(std::string_view name) const {
        return styles_.find(std::string(name)) != styles_.end();
    }

    void theme::remove(std::string_view name) {
        styles_.erase(std::string(name));
    }

    void theme::clear() {
        styles_.clear();
    }

    // ── Preset: dark ───────────────────────────────────────────────────────

    theme theme::dark() {
        theme th;

        th.define("text", {color::from_rgb(200, 200, 200), color::default_color(), attr::none});
        th.define("danger", {color::from_rgb(255, 85, 85), color::default_color(), attr::bold});
        th.define("error", {color::from_rgb(255, 85, 85), color::default_color(), attr::bold});
        th.define("warning", {color::from_rgb(255, 170, 0), color::default_color(), attr::bold});
        th.define("success", {color::from_rgb(80, 250, 123), color::default_color(), attr::none});
        th.define("info", {color::from_rgb(139, 233, 253), color::default_color(), attr::none});
        th.define("muted", {color::from_rgb(100, 100, 100), color::default_color(), attr::dim});
        th.define("accent", {color::from_rgb(189, 147, 249), color::default_color(), attr::none});
        th.define("highlight", {color::from_rgb(241, 250, 140), color::default_color(), attr::none});
        th.define("title", {color::from_rgb(255, 255, 255), color::default_color(), attr::bold});
        th.define("link", {color::from_rgb(80, 250, 123), color::default_color(), attr::bold});

        return th;
    }

    // ── Preset: light ──────────────────────────────────────────────────────

    theme theme::light() {
        theme th;

        th.define("text", {color::from_rgb(40, 40, 40), color::default_color(), attr::none});
        th.define("danger", {color::from_rgb(200, 0, 0), color::default_color(), attr::bold});
        th.define("error", {color::from_rgb(200, 0, 0), color::default_color(), attr::bold});
        th.define("warning", {color::from_rgb(180, 100, 0), color::default_color(), attr::bold});
        th.define("success", {color::from_rgb(0, 128, 0), color::default_color(), attr::none});
        th.define("info", {color::from_rgb(0, 100, 180), color::default_color(), attr::none});
        th.define("muted", {color::from_rgb(150, 150, 150), color::default_color(), attr::dim});
        th.define("accent", {color::from_rgb(128, 0, 128), color::default_color(), attr::none});
        th.define("highlight", {color::from_rgb(180, 140, 0), color::default_color(), attr::none});
        th.define("title", {color::from_rgb(0, 0, 0), color::default_color(), attr::bold});
        th.define("link", {color::from_rgb(0, 0, 200), color::default_color(), attr::underline});

        return th;
    }

    // ── Preset: monokai ────────────────────────────────────────────────────

    theme theme::monokai() {
        theme th;

        th.define("text", {color::from_rgb(248, 248, 242), color::default_color(), attr::none});
        th.define("danger", {color::from_rgb(249, 38, 114), color::default_color(), attr::bold});
        th.define("error", {color::from_rgb(249, 38, 114), color::default_color(), attr::bold});
        th.define("warning", {color::from_rgb(230, 219, 116), color::default_color(), attr::bold});
        th.define("success", {color::from_rgb(166, 226, 46), color::default_color(), attr::none});
        th.define("info", {color::from_rgb(102, 217, 239), color::default_color(), attr::none});
        th.define("muted", {color::from_rgb(117, 113, 94), color::default_color(), attr::dim});
        th.define("accent", {color::from_rgb(174, 129, 255), color::default_color(), attr::none});
        th.define("highlight", {color::from_rgb(230, 219, 116), color::default_color(), attr::none});
        th.define("title", {color::from_rgb(248, 248, 242), color::default_color(), attr::bold});
        th.define("link", {color::from_rgb(102, 217, 239), color::default_color(), attr::underline});

        return th;
    }

} // namespace tapiru
