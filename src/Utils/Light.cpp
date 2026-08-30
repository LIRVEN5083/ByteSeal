#include "Light.h"

glm::vec3 getLightDirByHour(float hour) {
    if (hour < 0.0f) hour = 0.0f;
    if (hour > 24.0f) hour = 24.0f;

    const glm::vec3 ZENITH_DIR = glm::vec3(0.0f, 0.0f, 1.0f);
    float angle = (hour - 12.0f) * (3.14159265f / 12.0f);

    glm::vec3 lightDir = glm::rotateY(ZENITH_DIR, angle);

    return glm::normalize(lightDir);
}