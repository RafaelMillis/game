//
// Created by amit on 5/27/25.
//

#ifndef TANKSBATTLEGAME_SATELLITEVIEW_H
#define TANKSBATTLEGAME_SATELLITEVIEW_H

#include <stdio.h>

class SatelliteView {
public:
    virtual ~SatelliteView() {}
    virtual char getObjectAt(size_t x, size_t y) const = 0;
};

#endif //TANKSBATTLEGAME_SATELLITEVIEW_H
