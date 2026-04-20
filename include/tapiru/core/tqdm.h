#pragma once

/**
 * @file tqdm.h
 * @brief Python tqdm-style progress bar for range-based for loops.
 *
 * Usage:
 *   for (auto& item : tapiru::tqdm(vec))                         { ... }
 *   for (auto& item : tapiru::tqdm(vec, "Loading"))              { ... }
 *   for (auto& x : tapiru::tqdm(vec).desc("Work").unit("files")) { ... }
 *   for (int i : tapiru::trange(100))                            { ... }
 *   for (int i : tapiru::trange(0, 1000, 10))                    { ... }
 *
 * Header-only. No dependency on tapiru shared library (uses raw ANSI).
 */

#include "tapiru/core/style.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

namespace tapiru {

// ── Integer range helper (like Python range()) ─────────────────────────

template <typename T = int> class int_range {
  public:
    int_range(T stop) : start_(0), stop_(stop), step_(1) {}
    int_range(T start, T stop) : start_(start), stop_(stop), step_(1) {}
    int_range(T start, T stop, T step) : start_(start), stop_(stop), step_(step) {}

    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = T;

        iterator(T val, T step) : val_(val), step_(step) {}
        T operator*() const { return val_; }
        iterator &operator++() {
            val_ += step_;
            return *this;
        }
        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator!=(const iterator &o) const { return step_ > 0 ? val_ < o.val_ : val_ > o.val_; }
        bool operator==(const iterator &o) const { return !(*this != o); }

      private:
        T val_, step_;
    };

    iterator begin() const { return iterator(start_, step_); }
    iterator end() const { return iterator(stop_, step_); }

    [[nodiscard]] size_t size() const {
        if (step_ > 0 && stop_ > start_) return static_cast<size_t>((stop_ - start_ + step_ - 1) / step_);
        if (step_ < 0 && start_ > stop_) return static_cast<size_t>((start_ - stop_ - step_ - 1) / (-step_));
        return 0;
    }

  private:
    T start_, stop_, step_;
};

// ── tqdm_bar: shared rendering state ───────────────────────────────────

namespace detail {

using tqdm_clock = std::chrono::steady_clock;
using tqdm_time = tqdm_clock::time_point;

inline void format_time(char *buf, size_t n, double secs) {
    if (secs < 0) secs = 0;
    int h = static_cast<int>(secs / 3600);
    int m = static_cast<int>(static_cast<int>(secs) % 3600 / 60);
    int s = static_cast<int>(secs) % 60;
    if (h > 0)
        std::snprintf(buf, n, "%d:%02d:%02d", h, m, s);
    else
        std::snprintf(buf, n, "%02d:%02d", m, s);
}

inline void format_rate(char *buf, size_t n, double rate, const char *unit) {
    if (rate < 1.0)
        std::snprintf(buf, n, "%.2f%s/s", rate, unit);
    else if (rate < 1000.0)
        std::snprintf(buf, n, "%.1f%s/s", rate, unit);
    else if (rate < 1e6)
        std::snprintf(buf, n, "%.1fk%s/s", rate / 1e3, unit);
    else
        std::snprintf(buf, n, "%.1fM%s/s", rate / 1e6, unit);
}

class tqdm_bar {
  public:
    tqdm_bar() = default;

    void init(size_t total, const std::string &desc, int bar_width, const std::string &unit, bool disable, bool leave,
              int miniters, double mininterval, color colour, FILE *file) {
        total_ = total;
        desc_ = desc;
        bar_width_ = bar_width;
        unit_ = unit;
        disable_ = disable;
        leave_ = leave;
        miniters_ = miniters;
        mininterval_ = mininterval;
        colour_ = colour;
        file_ = file;
        start_ = tqdm_clock::now();
        last_print_ = start_;
        started_ = true;
    }

    void update(size_t current) {
        if (disable_ || !started_) return;
        current_ = current;
        ++since_last_print_;

        if (since_last_print_ < static_cast<size_t>(miniters_)) return;

        auto now = tqdm_clock::now();
        double dt = std::chrono::duration<double>(now - last_print_).count();
        if (dt < mininterval_) return;

        print_bar(current_, false);
        last_print_ = now;
        since_last_print_ = 0;
    }

