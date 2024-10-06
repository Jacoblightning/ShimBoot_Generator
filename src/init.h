#pragma once

#include  <iostream>
#include <string>

#include <cpr/cpr.h>
#ifndef NO_BACKUP
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif // NO_BACKUP

std::vector<std::string> init();

bool check_for_board(const std::string &board);