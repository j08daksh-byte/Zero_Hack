#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cstdint>

namespace Nova {
    struct Node {
        std::string tag;
        std::string text;
        std::vector<std::pair<std::string, std::string>> attrs;
        std::vector<Node> children;

        std::string toString() const {
            std::ostringstream ss;
            ss << "<" << tag;
            for (const auto& attr : attrs) {
                ss << " " << attr.first << "=\"" << attr.second << "\"";
            }
            ss << ">" << text;
            for (const auto& child : children) {
                ss << child.toString();
            }
            ss << "</" << tag << ">";
            return ss.str();
        }
    };
}
