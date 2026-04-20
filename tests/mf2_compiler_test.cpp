/**
 * @file mf2_compiler_test.cpp
 * @brief Tests for the MF2 bytecode compiler AST types and compilation unit.
 */

#include "lemonade/i18n/mf2_compiler.h"

#include <gtest/gtest.h>

using namespace lemonade::i18n;

// ── mf2_expr_kind enum ──────────────────────────────────────────────────

TEST(Mf2CompilerTest, ExprKindValues) {
    EXPECT_EQ(static_cast<uint8_t>(mf2_expr_kind::literal), 0u);
    EXPECT_EQ(static_cast<uint8_t>(mf2_expr_kind::variable), 1u);
    EXPECT_EQ(static_cast<uint8_t>(mf2_expr_kind::function_call), 2u);
    EXPECT_EQ(static_cast<uint8_t>(mf2_expr_kind::markup_open), 3u);
    EXPECT_EQ(static_cast<uint8_t>(mf2_expr_kind::markup_close), 4u);
    EXPECT_EQ(static_cast<uint8_t>(mf2_expr_kind::markup_standalone), 5u);
}

// ── mf2_expression defaults ─────────────────────────────────────────────

TEST(Mf2CompilerTest, ExpressionDefaults) {
    mf2_expression expr;
    EXPECT_EQ(expr.kind, mf2_expr_kind::literal);
    EXPECT_TRUE(expr.operand.empty());
    EXPECT_TRUE(expr.annotation.function_name.empty());
    EXPECT_TRUE(expr.annotation.options.empty());
}

TEST(Mf2CompilerTest, ExpressionLiteral) {
    mf2_expression expr;
    expr.kind = mf2_expr_kind::literal;
    expr.operand = "Hello, world!";
    EXPECT_EQ(expr.kind, mf2_expr_kind::literal);
    EXPECT_EQ(expr.operand, "Hello, world!");
}

TEST(Mf2CompilerTest, ExpressionVariable) {
    mf2_expression expr;
    expr.kind = mf2_expr_kind::variable;
    expr.operand = "name";
    EXPECT_EQ(expr.kind, mf2_expr_kind::variable);
    EXPECT_EQ(expr.operand, "name");
}

TEST(Mf2CompilerTest, ExpressionFunctionCall) {
    mf2_expression expr;
    expr.kind = mf2_expr_kind::function_call;
    expr.operand = "count";
    expr.annotation.function_name = "number";
    expr.annotation.options.push_back({"minimumFractionDigits", "2"});
    EXPECT_EQ(expr.kind, mf2_expr_kind::function_call);
    EXPECT_EQ(expr.annotation.function_name, "number");
    ASSERT_EQ(expr.annotation.options.size(), 1u);
    EXPECT_EQ(expr.annotation.options[0].key, "minimumFractionDigits");
    EXPECT_EQ(expr.annotation.options[0].value, "2");
}

// ── mf2_annotation ──────────────────────────────────────────────────────

TEST(Mf2CompilerTest, AnnotationEmpty) {
    mf2_annotation ann;
    EXPECT_TRUE(ann.function_name.empty());
    EXPECT_TRUE(ann.options.empty());
}

TEST(Mf2CompilerTest, AnnotationWithMultipleOptions) {
    mf2_annotation ann;
    ann.function_name = "datetime";
    ann.options.push_back({"dateStyle", "long"});
    ann.options.push_back({"timeStyle", "short"});
    EXPECT_EQ(ann.function_name, "datetime");
    ASSERT_EQ(ann.options.size(), 2u);
    EXPECT_EQ(ann.options[0].key, "dateStyle");
    EXPECT_EQ(ann.options[1].value, "short");
}

// ── mf2_variant ─────────────────────────────────────────────────────────

TEST(Mf2CompilerTest, VariantSingleKey) {
    mf2_variant v;
    v.keys.push_back("one");
    mf2_expression expr;
    expr.kind = mf2_expr_kind::literal;
    expr.operand = "You have one item.";
    v.pattern.push_back(expr);
    ASSERT_EQ(v.keys.size(), 1u);
    EXPECT_EQ(v.keys[0], "one");
    ASSERT_EQ(v.pattern.size(), 1u);
}

TEST(Mf2CompilerTest, VariantWildcard) {
    mf2_variant v;
    v.keys.push_back("*");
    mf2_expression expr;
    expr.kind = mf2_expr_kind::literal;
    expr.operand = "You have items.";
    v.pattern.push_back(expr);
    EXPECT_EQ(v.keys[0], "*");
}

TEST(Mf2CompilerTest, VariantMultipleKeys) {
    mf2_variant v;
    v.keys.push_back("masculine");
    v.keys.push_back("one");
    ASSERT_EQ(v.keys.size(), 2u);
}

// ── mf2_declaration ─────────────────────────────────────────────────────

TEST(Mf2CompilerTest, DeclarationInput) {
    mf2_declaration decl;
    decl.variable_name = "count";
    decl.is_input = true;
    decl.expression.kind = mf2_expr_kind::variable;
    decl.expression.operand = "count";
    EXPECT_TRUE(decl.is_input);
    EXPECT_EQ(decl.variable_name, "count");
}

TEST(Mf2CompilerTest, DeclarationLocal) {
    mf2_declaration decl;
    decl.variable_name = "formatted";
    decl.is_input = false;
    decl.expression.kind = mf2_expr_kind::function_call;
    decl.expression.operand = "count";
    decl.expression.annotation.function_name = "number";
    EXPECT_FALSE(decl.is_input);
    EXPECT_EQ(decl.expression.annotation.function_name, "number");
}

