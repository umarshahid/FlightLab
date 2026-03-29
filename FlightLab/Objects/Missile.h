#pragma once
#include <iostream>
#include "../Utils/utils.cpp"
#include <cmath>
#include <string>
#include <utility>

class CoordinateSystem;

class Missile {
public:
    std::string force;
    Vector3 velocity;
    Vector3 acceleration;
    double speed;
    double maxAcceleration;
    bool active;
    bool hit = false;
    float heading;

    Missile(float heading, std::string force, float lat, float lon, CoordinateSystem* coordSystem, double missileSpeed, double maxAccel);
    void update(float targetLat, float targetLon, double dt);
    Vector3 get_position3() const;
    double getHeading();
    float get_heading();
    void set_heading(float new_heading);
    std::pair<int, int> get_target_position_xy() const;
    void updatePosition(float targetLat, float targetLon, double dt);

private:
    float latitude = 0.0f;
    float longitude = 0.0f;
    float target_latitude = 0.0f;
    float target_longitude = 0.0f;
    CoordinateSystem* coordinateSystem = nullptr;
};
