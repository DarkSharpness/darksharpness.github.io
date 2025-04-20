#include <bits/stdc++.h>


signed main(int argc, char **argv) {
    freopen("extract.txt", "w", stdout);
    for (int i = 1; i < argc; i++) {
        std::ifstream fin(argv[i]);
        std::string str;
        while (std::getline(fin, str)) {
            std::string_view s(str);
            if (s.starts_with("cover")) {
                std::cout << std::format("![]({})", s.substr(6)) << std::endl;
                break;
            }
        }
    }
    return 0;
}