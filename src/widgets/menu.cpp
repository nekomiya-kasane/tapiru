/**
 * @file menu.cpp
 * @brief menu_tree, menu_state, and menu_builder implementations.
 */

#include "tapiru/widgets/menu.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

#include <algorithm>
#include <cmath>

namespace tapiru {

// ── menu_tree ────────────────────────────────────────────────────────────

void menu_tree::build_node(node &n, const menu_item_builder &b) {
    n.label = b.label_;
    n.shortcut = b.shortcut_;
    n.separator = b.separator_;
    n.disabled = b.disabled_;
    n.checked = b.checked_;
    n.icon = b.icon_;

    if (b.separator_) {
        n.global_index = -1;
    } else {
        n.global_index = next_index_++;
    }

    for (const auto &child : b.children_) {
        node cn;
        build_node(cn, child);
        n.children.push_back(std::move(cn));
    }
}

void menu_tree::add_item(const menu_item_builder &b) {
    node n;
    build_node(n, b);
    roots_.push_back(std::move(n));
}

void menu_tree::add_separator() {
    node n;
    n.separator = true;
    n.global_index = -1;
    roots_.push_back(std::move(n));
}

const std::vector<menu_tree::node> &menu_tree::children_at(const std::vector<int> &path) const {
    const std::vector<node> *current = &roots_;
    for (int idx : path) {
        if (idx < 0 || idx >= static_cast<int>(current->size())) {
            return roots_;
        }
        current = &(*current)[static_cast<size_t>(idx)].children;
    }
    return *current;
}

int menu_tree::next_selectable(const std::vector<node> &items, int from, int dir) const {
    if (items.empty()) {
        return -1;
    }
    int n = static_cast<int>(items.size());
    int pos = from;
    for (int i = 0; i < n; ++i) {
        pos = (pos + dir + n) % n;
        if (!items[static_cast<size_t>(pos)].separator && !items[static_cast<size_t>(pos)].disabled) {
            return pos;
        }
    }
    return from; // no selectable item found
}

// ── menu_state ───────────────────────────────────────────────────────────

menu_state::menu_state() {
    path_.push_back(0);
}

int menu_state::cursor() const {
    return path_.empty() ? 0 : path_.back();
}

int menu_state::cursor_at(int d) const {
    if (d < 0 || d >= static_cast<int>(path_.size())) {
        return -1;
    }
    return path_[static_cast<size_t>(d)];
}

int menu_state::depth() const {
    return static_cast<int>(path_.size()) - 1;
}

bool menu_state::is_submenu_open() const {
    return path_.size() > 1;
}

int menu_state::selected_item() const {
    return selected_;
}

bool menu_state::was_selected() const {
    return was_selected_;
}

bool menu_state::is_keyboard_mode() const {
    return keyboard_mode_;
}

const std::vector<menu_tree::node> &menu_state::items_at_depth(const menu_tree &tree, int d) const {
    if (d == 0) {
        return tree.roots();
    }
    // Build path to depth d-1
    std::vector<int> sub_path(path_.begin(), path_.begin() + d);
    return tree.children_at(sub_path);
}

void menu_state::move_up(const menu_tree &tree) {
    keyboard_mode_ = true;
    auto &items = items_at_depth(tree, depth());
    int cur = cursor();
    int next = tree.next_selectable(items, cur, -1);
    path_.back() = next;
}

void menu_state::move_down(const menu_tree &tree) {
    keyboard_mode_ = true;
    auto &items = items_at_depth(tree, depth());
    int cur = cursor();
    int next = tree.next_selectable(items, cur, 1);
    path_.back() = next;
}

void menu_state::move_home(const menu_tree &tree) {
    keyboard_mode_ = true;
    auto &items = items_at_depth(tree, depth());
    int next = tree.next_selectable(items, static_cast<int>(items.size()) - 1, 1);
    path_.back() = next;
}

void menu_state::move_end(const menu_tree &tree) {
    keyboard_mode_ = true;
    auto &items = items_at_depth(tree, depth());
    int next = tree.next_selectable(items, 0, -1);
    path_.back() = next;
}

void menu_state::open_submenu(const menu_tree &tree) {
    keyboard_mode_ = true;
    auto &items = items_at_depth(tree, depth());
    int cur = cursor();
    if (cur < 0 || cur >= static_cast<int>(items.size())) {
        return;
    }
    const auto &item = items[static_cast<size_t>(cur)];
    if (!item.has_submenu() || item.disabled) {
        return;
    }

    // Find first selectable in submenu
    int first = tree.next_selectable(item.children, static_cast<int>(item.children.size()) - 1, 1);
    path_.push_back(first);
}

void menu_state::close_submenu() {
    keyboard_mode_ = true;
    if (path_.size() > 1) {
        path_.pop_back();
    }
}

void menu_state::select(const menu_tree &tree) {
    auto &items = items_at_depth(tree, depth());
    int cur = cursor();
    if (cur < 0 || cur >= static_cast<int>(items.size())) {
        return;
    }
    const auto &item = items[static_cast<size_t>(cur)];

    if (item.disabled || item.separator) {
        return;
    }

    if (item.has_submenu()) {
        open_submenu(tree);
        return;
    }

    selected_ = item.global_index;
    was_selected_ = true;
}

void menu_state::type_char(char32_t ch, const menu_tree &tree) {
    keyboard_mode_ = true;
    auto &items = items_at_depth(tree, depth());
    int cur = cursor();
    int n = static_cast<int>(items.size());

    // Search from current+1, wrapping around
    for (int i = 1; i <= n; ++i) {
        int idx = (cur + i) % n;
        const auto &item = items[static_cast<size_t>(idx)];
        if (item.separator || item.disabled) {
            continue;
        }
        if (!item.label.empty()) {
            char32_t first = static_cast<char32_t>(static_cast<unsigned char>(item.label[0]));
            // Case-insensitive compare for ASCII
            if (first >= 'A' && first <= 'Z') {
                first += 32;
            }
            char32_t target = ch;
            if (target >= 'A' && target <= 'Z') {
                target += 32;
            }
            if (first == target) {
                path_.back() = idx;
                return;
            }
        }
    }
}

void menu_state::hover(int hover_depth, int item_index, int mouse_x, int mouse_y, const menu_tree &tree) {
    keyboard_mode_ = false;
    leave_pending_ = false;

    // If hovering at a depth shallower than current, close deeper submenus
    if (hover_depth < depth()) {
        path_.resize(static_cast<size_t>(hover_depth) + 1);
    }

    // Ensure path is deep enough
    while (static_cast<int>(path_.size()) <= hover_depth) {
        path_.push_back(0);
    }

    auto &items = items_at_depth(tree, hover_depth);
    if (item_index < 0 || item_index >= static_cast<int>(items.size())) {
        return;
    }
    const auto &item = items[static_cast<size_t>(item_index)];
    if (item.separator) {
        return;
    }

    path_[static_cast<size_t>(hover_depth)] = item_index;

    // Handle submenu open/close on hover
    if (item.has_submenu() && !item.disabled) {
        safe_px_ = mouse_x;
        safe_py_ = mouse_y;
        pending_open_item_ = item_index;
        open_timer_ = open_delay_;
    } else {
        pending_open_item_ = -1;
        // Close any deeper submenus
        if (static_cast<int>(path_.size()) > hover_depth + 1) {
            close_pending_ = true;
            close_timer_ = close_delay_;
        }
    }
}

void menu_state::click(int click_depth, int item_index, const menu_tree &tree) {
    keyboard_mode_ = false;
    leave_pending_ = false;

    // Ensure path depth
    while (static_cast<int>(path_.size()) <= click_depth) {
        path_.push_back(0);
    }
    if (click_depth < depth()) {
        path_.resize(static_cast<size_t>(click_depth) + 1);
    }

    auto &items = items_at_depth(tree, click_depth);
    if (item_index < 0 || item_index >= static_cast<int>(items.size())) {
        return;
    }
    const auto &item = items[static_cast<size_t>(item_index)];

    if (item.separator || item.disabled) {
        return;
    }

    path_[static_cast<size_t>(click_depth)] = item_index;

    if (item.has_submenu()) {
        // Immediately open submenu on click (no delay)
        int first = tree.next_selectable(item.children, static_cast<int>(item.children.size()) - 1, 1);
        path_.push_back(first);
        pending_open_item_ = -1;
    } else {
        selected_ = item.global_index;
        was_selected_ = true;
    }
}

void menu_state::mouse_leave() {
    leave_pending_ = true;
    leave_timer_ = leave_delay_;
}

void menu_state::tick(std::chrono::milliseconds dt) {
    // Open delay
    if (pending_open_item_ >= 0) {
        if (open_timer_ <= dt) {
            // Open the submenu
            auto cur = cursor();
            if (cur == pending_open_item_ && static_cast<int>(path_.size()) == depth() + 1) {
                // path_ is at the right depth, just push
                // Actually we need the tree to check, but tick doesn't have it
                // So we just push a 0 cursor for the submenu
                path_.push_back(0);
            }
            pending_open_item_ = -1;
            open_timer_ = std::chrono::milliseconds{0};
        } else {
            open_timer_ -= dt;
        }
    }

    // Close delay
    if (close_pending_) {
        if (close_timer_ <= dt) {
            // Close deepest submenu
            if (path_.size() > 1) {
                path_.pop_back();
            }
            close_pending_ = false;
            close_timer_ = std::chrono::milliseconds{0};
        } else {
            close_timer_ -= dt;
        }
    }

    // Leave delay
    if (leave_pending_) {
        if (leave_timer_ <= dt) {
            close_all();
            leave_pending_ = false;
            leave_timer_ = std::chrono::milliseconds{0};
        } else {
            leave_timer_ -= dt;
        }
    }
}

void menu_state::clear_selection() {
    selected_ = -1;
    was_selected_ = false;
}

void menu_state::close_all() {
    path_.clear();
    path_.push_back(0);
    pending_open_item_ = -1;
    close_pending_ = false;
    leave_pending_ = false;
}

void menu_state::set_submenu_rect(int /*depth*/, rect r) {
    submenu_rect_ = r;
}

bool menu_state::is_in_safe_triangle(int mouse_x, int mouse_y) const {
    if (!is_submenu_open()) {
        return false;
    }
    if (submenu_rect_.empty()) {
        return false;
    }

    // Triangle: P1 = (safe_px_, safe_py_), P2 = submenu top-left, P3 = submenu bottom-left
    int p1x = safe_px_, p1y = safe_py_;
    int p2x = static_cast<int>(submenu_rect_.x);
    int p2y = static_cast<int>(submenu_rect_.y);
    int p3x = static_cast<int>(submenu_rect_.x);
    int p3y = static_cast<int>(submenu_rect_.y + submenu_rect_.h);

    // Cross product sign test for point-in-triangle
    auto sign = [](int ax, int ay, int bx, int by, int cx, int cy) -> int {
        return (ax - cx) * (by - cy) - (bx - cx) * (ay - cy);
    };

    int d1 = sign(mouse_x, mouse_y, p1x, p1y, p2x, p2y);
    int d2 = sign(mouse_x, mouse_y, p2x, p2y, p3x, p3y);
    int d3 = sign(mouse_x, mouse_y, p3x, p3y, p1x, p1y);

    bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

// ── menu_builder ─────────────────────────────────────────────────────────

menu_builder &menu_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id menu_builder::flatten_into(detail::scene &s) const {
    detail::menu_data md;

    // Recursive conversion from menu_item_builder to detail::menu_item
    std::function<void(const std::vector<menu_item_builder> &, std::vector<detail::menu_item> &)> convert =
        [&convert](const std::vector<menu_item_builder> &builders, std::vector<detail::menu_item> &out) {
            for (const auto &b : builders) {
                detail::menu_item mi;
                mi.label = b.label_;
                mi.shortcut = b.shortcut_;
                mi.separator = b.separator_;
                mi.disabled = b.disabled_;
                mi.checked = b.checked_;
                mi.icon = b.icon_;
                if (!b.children_.empty()) {
                    convert(b.children_, mi.children);
                }
                out.push_back(std::move(mi));
            }
        };
    convert(items_, md.items);

    md.cursor = cursor_;
    md.border = border_;
    md.border_sty = border_sty_;
    md.highlight_sty = highlight_sty_;
    md.shortcut_sty = shortcut_sty_;
    md.item_sty = item_sty_;
    md.disabled_sty = disabled_sty_;
    if (state_) {
        md.open_path = &state_->open_path();
    }
    if (shadow_) {
        md.shadow = detail::shadow_config{shadow_->offset_x, shadow_->offset_y, shadow_->blur, shadow_->shadow_color,
                                          shadow_->opacity};
    }

    auto pi = s.add_menu(std::move(md));
    auto id = s.add_node(detail::widget_type::menu, pi, detail::no_node, key_);
    if (z_order_ != 0) {
        s.set_z_order(id, z_order_);
    }
    return id;
}

} // namespace tapiru
