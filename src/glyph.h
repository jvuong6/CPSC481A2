#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <SFML/System/Vector2.hpp>

using Path = std::vector<sf::Vector2f>;  

std::vector<Path> loadGlyph(const std::string& filename)
{
    std::vector<Path> paths;
    std::ifstream in(filename);
    if (!in) return paths;     

    Path current;
    sf::Vector2f cur, start;
    char cmd;

    while (in >> cmd) {
        if (cmd == 'M') {
            if (!current.empty()) 
                paths.push_back(current);
            current.clear();
            in >> cur.x >> cur.y;
            start = cur;
            current.push_back(cur);
        }
        else if (cmd == 'L') {
            in >> cur.x >> cur.y;
            current.push_back(cur);
        }
        else if (cmd == 'C') {
            sf::Vector2f p1, p2, p3;    
            in >> p1.x >> p1.y >> p2.x >> p2.y >> p3.x >> p3.y;
                const int steps = 4;

                for (int i = 1; i <= steps; ++i) {
                    float t = float(i) / steps;
                    float u = 1.f - t;

                    current.push_back(
                        (u*u*u) * cur
                        + (3.f*t*u*u) * p1
                        + (3.f*t*t*u) * p2
                        + (t*t*t) * p3
                    );
                }
                cur = p3;
        }
        else if (cmd == 'Z') {
            current.push_back(start);
            cur = start;
        }
    }
    if (!current.empty()) paths.push_back(current);
    return paths;
}
