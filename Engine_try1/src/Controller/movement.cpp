#include "movement.hpp"

void CONTROLLER::IncreaseSpeed(float& speed) {
    speed += 0.1;
}

void CONTROLLER::DecreaseSpeed(float& speed) {
    if (speed > 0.2) {
        speed -= 0.1;
    }
}