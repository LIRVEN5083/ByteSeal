#include "movement.hpp"

void CONTROLLER::IncreaseSpeed(float& speed) {
    speed += 0.06;
}

void CONTROLLER::DecreaseSpeed(float& speed) {
    if (speed > 0.2) {
        speed -= 0.06;
    }
}

void CONTROLLER::update_time(Movement& _movement, Delta& _delta){
    auto currentTime = std::chrono::high_resolution_clock::now();

    static bool firstFrame = true;
    if (firstFrame) {
        _delta.lastFrameTime = currentTime;
        _delta.delta = 0.016f;
        _delta.moveStep = _delta.delta * _movement.speed;
        firstFrame = false;
        return;
    }

    _delta.delta = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - _delta.lastFrameTime).count();
    _delta.lastFrameTime = currentTime;
    _delta.moveStep = _delta.delta * _movement.speed;
}

void CONTROLLER::made_move(Movement& _movement, Camera& _camera, Delta& _delta){
    if (_camera.isCameraActive) {
        float sensitivity = 0.05f;
        _camera.yaw   -= _camera.mouseDeltaX * sensitivity;
        _camera.pitch += _camera.mouseDeltaY * sensitivity;

        if (_camera.pitch > 89.0f)  _camera.pitch = 89.0f;
        if (_camera.pitch < -89.0f) _camera.pitch = -89.0f;

        _camera.mouseDeltaX = 0.0f;
        _camera.mouseDeltaY = 0.0f;

        _camera.front.x = cos(glm::radians(_camera.yaw)) * cos(glm::radians(_camera.pitch));
        _camera.front.y = sin(glm::radians(_camera.yaw)) * cos(glm::radians(_camera.pitch));
        _camera.front.z = sin(glm::radians(_camera.pitch));
        _camera.front = glm::normalize(_camera.front);

        _camera.Wfront.x = cos(glm::radians(_camera.yaw));
        _camera.Wfront.y = sin(glm::radians(_camera.yaw));
        _camera.Wfront.z = 0.0f;

        glm::vec3 up = {0.0f, 0.0f, 1.0f};
        _camera.right = glm::normalize(glm::cross(_camera.Wfront, up));

        //std::cout<<"X: "<< _movement.valueX <<"\t"<<"Y: "<<_movement.valueY<<"\t"<<"Z: "<<_movement.valueZ<<"\n";
        int numkeys;
        const bool* keyboardState = SDL_GetKeyboardState(&numkeys);

        if (keyboardState[SDL_SCANCODE_W]) {
            _movement.valueY += _camera.Wfront.y * _delta.moveStep;
            _movement.valueX += _camera.Wfront.x * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_S]) {
            _movement.valueY -= _camera.Wfront.y * _delta.moveStep;
            _movement.valueX -= _camera.Wfront.x * _delta.moveStep;
        }

        if (keyboardState[SDL_SCANCODE_A]) {
            _movement.valueY -= _camera.right.y * _delta.moveStep;
            _movement.valueX -= _camera.right.x * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_D]) {
            _movement.valueY += _camera.right.y * _delta.moveStep;
            _movement.valueX += _camera.right.x * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_SPACE]) {
            _movement.valueZ += 1.0f * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_LSHIFT]) {
            _movement.valueZ -= 1.0f * _delta.moveStep;
        }
        if (keyboardState[SDL_SCANCODE_E]) {
            CONTROLLER::IncreaseSpeed(_movement.speed);
        }
        if (keyboardState[SDL_SCANCODE_Q]) {
            CONTROLLER::DecreaseSpeed(_movement.speed);
        }
    }
}
