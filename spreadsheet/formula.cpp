#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
// Реализуйте следующие методы:
        explicit Formula(std::string expression) : ast_(GetFormulaAST(std::move(expression))) {

}
        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return ast_.Execute(&sheet);
            }
            catch (const FormulaException& err) {
                if (std::string{err.what()} == "#DIV0") {
                    return FormulaError(FormulaError::Category::Div0);
                }
                return FormulaError(FormulaError::Category::Value);
            }
}
        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
}

        std::vector<Position> GetReferencedCells() const override {
            auto cells = ast_.GetCells();
            auto res = std::vector<Position>{cells.begin(), cells.end()};
            res.erase(std::unique(res.begin(), res.end()), res.end());
            return res;
}
    protected:
        FormulaAST GetFormulaAST(std::string expression) {
            std::istringstream in(expression);
            try {
                return ParseFormulaAST(in);
            }
            catch (const std::exception& err) {
                throw FormulaException("#ARITHM!");
            }
}
    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}