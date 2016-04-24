//
// Created by wastl on 24.04.16.
//

#include "ProgressBar.h"

#include <iostream>

namespace wikidata {
namespace primarysources {

void ProgressBar::Update(long value) {
    std::cout << "[";
    long pos = width * value/max;
    for (int i = 0; i < width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << (100 * value/max) << " %\r";
    std::cout.flush();
}

ProgressBar::~ProgressBar() {
    std::cout << std::endl;
}


}  // namespace primarysources
}  // namespaec wikidata
