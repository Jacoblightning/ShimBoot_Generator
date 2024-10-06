//
// Created by jacoblightning3 on 10/5/24.
//

#include "ui.h"
#include "../inet/inet.h"
#include "../vars.h"

#include <map>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <filesystem>

#include <sstream>

#include <cpr/cpr.h>

using namespace ftxui;

Component Wrap(const std::string& name, const Component& component) {
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
    std::map<std::string, std::string> versions;
    std::vector<int> selections;

    selections.reserve(boards.size());
    {
        int iters = 1;
        std::string csvdata = getcsv();
        auto stream = std::istringstream(csvdata);
        std::string board_name;
        while(std::getline(stream, board_name, ',')) {
            if (board_name == "board/model") {
                std::string field_name;
                while (std::getline(stream, field_name, ',')) {
                    if (field_name == "cr_128") {
                        break;
                    }
                    iters++;
                }
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
            for (auto i = 0; i < iters; i++) {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), ',');
            }
            std::string vs;
            getline(stream, vs, ',');
            if (vs == "no update") {
                for (auto i = 0; i < 3; i++) {
                    stream.ignore(std::numeric_limits<std::streamsize>::max(), ',');
                }
                getline(stream, vs, ',');
            }
            /*
            if (versions.contains(name)) {
                std::cerr << "Colision version " << name << " already exists. Currently on " << board << std::endl;
                exit(1);
            }
            */
            versions[name] = vs;
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

    std::pair<std::string, std::string> selected_book;
    auto sel = 0;

    auto names_with_event = CatchEvent(tab_container, [&] (const Event& e) {
        if (e != Event::Return) {
            return false;
        }
        selected_book.first = boards[tab_selected];
        selected_book.second = board_names[boards[tab_selected]][selections[tab_selected]];
        sel++;
        return true;
    });

    auto container = Container::Horizontal({
    tab_menu,
    names_with_event,
    });

    const auto renderer1 = Renderer(container, [tab_menu, tab_container] {
        return vbox({
            hbox({
                text("Info") | borderHeavy | bgcolor(Color::Blue) | flex,
                text("->"),
                text("Preparing") | borderHeavy | flex,
                text("->"),
                text("Downloading") | borderHeavy | flex,
                text("->"),
                text("Building") | borderHeavy | flex
            }),
            separator(),
            text("You can find your board by going to chrome://version and looking at the last word next to version."),
            text("I think you can find the name somewhere in chrome://version?"),
            text("Select your board and name:") | color(Color::Orange3),
            separator(),
            hbox({
                vbox({
                    text("Board:"),
                    tab_menu->Render()
                }),
                separator(),
                vbox({
                    text("Name:"),
                    tab_container->Render()
                })
            }) | border
        });
    });

    std::pair<std::string, std::string> download_urls;

    auto screen = ScreenInteractive::TerminalOutput();

    auto yes_btn = Button("Yes. I'm Sure", [&] {
        ++sel;
    });
    auto no_btn = Button("No, take me back", [&]{ --sel; });

    auto buttons = Container::Horizontal({
        no_btn,
        yes_btn,
    });

    const auto renderer2 = Renderer(buttons, [&] {
        return vbox({
            vbox({
                text("Are you sure you want the board "+selected_book.first+" and the name "+selected_book.second+"?") | color(Color::Red),
                text("This download takes a long time so it's good to be sure.") | color(Color::Green),
            }) | border,
            hbox({
                filler(),
                buttons->Render(),
                filler()
            })
        });
    });

    std::string status;
    float gaug = 0;
    std::thread prepper;
    bool do_prep = true;

    const auto renderer3 = Renderer([&] {
        if (do_prep) {
            do_prep = false;
            status = "Preparing... Please Wait (This might take around a minute or two.)";
            prepper = std::thread([&]{
            {
                const auto url1 = repo_url+"shimboot_"+selected_book.first+".zip";
                const auto url2 = "https://dl.darkn.bio/api/raw?path=/Shimboot/shimboot_"+selected_book.first+".zip";
                if (Head(cpr::Url{url1}).status_code != 200) {
                    if (Head(cpr::Url{url2}).status_code != 200) {
                        screen.ExitLoopClosure()();
                        system("clear");
                        std::cerr << "Could not find file shimboot_" << selected_book.first << ".zip in either hosting repo. Aborting." << std::endl;
                        exit(1);
                    }
                    download_urls.first = url2;
                }
                download_urls.first = url1;
            }
            {
                const auto url = "https://dl.google.com/dl/edgedl/chromeos/recovery/chromeos_"+versions[selected_book.second]+"_"+selected_book.first+"_recovery_stable-channel_mp-v";
                const std::string ending = ".bin.zip";
                int correctver = -1;
                for (auto i = 0; i < 100; i++) {
                    if (const std::string url_fmt = std::format("{}{}{}", url, i, ending); Head(cpr::Url{url_fmt}).status_code == 200) {
                        correctver = i;
                        break;
                    }
                    gaug = i/100;
                    screen.PostEvent(Event::Custom);
                }
                if (correctver == -1) {
                    screen.ExitLoopClosure()();
                    system("clear");
                    std::cerr << "Could not determine version" << std::endl;
                    exit(1);

                }
                download_urls.second = std::format("{}{}{}", url, correctver, ending);
            }
            sel++;
            screen.PostEvent(Event::Custom);
        });
            // This will only happen for one frame so it can be empty.
            return vbox({});
        }
        return vbox({
            hbox({
                text("Info") | borderHeavy | flex,
                text("->"),
                text("Preparing") | borderHeavy | bgcolor(Color::Blue) | flex,
                text("->"),
                text("Downloading") | borderHeavy | flex,
                text("->"),
                text("Building") | borderHeavy | flex
            }),
            text(status),
            hbox({
                gauge(gaug) | border,
                text(std::format("{}%", gaug*100))
            })
        });
    });

    bool downloading = false;
    float dl1 = 0, dl2 = 0, dl3 = 0;

    std::thread d1,d2,d3;

    std::array<std::thread, 3> threads;

    auto start_btn = Button("Start Downloading Required Files.", [&] {
        if (downloading)
            return;
        downloading = true;
        auto get_uri = [&screen](const std::string &url, float &writeto, std::string filename) {
            const std::string fname = std::format("{}.download", filename);

            std::ofstream Output(fname.c_str(), std::ofstream::out);
            cpr::Response r = Get(cpr::Url{url},
                cpr::ProgressCallback([&](const cpr::cpr_off_t downloadTotal, const cpr::cpr_off_t downloadNow, [[maybe_unused]] cpr::cpr_off_t uploadTotal, [[maybe_unused]] cpr::cpr_off_t uploadNow, [[maybe_unused]] intptr_t userdata) -> bool {
                    if (downloadTotal != 0) {
                        writeto = downloadNow/downloadTotal;
                    }
                    screen.PostEvent(Event::Custom);
                    return true;
                }),
                cpr::WriteCallback([&](const std::string_view data, [[maybe_unused]] intptr_t userdata) -> bool {
                    Output << data;
                    return true;
                }));
            Output.close();
            try {
                std::filesystem::rename(fname, filename);
            } catch (std::filesystem::filesystem_error& e) {
                screen.ExitLoopClosure()();
                system("clear");
                std::cout << e.what() << '\n';
                exit(1);
            }
        };
        threads[0] = std::thread(get_uri, selected_book.first, std::ref(dl1), "shim.zip");
        threads[1] = std::thread(get_uri, selected_book.second, std::ref(dl2), "recov.zip");
        threads[2] = std::thread(get_uri, "https://github.com/ading2210/shimboot/archive/refs/heads/main.zip", std::ref(dl3), "git.zip");

    });

    const auto renderer4 = Renderer(start_btn, [&] {
        if (!downloading){
            return vbox({
                hbox({
                    text("Info") | borderHeavy | flex,
                    text("\n->"),
                    text("Preparing") | borderHeavy | flex,
                    text("\n->"),
                    text("Downloading") | borderHeavy | bgcolor(Color::Blue) | flex,
                    text("\n->"),
                    text("Building") | borderHeavy | flex
                }),
                text("Press the below button to download files."),
                start_btn->Render()
            });
        }
        return vbox({
            hbox({
                text("Info") | borderHeavy | flex,
                text("\n->"),
                text("Preparing") | borderHeavy | flex,
                text("\n->"),
                text("Downloading") | borderHeavy | bgcolor(Color::Blue) | flex,
                text("\n->"),
                text("Building") | borderHeavy | flex
            }),
            text("Download progress:"),
            vbox({
                vbox({
                    text(std::format("Shim: {}%", dl1*100)),
                    gauge(dl1) | border | flex_grow
                }) | border,
                vbox({
                    text(std::format("Image: {}%", dl2*100)),
                    gauge(dl2) | border | flex_grow
                }) | border,
                vbox({
                    text(std::format("Repo: {}%", dl2*100)),
                    gauge(dl3) | border | flex_grow
                }) | border
            }) | border,
        });
    });

    const auto maintab = Container::Tab(
        {
            renderer1,
            renderer2,
            renderer3,
            renderer4
        },
        &sel
        );

    screen.Loop(maintab);
}