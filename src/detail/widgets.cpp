/**
 * @file widgets.cpp
 * @brief Widget measure/render implementations for all widget types.
 */

#include "detail/widget_registry.h"
#include "tapiru/core/cell.h"
#include "tapiru/core/unicode_width.h"

#include <algorithm>
#include <string>

namespace tapiru::detail {

    using tapiru::cell;

    namespace {

        // ── Helpers ─────────────────────────────────────────────────────────────

        /** @brief Encode a char32_t to UTF-8 and append to string. */
        void append_utf8(std::string &out, char32_t cp) {
            if (cp < 0x80) {
                out += static_cast<char>(cp);
            } else if (cp < 0x800) {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }

        /** @brief A single visual line: a slice of fragments with no embedded newlines. */
        using frag_line = std::vector<text_fragment>;

        /** @brief Split fragments by '\n' into separate lines. */
        std::vector<frag_line> split_into_lines(const std::vector<text_fragment> &frags) {
            std::vector<frag_line> lines;
            lines.emplace_back();
            for (const auto &f : frags) {
                size_t start = 0;
                while (start < f.text.size()) {
                    auto nl = f.text.find('\n', start);
                    if (nl == std::string::npos) {
                        lines.back().push_back({f.text.substr(start), f.sty});
                        break;
                    }
                    if (nl > start) {
                        lines.back().push_back({f.text.substr(start, nl - start), f.sty});
                    }
                    lines.emplace_back(); // start new line
                    start = nl + 1;
                }
            }
            return lines;
        }

        /** @brief Compute total display width of fragments (single line, no newlines expected). */
        uint32_t fragments_width(const std::vector<text_fragment> &frags) {
            uint32_t w = 0;
            for (const auto &f : frags) {
                w += static_cast<uint32_t>(string_width(f.text));
            }
            return w;
        }

        /** @brief Compute max width and line count from pre-split lines. */
        std::pair<uint32_t, uint32_t> lines_dimensions(const std::vector<frag_line> &lines) {
            uint32_t max_w = 0;
            for (const auto &line : lines) {
                max_w = std::max(max_w, fragments_width(line));
            }
            return {max_w, static_cast<uint32_t>(lines.size())};
        }

        /** @brief Write a codepoint into canvas at (x, y) with given style_id, width, and alpha. */
        void canvas_put(canvas &c, uint32_t x, uint32_t y, char32_t cp, style_id sid, uint8_t w, uint8_t alpha = 255) {
            if (x < c.width() && y < c.height()) {
                c.set(x, y, cell{cp, sid, w, alpha});
            }
        }

        /** @brief Write a string of fragments into canvas starting at (x, y), clipping to max_w. Returns columns
         * consumed. */
        uint32_t render_fragments_line(const std::vector<text_fragment> &frags, canvas &c, uint32_t x, uint32_t y,
                                       uint32_t max_w, const style_table &styles) {
            uint32_t col = 0;
            for (const auto &frag : frags) {
                auto sid = const_cast<style_table &>(styles).intern(frag.sty);
                const char *p = frag.text.data();
                size_t rem = frag.text.size();
                while (rem > 0 && col < max_w) {
                    char32_t cp;
                    int bytes = utf8_decode(p, rem, cp);
                    if (bytes == 0) {
                        break;
                    }
                    int cw = char_width(cp);
                    if (cw <= 0) {
                        p += bytes;
                        rem -= bytes;
                        continue;
                    }
                    if (col + static_cast<uint32_t>(cw) > max_w) {
                        break;
                    }
                    canvas_put(c, x + col, y, cp, sid, static_cast<uint8_t>(cw));
                    col += static_cast<uint32_t>(cw);
                    p += bytes;
                    rem -= bytes;
                }
            }
            return col;
        }

        /** @brief Fill a horizontal span with a character. */
        void fill_h(canvas &c, uint32_t x, uint32_t y, uint32_t w, char32_t ch, style_id sid, uint8_t alpha = 255) {
            for (uint32_t i = 0; i < w; ++i) {
                canvas_put(c, x + i, y, ch, sid, 1, alpha);
            }
        }

        /** @brief Render a box shadow/glow around a rect. Reusable by panel, menu, table. */
        void render_shadow(canvas &c, const style_table &styles, rect area, const shadow_config &sh) {
            style shadow_sty;
            shadow_sty.bg = sh.shadow_color;
            auto sh_sid = const_cast<style_table &>(styles).intern(shadow_sty);

            int bx = static_cast<int>(sh.blur) * 2;
            int by = static_cast<int>(sh.blur);
            int ox = static_cast<int>(sh.offset_x);
            int oy = static_cast<int>(sh.offset_y);

            int x0 = static_cast<int>(area.x) + ox - bx;
            int y0 = static_cast<int>(area.y) + oy - by;
            int x1 = static_cast<int>(area.x + area.w) + ox + bx;
            int y1 = static_cast<int>(area.y + area.h) + oy + by;

            for (int sy = y0; sy < y1; ++sy) {
                for (int sx = x0; sx < x1; ++sx) {
                    if (sx < 0 || sy < 0 || static_cast<uint32_t>(sx) >= c.width() ||
                        static_cast<uint32_t>(sy) >= c.height()) {
                        continue;
                    }

                    if (static_cast<uint32_t>(sx) >= area.x && static_cast<uint32_t>(sx) < area.x + area.w &&
                        static_cast<uint32_t>(sy) >= area.y && static_cast<uint32_t>(sy) < area.y + area.h) {
                        continue;
                    }

                    int shadow_x0 = static_cast<int>(area.x) + ox;
                    int shadow_y0 = static_cast<int>(area.y) + oy;
                    int shadow_x1 = shadow_x0 + static_cast<int>(area.w);
                    int shadow_y1 = shadow_y0 + static_cast<int>(area.h);

                    int dx = 0, dy = 0;
                    if (sx < shadow_x0) {
                        dx = shadow_x0 - sx;
                    } else if (sx >= shadow_x1) {
                        dx = sx - shadow_x1 + 1;
                    }
                    if (sy < shadow_y0) {
                        dy = shadow_y0 - sy;
                    } else if (sy >= shadow_y1) {
                        dy = sy - shadow_y1 + 1;
                    }

                    float dist = 0.0f;
                    if (bx > 0 || by > 0) {
                        float nx = (bx > 0) ? static_cast<float>(dx) / static_cast<float>(bx) : 0.0f;
                        float ny = (by > 0) ? static_cast<float>(dy) / static_cast<float>(by) : 0.0f;
                        dist = (nx > ny) ? nx : ny;
                    }

                    if (dist >= 1.0f) {
                        continue;
                    }

                    uint8_t alpha = static_cast<uint8_t>(static_cast<float>(sh.opacity) * (1.0f - dist));
                    if (alpha == 0) {
                        continue;
                    }

                    c.set_blended(static_cast<uint32_t>(sx), static_cast<uint32_t>(sy), cell{U' ', sh_sid, 1, alpha},
                                  const_cast<style_table &>(styles));
                }
            }
        }

    } // anonymous namespace

    // ── Text ────────────────────────────────────────────────────────────────

