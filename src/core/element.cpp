/**
 * @file element.cpp
 * @brief element type-erased wrapper implementation.
 */

#include "tapiru/core/element.h"

#include "tapiru/widgets/builders.h"

namespace tapiru {

    node_id element::flatten_into(detail::scene &s) const {
        if (!impl_) {
            // Empty element: flatten as a zero-size spacer
            return spacer_builder().flatten_into(s);
        }
        return impl_->flatten(s);
    }

    element operator|(element e, const decorator &d) {
        if (!d) {
            return e;
        }
        return d(std::move(e));
    }

    element operator|(element e, const style &s) {
        // Style shortcut: wrap the element in a styled container.
        // For now, this is a pass-through until decorator.cpp provides
        // the full style-application decorator. We store the element
        // and style together via a small internal builder.
        if (s.is_default()) {
            return e;
        }

        struct styled_builder {
            element inner;
            style sty;
            node_id flatten_into(detail::scene &sc) const {
                // Delegate to inner — style application happens at the
                // decorator level once the full decorator infrastructure
                // is wired up. For now, just flatten the inner element.
                return inner.flatten_into(sc);
            }
        };

        return element(styled_builder{std::move(e), s});
    }

} // namespace tapiru
