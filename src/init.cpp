//
// Created by jacoblightning3 on 10/5/24.
//

#include "init.h"
#include "vars.h"

std::string repo_url;

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    }
    return false;
}

void internet_check() {
    std::cout << "Checking for internet." << std::endl;
    cpr::Response r = Get(cpr::Url{"https://google.com"});
    if (r.status_code != 200) {
        std::cerr << "No internet detected. Aborting." << std::endl;
        exit(1);
    }
    std::cout << "Internet works. Continuing." << std::endl;
}

std::vector<std::string> init() {
    internet_check();
    const std::string alt_api = "https://dl.darkn.bio/api";
    const std::string api_url = "https://api.github.com/repos/ading2210/shimboot/releases/latest";
    std::cout << "Checking if main repo is up." << std::endl;
    cpr::Response r = Get(cpr::Url{api_url});
    if (r.status_code != 200) {
        std::cerr << "Hosing service is down. Aborting." << std::endl;
        exit(1);
    }
    std::vector<std::string> sboots;
    std::cout << "Scanning contents of repo." << std::endl;
    const json repo = json::parse(r.text);
    const std::string download_url = repo["assets"][0]["browser_download_url"].get<std::string>();
    repo_url = download_url.substr(0,download_url.find_last_of("/")+1);
    std::cout << "Using repo: " << repo_url << std::endl;
    for (const auto& asset : repo["assets"]) {
        auto asset_name = asset["name"].get<std::string>();
        auto asseted = asset_name.substr(asset_name.find_last_of('_')+1);
        asseted = asseted.substr(0, asseted.find_last_of('.'));
        sboots.emplace_back(asseted);
    }
    r = Get(cpr::Url{alt_api}, cpr::Parameters{{"path", "%2FShimboot%2F"}});
    if (r.status_code != 200) {
        return sboots;
    }
    for (const json alto = json::parse(r.text); const auto& asset : alto["folder"]["value"]) {
        auto asset_name = asset["name"].get<std::string>();
        if (!hasEnding(asset_name, ".zip")) {
            continue;
        }
        auto asseted = asset_name.substr(asset_name.find_last_of('_')+1);
        asseted = asseted.substr(0, asseted.find_last_of('.'));
        if (std::ranges::find(sboots, asseted) == sboots.end()) {
            sboots.emplace_back(asseted);
        }
    }
    std::ranges::sort(sboots);
    return sboots;
}