#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <memory>
#include <optional>
#include <deque>
#include <map>
#include <set>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Можете дополнить ваш класс нужными полями и методами

protected:
    void FindRefCells(std::vector<Position> cells, Position pos);
    void SetCellGuardly(Position pos, std::string text);
private:
    // Можете дополнить ваш класс нужными полями и методами

    struct CellHasher {
        size_t operator()(Position pos) const {
            return pos.row * Position::MAX_ROWS + pos.col;
        }
    };

    struct CellStruct {
        explicit CellStruct(Sheet& sheet) : cur_cell(sheet){}
        Cell cur_cell;
        std::variant<std::nullptr_t, Cell::Value> cached_value;
        bool is_changed = false;
        std::unordered_set<Position, CellHasher> depended_cells; //от меня зависят
    };
    mutable std::map<int, std::map<int, CellStruct>> p_cells;
    mutable std::set<Size> cur_size = std::set<Size>{Size{0, 0}};
};