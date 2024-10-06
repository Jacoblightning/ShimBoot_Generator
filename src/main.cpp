#include  <iostream>
#include <string>

#include <cpr/cpr.h>


#include "ui/ui.h"
#include "init.h"

int main() {
    auto boards = init();
    runui(boards);
}