    measurement measure_text(const scene &s, node_id id, box_constraints bc) {
        const auto &td = s.get_text(s.pool_of(id));
        auto lines = split_into_lines(td.fragments);
        auto [max_w, nlines] = lines_dimensions(lines);

        if (td.overflow == overflow_mode::wrap && max_w > bc.max_w && bc.max_w > 0) {
            uint32_t total_lines = 0;
            for (const auto &line : lines) {
                uint32_t lw = fragments_width(line);
                total_lines += lw > 0 ? (lw + bc.max_w - 1) / bc.max_w : 1;
            }
            return bc.constrain({bc.max_w, total_lines});
        }

        return bc.constrain({max_w, nlines});
    }

    void render_text(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &s = ctx.sc;
        auto &c = ctx.cv;
        const auto &styles = ctx.styles;
        const auto &td = s.get_text(s.pool_of(id));
        auto lines = split_into_lines(td.fragments);
        uint32_t avail = area.w;

        for (uint32_t row = 0; row < static_cast<uint32_t>(lines.size()) && row < area.h; ++row) {
            const auto &line = lines[row];
            uint32_t line_w = fragments_width(line);
            uint32_t offset = 0;

            switch (td.align) {
            case justify::left:
                offset = 0;
                break;
            case justify::center:
                offset = line_w < avail ? (avail - line_w) / 2 : 0;
                break;
            case justify::right:
                offset = line_w < avail ? avail - line_w : 0;
                break;
            default:
                offset = 0;
                break;
            }

            (void)render_fragments_line(line, c, area.x + offset, area.y + row, avail - offset, styles);
        }
    }

    // ── Panel ───────────────────────────────────────────────────────────────

    measurement measure_panel(const scene &s, node_id id, box_constraints bc) {
        const auto &pd = s.get_panel(s.pool_of(id));
        bool has_border = pd.border != border_style::none;
        uint32_t border_h = has_border ? 2 : 0;
        uint32_t border_w = has_border ? 2 : 0;

        measurement child_m{0, 0};
        if (pd.content != no_node) {
            box_constraints child_bc = bc;
            if (child_bc.max_w > border_w) {
                child_bc.max_w -= border_w;
            } else {
                child_bc.max_w = 0;
            }
            if (child_bc.max_h > border_h) {
                child_bc.max_h -= border_h;
            } else {
                child_bc.max_h = 0;
            }
            if (child_bc.min_w > child_bc.max_w) {
                child_bc.min_w = child_bc.max_w;
            }
            if (child_bc.min_h > child_bc.max_h) {
                child_bc.min_h = child_bc.max_h;
            }
            child_m = dispatch_measure(s, pd.content, child_bc);
        }

        return bc.constrain({child_m.width + border_w, child_m.height + border_h});
    }

    void render_panel(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &s = ctx.sc;
        auto &c = ctx.cv;
        const auto &styles = ctx.styles;
        const auto &pd = s.get_panel(s.pool_of(id));
        bool has_border = pd.border != border_style::none;

        // Shadow/glow pass (rendered before panel so panel overwrites overlap)
        if (pd.shadow) {
            const auto &sh = *pd.shadow;
            style shadow_sty;
            shadow_sty.bg = sh.shadow_color;
            auto sh_sid = const_cast<style_table &>(styles).intern(shadow_sty);

            // Aspect-ratio-aware blur extents: x is doubled for ~2:1 terminal cells
            int bx = static_cast<int>(sh.blur) * 2; // horizontal blur extent
            int by = static_cast<int>(sh.blur);     // vertical blur extent

            int ox = static_cast<int>(sh.offset_x);
            int oy = static_cast<int>(sh.offset_y);

            // Scan a region around the panel offset by shadow direction
            int x0 = static_cast<int>(area.x) + ox - bx;
            int y0 = static_cast<int>(area.y) + oy - by;
            int x1 = static_cast<int>(area.x + area.w) + ox + bx;
            int y1 = static_cast<int>(area.y + area.h) + oy + by;

            for (int sy = y0; sy < y1; ++sy) {
                for (int sx = x0; sx < x1; ++sx) {
                    if (sx < 0 || sy < 0 || static_cast<uint32_t>(sx) >= c.width() ||
                        static_cast<uint32_t>(sy) >= c.height()) {
                        continue;
                    }

                    // Skip cells that are inside the panel rect (panel will overwrite)
                    if (static_cast<uint32_t>(sx) >= area.x && static_cast<uint32_t>(sx) < area.x + area.w &&
                        static_cast<uint32_t>(sy) >= area.y && static_cast<uint32_t>(sy) < area.y + area.h) {
                        continue;
                    }

                    // Distance from shadow rect edge (in cells)
                    int shadow_x0 = static_cast<int>(area.x) + ox;
                    int shadow_y0 = static_cast<int>(area.y) + oy;
                    int shadow_x1 = shadow_x0 + static_cast<int>(area.w);
                    int shadow_y1 = shadow_y0 + static_cast<int>(area.h);

                    int dx = 0, dy = 0;
                    if (sx < shadow_x0) {
                        dx = shadow_x0 - sx;
                    } else if (sx >= shadow_x1) {
                        dx = sx - shadow_x1 + 1;
                    }
                    if (sy < shadow_y0) {
                        dy = shadow_y0 - sy;
                    } else if (sy >= shadow_y1) {
                        dy = sy - shadow_y1 + 1;
                    }

                    // Normalize dx for aspect ratio (bx is already doubled)
                    float dist = 0.0f;
                    if (bx > 0 || by > 0) {
                        float nx = (bx > 0) ? static_cast<float>(dx) / static_cast<float>(bx) : 0.0f;
                        float ny = (by > 0) ? static_cast<float>(dy) / static_cast<float>(by) : 0.0f;
                        dist = (nx > ny) ? nx : ny;
                    }

                    if (dist >= 1.0f) {
                        continue;
                    }

                    uint8_t alpha = static_cast<uint8_t>(static_cast<float>(sh.opacity) * (1.0f - dist));
                    if (alpha == 0) {
                        continue;
                    }

                    c.set_blended(static_cast<uint32_t>(sx), static_cast<uint32_t>(sy), cell{U' ', sh_sid, 1, alpha},
                                  const_cast<style_table &>(styles));
                }
            }
        }

        if (has_border && area.w >= 2 && area.h >= 2) {
            auto bc = get_border_chars(pd.border);
            auto sid = const_cast<style_table &>(styles).intern(pd.border_sty);

            uint8_t a = pd.alpha;

            // Corners
            canvas_put(c, area.x, area.y, bc.top_left, sid, 1, a);
            canvas_put(c, area.x + area.w - 1, area.y, bc.top_right, sid, 1, a);
            canvas_put(c, area.x, area.y + area.h - 1, bc.bottom_left, sid, 1, a);
            canvas_put(c, area.x + area.w - 1, area.y + area.h - 1, bc.bottom_right, sid, 1, a);

            // Top/bottom edges
            fill_h(c, area.x + 1, area.y, area.w - 2, bc.horizontal, sid, a);
            fill_h(c, area.x + 1, area.y + area.h - 1, area.w - 2, bc.horizontal, sid, a);

            // Left/right edges
            for (uint32_t row = 1; row < area.h - 1; ++row) {
                canvas_put(c, area.x, area.y + row, bc.vertical, sid, 1, a);
                canvas_put(c, area.x + area.w - 1, area.y + row, bc.vertical, sid, 1, a);
            }

            // Title on top border
            if (!pd.title.empty() && area.w > 4) {
                auto title_sid = const_cast<style_table &>(styles).intern(pd.title_sty);
                // " Title " centered on top border
                uint32_t title_w = static_cast<uint32_t>(string_width(pd.title));
                uint32_t max_title = area.w - 4;
                if (title_w > max_title) {
                    title_w = max_title;
                }
                uint32_t tx = area.x + 2;

                // Space before title
                canvas_put(c, tx - 1, area.y, U' ', sid, 1, a);
                // Title text
                const char *p = pd.title.data();
                size_t rem = pd.title.size();
                uint32_t col = 0;
                while (rem > 0 && col < title_w) {
                    char32_t cp;
                    int bytes = utf8_decode(p, rem, cp);
                    if (bytes == 0) {
                        break;
                    }
                    int cw = char_width(cp);
                    if (cw <= 0) {
                        p += bytes;
                        rem -= bytes;
                        continue;
                    }
                    if (col + static_cast<uint32_t>(cw) > title_w) {
                        break;
                    }
                    canvas_put(c, tx + col, area.y, cp, title_sid, static_cast<uint8_t>(cw), a);
                    col += static_cast<uint32_t>(cw);
                    p += bytes;
                    rem -= bytes;
                }
                // Space after title
                if (tx + col < area.x + area.w - 1) {
                    canvas_put(c, tx + col, area.y, U' ', sid, 1, a);
                }
            }
        }

        // Background gradient fill (inside border)
        if (pd.bg_gradient) {
            rect inner = has_border ? area.inset(1, 1, 1, 1) : area;
            for (uint32_t gy = 0; gy < inner.h; ++gy) {
                for (uint32_t gx = 0; gx < inner.w; ++gx) {
                    auto gc = resolve_gradient(*pd.bg_gradient, inner.x + gx, inner.y + gy, inner);
                    style gs;
                    gs.bg = gc;
                    auto gsid = const_cast<style_table &>(styles).intern(gs);
                    canvas_put(c, inner.x + gx, inner.y + gy, U' ', gsid, 1, pd.alpha);
                }
            }
        }

        // Render content
        if (pd.content != no_node) {
            rect inner = has_border ? area.inset(1, 1, 1, 1) : area;
            dispatch_render(ctx, pd.content, inner);
        }

        // Shader post-process (after content, before border_join)
        if (pd.shader) {
            pd.shader(c, const_cast<style_table &>(styles), area, ctx.frame_time);
        }
    }

