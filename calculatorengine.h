#pragma once

#include <exception>
#include <string>

// Pure C++ model: no dependency on Qt, widgets, or the event loop.
class CalculatorEngine {
public:
    using Decimal = long double;
    enum class Unary { Percent, Sqrt, Square, Reciprocal };
    enum class Memory { Clear, Recall, Add, Subtract, Store };
    void digit(char digit);
    void decimalPoint();
    void toggleSign();
    void backspace();
    void clearEntry();
    void clear();
    void binary(char operation);
    void equals();
    void unary(Unary operation);
    void memory(Memory operation);
    std::string display() const;
    const std::string& expression() const { return expression_; }
    bool hasMemory() const { return hasMemory_; }
    bool hasError() const { return !error_.empty(); }

private:
    Decimal value_ = 0, left_ = 0, repeatRight_ = 0, memory_ = 0;
    std::string entry_ = "0", expression_, error_;
    char pending_ = 0, repeat_ = 0;
    bool editing_ = false, operandReady_ = false, hasMemory_ = false;
    static std::string format(const Decimal& value);
    static Decimal calculate(const Decimal& a, char op, const Decimal& b);
    static Decimal checked(Decimal value);
    void beginEntry();
    void syncEntry();
    void fail(const std::exception& error);
};

