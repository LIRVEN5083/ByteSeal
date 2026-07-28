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