    // ── Rule ────────────────────────────────────────────────────────────────

    measurement measure_rule(const scene &s, node_id id, box_constraints bc) {
        (void)s;
        (void)id;
        return bc.constrain({bc.max_w, 1});
    }

    void render_rule(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &s = ctx.sc;
        auto &c = ctx.cv;
        const auto &styles = ctx.styles;
        const auto &rd = s.get_rule(s.pool_of(id));
        auto sid = const_cast<style_table &>(styles).intern(rd.rule_sty);

        if (rd.gradient) {
            // Gradient rule: each column gets a gradient-resolved color
            for (uint32_t i = 0; i < area.w; ++i) {
                auto gc = resolve_gradient(*rd.gradient, area.x + i, area.y, area);
                style gs = rd.rule_sty;
                gs.fg = gc;
                auto gsid = const_cast<style_table &>(styles).intern(gs);
                canvas_put(c, area.x + i, area.y, rd.ch, gsid, 1);
            }
        } else {
            fill_h(c, area.x, area.y, area.w, rd.ch, sid);
        }

        // Title overlay
        if (!rd.title.empty() && area.w > 4) {
            uint32_t title_w = static_cast<uint32_t>(string_width(rd.title));
            uint32_t max_title = area.w - 4;
            if (title_w > max_title) {
                title_w = max_title;
            }

            uint32_t tx;
            switch (rd.align) {
            case justify::left:
                tx = area.x + 1;
                break;
            case justify::right:
                tx = area.x + area.w - title_w - 2;
                break;
            case justify::center:
            default:
                tx = area.x + (area.w - title_w - 2) / 2 + 1;
                break;
            }

            canvas_put(c, tx - 1, area.y, U' ', sid, 1);
            const char *p = rd.title.data();
            size_t rem = rd.title.size();
            uint32_t col = 0;
            while (rem > 0 && col < title_w) {
                char32_t cp;
                int bytes = utf8_decode(p, rem, cp);
                if (bytes == 0) {
                    break;
                }
                int cw = char_width(cp);
                if (cw <= 0) {
                    p += bytes;
                    rem -= bytes;
                    continue;
                }
                if (col + static_cast<uint32_t>(cw) > title_w) {
                    break;
                }
                canvas_put(c, tx + col, area.y, cp, sid, static_cast<uint8_t>(cw));
                col += static_cast<uint32_t>(cw);
                p += bytes;
                rem -= bytes;
            }
            if (tx + col < area.x + area.w) {
                canvas_put(c, tx + col, area.y, U' ', sid, 1);
            }
        }
    }

    // ── Padding ─────────────────────────────────────────────────────────────

    measurement measure_padding(const scene &s, node_id id, box_constraints bc) {
        const auto &pd = s.get_padding(s.pool_of(id));
        uint32_t pad_w = pd.left + pd.right;
        uint32_t pad_h = pd.top + pd.bottom;

        measurement child_m{0, 0};
        if (pd.content != no_node) {
            box_constraints child_bc = bc;
            if (child_bc.max_w > pad_w) {
                child_bc.max_w -= pad_w;
            } else {
                child_bc.max_w = 0;
            }
            if (child_bc.max_h > pad_h) {
                child_bc.max_h -= pad_h;
            } else {
                child_bc.max_h = 0;
            }
            if (child_bc.min_w > child_bc.max_w) {
                child_bc.min_w = child_bc.max_w;
            }
            if (child_bc.min_h > child_bc.max_h) {
                child_bc.min_h = child_bc.max_h;
            }
            child_m = dispatch_measure(s, pd.content, child_bc);
        }

        return bc.constrain({child_m.width + pad_w, child_m.height + pad_h});
    }

    void render_padding(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &pd = ctx.sc.get_padding(ctx.sc.pool_of(id));

        if (pd.content != no_node) {
            rect inner = area.inset(pd.top, pd.right, pd.bottom, pd.left);
            dispatch_render(ctx, pd.content, inner);
        }
    }

    // ── Columns ─────────────────────────────────────────────────────────────

    measurement measure_columns(const scene &s, node_id id, box_constraints bc) {
        const auto &cd = s.get_columns(s.pool_of(id));
        uint32_t n = static_cast<uint32_t>(cd.children.size());
        if (n == 0) {
            return bc.constrain({0, 0});
        }

        uint32_t total_gap = (n - 1) * cd.gap;
        uint32_t avail = bc.max_w > total_gap ? bc.max_w - total_gap : 0;

        // First pass: measure auto-sized columns
        uint32_t total_flex = 0;
        uint32_t auto_width = 0;
        std::vector<uint32_t> widths(n, 0);

        for (uint32_t i = 0; i < n; ++i) {
            uint32_t flex = i < cd.flex_ratios.size() ? cd.flex_ratios[i] : 0;
            if (flex > 0) {
                total_flex += flex;
            } else {
                auto child_bc = box_constraints::loose(avail, bc.max_h);
                auto m = dispatch_measure(s, cd.children[i], child_bc);
                widths[i] = m.width;
                auto_width += m.width;
            }
        }

        // Second pass: distribute remaining space to flex columns
        uint32_t flex_avail = avail > auto_width ? avail - auto_width : 0;
        if (total_flex > 0) {
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t flex = i < cd.flex_ratios.size() ? cd.flex_ratios[i] : 0;
                if (flex > 0) {
                    widths[i] = flex_avail * flex / total_flex;
                }
            }
        }

