#include "helper.h"
#include <fstream>
#include <vector>
#include <filesystem>

std::vector <std::string_view> split(std::string_view line) {
    std::vector <std::string_view> result;
    auto pos = line.find("||");
    while (pos != std::string_view::npos) {
        result.push_back(line.substr(0, pos));
        line.remove_prefix(pos + 2);
        pos = line.find("||");
    }
    result.push_back(line);
    for (auto &s : result) {
        while (s.starts_with(' ')) s.remove_prefix(1);
        while (s.ends_with(' ')) s.remove_suffix(1);
    }
    return result;
}


signed main() {
    std::ifstream fin("test.md");
    std::filesystem::remove("index.md");
    std::filesystem::copy_file("test.md", "index.md");

    std::ifstream pixiv(pixiv_path);
    std::ifstream twitter(twitter_path);
    std::ofstream fout("index.md", std::ios::app);

    fout << "## Pixiv part\n";
    std::string line;

    while (std::getline(pixiv, line)) {
        auto parts = split(line);
        assert(parts.size() == 3, "pixiv.tmp format error");
        fout << std::format(
            "![]({})\n\n"
            "<center><font color=grey><a href=\"https://www.pixiv.net/artworks/{}\"> "
            "Artwork {} by {} "
            "</a></font></center><br/>\n\n", parts[0], parts[1], parts[1], parts[2]);
    }

    fout << "## Twitter part\n\n";
    while (std::getline(twitter, line)) {
        auto parts = split(line);
        assert(parts.size() == 2, "twitter.tmp format error");
        fout << std::format("![{}]({})\n\n", parts[1], parts[0]);
    }

    return 0;
}
