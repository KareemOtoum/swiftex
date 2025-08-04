#include "server.h"
#include "mapbased_matcher.h"

int main() {
    start_server<MapBasedMatcher>(server::k_default_port);
    return 0;
}
