#include "Light.h"

glm::vec3 getLightDirByHour(float hour) {
    const glm::vec3 ZENITH_DIR = glm::vec3(0.0f, 0.0f, 1.0f);

    if (hour < 0.0f) hour = 0.0f;
    if (hour > 24.0f) hour = 24.0f;

    const float noon = 13.0f;

    float angle;
    if (hour >= 6.0f && hour <= 20.0f) {
        angle = (hour - noon) * (3.14159265f / 14.0f * 2.0f);
    } else {
        float nightHour = hour;
        if (nightHour < 6.0f) nightHour += 24.0f;

        const float nightNoon = 25.0f;
        angle = (nightHour - nightNoon) * (3.14159265f / 10.0f * 2.0f);
    }

    glm::vec3 lightDir = glm::rotateY(ZENITH_DIR, angle);

    return glm::normalize(lightDir);
}