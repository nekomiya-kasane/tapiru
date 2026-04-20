#pragma once

/**
 * @file builders.h
 * @brief User-facing widget builder classes.
 *
 * Users construct builders, then pass them to console.print().
 * Internally, builders flatten into a detail::scene for layout and rendering.
 */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tapiru/core/gradient.h"
#include "tapiru/core/shader.h"
#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"
#include "tapiru/text/markup.h"

namespace tapiru {

namespace detail {
class scene;

/** @brief FNV-1a hash for widget key strings. */
inline uint64_t fnv1a_hash(std::string_view sv) noexcept {
    uint64_t h = 14695981039346656037ULL;
    for (char c : sv) {
        h ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        h *= 1099511628211ULL;
    }
    return h;
}
}  // namespace detail

// ── Forward: flatten_into returns a node_id (uint32_t) ──────────────────
using node_id = uint32_t;

// ── Text builder ────────────────────────────────────────────────────────

class TAPIRU_API text_builder {
public:
    explicit text_builder(std::string_view markup) : markup_(markup) {}

    text_builder& align(justify j)       { align_ = j; return *this; }
    text_builder& overflow(overflow_mode m) { overflow_ = m; return *this; }
    text_builder& style_override(const style& s) { style_ = s; has_style_ = true; return *this; }
    text_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    text_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    std::string   markup_;
    justify       align_    = justify::left;
    overflow_mode overflow_ = overflow_mode::wrap;
    style         style_;
    bool          has_style_ = false;
    int16_t       z_order_   = 0;
    uint64_t      key_       = 0;
};

// ── Rule builder ────────────────────────────────────────────────────────

class TAPIRU_API rule_builder {
public:
    rule_builder() = default;
    explicit rule_builder(std::string_view title) : title_(title) {}

    rule_builder& title(std::string_view t) { title_ = t; return *this; }
    rule_builder& align(justify j)          { align_ = j; return *this; }
    rule_builder& character(char32_t ch)    { ch_ = ch; return *this; }
    rule_builder& rule_style(const style& s) { style_ = s; return *this; }
    rule_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    rule_builder& gradient(const linear_gradient& g) { gradient_ = g; return *this; }
    rule_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    std::string title_;
    justify     align_ = justify::center;
    char32_t    ch_    = U'\x2500';
    style       style_;
    int16_t     z_order_ = 0;
    std::optional<linear_gradient> gradient_;
    uint64_t    key_     = 0;
};

// ── Padding builder ─────────────────────────────────────────────────────

class TAPIRU_API padding_builder {
public:
    template <typename B>
    explicit padding_builder(B&& content) : content_(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))) {}

    padding_builder(padding_builder&&) noexcept = default;
    padding_builder& operator=(padding_builder&&) noexcept = default;
    padding_builder(const padding_builder&) = delete;
    padding_builder& operator=(const padding_builder&) = delete;

    padding_builder& pad(uint8_t top, uint8_t right, uint8_t bottom, uint8_t left) {
        top_ = top; right_ = right; bottom_ = bottom; left_ = left;
        return *this;
    }
    padding_builder& pad(uint8_t vertical, uint8_t horizontal) {
        return pad(vertical, horizontal, vertical, horizontal);
    }
    padding_builder& pad(uint8_t all) {
        return pad(all, all, all, all);
    }
    padding_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    padding_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ { virtual ~concept_() = default; virtual node_id flatten(detail::scene& s) const = 0; };
    template <typename B>
    struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene& s) const override { return builder.flatten_into(s); }
    };
    std::unique_ptr<concept_> content_;
    uint8_t top_ = 0, right_ = 0, bottom_ = 0, left_ = 0;
    int16_t z_order_ = 0;
    uint64_t key_    = 0;
};

// ── Panel builder ───────────────────────────────────────────────────────

class TAPIRU_API panel_builder {
public:
    template <typename B>
    explicit panel_builder(B&& content) : content_(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))) {}

    panel_builder(panel_builder&&) noexcept = default;
    panel_builder& operator=(panel_builder&&) noexcept = default;
    panel_builder(const panel_builder&) = delete;
    panel_builder& operator=(const panel_builder&) = delete;

    panel_builder& border(border_style bs)       { border_ = bs; return *this; }
    panel_builder& border_style_override(const style& s) { border_sty_ = s; return *this; }
    panel_builder& title(std::string_view t)     { title_ = t; return *this; }
    panel_builder& title_style(const style& s)   { title_sty_ = s; return *this; }
    panel_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    panel_builder& opacity(float o) { alpha_ = static_cast<uint8_t>(o < 0.f ? 0 : o > 1.f ? 255 : o * 255.f); return *this; }
    panel_builder& background_gradient(const linear_gradient& g) { bg_gradient_ = g; return *this; }
    panel_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    panel_builder& shadow(int8_t offset_x = 2, int8_t offset_y = 1, uint8_t blur = 1,
                          color shadow_color = color::from_rgb(0,0,0), uint8_t opacity = 128) {
        shadow_ = {offset_x, offset_y, blur, shadow_color, opacity};
        return *this;
    }
    panel_builder& glow(color glow_color, uint8_t blur = 1, uint8_t opacity = 100) {
        shadow_ = {0, 0, blur, glow_color, opacity};
        return *this;
    }
    panel_builder& shader(shader_fn fn) { shader_ = std::move(fn); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ { virtual ~concept_() = default; virtual node_id flatten(detail::scene& s) const = 0; };
    template <typename B>
    struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene& s) const override { return builder.flatten_into(s); }
    };
    std::unique_ptr<concept_> content_;
    border_style border_     = border_style::rounded;
    style        border_sty_;
    std::string  title_;
    style        title_sty_;
    int16_t      z_order_    = 0;
    uint8_t      alpha_      = 255;
    std::optional<linear_gradient> bg_gradient_;
    uint64_t     key_        = 0;
    struct shadow_cfg {
        int8_t  offset_x = 2;
        int8_t  offset_y = 1;
        uint8_t blur     = 1;
        color   shadow_color = color::from_rgb(0,0,0);
        uint8_t opacity  = 128;
    };
    std::optional<shadow_cfg> shadow_;
    shader_fn    shader_;
};

