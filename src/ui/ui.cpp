//
// Created by jacoblightning3 on 10/5/24.
//

#include "ui.h"
#include "../inet/inet.h"

#include <map>
#include <iostream>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

using namespace ftxui;

Component Wrap(std::string name, Component component) {
    return Renderer(component, [name, component] {
      return hbox({
                 text(name),
                 separator(),
                 component->Render() | xflex,
             }) |
             xflex;
    });
}

void runui(std::vector<std::string> &boards) {
    std::map<std::string, std::vector<std::string>> board_names;
    std::vector<int> selections;

    selections.reserve(boards.size());
    {
        std::string csvdata = getcsv();
        auto stream = std::istringstream(csvdata);
        std::string board_name;
        while(std::getline(stream, board_name, ',')) {
            if (board_name == "board/model") {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            const size_t idx = board_name.find('.');
            if (idx == std::string::npos) {
                // Wrong type
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            std::string board = board_name.substr(0, idx);
            std::string name = board_name.substr(idx + 1);
            if (std::ranges::find(boards, board) == boards.end()) {
                // Non supported board
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            if (!board_names.contains(board)) {
                board_names[board] = std::vector<std::string>();
            }
            board_names[board].push_back(name);
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    {
        std::vector boards_copy(boards);
        int idx = 0;
        // We need to progressively offset the removed index.
        int removed = 0;
        for (auto &board : boards_copy) {
            if (!board_names.contains(board)) {
                // We have the shim but not the image. We can't use this.
                boards.erase(boards.begin()+idx-removed++);
            }
            ++idx;
        }
    }

    size_t bsize = board_names.size();
    selections.reserve(bsize);
    for (auto i = 0; i < bsize; i++) {
        selections.push_back(0);
    }
    system("clear");

    std::string selected_text;

    Components comps;
    {
        int idx = 0;
        for (const auto& board: std::views::elements<1>(board_names)) {
            comps.push_back(Menu(&board, &selections[idx++]));
        }
    }

    auto tab_selected = 0;
    const auto tab_menu = Menu(&boards, &tab_selected);
    auto tab_container = Container::Tab(comps, &tab_selected);

    auto container = Container::Horizontal({
    tab_menu,
    tab_container,
    });

    const auto renderer = Renderer(container, [tab_menu, tab_container] {
        return vbox({
            hbox({
                text("Info") | borderHeavy | bgcolor(Color::Blue) | flex,
                text("->"),
                text("Building") | borderHeavy | flex
            }),
            separator(),
            text("You can find your board by going to chrome://version and looking at the last word next to version."),
            text("Select your board:") | color(Color::Orange3),
            separator(),
            hbox({
               tab_menu->Render(),
               separator(),
               tab_container->Render(),
            }) | border
        });
    });

    auto screen = ScreenInteractive::TerminalOutput();

    screen.Loop(renderer);

    if (std::string content; !content.empty()) {
        selected_text = content;
    }else {
        selected_text = boards[tab_selected];
    }
}