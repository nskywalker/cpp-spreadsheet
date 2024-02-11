#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid pos");
    }

    if (p_cells[pos.row].find(pos.col) != p_cells[pos.row].end()) {
        auto& cell = p_cells[pos.row].at(pos.col);
        cell.is_changed = cell.cur_cell.GetText() != text;
        if (cell.is_changed) {
            SetCellGuardly(pos, std::move(text));
            for (const auto cur_p : cell.depended_cells) {
                p_cells[cur_p.row].at(cur_p.col).depended_cells.erase(pos); //удаление ячейки из завимых - нам не нужно уведомлять её при смене своего значения
            }
        }
    }
    else {
        p_cells[pos.row].emplace(pos.col, *this);
        SetCellGuardly(pos, std::move(text));
    }

    auto& cell = p_cells[pos.row].at(pos.col);

    if (!cell.cur_cell.GetText().empty()) {
        int max_rows = std::max(cur_size.rbegin()->rows, pos.row + 1);
        int max_cols = std::max(cur_size.rbegin()->cols, pos.col + 1);
        cur_size.insert(Size{max_rows, max_cols});
    }

    auto referenced_cells = cell.cur_cell.GetReferencedCells();


    for (auto& cur_c : referenced_cells) {
        auto it = p_cells[cur_c.row].find(cur_c.col);
        if (it == p_cells[cur_c.row].end()) {
//            SetCell(cur_c, "");
            p_cells[cur_c.row].emplace(cur_c.col, *this);
        }
        p_cells[cur_c.row].at(cur_c.col).depended_cells.emplace(pos);
    }

}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid pos");
    }
    auto size = GetPrintableSize();
    return p_cells[pos.row].find(pos.col) != p_cells[pos.row].end() and pos < Position{size.rows, size.cols}
           ? &p_cells.at(pos.row).at(pos.col).cur_cell
           : nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    /*if (!pos.IsValid()) {
        throw InvalidPositionException("invalid pos");
    }
    SetCell(pos, "");
    return &p_cells[pos.row].at(pos.col).cur_cell;*/
    return const_cast<CellInterface*>(const_cast<const Sheet*>(this)->GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid pos");
    }
    auto it = p_cells[pos.row].find(pos.col);
    if (it == p_cells[pos.row].end()) {
        return;
    }
    SetCell(pos, "");
    auto cur_pos_size = Size{pos.row + 1, pos.col + 1};
    if (cur_size.find(cur_pos_size) != cur_size.end()) {
        cur_size.erase(cur_pos_size);
    }
}

Size Sheet::GetPrintableSize() const {
    return *cur_size.rbegin();
}

void Sheet::PrintValues(std::ostream &output) const {
    auto size = GetPrintableSize();
    for (int r = 0; r < size.rows; ++r) {
        for (int c = 0; c < size.cols; ++c) {
            auto it = p_cells[r].find(c);
            if (it != p_cells[r].end()) {
                auto v = it->second.is_changed ? it->second.cur_cell.GetValue() : it->second.cached_value;
                it->second.is_changed = false;
                auto val = std::holds_alternative<nullptr_t>(v) ? it->second.cur_cell.GetValue() : std::get<Cell::Value>(v);
                if (std::holds_alternative<double>(val)) {
                    output << std::get<double>(val);
                } else if (std::holds_alternative<FormulaError>(val)) {
                    output << std::get<FormulaError>(val);
                } else {
                    output << std::get<std::string>(val);
                }
            }
            if (c != size.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    auto size = GetPrintableSize();
    for (int r = 0; r < size.rows; ++r) {
        for (int c = 0; c < size.cols; ++c) {
            auto it = p_cells[r].find(c);
            if (it != p_cells[r].end()) {
                output << it->second.cur_cell.GetText();
            }
            if (c != size.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}


void Sheet::FindRefCells(std::vector<Position> cells, const Position item_cell) {
    if (cells.empty()) {
        return;
    }
    auto it = std::lower_bound(cells.begin(), cells.end(), item_cell);
    if (it != cells.end() and *it == item_cell) {
        throw CircularDependencyException("#REF");
    }
    for (const auto pos : cells) {
        const auto cell = GetCell(pos);
        if (cell) {
            FindRefCells(cell->GetReferencedCells(), item_cell);
        }
    }

}

void Sheet::SetCellGuardly(Position pos, std::string text) {
    auto& cell = p_cells[pos.row].at(pos.col);

    auto prev_text = cell.cur_cell.GetText();
    cell.cur_cell.Set(std::move(text));
    try {
        FindRefCells(cell.cur_cell.GetReferencedCells(), pos);
    }
    catch (const CircularDependencyException& err) {
        cell.cur_cell.Set(std::move(prev_text));
        cell.is_changed = false;
        throw CircularDependencyException(err.what());
    }
}


std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}