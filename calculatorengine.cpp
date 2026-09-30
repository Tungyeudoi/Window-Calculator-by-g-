#include "calculatorengine.h"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <algorithm>

std::string CalculatorEngine::format(const Decimal& value) {
    if (value == 0) return "0";
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(15) << std::defaultfloat << value;
    return out.str();
}

CalculatorEngine::Decimal CalculatorEngine::checked(Decimal value) {
    if (!std::isfinite(value)) throw std::overflow_error("Overflow");
    if (value != 0 && (std::abs(value) > 1e100L || std::abs(value) < 1e-100L))
        throw std::overflow_error("Result out of range");
    return value;
}

CalculatorEngine::Decimal CalculatorEngine::calculate(const Decimal& a, char op, const Decimal& b) {
    switch (op) {
    case '+': return checked(Decimal(a + b));
    case '-': return checked(Decimal(a - b));
    case '*': return checked(Decimal(a * b));
    case '/':
        if (b == 0) throw std::domain_error("Cannot divide by zero");
        return checked(Decimal(a / b));
    default: throw std::invalid_argument("Unknown operation");
    }
}

std::string CalculatorEngine::display() const {
    if (hasError()) return error_;
    return editing_ ? entry_ : format(value_);
}

void CalculatorEngine::clear() {
    value_ = left_ = repeatRight_ = 0;
    entry_ = "0";
    expression_.clear(); error_.clear();
    pending_ = repeat_ = 0;
    editing_ = operandReady_ = false;
    // C intentionally preserves the memory register.
}

void CalculatorEngine::fail(const std::exception& error) {
    clear();
    error_ = error.what();
}

void CalculatorEngine::beginEntry() {
    if (hasError()) clear();
    if (!editing_) {
        entry_ = "0";
        editing_ = true;
        if (!pending_) expression_.clear();
    }
    operandReady_ = true;
    repeat_ = 0;
}

void CalculatorEngine::syncEntry() {
    // Parse with the classic locale; arithmetic uses finite binary precision.
    std::istringstream input(entry_); input.imbue(std::locale::classic()); input >> value_;
}

void CalculatorEngine::digit(char digit) {
    if (digit < '0' || digit > '9') return;
    beginEntry();
    if (entry_ == "0" || entry_ == "-0") {
        if (digit != '0') entry_.replace(entry_.size() - 1, 1, 1, digit);
    } else {
        auto digits = std::count_if(entry_.begin(), entry_.end(), [](char c) { return c >= '0' && c <= '9'; });
        if (digits >= 15) return;
        entry_ += digit;
    }
    syncEntry();
}

void CalculatorEngine::decimalPoint() {
    beginEntry();
    if (entry_.find('.') == std::string::npos) entry_ += '.';
    syncEntry();
}

void CalculatorEngine::toggleSign() {
    if (hasError()) return;
    if (editing_) {
        if (entry_[0] == '-') entry_.erase(0, 1);
        else entry_.insert(0, "-");
        syncEntry();
    } else value_ = -value_;
    operandReady_ = true;
    repeat_ = 0;
    if (!pending_) expression_.clear();
}

void CalculatorEngine::backspace() {
    if (hasError()) { clear(); return; }
    if (!editing_) return;
    entry_.pop_back();
    if (entry_.empty() || entry_ == "-") entry_ = "0";
    syncEntry();
}

void CalculatorEngine::clearEntry() {
    if (hasError()) { clear(); return; }
    value_ = 0; entry_ = "0";
    editing_ = true; operandReady_ = true; repeat_ = 0;
    if (!pending_) expression_.clear();
}

void CalculatorEngine::binary(char op) {
    if (hasError() || std::string("+-*/").find(op) == std::string::npos) return;
    try {
        if (pending_ && operandReady_) value_ = calculate(left_, pending_, value_);
        left_ = value_;
        pending_ = op; repeat_ = 0;
        editing_ = operandReady_ = false;
        expression_ = format(left_) + " " + op;
    } catch (const std::exception& error) { fail(error); }
}

void CalculatorEngine::equals() {
    if (hasError()) return;
    try {
        if (pending_) {
            const Decimal right = operandReady_ ? value_ : left_;
            const auto expression = format(left_) + " " + pending_ + " " + format(right) + " =";
            value_ = calculate(left_, pending_, right);
            expression_ = expression;
            repeat_ = pending_; repeatRight_ = right; pending_ = 0;
        } else if (repeat_) {
            const auto expression = format(value_) + " " + repeat_ + " " + format(repeatRight_) + " =";
            value_ = calculate(value_, repeat_, repeatRight_);
            expression_ = expression;
        } else expression_ = format(value_) + " =";
        editing_ = false; operandReady_ = true;
    } catch (const std::exception& error) { fail(error); }
}

void CalculatorEngine::unary(Unary op) {
    if (hasError()) return;
    try {
        const auto before = format(value_);
        std::string label;
        switch (op) {
        case Unary::Percent:
            // For + and -, percentage is relative to the left operand.
            value_ = checked(Decimal((pending_ == '+' || pending_ == '-')
                ? Decimal(left_ * value_ / 100) : Decimal(value_ / 100)));
            label = before + "%"; break;
        case Unary::Sqrt:
            if (value_ < 0) throw std::domain_error("Invalid input");
            value_ = checked(Decimal(std::sqrt(value_))); label = "sqrt(" + before + ")"; break;
        case Unary::Square:
            value_ = checked(Decimal(value_ * value_)); label = "sqr(" + before + ")"; break;
        case Unary::Reciprocal:
            value_ = calculate(Decimal(1), '/', value_); label = "1/(" + before + ")"; break;
        }
        expression_ = pending_ ? format(left_) + " " + pending_ + " " + label : label;
        editing_ = false; operandReady_ = true; repeat_ = 0;
    } catch (const std::exception& error) { fail(error); }
}

void CalculatorEngine::memory(Memory op) {
    if (op == Memory::Clear) { memory_ = 0; hasMemory_ = false; return; }
    if (op == Memory::Recall) {
        if (!hasMemory_) return;
        if (hasError()) clear();
        value_ = memory_; editing_ = false; operandReady_ = true; repeat_ = 0;
        if (!pending_) expression_.clear();
        return;
    }
    if (hasError()) return;
    try {
        switch (op) {
        case Memory::Store: memory_ = value_; break;
        case Memory::Add: memory_ = checked(Decimal(memory_ + value_)); break;
        case Memory::Subtract: memory_ = checked(Decimal(memory_ - value_)); break;
        default: break;
        }
        hasMemory_ = true;
        editing_ = false;
    } catch (const std::exception& error) { fail(error); }
}