        // Compute total width and max height
        uint32_t total_w = total_gap;
        uint32_t max_h = 0;
        for (uint32_t i = 0; i < n; ++i) {
            total_w += widths[i];
            auto child_bc = box_constraints::loose(widths[i], bc.max_h);
            auto m = dispatch_measure(s, cd.children[i], child_bc);
            max_h = std::max(max_h, m.height);
        }

        return bc.constrain({total_w, max_h});
    }

    void render_columns(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &s = ctx.sc;
        const auto &cd = s.get_columns(s.pool_of(id));
        uint32_t n = static_cast<uint32_t>(cd.children.size());
        if (n == 0) {
            return;
        }

        uint32_t total_gap = (n - 1) * cd.gap;
        uint32_t avail = area.w > total_gap ? area.w - total_gap : 0;

        // Same width calculation as measure
        uint32_t total_flex = 0;
        uint32_t auto_width = 0;
        std::vector<uint32_t> widths(n, 0);

        for (uint32_t i = 0; i < n; ++i) {
            uint32_t flex = i < cd.flex_ratios.size() ? cd.flex_ratios[i] : 0;
            if (flex > 0) {
                total_flex += flex;
            } else {
                auto child_bc = box_constraints::loose(avail, area.h);
                auto m = dispatch_measure(s, cd.children[i], child_bc);
                widths[i] = m.width;
                auto_width += m.width;
            }
        }

        uint32_t flex_avail = avail > auto_width ? avail - auto_width : 0;
        if (total_flex > 0) {
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t flex = i < cd.flex_ratios.size() ? cd.flex_ratios[i] : 0;
                if (flex > 0) {
                    widths[i] = flex_avail * flex / total_flex;
                }
            }
        }