// ── mf2_message ─────────────────────────────────────────────────────────

TEST(Mf2CompilerTest, SimpleMessage) {
    mf2_message msg;
    mf2_expression lit;
    lit.kind = mf2_expr_kind::literal;
    lit.operand = "Hello!";
    msg.pattern.push_back(lit);
    EXPECT_FALSE(msg.is_complex());
    EXPECT_TRUE(msg.selectors.empty());
    EXPECT_TRUE(msg.variants.empty());
    ASSERT_EQ(msg.pattern.size(), 1u);
}

TEST(Mf2CompilerTest, ComplexMessage) {
    mf2_message msg;
    // Add selector
    mf2_expression sel;
    sel.kind = mf2_expr_kind::variable;
    sel.operand = "count";
    sel.annotation.function_name = "plural";
    msg.selectors.push_back(sel);
    // Add variants
    mf2_variant v1;
    v1.keys.push_back("one");
    msg.variants.push_back(v1);
    mf2_variant v2;
    v2.keys.push_back("*");
    msg.variants.push_back(v2);
    EXPECT_TRUE(msg.is_complex());
    ASSERT_EQ(msg.selectors.size(), 1u);
    ASSERT_EQ(msg.variants.size(), 2u);
}

// ── mf2_compiled_unit ───────────────────────────────────────────────────

TEST(Mf2CompilerTest, CompiledUnitDefaults) {
    mf2_compiled_unit unit;
    EXPECT_TRUE(unit.bytecode.empty());
    EXPECT_TRUE(unit.string_table.empty());
    EXPECT_TRUE(unit.string_offsets.empty());
    EXPECT_EQ(unit.arg_count, 0u);
}

TEST(Mf2CompilerTest, CompiledUnitManual) {
    mf2_compiled_unit unit;
    // Simulate compiling "Hello {$name}!"
    // String table: "Hello " + NUL + "!" + NUL
    unit.string_table = std::string("Hello \0!\0", 9);
    unit.string_offsets = {0, 7};

    // Bytecode: print_text(0), print_str_var(name_hash), print_text(1), end
    unit.bytecode.push_back({op::print_text, 0, 0, 0});
    unit.bytecode.push_back({op::print_str_var, 0, 0, 0x12345678});
    unit.bytecode.push_back({op::print_text, 0, 1, 0});
    unit.bytecode.push_back({op::end, 0, 0, 0});
    unit.arg_count = 1;

    ASSERT_EQ(unit.bytecode.size(), 4u);
    EXPECT_EQ(unit.bytecode[0].opcode, op::print_text);
    EXPECT_EQ(unit.bytecode[1].opcode, op::print_str_var);
    EXPECT_EQ(unit.bytecode[3].opcode, op::end);
    EXPECT_EQ(unit.arg_count, 1u);
    ASSERT_EQ(unit.string_offsets.size(), 2u);
}

// ── instruction format ──────────────────────────────────────────────────

TEST(Mf2CompilerTest, InstructionSize) {
    EXPECT_EQ(sizeof(instruction), 8u);
}

TEST(Mf2CompilerTest, InstructionLayout) {
    instruction instr{op::format_number, 0x42, 100, 999};
    EXPECT_EQ(instr.opcode, op::format_number);
    EXPECT_EQ(instr.flags, 0x42);
    EXPECT_EQ(instr.arg1, 100u);
    EXPECT_EQ(instr.arg2, 999u);
}

// ── Opcode coverage ─────────────────────────────────────────────────────

TEST(Mf2CompilerTest, AllOpcodesDefined) {
    // Verify opcodes are distinct and in expected range
    EXPECT_EQ(static_cast<uint8_t>(op::end), 0u);
    EXPECT_EQ(static_cast<uint8_t>(op::print_text), 1u);
    EXPECT_EQ(static_cast<uint8_t>(op::print_str_var), 2u);
    EXPECT_EQ(static_cast<uint8_t>(op::print_num_var), 3u);
    EXPECT_EQ(static_cast<uint8_t>(op::jump), 4u);
    EXPECT_EQ(static_cast<uint8_t>(op::match_select), 5u);
    EXPECT_EQ(static_cast<uint8_t>(op::match_plural), 6u);
    EXPECT_EQ(static_cast<uint8_t>(op::match_ordinal), 7u);
    EXPECT_EQ(static_cast<uint8_t>(op::format_number), 12u);
    EXPECT_EQ(static_cast<uint8_t>(op::format_date), 13u);
    EXPECT_EQ(static_cast<uint8_t>(op::call_function), 25u);
    EXPECT_EQ(static_cast<uint8_t>(op::match_gender), 26u);
}

// ── Markup expressions ──────────────────────────────────────────────────

TEST(Mf2CompilerTest, MarkupOpen) {
    mf2_expression expr;
    expr.kind = mf2_expr_kind::markup_open;
    expr.operand = "bold";
    EXPECT_EQ(expr.kind, mf2_expr_kind::markup_open);
    EXPECT_EQ(expr.operand, "bold");
}

TEST(Mf2CompilerTest, MarkupClose) {
    mf2_expression expr;
    expr.kind = mf2_expr_kind::markup_close;
    expr.operand = "bold";
    EXPECT_EQ(expr.kind, mf2_expr_kind::markup_close);
}

TEST(Mf2CompilerTest, MarkupStandalone) {
    mf2_expression expr;
    expr.kind = mf2_expr_kind::markup_standalone;
    expr.operand = "br";
    EXPECT_EQ(expr.kind, mf2_expr_kind::markup_standalone);
    EXPECT_EQ(expr.operand, "br");
}