// ── Columns builder ─────────────────────────────────────────────────────

class TAPIRU_API columns_builder {
public:
    columns_builder() = default;
    columns_builder(columns_builder&&) noexcept = default;
    columns_builder& operator=(columns_builder&&) noexcept = default;
    columns_builder(const columns_builder&) = delete;
    columns_builder& operator=(const columns_builder&) = delete;

    template <typename B>
    columns_builder& add(B&& child, uint32_t flex = 0) {
        children_.push_back(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(child)));
        flex_ratios_.push_back(flex);
        return *this;
    }

    columns_builder& gap(uint32_t g) { gap_ = g; return *this; }
    columns_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    columns_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    columns_builder& opacity(float o) { alpha_ = static_cast<uint8_t>(o < 0.f ? 0 : o > 1.f ? 255 : o * 255.f); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ { virtual ~concept_() = default; virtual node_id flatten(detail::scene& s) const = 0; };
    template <typename B>
    struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene& s) const override { return builder.flatten_into(s); }
    };
    std::vector<std::unique_ptr<concept_>> children_;
    std::vector<uint32_t> flex_ratios_;
    uint32_t gap_ = 1;
    int16_t  z_order_ = 0;
    uint64_t key_     = 0;
    uint8_t  alpha_   = 255;
};

// ── Table builder ───────────────────────────────────────────────────────

struct column_opts {
    justify  align     = justify::left;
    uint32_t min_width = 0;
    uint32_t max_width = unbounded;
};

class TAPIRU_API table_builder {
public:
    table_builder() = default;

    table_builder& add_column(std::string_view name, column_opts opts = {}) {
        columns_.push_back({std::string(name), opts.align, opts.min_width, opts.max_width});
        return *this;
    }

    table_builder& add_row(std::vector<std::string> cells) {
        rows_.push_back(std::move(cells));
        return *this;
    }

    table_builder& border(border_style bs)       { border_ = bs; return *this; }
    table_builder& border_style_override(const style& s) { border_sty_ = s; return *this; }
    table_builder& header_style(const style& s)  { header_sty_ = s; return *this; }
    table_builder& show_header(bool v = true)    { show_header_ = v; return *this; }
    table_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    table_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }

    table_builder& shadow(int8_t offset_x = 2, int8_t offset_y = 1, uint8_t blur = 1,
                          color shadow_color = color::from_rgb(0,0,0), uint8_t opacity = 128) {
        shadow_ = {offset_x, offset_y, blur, shadow_color, opacity};
        return *this;
    }
    table_builder& glow(color glow_color, uint8_t blur = 1, uint8_t opacity = 100) {
        shadow_ = {0, 0, blur, glow_color, opacity};
        return *this;
    }
    table_builder& border_gradient(const linear_gradient& g) { border_gradient_ = g; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct col_def {
        std::string header;
        justify     align;
        uint32_t    min_width;
        uint32_t    max_width;
    };
    struct shadow_cfg {
        int8_t  offset_x = 2;
        int8_t  offset_y = 1;
        uint8_t blur     = 1;
        color   shadow_color = color::from_rgb(0,0,0);
        uint8_t opacity  = 128;
    };
    std::vector<col_def>              columns_;
    std::vector<std::vector<std::string>> rows_;
    border_style border_      = border_style::rounded;
    style        border_sty_;
    style        header_sty_;
    bool         show_header_ = true;
    int16_t      z_order_     = 0;
    uint64_t     key_         = 0;
    std::optional<shadow_cfg>      shadow_;
    std::optional<linear_gradient> border_gradient_;
};

// ── Overlay builder ─────────────────────────────────────────────────────

class TAPIRU_API overlay_builder {
public:
    template <typename Base, typename Over>
    overlay_builder(Base&& base, Over&& over)
        : base_(std::make_unique<model<std::decay_t<Base>>>(std::forward<Base>(base)))
        , over_(std::make_unique<model<std::decay_t<Over>>>(std::forward<Over>(over))) {}