    void close() {
        if (disable_ || !started_ || closed_) return;
        closed_ = true;
        if (total_ > 0) current_ = total_; // ensure 100% on final print
        print_bar(current_, true);
        if (leave_) {
            std::fputc('\n', file_);
        } else {
            std::fprintf(file_, "\r\x1b[2K");
        }
        std::fflush(file_);
    }

  private:
    void print_bar(size_t current, bool final) {
        double elapsed = std::chrono::duration<double>(tqdm_clock::now() - start_).count();
        double rate = elapsed > 0 ? static_cast<double>(current) / elapsed : 0;

        char line[512];
        char *p = line;
        char *end = line + sizeof(line) - 1;

        // \r to overwrite
        *p++ = '\r';

        // Description
        if (!desc_.empty()) {
            int n = std::snprintf(p, static_cast<size_t>(end - p), "%s: ", desc_.c_str());
            if (n > 0) p += n;
        }

        if (total_ > 0) {
            // Percentage
            double pct = static_cast<double>(current) / static_cast<double>(total_) * 100.0;
            if (pct > 100.0) pct = 100.0;
            int n = std::snprintf(p, static_cast<size_t>(end - p), "%3.0f%%|", pct);
            if (n > 0) p += n;

            // Bar with colour
            int filled = static_cast<int>(pct / 100.0 * bar_width_);
            if (filled > bar_width_) filled = bar_width_;

            // ANSI colour start
            if (colour_.r || colour_.g || colour_.b) {
                n = std::snprintf(p, static_cast<size_t>(end - p), "\x1b[38;2;%u;%u;%um", colour_.r, colour_.g,
                                  colour_.b);
                if (n > 0) p += n;
            }

            for (int i = 0; i < filled && p < end; ++i) {
                // UTF-8 for █ (U+2588): 0xE2 0x96 0x88
                if (p + 3 <= end) {
                    *p++ = '\xe2';
                    *p++ = '\x96';
                    *p++ = '\x88';
                }
            }

            // Reset colour for unfilled
            if (colour_.r || colour_.g || colour_.b) {
                n = std::snprintf(p, static_cast<size_t>(end - p), "\x1b[0m");
                if (n > 0) p += n;
            }

            for (int i = filled; i < bar_width_ && p < end; ++i) {
                // UTF-8 for ░ (U+2591): 0xE2 0x96 0x91
                if (p + 3 <= end) {
                    *p++ = '\xe2';
                    *p++ = '\x96';
                    *p++ = '\x91';
                }
            }

            *p++ = '|';
            *p++ = ' ';

            // Counter
            n = std::snprintf(p, static_cast<size_t>(end - p), "%zu/%zu", current, total_);
            if (n > 0) p += n;

            // Rate
            char rate_buf[32];
            format_rate(rate_buf, sizeof(rate_buf), rate, unit_.c_str());

            // Elapsed
            char elapsed_buf[16];
            format_time(elapsed_buf, sizeof(elapsed_buf), elapsed);

            // ETA
            char eta_buf[16];
            if (current > 0 && current < total_) {
                double remaining = elapsed / static_cast<double>(current) * static_cast<double>(total_ - current);
                format_time(eta_buf, sizeof(eta_buf), remaining);
            } else if (current >= total_) {
                format_time(eta_buf, sizeof(eta_buf), 0);
            } else {
                std::snprintf(eta_buf, sizeof(eta_buf), "?");
            }

            n = std::snprintf(p, static_cast<size_t>(end - p), " [%s<%s, %s]", elapsed_buf, eta_buf, rate_buf);
            if (n > 0) p += n;
        } else {
            // Unknown total: just show count + rate + elapsed
            char elapsed_buf[16];
            format_time(elapsed_buf, sizeof(elapsed_buf), elapsed);
            char rate_buf[32];
            format_rate(rate_buf, sizeof(rate_buf), rate, unit_.c_str());

            int n = std::snprintf(p, static_cast<size_t>(end - p), "%zu%s [%s, %s]", current, unit_.c_str(),
                                  elapsed_buf, rate_buf);
            if (n > 0) p += n;
        }

        // Clear to end of line
        if (p + 4 <= end) {
            *p++ = '\x1b';
            *p++ = '[';
            *p++ = 'K';
        }

        *p = '\0';
        std::fputs(line, file_);
        std::fflush(file_);
    }

