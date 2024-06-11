#include "helper.h"
#include <fstream>

signed main() {
    std::ifstream fin("index.md");
    std::ofstream pixiv(pixiv_path);
    std::ofstream twitter(twitter_path);

    std::string line;
    std::size_t stage = 0;
    std::string link;
    while (std::getline(fin, line)) {
        std::string_view view {line};
        if (view.starts_with("## Pixiv part")) {
            assert(stage != 2);
            stage = 1;
        } else if (view.starts_with("![](")) {
            assert(stage == 1);
            stage = 2;
            view.remove_prefix(4);
            view = view.substr(0, view.find(")"));
            link = std::string(view);
        } else if (view.starts_with("<center><font color=grey>")) {
            assert(stage == 2);
            stage = 1;

            view = view.substr(view.find('\"') + 1);
            auto pos = view.find('\"');
            auto href = view.substr(0, pos);
            view = view.substr(pos + 1);
            view = view.substr(view.find("Artwork ") + 8);
            pos = view.find(" by ");

            auto pid = view.substr(0, pos);

            view = view.substr(pos + 4);
            view = view.substr(view.find('\"') + 1);
            pos = view.find('\"');
            auto author = view.substr(0, pos);
            author = author.substr(0, author.find_first_of('<'));
            while (author.ends_with(' ')) author.remove_suffix(1);

            pos = href.find_last_of('/');
            assert(href.substr(pos + 1) == pid, "{} {}", pid, href.substr(pos + 1));

            pixiv << std::format("{:<60} || {:<12} || {}\n", link, pid, author);
        } else if (view.starts_with("## Twitter part")) {
            assert(stage != 2);
            stage = 3;
        } else if (view.starts_with("![")) {
            assert(stage == 3);
            view.remove_prefix(2);
            auto pos = view.find("](");
            assert(pos != std::string_view::npos);
            auto href = view.substr(0, pos);
            view = view.substr(pos + 2);
            auto link = view.substr(0, view.find(")"));
            twitter << std::format("{:<60} || {}\n", link, href);
        }
    }
    return 0;
}