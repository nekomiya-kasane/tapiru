#pragma once

/**
 * @file widget_registry.h
 * @brief Static dispatch table for widget measure/render operations (internal).
 */

#include "detail/scene.h"
#include "tapiru/core/canvas.h"
#include "tapiru/core/style_table.h"
#include "tapiru/layout/types.h"

namespace tapiru::detail {

/**
 * @brief Context passed to all render functions.
 *
 * Bundles mutable scene (for rect writeback), canvas, style table,
 * and frame timing information for animation.
 */
struct render_context {
    scene&              sc;
    canvas&             cv;
    const style_table&  styles;
    double              frame_time   = 0.0;   // seconds since live start, for animation
    uint32_t            frame_number = 0;
};

/** @brief Measure a widget: returns desired size given constraints. */
using measure_fn = measurement(*)(const scene& s, node_id id, box_constraints bc);

/** @brief Render a widget into a canvas region. */
using render_fn = void(*)(render_context& ctx, node_id id, rect area);

/** @brief Function pointers for a widget type. */
struct widget_ops {
    measure_fn measure;
    render_fn  render;
};

// Forward declarations — each widget type implements these
measurement measure_text(const scene& s, node_id id, box_constraints bc);
void        render_text(render_context& ctx, node_id id, rect area);

measurement measure_panel(const scene& s, node_id id, box_constraints bc);
void        render_panel(render_context& ctx, node_id id, rect area);

measurement measure_rule(const scene& s, node_id id, box_constraints bc);
void        render_rule(render_context& ctx, node_id id, rect area);

measurement measure_padding(const scene& s, node_id id, box_constraints bc);
void        render_padding(render_context& ctx, node_id id, rect area);

measurement measure_columns(const scene& s, node_id id, box_constraints bc);
void        render_columns(render_context& ctx, node_id id, rect area);

measurement measure_table(const scene& s, node_id id, box_constraints bc);
void        render_table(render_context& ctx, node_id id, rect area);

measurement measure_overlay(const scene& s, node_id id, box_constraints bc);
void        render_overlay(render_context& ctx, node_id id, rect area);

measurement measure_rows(const scene& s, node_id id, box_constraints bc);
void        render_rows(render_context& ctx, node_id id, rect area);

measurement measure_spacer(const scene& s, node_id id, box_constraints bc);
void        render_spacer(render_context& ctx, node_id id, rect area);

measurement measure_menu(const scene& s, node_id id, box_constraints bc);
void        render_menu(render_context& ctx, node_id id, rect area);

measurement measure_sized_box(const scene& s, node_id id, box_constraints bc);
void        render_sized_box(render_context& ctx, node_id id, rect area);

measurement measure_center(const scene& s, node_id id, box_constraints bc);
void        render_center(render_context& ctx, node_id id, rect area);

measurement measure_scroll_view(const scene& s, node_id id, box_constraints bc);
void        render_scroll_view(render_context& ctx, node_id id, rect area);

measurement measure_canvas_widget(const scene& s, node_id id, box_constraints bc);
void        render_canvas_widget(render_context& ctx, node_id id, rect area);

/** @brief Global dispatch table indexed by widget_type. */
inline const widget_ops g_widget_ops[] = {
    /* text    */ { measure_text,    render_text    },
    /* panel   */ { measure_panel,   render_panel   },
    /* rule    */ { measure_rule,    render_rule    },
    /* padding */ { measure_padding, render_padding },
    /* columns */ { measure_columns, render_columns },
    /* table   */ { measure_table,   render_table   },
    /* overlay */ { measure_overlay, render_overlay },
    /* rows    */ { measure_rows,    render_rows    },
    /* spacer  */ { measure_spacer,  render_spacer  },
    /* menu    */      { measure_menu,      render_menu      },
    /* sized_box */    { measure_sized_box, render_sized_box },
    /* center */       { measure_center,    render_center    },
    /* scroll_view */  { measure_scroll_view, render_scroll_view },
    /* canvas_widget */ { measure_canvas_widget, render_canvas_widget },
};

static_assert(sizeof(g_widget_ops) / sizeof(g_widget_ops[0]) ==
              static_cast<size_t>(widget_type::count_));

/** @brief Dispatch measure for any node. */
inline measurement dispatch_measure(const scene& s, node_id id, box_constraints bc) {
    return g_widget_ops[static_cast<uint8_t>(s.type_of(id))].measure(s, id, bc);
}

/** @brief Dispatch render for any node. */
inline void dispatch_render(render_context& ctx, node_id id, rect area) {
    g_widget_ops[static_cast<uint8_t>(ctx.sc.type_of(id))].render(ctx, id, area);
}

/** @brief Convenience overload: creates a render_context and dispatches. */
inline void dispatch_render(scene& s, node_id id, canvas& c, rect area,
                            const style_table& styles) {
    render_context ctx{s, c, styles};
    dispatch_render(ctx, id, area);
}

}  // namespace tapiru::detail
