#include "movement.hpp"

void IncreaseSpeed(float& speed) {
    speed += 0.1;
}

void DecreaseSpeed(float& speed) {
    if (speed > 0.2) {
        speed -= 0.1;
    }
}