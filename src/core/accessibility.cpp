/**
 * @file accessibility.cpp
 * @brief Implementation of accessibility role utilities and decorator stubs.
 */

#include "tapiru/core/accessibility.h"

namespace tapiru {

    std::string_view role_name(aria_role r) noexcept {
        switch (r) {
        case aria_role::none:
            return "none";
        case aria_role::banner:
            return "banner";
        case aria_role::navigation:
            return "navigation";
        case aria_role::main:
            return "main";
        case aria_role::complementary:
            return "complementary";
        case aria_role::contentinfo:
            return "contentinfo";
        case aria_role::form:
            return "form";
        case aria_role::search:
            return "search";
        case aria_role::region:
            return "region";
        case aria_role::button:
            return "button";
        case aria_role::checkbox:
            return "checkbox";
        case aria_role::dialog:
            return "dialog";
        case aria_role::link:
            return "link";
        case aria_role::listbox:
            return "listbox";
        case aria_role::list_item:
            return "list_item";
        case aria_role::menu:
            return "menu";
        case aria_role::menu_item:
            return "menu_item";
        case aria_role::option:
            return "option";
        case aria_role::progressbar:
            return "progressbar";
        case aria_role::radio:
            return "radio";
        case aria_role::scrollbar:
            return "scrollbar";
        case aria_role::separator:
            return "separator";
        case aria_role::slider:
            return "slider";
        case aria_role::spinbutton:
            return "spinbutton";
        case aria_role::status:
            return "status";
        case aria_role::tab:
            return "tab";
        case aria_role::tab_panel:
            return "tab_panel";
        case aria_role::textbox:
            return "textbox";
        case aria_role::toolbar:
            return "toolbar";
        case aria_role::tooltip:
            return "tooltip";
        case aria_role::tree:
            return "tree";
        case aria_role::tree_item:
            return "tree_item";
        case aria_role::alert:
            return "alert";
        case aria_role::log:
            return "log";
        case aria_role::marquee:
            return "marquee";
        case aria_role::timer:
            return "timer";
        case aria_role::heading:
            return "heading";
        case aria_role::img:
            return "img";
        case aria_role::table:
            return "table";
        case aria_role::row:
            return "row";
        case aria_role::cell:
            return "cell";
        case aria_role::group:
            return "group";
        default:
            return "unknown";
        }
    }

    decorator role(aria_role r, std::string_view label) {
        // TODO: integrate with element a11y tree once element gains a11y_props member
        return [r, lbl = std::string(label)](element e) -> element {
            (void)r;
            (void)lbl;
            return e;
        };
    }

    decorator accessible(a11y_props props) {
        // TODO: integrate with element a11y tree once element gains a11y_props member
        return [p = std::move(props)](element e) -> element {
            (void)p;
            return e;
        };
    }

} // namespace tapiru