        // Render each column
        uint32_t x = area.x;
        for (uint32_t i = 0; i < n; ++i) {
            rect col_area{x, area.y, widths[i], area.h};
            dispatch_render(ctx, cd.children[i], col_area);
            x += widths[i] + cd.gap;
        }
    }

    // ── Table ───────────────────────────────────────────────────────────────

    measurement measure_table(const scene &s, node_id id, box_constraints bc) {
        const auto &td = s.get_table(s.pool_of(id));
        uint32_t ncols = static_cast<uint32_t>(td.column_defs.size());
        uint32_t nrows = ncols > 0 ? static_cast<uint32_t>(td.cells.size()) / ncols : 0;
        bool has_border = td.border != border_style::none;

        // Compute column widths from content
        std::vector<uint32_t> col_widths(ncols, 0);
        for (uint32_t c = 0; c < ncols; ++c) {
            // Header width
            if (td.show_header) {
                col_widths[c] = std::max(col_widths[c], static_cast<uint32_t>(string_width(td.column_defs[c].header)));
            }
            // Cell widths
            for (uint32_t r = 0; r < nrows; ++r) {
                uint32_t idx = r * ncols + c;
                if (idx < td.cells.size()) {
                    col_widths[c] = std::max(col_widths[c], fragments_width(td.cells[idx]));
                }
            }
            col_widths[c] = std::clamp(col_widths[c], td.column_defs[c].min_width, td.column_defs[c].max_width);
        }

        // Total width: borders + separators + content
        uint32_t total_w = 0;
        for (auto w : col_widths) {
            total_w += w;
        }
        if (has_border) {
            total_w += ncols + 1; // vertical borders between and around columns
        }

        // Total height: header + separator + rows + borders
        uint32_t total_h = nrows;
        if (td.show_header) {
            total_h += 1; // header row
        }
        if (td.show_header && has_border) {
            total_h += 1; // header separator
        }
        if (has_border) {
            total_h += 2; // top + bottom border
        }

        return bc.constrain({total_w, total_h});
    }

    void render_table(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &s = ctx.sc;
        auto &c = ctx.cv;
        const auto &styles = ctx.styles;
        const auto &td = s.get_table(s.pool_of(id));
        uint32_t ncols = static_cast<uint32_t>(td.column_defs.size());
        uint32_t nrows = ncols > 0 ? static_cast<uint32_t>(td.cells.size()) / ncols : 0;
        bool has_border = td.border != border_style::none;

        if (ncols == 0) {
            return;
        }

        // Shadow pass
        if (td.shadow) {
            render_shadow(c, styles, area, *td.shadow);
        }

        // Compute column widths (same as measure)
        std::vector<uint32_t> col_widths(ncols, 0);
        for (uint32_t col = 0; col < ncols; ++col) {
            if (td.show_header) {
                col_widths[col] =
                    std::max(col_widths[col], static_cast<uint32_t>(string_width(td.column_defs[col].header)));
            }
            for (uint32_t r = 0; r < nrows; ++r) {
                uint32_t idx = r * ncols + col;
                if (idx < td.cells.size()) {
                    col_widths[col] = std::max(col_widths[col], fragments_width(td.cells[idx]));
                }
            }
            col_widths[col] = std::clamp(col_widths[col], td.column_defs[col].min_width, td.column_defs[col].max_width);
        }

        auto bch = get_border_chars(td.border);
        auto bsid = const_cast<style_table &>(styles).intern(td.border_sty);
        auto hsid = const_cast<style_table &>(styles).intern(td.header_sty);

        uint32_t row_y = area.y;

        // Helper: draw a horizontal separator line
        auto draw_hsep = [&](uint32_t y, char32_t left, char32_t mid, char32_t right, char32_t fill) {
            if (!has_border) {
                return;
            }
            uint32_t x = area.x;
            canvas_put(c, x++, y, left, bsid, 1);
            for (uint32_t col = 0; col < ncols; ++col) {
                for (uint32_t i = 0; i < col_widths[col]; ++i) {
                    canvas_put(c, x++, y, fill, bsid, 1);
                }
                if (col < ncols - 1) {
                    canvas_put(c, x++, y, mid, bsid, 1);
                }
            }
            canvas_put(c, x, y, right, bsid, 1);
        };

        // Helper: render a data row
        auto draw_row = [&](uint32_t y, const std::vector<text_fragment> *row_cells, const std::string *headers,
                            style_id text_sid) {
            uint32_t x = area.x;
            if (has_border) {
                canvas_put(c, x++, y, bch.vertical, bsid, 1);
            }
            for (uint32_t col = 0; col < ncols; ++col) {
                if (headers) {
                    // Render header text
                    std::vector<text_fragment> hf;
                    hf.push_back({headers[col], styles.lookup(text_sid)});
                    (void)render_fragments_line(hf, c, x, y, col_widths[col], styles);
                } else if (row_cells) {
                    (void)render_fragments_line(row_cells[col], c, x, y, col_widths[col], styles);
                }
                x += col_widths[col];
                if (has_border) {
                    if (col < ncols - 1) {
                        canvas_put(c, x++, y, bch.vertical, bsid, 1);
                    }
                }
            }
            if (has_border) {
                canvas_put(c, x, y, bch.vertical, bsid, 1);
            }
        };

        // Top border
        if (has_border) {
            draw_hsep(row_y, bch.top_left, bch.t_down, bch.top_right, bch.horizontal);
            row_y++;
        }

        // Header row
        if (td.show_header) {
            std::vector<std::string> hdrs(ncols);
            for (uint32_t col = 0; col < ncols; ++col) {
                hdrs[col] = td.column_defs[col].header;
            }
            draw_row(row_y, nullptr, hdrs.data(), hsid);
            row_y++;

            // Header separator
            if (has_border) {
                draw_hsep(row_y, bch.t_right, bch.cross, bch.t_left, bch.horizontal);
                row_y++;
            }
        }

        // Data rows
        for (uint32_t r = 0; r < nrows; ++r) {
            uint32_t base_idx = r * ncols;
            if (base_idx + ncols <= td.cells.size()) {
                draw_row(row_y, &td.cells[base_idx], nullptr, style_id{0});
            }
            row_y++;
        }

        // Bottom border
        if (has_border) {
            draw_hsep(row_y, bch.bottom_left, bch.t_up, bch.bottom_right, bch.horizontal);
        }
    }

    // ── Overlay ─────────────────────────────────────────────────────────────

    measurement measure_overlay(const scene &s, node_id id, box_constraints bc) {
        const auto &od = s.get_overlay(s.pool_of(id));
        measurement base_m{0, 0};
        measurement over_m{0, 0};

        if (od.base != no_node) {
            base_m = dispatch_measure(s, od.base, bc);
        }
        if (od.overlay != no_node) {
            over_m = dispatch_measure(s, od.overlay, bc);
        }

        return bc.constrain({std::max(base_m.width, over_m.width), std::max(base_m.height, over_m.height)});
    }

    void render_overlay(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &od = ctx.sc.get_overlay(ctx.sc.pool_of(id));

        // Render base first (lower z-order)
        if (od.base != no_node) {
            dispatch_render(ctx, od.base, area);
        }
        // Render overlay on top (higher z-order, overwrites base cells)
        if (od.overlay != no_node) {
            dispatch_render(ctx, od.overlay, area);
        }
    }

    // ── Rows ─────────────────────────────────────────────────────────────────

    measurement measure_rows(const scene &s, node_id id, box_constraints bc) {
        const auto &rd = s.get_rows(s.pool_of(id));
        uint32_t n = static_cast<uint32_t>(rd.children.size());
        if (n == 0) {
            return bc.constrain({0, 0});
        }

        uint32_t total_gap = (n - 1) * rd.gap;
        bool bounded = bc.max_h < unbounded;
        uint32_t avail_h = bounded && bc.max_h > total_gap ? bc.max_h - total_gap : 0;

        // First pass: measure auto-sized rows (flex=0)
        uint32_t total_flex = 0;
        uint32_t auto_height = 0;
        std::vector<uint32_t> heights(n, 0);

        for (uint32_t i = 0; i < n; ++i) {
            uint32_t flex = i < rd.flex_ratios.size() ? rd.flex_ratios[i] : 0;
            if (flex > 0) {
                total_flex += flex;
            } else {
                auto child_bc = box_constraints::loose(bc.max_w, bounded ? avail_h : unbounded);
                auto m = dispatch_measure(s, rd.children[i], child_bc);
                heights[i] = m.height;
                auto_height += m.height;
            }
        }

        // Second pass: distribute remaining height to flex rows
        // Flex only works when height is bounded; otherwise flex rows get 0
        if (bounded && total_flex > 0) {
            uint32_t flex_avail = avail_h > auto_height ? avail_h - auto_height : 0;
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t flex = i < rd.flex_ratios.size() ? rd.flex_ratios[i] : 0;
                if (flex > 0) {
                    heights[i] = flex_avail * flex / total_flex;
                }
            }
        }

        // Compute total height and max width
        uint32_t total_h = total_gap;
        uint32_t max_w = 0;
        for (uint32_t i = 0; i < n; ++i) {
            total_h += heights[i];
            auto child_bc = box_constraints::loose(bc.max_w, heights[i] > 0 ? heights[i] : unbounded);
            auto m = dispatch_measure(s, rd.children[i], child_bc);
            max_w = std::max(max_w, m.width);
        }

        return bc.constrain({max_w, total_h});
    }

    void render_rows(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &s = ctx.sc;
        const auto &rd = s.get_rows(s.pool_of(id));
        uint32_t n = static_cast<uint32_t>(rd.children.size());
        if (n == 0) {
            return;
        }

        uint32_t total_gap = (n - 1) * rd.gap;
        bool bounded = area.h < unbounded;
        uint32_t avail_h = bounded && area.h > total_gap ? area.h - total_gap : 0;

        // Same height calculation as measure
        uint32_t total_flex = 0;
        uint32_t auto_height = 0;
        std::vector<uint32_t> heights(n, 0);

        for (uint32_t i = 0; i < n; ++i) {
            uint32_t flex = i < rd.flex_ratios.size() ? rd.flex_ratios[i] : 0;
            if (flex > 0) {
                total_flex += flex;
            } else {
                auto child_bc = box_constraints::loose(area.w, bounded ? avail_h : unbounded);
                auto m = dispatch_measure(s, rd.children[i], child_bc);
                heights[i] = m.height;
                auto_height += m.height;
            }
        }

        if (bounded && total_flex > 0) {
            uint32_t flex_avail = avail_h > auto_height ? avail_h - auto_height : 0;
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t flex = i < rd.flex_ratios.size() ? rd.flex_ratios[i] : 0;
                if (flex > 0) {
                    heights[i] = flex_avail * flex / total_flex;
                }
            }
        }

        // Render each row
        uint32_t y = area.y;
        for (uint32_t i = 0; i < n; ++i) {
            rect row_area{area.x, y, area.w, heights[i]};
            dispatch_render(ctx, rd.children[i], row_area);
            y += heights[i] + rd.gap;
        }
    }

    // ── Spacer ───────────────────────────────────────────────────────────────

    measurement measure_spacer(const scene &s, node_id id, box_constraints bc) {
        (void)s;
        (void)id;
        return bc.constrain({0, 0});
    }

    void render_spacer(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        // Spacer renders nothing — it just occupies space
    }

    // ── Sized Box ────────────────────────────────────────────────────────────

    measurement measure_sized_box(const scene &s, node_id id, box_constraints bc) {
        const auto &sb = s.get_sized_box(s.pool_of(id));

        // Build inner constraints from sized_box config
        uint32_t inner_min_w = sb.min_w;
        uint32_t inner_max_w = sb.max_w;
        uint32_t inner_min_h = sb.min_h;
        uint32_t inner_max_h = sb.max_h;

        // fixed_w/h override both min and max
        if (sb.fixed_w > 0) {
            inner_min_w = sb.fixed_w;
            inner_max_w = sb.fixed_w;
        }
        if (sb.fixed_h > 0) {
            inner_min_h = sb.fixed_h;
            inner_max_h = sb.fixed_h;
        }

        // Clamp inner constraints to parent constraints
        inner_min_w = std::clamp(inner_min_w, bc.min_w, bc.max_w);
        inner_max_w = std::clamp(inner_max_w, bc.min_w, bc.max_w);
        inner_min_h = std::clamp(inner_min_h, bc.min_h, bc.max_h);
        inner_max_h = std::clamp(inner_max_h, bc.min_h, bc.max_h);

        // Ensure min <= max after clamping
        if (inner_min_w > inner_max_w) {
            inner_min_w = inner_max_w;
        }
        if (inner_min_h > inner_max_h) {
            inner_min_h = inner_max_h;
        }

        box_constraints inner_bc{inner_min_w, inner_max_w, inner_min_h, inner_max_h};

        if (sb.content != no_node) {
            auto child_m = dispatch_measure(s, sb.content, inner_bc);
            // Clamp by the sized_box's own constraints first, then by parent
            child_m.width = std::clamp(child_m.width, inner_min_w, inner_max_w);
            child_m.height = std::clamp(child_m.height, inner_min_h, inner_max_h);
            return bc.constrain(child_m);
        }

        // No content: use the fixed/min size
        return bc.constrain({inner_min_w, inner_min_h});
    }

    void render_sized_box(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;

        const auto &sb = ctx.sc.get_sized_box(ctx.sc.pool_of(id));
        if (sb.content != no_node) {
            dispatch_render(ctx, sb.content, area);
        }
    }

    // ── Center ───────────────────────────────────────────────────────────────

    measurement measure_center(const scene &s, node_id id, box_constraints bc) {
        const auto &cd = s.get_center(s.pool_of(id));

        if (cd.content != no_node) {
            // Measure child with loose constraints
            box_constraints loose{0, bc.max_w, 0, bc.max_h};
            auto child_m = dispatch_measure(s, cd.content, loose);
            // Center wants to fill available space
            return bc.constrain(
                {bc.max_w != unbounded ? bc.max_w : child_m.width, bc.max_h != unbounded ? bc.max_h : child_m.height});
        }

        return bc.constrain({0, 0});
    }

    void render_center(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;

        const auto &cd = ctx.sc.get_center(ctx.sc.pool_of(id));
        if (cd.content == no_node) {
            return;
        }

        // Measure child to get its natural size
        box_constraints loose{0, area.w, 0, area.h};
        auto child_m = dispatch_measure(ctx.sc, cd.content, loose);

        uint32_t cw = std::min(child_m.width, area.w);
        uint32_t ch = std::min(child_m.height, area.h);

        uint32_t cx = area.x;
        uint32_t cy = area.y;
        if (cd.horizontal && cw < area.w) {
            cx = area.x + (area.w - cw) / 2;
        }
        if (cd.vertical && ch < area.h) {
            cy = area.y + (area.h - ch) / 2;
        }

        dispatch_render(ctx, cd.content, {cx, cy, cw, ch});
    }

    // ── Scroll View ──────────────────────────────────────────────────────────

    measurement measure_scroll_view(const scene &s, node_id id, box_constraints bc) {
        const auto &sv = s.get_scroll_view(s.pool_of(id));
        bool has_border = sv.border != border_style::none;
        uint32_t border_pad = has_border ? 2 : 0;
        uint32_t scrollbar_w = sv.show_scrollbar_v ? 1 : 0;

        // Viewport size: use fixed if set, otherwise fill parent
        uint32_t vw = sv.view_w > 0 ? sv.view_w : (bc.max_w != unbounded ? bc.max_w - border_pad - scrollbar_w : 40);
        uint32_t vh = sv.view_h > 0 ? sv.view_h : (bc.max_h != unbounded ? bc.max_h - border_pad : 10);

        uint32_t total_w = vw + border_pad + scrollbar_w;
        uint32_t total_h = vh + border_pad;

        return bc.constrain({total_w, total_h});
    }

    void render_scroll_view(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;

        const auto &sv = ctx.sc.get_scroll_view(ctx.sc.pool_of(id));
        bool has_border = sv.border != border_style::none;
        uint32_t border_pad = has_border ? 1 : 0;
        uint32_t scrollbar_w = sv.show_scrollbar_v ? 1 : 0;

        auto &c = ctx.cv;
        const auto &styles = ctx.styles;

        auto bsid = const_cast<style_table &>(styles).intern(sv.border_sty);

        // Draw border
        if (has_border && area.w >= 2 && area.h >= 2) {
            auto bc = get_border_chars(sv.border);
            canvas_put(c, area.x, area.y, bc.top_left, bsid, 1);
            canvas_put(c, area.x + area.w - 1, area.y, bc.top_right, bsid, 1);
            canvas_put(c, area.x, area.y + area.h - 1, bc.bottom_left, bsid, 1);
            canvas_put(c, area.x + area.w - 1, area.y + area.h - 1, bc.bottom_right, bsid, 1);
            fill_h(c, area.x + 1, area.y, area.w - 2, bc.horizontal, bsid);
            fill_h(c, area.x + 1, area.y + area.h - 1, area.w - 2, bc.horizontal, bsid);
            for (uint32_t row = 1; row < area.h - 1; ++row) {
                canvas_put(c, area.x, area.y + row, bc.vertical, bsid, 1);
                canvas_put(c, area.x + area.w - 1, area.y + row, bc.vertical, bsid, 1);
            }
        }

        // Inner viewport area
        rect inner = has_border ? area.inset(1, 1, 1, 1) : area;
        if (inner.empty()) {
            return;
        }

        // Reserve space for scrollbar
        uint32_t content_w = inner.w > scrollbar_w ? inner.w - scrollbar_w : inner.w;
        uint32_t content_h = inner.h;

        if (sv.content == no_node || content_w == 0 || content_h == 0) {
            return;
        }

        int scroll_x = sv.scroll_x ? *sv.scroll_x : 0;
        int scroll_y = sv.scroll_y ? *sv.scroll_y : 0;
        if (scroll_x < 0) {
            scroll_x = 0;
        }
        if (scroll_y < 0) {
            scroll_y = 0;
        }

        // Measure child with unbounded height to get full content size
        box_constraints child_bc{content_w, content_w, 0, unbounded};
        auto child_m = dispatch_measure(ctx.sc, sv.content, child_bc);

        uint32_t total_content_h = child_m.height;
        uint32_t total_content_w = child_m.width;

        // Clamp scroll offsets
        if (total_content_h > content_h) {
            if (static_cast<uint32_t>(scroll_y) > total_content_h - content_h) {
                scroll_y = static_cast<int>(total_content_h - content_h);
            }
        } else {
            scroll_y = 0;
        }

        // Render child into a temporary canvas
        uint32_t tmp_w = std::max(content_w, total_content_w);
        uint32_t tmp_h = std::max(content_h, total_content_h);
        canvas tmp_canvas(tmp_w, tmp_h);
        style_table tmp_styles;

        // Copy styles from main context
        scene &sc = ctx.sc;
        render_context tmp_ctx{sc, tmp_canvas, tmp_styles, ctx.frame_time, ctx.frame_number};
        dispatch_render(tmp_ctx, sv.content, {0, 0, tmp_w, tmp_h});

        // Blit visible window from tmp_canvas to main canvas
        for (uint32_t vy = 0; vy < content_h; ++vy) {
            uint32_t src_y = static_cast<uint32_t>(scroll_y) + vy;
            if (src_y >= tmp_h) {
                break;
            }
            for (uint32_t vx = 0; vx < content_w; ++vx) {
                uint32_t src_x = static_cast<uint32_t>(scroll_x) + vx;
                if (src_x >= tmp_w) {
                    break;
                }
                const auto &src_cell = tmp_canvas.get(src_x, src_y);
                if (src_cell.codepoint != U' ' || src_cell.sid != default_style_id) {
                    // Re-intern the style from tmp_styles into main styles
                    const auto &src_sty = tmp_styles.lookup(src_cell.sid);
                    auto dst_sid = const_cast<style_table &>(styles).intern(src_sty);
                    uint32_t dx = inner.x + vx;
                    uint32_t dy = inner.y + vy;
                    if (dx < c.width() && dy < c.height()) {
                        c.set(dx, dy, cell{src_cell.codepoint, dst_sid, src_cell.width, src_cell.alpha});
                    }
                }
            }
        }

        // Draw vertical scrollbar
        if (sv.show_scrollbar_v && scrollbar_w > 0 && total_content_h > content_h && content_h > 0) {
            uint32_t sb_x = inner.x + content_w;
            // Scrollbar track
            auto track_sid = const_cast<style_table &>(styles).intern(
                style{color::default_color(), color::default_color(), attr::dim});
            for (uint32_t row = 0; row < content_h; ++row) {
                canvas_put(c, sb_x, inner.y + row, U'\x2502', track_sid, 1); // │
            }
            // Scrollbar thumb
            uint32_t thumb_h = std::max(1u, content_h * content_h / total_content_h);
            uint32_t thumb_y = static_cast<uint32_t>(scroll_y) * (content_h - thumb_h) / (total_content_h - content_h);
            auto thumb_sid = const_cast<style_table &>(styles).intern(style{});
            for (uint32_t row = 0; row < thumb_h; ++row) {
                uint32_t ty = inner.y + thumb_y + row;
                if (ty < inner.y + content_h) {
                    canvas_put(c, sb_x, ty, U'\x2588', thumb_sid, 1); // █
                }
            }
        }
    }

    // ── Menu ─────────────────────────────────────────────────────────────────

    measurement measure_menu(const scene &s, node_id id, box_constraints bc) {
        const auto &md = s.get_menu(s.pool_of(id));
        bool has_border = md.border != border_style::none;
        uint32_t border_w = has_border ? 2 : 0;
        uint32_t border_h = has_border ? 2 : 0;

        // Compute content width: max(label_width + gap + shortcut_width)
        uint32_t max_content_w = 0;
        uint32_t item_count = 0;
        for (const auto &item : md.items) {
            if (item.separator) {
                item_count++;
                continue;
            }
            uint32_t lw = static_cast<uint32_t>(string_width(item.label));
            uint32_t sw = item.shortcut.empty() ? 0 : static_cast<uint32_t>(string_width(item.shortcut));
            uint32_t row_w = lw + (sw > 0 ? 2 + sw : 0); // 2 = gap between label and shortcut
            max_content_w = std::max(max_content_w, row_w);
            item_count++;
        }

        // Add 2 for left/right padding inside border
        uint32_t total_w = max_content_w + 2 + border_w;
        uint32_t total_h = item_count + border_h;

        return bc.constrain({total_w, total_h});
    }

    void render_menu(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &s = ctx.sc;
        auto &c = ctx.cv;
        const auto &styles = ctx.styles;
        const auto &md = s.get_menu(s.pool_of(id));
        bool has_border = md.border != border_style::none;

        // Shadow pass
        if (md.shadow) {
            render_shadow(c, styles, area, *md.shadow);
        }

        auto bsid = const_cast<style_table &>(styles).intern(md.border_sty);
        auto item_sid = const_cast<style_table &>(styles).intern(md.item_sty);
        auto hl_sid = const_cast<style_table &>(styles).intern(md.highlight_sty);
        auto sc_sid = const_cast<style_table &>(styles).intern(md.shortcut_sty);

        // Border
        if (has_border && area.w >= 2 && area.h >= 2) {
            auto bc = get_border_chars(md.border);

            canvas_put(c, area.x, area.y, bc.top_left, bsid, 1);
            canvas_put(c, area.x + area.w - 1, area.y, bc.top_right, bsid, 1);
            canvas_put(c, area.x, area.y + area.h - 1, bc.bottom_left, bsid, 1);
            canvas_put(c, area.x + area.w - 1, area.y + area.h - 1, bc.bottom_right, bsid, 1);

            fill_h(c, area.x + 1, area.y, area.w - 2, bc.horizontal, bsid);
            fill_h(c, area.x + 1, area.y + area.h - 1, area.w - 2, bc.horizontal, bsid);

            for (uint32_t row = 1; row < area.h - 1; ++row) {
                canvas_put(c, area.x, area.y + row, bc.vertical, bsid, 1);
                canvas_put(c, area.x + area.w - 1, area.y + row, bc.vertical, bsid, 1);
            }
        }

        // Content area
        rect inner = has_border ? area.inset(1, 1, 1, 1) : area;
        if (inner.empty()) {
            return;
        }

        int cursor_val = md.cursor ? *md.cursor : -1;
        uint32_t row_y = inner.y;
        int item_idx = 0;

        for (const auto &item : md.items) {
            if (row_y >= inner.y + inner.h) {
                break;
            }

            if (item.separator) {
                // Draw horizontal separator
                if (has_border) {
                    auto bc = get_border_chars(md.border);
                    canvas_put(c, area.x, row_y, bc.t_right, bsid, 1);
                    fill_h(c, inner.x, row_y, inner.w, bc.horizontal, bsid);
                    canvas_put(c, area.x + area.w - 1, row_y, bc.t_left, bsid, 1);
                } else {
                    fill_h(c, inner.x, row_y, inner.w, U'\x2500', bsid);
                }
                row_y++;
                item_idx++;
                continue;
            }

            bool selected = (item_idx == cursor_val);
            auto row_sid = selected ? hl_sid : item_sid;

            // Fill background for the row
            fill_h(c, inner.x, row_y, inner.w, U' ', row_sid);

            // Render label (left-aligned, 1 cell padding)
            uint32_t lw = static_cast<uint32_t>(string_width(item.label));
            uint32_t label_max = inner.w > 1 ? inner.w - 1 : 0;
            std::vector<text_fragment> label_frags;
            label_frags.push_back({item.label, selected ? md.highlight_sty : md.item_sty});
            render_fragments_line(label_frags, c, inner.x + 1, row_y, label_max, styles);

            // Render shortcut (right-aligned)
            if (!item.shortcut.empty()) {
                uint32_t sw = static_cast<uint32_t>(string_width(item.shortcut));
                if (sw + lw + 3 <= inner.w) { // 1 left pad + label + 2 gap + shortcut
                    uint32_t sx = inner.x + inner.w - sw - 1;
                    std::vector<text_fragment> sc_frags;
                    sc_frags.push_back({item.shortcut, selected ? md.highlight_sty : md.shortcut_sty});
                    render_fragments_line(sc_frags, c, sx, row_y, sw, styles);
                }
            }

            row_y++;
            item_idx++;
        }
    }

    // ── Canvas Widget ────────────────────────────────────────────────────────

    measurement measure_canvas_widget(const scene &s, node_id id, box_constraints bc) {
        const auto &cw = s.get_canvas_widget(s.pool_of(id));
        // Braille: each cell = 2 pixels wide × 4 pixels tall
        uint32_t cell_w = (cw.pixel_w + 1) / 2;
        uint32_t cell_h = (cw.pixel_h + 3) / 4;
        return bc.constrain({cell_w, cell_h});
    }

    void render_canvas_widget(render_context &ctx, node_id id, rect area) {
        if (area.empty()) {
            return;
        }
        ctx.sc.area(id) = area;
        const auto &cw = ctx.sc.get_canvas_widget(ctx.sc.pool_of(id));
        auto &c = ctx.cv;
        auto &styles = const_cast<style_table &>(ctx.styles);

        uint32_t cell_w = area.w;
        uint32_t cell_h = area.h;

        // Braille grid: each cell encodes a 2×4 pixel block
        // Unicode braille starts at U+2800, bits map:
        //   dot 1 (0,0) = bit 0    dot 4 (1,0) = bit 3
        //   dot 2 (0,1) = bit 1    dot 5 (1,1) = bit 4
        //   dot 3 (0,2) = bit 2    dot 6 (1,2) = bit 5
        //   dot 7 (0,3) = bit 6    dot 8 (1,3) = bit 7

        // Allocate braille grid + color per cell
        struct braille_cell {
            uint8_t bits = 0;
            color fg;
        };
        std::vector<braille_cell> grid(static_cast<size_t>(cell_w) * cell_h);

        auto set_pixel = [&](int px, int py, const color &col) {
            if (px < 0 || py < 0) {
                return;
            }
            int cx = px / 2;
            int cy = py / 4;
            if (cx < 0 || static_cast<uint32_t>(cx) >= cell_w || cy < 0 || static_cast<uint32_t>(cy) >= cell_h) {
                return;
            }
            int lx = px % 2;
            int ly = py % 4;
            // Braille bit mapping
            static const uint8_t bit_map[2][4] = {
                {0x01, 0x02, 0x04, 0x40}, // left column
                {0x08, 0x10, 0x20, 0x80}, // right column
            };
            auto &cell = grid[static_cast<size_t>(cy) * cell_w + cx];
            cell.bits |= bit_map[lx][ly];
            cell.fg = col;
        };

        // Bresenham line drawing
        auto draw_line_pixels = [&](int x1, int y1, int x2, int y2, const color &col) {
            int dx = std::abs(x2 - x1);
            int dy = std::abs(y2 - y1);
            int sx = x1 < x2 ? 1 : -1;
            int sy = y1 < y2 ? 1 : -1;
            int err = dx - dy;
            while (true) {
                set_pixel(x1, y1, col);
                if (x1 == x2 && y1 == y2) {
                    break;
                }
                int e2 = 2 * err;
                if (e2 > -dy) {
                    err -= dy;
                    x1 += sx;
                }
                if (e2 < dx) {
                    err += dx;
                    y1 += sy;
                }
            }
        };

        // Draw points
        for (const auto &p : cw.points) {
            set_pixel(p.x, p.y, p.c);
        }

        // Draw lines
        for (const auto &ln : cw.lines) {
            if (ln.block_mode) {
                // Block mode: use full-block characters instead of braille
                // (handled separately below)
                continue;
            }
            draw_line_pixels(ln.x1, ln.y1, ln.x2, ln.y2, ln.c);
        }

        // Draw circles (midpoint algorithm)
        for (const auto &ci : cw.circles) {
            int x = ci.r, y = 0;
            int d = 1 - ci.r;
            while (x >= y) {
                set_pixel(ci.cx + x, ci.cy + y, ci.c);
                set_pixel(ci.cx - x, ci.cy + y, ci.c);
                set_pixel(ci.cx + x, ci.cy - y, ci.c);
                set_pixel(ci.cx - x, ci.cy - y, ci.c);
                set_pixel(ci.cx + y, ci.cy + x, ci.c);
                set_pixel(ci.cx - y, ci.cy + x, ci.c);
                set_pixel(ci.cx + y, ci.cy - x, ci.c);
                set_pixel(ci.cx - y, ci.cy - x, ci.c);
                y++;
                if (d <= 0) {
                    d += 2 * y + 1;
                } else {
                    x--;
                    d += 2 * (y - x) + 1;
                }
            }
        }

        // Draw rects (outline)
        for (const auto &r : cw.rects) {
            for (int i = 0; i < r.w; ++i) {
                set_pixel(r.x + i, r.y, r.c);
                set_pixel(r.x + i, r.y + r.h - 1, r.c);
            }
            for (int i = 0; i < r.h; ++i) {
                set_pixel(r.x, r.y + i, r.c);
                set_pixel(r.x + r.w - 1, r.y + i, r.c);
            }
        }

        // Render braille cells to canvas
        for (uint32_t cy = 0; cy < cell_h; ++cy) {
            for (uint32_t cx = 0; cx < cell_w; ++cx) {
                const auto &bc = grid[static_cast<size_t>(cy) * cell_w + cx];
                if (bc.bits == 0) {
                    continue;
                }
                char32_t braille = U'\x2800' + bc.bits;
                style sty;
                sty.fg = bc.fg;
                auto sid = styles.intern(sty);
                uint32_t dx = area.x + cx;
                uint32_t dy = area.y + cy;
                if (dx < c.width() && dy < c.height()) {
                    canvas_put(c, dx, dy, braille, sid, 1);
                }
            }
        }

        // Block-mode lines: use half-block characters (▀▄█)
        for (const auto &ln : cw.lines) {
            if (!ln.block_mode) {
                continue;
            }
            // For block mode, each cell = 1×2 pixels
            int dx_abs = std::abs(ln.x2 - ln.x1);
            int dy_abs = std::abs(ln.y2 - ln.y1);
            int sx = ln.x1 < ln.x2 ? 1 : -1;
            int sy = ln.y1 < ln.y2 ? 1 : -1;
            int err = dx_abs - dy_abs;
            int x = ln.x1, y = ln.y1;
            style sty;
            sty.fg = ln.c;
            auto sid = styles.intern(sty);
            while (true) {
                // Map pixel (x, y) to cell
                uint32_t cx = static_cast<uint32_t>(x);
                uint32_t cy = static_cast<uint32_t>(y / 2);
                if (cx < cell_w && cy < cell_h) {
                    uint32_t dx2 = area.x + cx;
                    uint32_t dy2 = area.y + cy;
                    if (dx2 < c.width() && dy2 < c.height()) {
                        char32_t ch = (y % 2 == 0) ? U'\x2580' : U'\x2584'; // ▀ or ▄
                        canvas_put(c, dx2, dy2, ch, sid, 1);
                    }
                }
                if (x == ln.x2 && y == ln.y2) {
                    break;
                }
                int e2 = 2 * err;
                if (e2 > -dy_abs) {
                    err -= dy_abs;
                    x += sx;
                }
                if (e2 < dx_abs) {
                    err += dx_abs;
                    y += sy;
                }
            }
        }

        // Draw text commands directly onto canvas
        for (const auto &t : cw.texts) {
            auto sid = styles.intern(t.sty);
            uint32_t tx = area.x + static_cast<uint32_t>(t.x);
            uint32_t ty = area.y + static_cast<uint32_t>(t.y);
            for (size_t i = 0; i < t.text.size(); ++i) {
                uint32_t px = tx + static_cast<uint32_t>(i);
                if (px < c.width() && ty < c.height()) {
                    canvas_put(c, px, ty, static_cast<char32_t>(t.text[i]), sid, 1);
                }
            }
        }
    }

} // namespace tapiru::detail
