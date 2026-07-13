#ifndef _MOVEMENT_HPP
#define _MOVEMENT_HPP

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <chrono>
#include <iostream>

extern uint32_t WindowSizeX;
extern uint32_t WindowSizeY;

//Movement
extern float valueZ;
extern float valueX;
extern float valueY;
extern float speed;

//For view
extern float lastX;
extern float lastY;
extern float yaw;
extern float pitch;

extern float speed;
extern int times;

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void IncreaseSpeed(float& speed);

void DecreaseSpeed(float& speed);

float getTime();

void RegSpeed(GLFWwindow* window, int key, int scancode, int action, int mode);

#endif