    overlay_builder(overlay_builder&&) noexcept = default;
    overlay_builder& operator=(overlay_builder&&) noexcept = default;
    overlay_builder(const overlay_builder&) = delete;
    overlay_builder& operator=(const overlay_builder&) = delete;

    overlay_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    overlay_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    overlay_builder& opacity(float o) { alpha_ = static_cast<uint8_t>(o < 0.f ? 0 : o > 1.f ? 255 : o * 255.f); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ { virtual ~concept_() = default; virtual node_id flatten(detail::scene& s) const = 0; };
    template <typename B>
    struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene& s) const override { return builder.flatten_into(s); }
    };
    std::unique_ptr<concept_> base_;
    std::unique_ptr<concept_> over_;
    int16_t z_order_ = 0;
    uint64_t key_    = 0;
    uint8_t  alpha_  = 255;
};

// ── Rows builder ───────────────────────────────────────────────────────

class TAPIRU_API rows_builder {
public:
    rows_builder() = default;
    rows_builder(rows_builder&&) noexcept = default;
    rows_builder& operator=(rows_builder&&) noexcept = default;
    rows_builder(const rows_builder&) = delete;
    rows_builder& operator=(const rows_builder&) = delete;

    template <typename B>
    rows_builder& add(B&& child, uint32_t flex = 0) {
        children_.push_back(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(child)));
        flex_ratios_.push_back(flex);
        return *this;
    }

    rows_builder& gap(uint32_t g) { gap_ = g; return *this; }
    rows_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    rows_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    rows_builder& opacity(float o) { alpha_ = static_cast<uint8_t>(o < 0.f ? 0 : o > 1.f ? 255 : o * 255.f); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ { virtual ~concept_() = default; virtual node_id flatten(detail::scene& s) const = 0; };
    template <typename B>
    struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene& s) const override { return builder.flatten_into(s); }
    };
    std::vector<std::unique_ptr<concept_>> children_;
    std::vector<uint32_t> flex_ratios_;
    uint32_t gap_ = 0;
    int16_t  z_order_ = 0;
    uint64_t key_     = 0;
    uint8_t  alpha_   = 255;
};

// ── Spacer builder ─────────────────────────────────────────────────────

class TAPIRU_API spacer_builder {
public:
    spacer_builder() = default;

    spacer_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    spacer_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    int16_t  z_order_ = 0;
    uint64_t key_     = 0;
};

// ── Sized box builder ─────────────────────────────────────────────────

class TAPIRU_API sized_box_builder {
public:
    template <typename B>
    explicit sized_box_builder(B&& content) : content_(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))) {}

    sized_box_builder() = default;
    sized_box_builder(sized_box_builder&&) noexcept = default;
    sized_box_builder& operator=(sized_box_builder&&) noexcept = default;
    sized_box_builder(const sized_box_builder&) = delete;
    sized_box_builder& operator=(const sized_box_builder&) = delete;

    sized_box_builder& width(uint32_t w)      { fixed_w_ = w; return *this; }
    sized_box_builder& height(uint32_t h)     { fixed_h_ = h; return *this; }
    sized_box_builder& min_width(uint32_t w)  { min_w_ = w; return *this; }
    sized_box_builder& min_height(uint32_t h) { min_h_ = h; return *this; }
    sized_box_builder& max_width(uint32_t w)  { max_w_ = w; return *this; }
    sized_box_builder& max_height(uint32_t h) { max_h_ = h; return *this; }
    sized_box_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    sized_box_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ { virtual ~concept_() = default; virtual node_id flatten(detail::scene& s) const = 0; };
    template <typename B>
    struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene& s) const override { return builder.flatten_into(s); }
    };
    std::unique_ptr<concept_> content_;
    uint32_t fixed_w_ = 0;
    uint32_t fixed_h_ = 0;
    uint32_t min_w_   = 0;
    uint32_t min_h_   = 0;
    uint32_t max_w_   = unbounded;
    uint32_t max_h_   = unbounded;
    int16_t  z_order_ = 0;
    uint64_t key_     = 0;
};

// ── Center builder ────────────────────────────────────────────────────

class TAPIRU_API center_builder {
public:
    template <typename B>
    explicit center_builder(B&& content) : content_(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))) {}

    center_builder(center_builder&&) noexcept = default;
    center_builder& operator=(center_builder&&) noexcept = default;
    center_builder(const center_builder&) = delete;
    center_builder& operator=(const center_builder&) = delete;

    center_builder& horizontal_only() { horizontal_ = true; vertical_ = false; return *this; }
    center_builder& vertical_only()   { horizontal_ = false; vertical_ = true; return *this; }
    center_builder& z_order(int16_t z) { z_order_ = z; return *this; }
    center_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ { virtual ~concept_() = default; virtual node_id flatten(detail::scene& s) const = 0; };
    template <typename B>
    struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene& s) const override { return builder.flatten_into(s); }
    };
    std::unique_ptr<concept_> content_;
    bool     horizontal_ = true;
    bool     vertical_   = true;
    int16_t  z_order_    = 0;
    uint64_t key_        = 0;
};

}  // namespace tapiru
