#include "inet.h"

#include <iostream>
#include <cpr/cpr.h>

using namespace cpr;

std::string getcsv() {
    const std::string url = "https://chromiumdash.appspot.com/cros/download_serving_builds_csv?deviceCategory=ChromeOS";
    Response r = Get(Url{url});
    if (r.status_code != 200) {
        system("clear");
        std::cerr << "Could not get names. Aborting." << std::endl;
        exit(1);
    }
    return r.text;
}