    size_t total_ = 0;
    std::string desc_;
    int bar_width_ = 30;
    std::string unit_ = "it";
    bool disable_ = false;
    bool leave_ = true;
    int miniters_ = 1;
    double mininterval_ = 0.1;
    color colour_ = color::from_rgb(0, 180, 0);
    FILE *file_ = stderr;

    tqdm_time start_;
    tqdm_time last_print_;
    size_t current_ = 0;
    size_t since_last_print_ = 0;
    bool started_ = false;
    bool closed_ = false;
};

} // namespace detail

// ── tqdm_range: wraps any iterable ─────────────────────────────────────

template <typename Range> class tqdm_range {
    using Iter = decltype(std::begin(std::declval<Range &>()));
    using Val = typename std::iterator_traits<Iter>::value_type;

  public:
    explicit tqdm_range(Range &range) : range_(&range) { total_ = compute_size(range); }

    tqdm_range(Range &range, const char *description) : range_(&range), desc_(description) {
        total_ = compute_size(range);
    }

    // Builder methods (return *this for chaining)
    tqdm_range &desc(const char *d) {
        desc_ = d;
        return *this;
    }
    tqdm_range &desc(const std::string &d) {
        desc_ = d;
        return *this;
    }
    tqdm_range &bar_width(int w) {
        bar_width_ = w;
        return *this;
    }
    tqdm_range &unit(const char *u) {
        unit_ = u;
        return *this;
    }
    tqdm_range &unit(const std::string &u) {
        unit_ = u;
        return *this;
    }
    tqdm_range &total(size_t t) {
        total_ = t;
        return *this;
    }
    tqdm_range &disable(bool d = true) {
        disable_ = d;
        return *this;
    }
    tqdm_range &leave(bool l = true) {
        leave_ = l;
        return *this;
    }
    tqdm_range &miniters(int m) {
        miniters_ = m;
        return *this;
    }
    tqdm_range &mininterval(double s) {
        mininterval_ = s;
        return *this;
    }
    tqdm_range &colour(color c) {
        colour_ = c;
        return *this;
    }
    tqdm_range &file(FILE *f) {
        file_ = f;
        return *this;
    }

    // Iterator that wraps the underlying iterator and updates the bar
    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Val;
        using difference_type = std::ptrdiff_t;
        using pointer = Val *;
        using reference = decltype(*std::declval<Iter>());

        iterator(Iter it, detail::tqdm_bar *bar, size_t idx) : it_(it), bar_(bar), idx_(idx) {}

        reference operator*() const { return *it_; }
        pointer operator->() const { return &(*it_); }

        iterator &operator++() {
            ++it_;
            ++idx_;
            if (bar_) bar_->update(idx_);
            return *this;
        }

        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator!=(const iterator &o) const { return it_ != o.it_; }
        bool operator==(const iterator &o) const { return it_ == o.it_; }

      private:
        Iter it_;
        detail::tqdm_bar *bar_;
        size_t idx_;
    };

    iterator begin() {
        bar_.init(total_, desc_, bar_width_, unit_, disable_, leave_, miniters_, mininterval_, colour_, file_);
        bar_.update(0);
        return iterator(std::begin(*range_), &bar_, 0);
    }

    iterator end() { return iterator(std::end(*range_), nullptr, total_); }

    ~tqdm_range() { bar_.close(); }

    // Non-copyable, movable
    tqdm_range(const tqdm_range &) = delete;
    tqdm_range &operator=(const tqdm_range &) = delete;
    tqdm_range(tqdm_range &&) = default;
    tqdm_range &operator=(tqdm_range &&) = default;

  private:
    template <typename R> static size_t compute_size(R &r) {
        if constexpr (requires { r.size(); }) {
            return static_cast<size_t>(r.size());
        } else if constexpr (requires { std::distance(std::begin(r), std::end(r)); }) {
            return static_cast<size_t>(std::distance(std::begin(r), std::end(r)));
        } else {
            return 0;
        }
    }

    Range *range_;
    detail::tqdm_bar bar_;
    std::string desc_;
    int bar_width_ = 30;
    std::string unit_ = "it";
    size_t total_ = 0;
    bool disable_ = false;
    bool leave_ = true;
    int miniters_ = 1;
    double mininterval_ = 0.1;
    color colour_ = color::from_rgb(0, 180, 0);
    FILE *file_ = stderr;
};

