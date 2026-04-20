#pragma once

/**
 * @file element.h
 * @brief Type-erased element wrapper + decorator pipe operator.
 *
 * Any builder with a `flatten_into(detail::scene&) -> node_id` method
 * can be implicitly converted to an element. Elements compose via
 * decorator pipe syntax: `elem | border() | bold()`.
 */

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>

#include "tapiru/core/style.h"
#include "tapiru/exports.h"

namespace tapiru {

namespace detail { class scene; }

using node_id = uint32_t;

// ── flatten_into concept ────────────────────────────────────────────────

namespace detail {

template <typename B>
concept flattenable = requires(B b, detail::scene& s) {
    { b.flatten_into(s) } -> std::same_as<node_id>;
};

}  // namespace detail

// ── element ─────────────────────────────────────────────────────────────

class TAPIRU_API element {
public:
    element() = default;

    template <detail::flattenable B>
        requires (!std::same_as<std::decay_t<B>, element>)
    element(B&& builder)  // NOLINT(google-explicit-constructor)
        : impl_(std::make_shared<model_<std::decay_t<B>>>(std::forward<B>(builder))) {}

    [[nodiscard]] bool empty() const noexcept { return !impl_; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct concept_ {
        virtual ~concept_() = default;
        virtual node_id flatten(detail::scene& s) const = 0;
    };

    template <typename B>
    struct model_ final : concept_ {
        B builder_;
        explicit model_(B b) : builder_(std::move(b)) {}
        node_id flatten(detail::scene& s) const override {
            return builder_.flatten_into(s);
        }
    };

    std::shared_ptr<concept_> impl_;
};

// ── decorator ───────────────────────────────────────────────────────────

using decorator = std::function<element(element)>;

TAPIRU_API element operator|(element e, const decorator& d);
TAPIRU_API element operator|(element e, const style& s);

}  // namespace tapiru
