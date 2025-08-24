#include "Tank.h"

// Initialize static counter
int Tank::next_tank_id = 0;
int Tank::initial_shells = 16; // Default value, will be updated from map file 