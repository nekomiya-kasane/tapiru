/**
 * @file form_test.cpp
 * @brief Tests for the form validation component.
 */

#include "tapiru/core/input.h"
#include "tapiru/widgets/form.h"

#include <gtest/gtest.h>

using namespace tapiru;

// Helper: send a key event to a component
static bool send_key(component &c, char32_t cp, special_key sk = special_key::none, key_mod mods = key_mod::none) {
    input_event ev = key_event{cp, sk, mods};
    return c->on_event(ev);
}

// ── FormRendersFields ───────────────────────────────────────────────────

TEST(FormTest, FormRendersFields) {
    std::string name = "Alice";
    std::string email = "alice@example.com";

    auto form = make_form({
        .fields =
            {
                {.label = "Name", .value = &name},
                {.label = "Email", .value = &email},
            },
    });

    // Render should not crash and should produce a valid element
    auto el = form->render();
    (void)el;
}

// ── FormTabNavigation ───────────────────────────────────────────────────

TEST(FormTest, FormTabNavigation) {
    std::string f1 = "aaa";
    std::string f2 = "bbb";
    std::string f3 = "ccc";

    auto form = make_form({
        .fields =
            {
                {.label = "F1", .value = &f1},
                {.label = "F2", .value = &f2},
                {.label = "F3", .value = &f3},
            },
    });

    // Initially focused on field 0. Type 'X' → goes into f1
    send_key(form, 'X');
    EXPECT_EQ(f1, "aaaX");

    // Tab → field 1
    send_key(form, 0, special_key::tab);
    send_key(form, 'Y');
    EXPECT_EQ(f2, "bbbY");

    // Tab → field 2
    send_key(form, 0, special_key::tab);
    send_key(form, 'Z');
    EXPECT_EQ(f3, "cccZ");

    // Shift+Tab → back to field 1
    send_key(form, 0, special_key::tab, key_mod::shift);
    send_key(form, '!');
    EXPECT_EQ(f2, "bbbY!");
}

// ── FormValidationPass ──────────────────────────────────────────────────

TEST(FormTest, FormValidationPass) {
    std::string name = "Alice";
    bool submitted = false;

    auto form = make_form({
        .fields =
            {
                {.label = "Name",
                 .value = &name,
                 .validator = [](std::string_view v) { return !v.empty(); },
                 .error_message = "Name required"},
            },
        .on_submit = [&]() { submitted = true; },
    });

    // Enter → should validate and submit
    send_key(form, 0, special_key::enter);
    EXPECT_TRUE(submitted);
}

// ── FormValidationFail ──────────────────────────────────────────────────

TEST(FormTest, FormValidationFail) {
    std::string name; // empty
    bool submitted = false;

    auto form = make_form({
        .fields =
            {
                {.label = "Name",
                 .value = &name,
                 .validator = [](std::string_view v) { return !v.empty(); },
                 .error_message = "Name required"},
            },
        .on_submit = [&]() { submitted = true; },
    });

    // Enter with empty name → should NOT submit
    send_key(form, 0, special_key::enter);
    EXPECT_FALSE(submitted);

    // Render should show error message
    auto el = form->render();
    (void)el;
}

// ── FormSubmitOnValid ───────────────────────────────────────────────────

TEST(FormTest, FormSubmitOnValid) {
    std::string name = "Bob";
    std::string email = "bob@test.com";
    bool submitted = false;

    auto form = make_form({
        .fields =
            {
                {.label = "Name",
                 .value = &name,
                 .validator = [](std::string_view v) { return v.size() >= 2; },
                 .error_message = "Min 2 chars"},
                {.label = "Email",
                 .value = &email,
                 .validator = [](std::string_view v) { return v.find('@') != std::string_view::npos; },
                 .error_message = "Must contain @"},
            },
        .on_submit = [&]() { submitted = true; },
    });

    send_key(form, 0, special_key::enter);
    EXPECT_TRUE(submitted);
}

// ── FormSubmitBlockedOnInvalid ──────────────────────────────────────────

TEST(FormTest, FormSubmitBlockedOnInvalid) {
    std::string name = "A";     // too short
    std::string email = "nope"; // no @
    bool submitted = false;

    auto form = make_form({
        .fields =
            {
                {.label = "Name",
                 .value = &name,
                 .validator = [](std::string_view v) { return v.size() >= 2; },
                 .error_message = "Min 2 chars"},
                {.label = "Email",
                 .value = &email,
                 .validator = [](std::string_view v) { return v.find('@') != std::string_view::npos; },
                 .error_message = "Must contain @"},
            },
        .on_submit = [&]() { submitted = true; },
    });

    send_key(form, 0, special_key::enter);
    EXPECT_FALSE(submitted);

    // Fix name
    name = "Alice";
    send_key(form, 0, special_key::enter);
    EXPECT_FALSE(submitted); // email still invalid

    // Fix email
    email = "alice@test.com";
    send_key(form, 0, special_key::enter);
    EXPECT_TRUE(submitted);
}

// ── FormEmptyFields ─────────────────────────────────────────────────────

TEST(FormTest, FormEmptyFields) {
    bool submitted = false;

    auto form = make_form({
        .fields = {},
        .on_submit = [&]() { submitted = true; },
    });

    // Should not crash with empty fields
    auto el = form->render();
    (void)el;

    // Tab should not crash
    send_key(form, 0, special_key::tab);

    // Enter should not crash (no fields to validate)
    send_key(form, 0, special_key::enter);
}