// ── Factory functions ──────────────────────────────────────────────────

/** @brief Wrap any range with a tqdm progress bar. */
template <typename Range> tqdm_range<Range> tqdm(Range &range) {
    return tqdm_range<Range>(range);
}

/** @brief Wrap any range with a tqdm progress bar and description. */
template <typename Range> tqdm_range<Range> tqdm(Range &range, const char *desc) {
    return tqdm_range<Range>(range, desc);
}

// ── trange: integer range with built-in progress bar ───────────────────

template <typename T = int> class trange_range {
  public:
    trange_range(T start, T stop, T step) : range_(start, stop, step), total_(range_.size()) {}

    trange_range &desc(const char *d) {
        desc_ = d;
        return *this;
    }
    trange_range &desc(const std::string &d) {
        desc_ = d;
        return *this;
    }
    trange_range &bar_width(int w) {
        bar_width_ = w;
        return *this;
    }
    trange_range &unit(const char *u) {
        unit_ = u;
        return *this;
    }
    trange_range &unit(const std::string &u) {
        unit_ = u;
        return *this;
    }
    trange_range &total(size_t t) {
        total_ = t;
        return *this;
    }
    trange_range &disable(bool d = true) {
        disable_ = d;
        return *this;
    }
    trange_range &leave(bool l = true) {
        leave_ = l;
        return *this;
    }
    trange_range &miniters(int m) {
        miniters_ = m;
        return *this;
    }
    trange_range &mininterval(double s) {
        mininterval_ = s;
        return *this;
    }
    trange_range &colour(color c) {
        colour_ = c;
        return *this;
    }
    trange_range &file(FILE *f) {
        file_ = f;
        return *this;
    }

    using inner_iter = typename int_range<T>::iterator;

    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = T;

        iterator(inner_iter it, detail::tqdm_bar *bar, size_t idx) : it_(it), bar_(bar), idx_(idx) {}

        T operator*() const { return *it_; }
        iterator &operator++() {
            ++it_;
            ++idx_;
            if (bar_) bar_->update(idx_);
            return *this;
        }
        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator!=(const iterator &o) const { return it_ != o.it_; }
        bool operator==(const iterator &o) const { return it_ == o.it_; }

      private:
        inner_iter it_;
        detail::tqdm_bar *bar_;
        size_t idx_;
    };

    iterator begin() {
        bar_.init(total_, desc_, bar_width_, unit_, disable_, leave_, miniters_, mininterval_, colour_, file_);
        bar_.update(0);
        return iterator(range_.begin(), &bar_, 0);
    }

    iterator end() { return iterator(range_.end(), nullptr, total_); }

    ~trange_range() { bar_.close(); }

    trange_range(const trange_range &) = delete;
    trange_range &operator=(const trange_range &) = delete;
    trange_range(trange_range &&) = default;
    trange_range &operator=(trange_range &&) = default;

  private:
    int_range<T> range_;
    detail::tqdm_bar bar_;
    size_t total_;
    std::string desc_;
    int bar_width_ = 30;
    std::string unit_ = "it";
    bool disable_ = false;
    bool leave_ = true;
    int miniters_ = 1;
    double mininterval_ = 0.1;
    color colour_ = color::from_rgb(0, 180, 0);
    FILE *file_ = stderr;
};

/** @brief Integer range with progress bar (like Python tqdm(range(n))). */
template <typename T = int> trange_range<T> trange(T stop) {
    return trange_range<T>(T(0), stop, T(1));
}

/** @brief Integer range with progress bar. */
template <typename T = int> trange_range<T> trange(T start, T stop) {
    return trange_range<T>(start, stop, T(1));
}

/** @brief Integer range with progress bar and step. */
template <typename T = int> trange_range<T> trange(T start, T stop, T step) {
    return trange_range<T>(start, stop, step);
}

} // namespace tapiru
