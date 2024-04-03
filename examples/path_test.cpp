
#include <iostream>
#include "../include/fsmod.hpp"

int main() {

    // Path simplification
    std::vector<std::string> paths_to_simplify = {
            "////",
            "////foo",
            "/////foo",
            "../../../././././/////foo",
            "foo/bar/../",
            "/////foo/////././././bar/../bar/..///../../",
            "./foo/.../........"
    };

    for (const auto &path: paths_to_simplify) {
        std::cout << "\"" << path << "\" --> \"" << simgrid::module::fs::Path::simplify_path_string(path) << "\"\n";
    }

}
