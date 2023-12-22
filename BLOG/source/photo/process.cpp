#include <bits/stdc++.h>


signed main() {
    freopen("test.md", "r", stdin);
    freopen("index.md", "w", stdout);
    std::string str;
    while (std::getline(std::cin, str)) {
        std::string_view s(str);
        if (!s.starts_with("![pixiv")) { std::cout << str << std::endl; continue; }
        std::size_t pos = s.find("(");
        std::cout << std::format("![]{}\n", s.substr(pos));
        s = s.substr(8, pos - 2 - 8);
        std::string_view __pre = s.substr(0, s.find(" "));
        std::string_view __suf = s.substr(s.find("y") + 3);
        std::cout << std::format(
            "<center><font color=grey>"
            "<a href=\"https://www.pixiv.net/artworks/{}\">"
            " Artwork {} by \"{}\" </a></font></center><br/>\n",
            __pre, __pre, __suf
        );
    }

    return 0;
}