#include "calculatorengine.h"
#include <iostream>
#include <stdexcept>

using Engine = CalculatorEngine;
using U = Engine::Unary;
using M = Engine::Memory;
static int checks = 0;
void expect(bool condition, const std::string& message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void result(const Engine& engine, const std::string& expected) {
    expect(engine.display() == expected, "Expected " + expected + ", got " + engine.display());
}
void enter(Engine& engine, const std::string& text) {
    for (char c : text) {
        if (c == '.') engine.decimalPoint();
        else engine.digit(c);
    }
}
int main() {
    try {
        Engine e;
        enter(e, "0.1"); e.binary('+'); enter(e, "0.2"); e.equals(); result(e, "0.3");
        expect(e.expression() == "0.1 + 0.2 =", "Expression mismatch");
        e.clear(); enter(e, "2"); e.binary('+'); enter(e, "3"); e.binary('*'); enter(e, "4"); e.equals(); result(e, "20");
        e.clear(); enter(e, "2"); e.binary('+'); e.binary('*'); enter(e, "3"); e.equals(); result(e, "6");
        e.equals(); result(e, "18"); // repeated equals reuses the right operand
        enter(e, "7"); result(e, "7"); e.equals(); result(e, "7");
        e.clear(); enter(e, "5"); e.binary('+'); e.equals(); result(e, "10");
        e.clear(); enter(e, "8"); e.binary('/'); enter(e, "0"); e.equals(); result(e, "Cannot divide by zero");
        expect(e.hasError(), "Missing error state");
        enter(e, "9"); result(e, "9"); expect(!e.hasError(), "Error recovery failed");
        e.toggleSign(); e.unary(U::Sqrt); result(e, "Invalid input");
        e.clearEntry(); result(e, "0"); e.unary(U::Reciprocal); result(e, "Cannot divide by zero");
        e.clear(); enter(e, "9"); e.unary(U::Sqrt); result(e, "3");
        e.unary(U::Square); result(e, "9");
        e.clear(); enter(e, "4"); e.unary(U::Reciprocal); result(e, "0.25");
        e.clear(); enter(e, "200"); e.binary('+'); enter(e, "10"); e.unary(U::Percent); result(e, "20"); e.equals(); result(e, "220");
        e.clear(); enter(e, "200"); e.binary('-'); enter(e, "10"); e.unary(U::Percent); e.equals(); result(e, "180");
        e.clear(); enter(e, "200"); e.binary('*'); enter(e, "10"); e.unary(U::Percent); e.equals(); result(e, "20");
        e.clear(); enter(e, "200"); e.binary('/'); enter(e, "10"); e.unary(U::Percent); e.equals(); result(e, "2000");
        e.clear(); enter(e, "10"); e.unary(U::Percent); result(e, "0.1");
        e.clear(); enter(e, "12"); e.binary('+'); enter(e, "99"); e.clearEntry(); enter(e, "3"); e.equals(); result(e, "15");
        e.clear(); enter(e, "12.30"); e.backspace(); result(e, "12.3"); e.backspace(); result(e, "12.");
        e.toggleSign(); result(e, "-12."); e.backspace(); e.backspace(); e.backspace(); result(e, "0");
        e.clear(); e.toggleSign(); enter(e, "5"); result(e, "5"); // sign on a result is replaced by fresh input
        e.clear(); e.digit('0'); e.toggleSign(); e.decimalPoint(); e.digit('5'); result(e, "-0.5");
        e.clear(); enter(e, "1.2.3"); result(e, "1.23");
        e.clear(); enter(e, "12345678901234567"); result(e, "123456789012345");
        e.memory(M::Store); expect(e.hasMemory(), "Memory not set"); e.clear(); e.memory(M::Recall); result(e, "123456789012345");
        e.memory(M::Clear); expect(!e.hasMemory(), "Memory not cleared");
        e.clear(); enter(e, "10"); e.memory(M::Store); enter(e, "5"); e.memory(M::Add);
        enter(e, "2"); e.memory(M::Subtract); e.clear(); e.memory(M::Recall); result(e, "13");
        e.binary('+'); e.memory(M::Recall); e.equals(); result(e, "26");
        e.clear(); enter(e, "8"); e.binary('/'); e.digit('0'); e.equals(); e.memory(M::Recall); result(e, "13");
        expect(!e.hasError(), "Memory recall should recover from an error");
        e.clear(); enter(e, "1"); e.binary('/'); enter(e, "3"); e.equals(); result(e, "0.333333333333333");
        e.binary('*'); enter(e, "3"); e.equals(); result(e, "1");
        e.clear(); enter(e, "10");
        for (int i = 0; i < 10; ++i) e.unary(U::Square);
        expect(e.hasError(), "Range limit not enforced");
        result(e, "Result out of range");
        std::cout << "Passed " << checks << " checks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}

