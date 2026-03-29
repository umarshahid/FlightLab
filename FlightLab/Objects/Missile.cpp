#include "Missile.h"
#include "../Randerer/RenderManager.h"
#include "../Engine/World/CoordinateSystem.h"
#include <algorithm>

// Constructor
Missile::Missile(float heading, std::string force, float lat, float lon, CoordinateSystem* coordSystem, double missileSpeed, double maxAccel)
    : heading(heading), force(force), speed(missileSpeed), maxAcceleration(maxAccel), active(true),
    latitude(lat), longitude(lon), coordinateSystem(coordSystem) {
}

// Update Function
void Missile::update(float targetLat, float targetLon, double dt) {
    if (!active) return;

    target_latitude = targetLat;
    target_longitude = targetLon;

    updatePosition(targetLat, targetLon, dt);
    RenderManager::get_instance().drawMissile(this);
}

void Missile::updatePosition(float targetLat, float targetLon, double dt) {
    if (!active) return;

    float dlat = targetLat - latitude;
    float dlon = targetLon - longitude;
    float distance = std::sqrt(dlat * dlat + dlon * dlon);

    if (distance <= 0.0005f) {
        latitude = targetLat;
        longitude = targetLon;
        active = false;
		hit = true;
        std::cout << "reached.......";
        return;
    }

    float target_heading = std::atan2(dlon, dlat) * 180.0f / M_PI;
    float rotation_speed = 180.0f * dt;
    float heading_diff = target_heading - heading;
    if (heading_diff > 180.0f) heading_diff -= 360.0f;
    if (heading_diff < -180.0f) heading_diff += 360.0f;
    heading += std::clamp(heading_diff, -rotation_speed, rotation_speed);

    float move_speed = static_cast<float>(speed * dt);
    if (move_speed >= distance) {
        latitude = targetLat;
        longitude = targetLon;
        active = false;
        hit = true;
        return;
    }

    latitude += (dlat / distance) * move_speed;
    longitude += (dlon / distance) * move_speed;

    if (coordinateSystem) {
        coordinateSystem->wrap_coordinates(latitude, longitude);
    }
}

Vector3 Missile::get_position3() const {
    if (!coordinateSystem) return Vector3(0, 0, 0);
    auto screen = coordinateSystem->to_screen_coordinates(latitude, longitude);
    return Vector3(screen.first, screen.second, 0.0);
}

double Missile::getHeading() {
    double heading = std::atan2(velocity.y, velocity.x) * 180.0 / M_PI; // Convert to degrees
    if (heading < 0) {
        heading += 360.0;  // Ensure heading is in [0, 360] range
    }
    return heading;
}

float Missile::get_heading() {
    return heading;
}

void Missile::set_heading(float new_heading) {
    heading = std::fmod(new_heading, 360.0f); // Ensure heading is within 0-359 degrees
    if (heading < 0) {
        heading += 360.0f; // Normalize negative headings
    }
}

std::pair<int, int> Missile::get_target_position_xy() const {
    if (!coordinateSystem) return { 0, 0 };
    return coordinateSystem->to_screen_coordinates(target_latitude, target_longitude);
}
