#include "cell.h"

#include <iostream>
#include <string>
#include <optional>

void Cell::Set(std::string text) {
    if (!text.empty()) {
        if (text[0] == FORMULA_SIGN) {
            formula = ParseFormula(text.substr(1));
        } else if ((std::all_of(text.begin(), text.end(), [&](char c) { return c >= '0' and c <= '9'; }))) {
            formula = ParseFormula(text);
        } else {
            formula.reset(nullptr);
        }
    } else {
        formula.reset(nullptr);
    }
    expression = std::move(text);
}

void Cell::Clear() {
    expression.clear();
    formula.reset(nullptr);
}

Cell::Value Cell::GetValue() const {
    if (formula) {
        auto res = formula->Evaluate(reinterpret_cast<const SheetInterface &>(sheet_));
        if (std::holds_alternative<double>(res)) {
            return std::get<double>(res);
        }
        return std::get<FormulaError>(res);
    }
    if (!expression.empty() and expression[0] == ESCAPE_SIGN) {
        return expression.size() == 1 ? "" : expression.substr(1);
    }
    return expression;
}
std::string Cell::GetText() const {
    if (formula and !std::all_of(expression.begin(), expression.end(), [](char c){return c >= '0' and c <= '9';})) {
        return FORMULA_SIGN + formula->GetExpression();
    }
    return expression;
}

Cell::Cell(Sheet &sheet) : sheet_(sheet) {

}

std::vector<Position> Cell::GetReferencedCells() const {
    if (formula) {
        return formula->GetReferencedCells();
    }
    return {};
}

