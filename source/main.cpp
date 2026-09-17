#include "ARC_cache.hpp"

#include <iostream>
#include <string>

int main() {
    ARC::Cache<std::string> cache;
    for (const std::string url : {"/a", "/b", "/c", "/d"}) {
        if (cache.insert(url, "Content of " + url) != ARC::Status::success) {
            return 1;
        }
    }

    const std::string* data = nullptr;
    if (cache.get("/a", data) != ARC::Status::success) {
        return 1;
    }
    std::cout << *data << '\n';

    if (cache.insert("/e", "Content of /e") != ARC::Status::success) {
        return 1;
    }
}
