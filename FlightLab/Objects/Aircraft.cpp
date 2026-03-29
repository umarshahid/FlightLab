#include "Aircraft.h"
#include <iostream>
#include <cmath> // For sin and cos
#include "../Engine/World/CoordinateSystem.h"
#include "../Utils/FileLoader.cpp"
#include "../Engine/Simulation.h"
#include "../Randerer/RenderManager.h"

int Aircraft::nextID = 1;

// Constructor
Aircraft::Aircraft(const std::string& name, const std::string& force, int health,
    float startLatitude, float startLongitude, float heading, float speed, CoordinateSystem& coordinateSystem)
	: name(name), health(health), latitude(startLatitude), longitude(startLongitude), altitude(0.0f),
    coordinateSystem(coordinateSystem), heading(heading), speed(speed), is_moving(false), id(nextID++) { // Default heading is North
    if (force != "Red" && force != "Blue") {
        std::cerr << "Invalid force value: " << force << ". Force must be 'Red' or 'Blue'.\n";
        this->force = "Blue"; // Default to Blue if invalid
    }
    else {
        this->force = force;
    }
    position.x = get_position_xy().first;
    position.y = get_position_xy().second;
    position.z = altitude;
    velocity = calculateVelocityFromAirspeed(speed, heading, 5.0);
    radar = Radar(150, 45.0f);
}

//Aircraft::~Aircraft(){
//    
//}

void Aircraft::adjustHeadingToNorth() {
    heading = 90.0f - heading;  // Adjust so 0 degrees points north
    if (heading < 0.0f) {
        heading += 360.0f;  // Ensure heading stays within 0-360 range
    }
}

Vector3 Aircraft::calculateVelocityFromAirspeed(double airspeed, double heading, double climbAngle) {
    double headingRad = heading * M_PI / 180.0;  // Convert degrees to radians
    double climbRad = climbAngle * M_PI / 180.0;

    double vx = airspeed * cos(climbRad) * sin(headingRad);
    double vy = airspeed * cos(climbRad) * cos(headingRad);
    double vz = airspeed * sin(climbRad);

    return Vector3(vx, vy, vz);
}

Vector3 Aircraft::get_velocity() const {
	return velocity;
}

Vector3 Aircraft::get_position3() const {
    return position;
}

// Getter Methods
int Aircraft::get_id() const {
    return id;
}

std::string Aircraft::get_name() const {
    return name;
}

std::string Aircraft::get_force() const {
    return force;
}

int Aircraft::get_health() const {
    return health;
}

float Aircraft::get_heading() const {
    return heading;
}

std::pair<float, float> Aircraft::get_position() const {
    return { latitude, longitude };
}

// Set the heading
void Aircraft::set_heading(float new_heading) {
    heading = std::fmod(new_heading, 360.0f); // Ensure heading is within 0-359 degrees
    if (heading < 0) {
        heading += 360.0f; // Normalize negative headings
    }
}


std::pair<int, int> Aircraft::get_position_xy() const {
    return coordinateSystem.to_screen_coordinates(latitude, longitude);
}

std::pair<int, int> Aircraft::get_target_position_xy() const {
    return coordinateSystem.to_screen_coordinates(target_latitude, target_longitude);
}

void Aircraft::move_to(float newLatitude, float newLongitude) {
    target_latitude = newLatitude;
    target_longitude = newLongitude;
    is_moving = true; // Start moving
}

void Aircraft::set_path(const std::vector<std::pair<float, float>>& path) {
    path_queue.clear();
    for (const auto& p : path) {
        path_queue.push_back(p);
    }
}

void Aircraft::clear_path() {
    path_queue.clear();
}

bool Aircraft::has_path() const {
    return !path_queue.empty();
}

const std::deque<std::pair<float, float>>& Aircraft::get_path() const {
    return path_queue;
}

void Aircraft::update_position(double dt) {
    if (!is_moving) return;

    float dlat = target_latitude - latitude;
    float dlon = target_longitude - longitude;
    float distance = std::sqrt(dlat * dlat + dlon * dlon);

    if (distance <= 0.0001f) {
        latitude = target_latitude;
        longitude = target_longitude;
        is_moving = false;
    } else {
        float target_heading = std::atan2(dlon, dlat) * 180.0f / M_PI;

        float rotation_speed = 90.0f * dt;
        float heading_diff = target_heading - heading;
        if (heading_diff > 180.0f) heading_diff -= 360.0f;
        if (heading_diff < -180.0f) heading_diff += 360.0f;
        heading += std::clamp(heading_diff, -rotation_speed, rotation_speed);

        float move_speed = speed * static_cast<float>(dt);
        if (move_speed >= distance) {
            latitude = target_latitude;
            longitude = target_longitude;
            is_moving = false;
        } else {
            latitude += (dlat / distance) * move_speed;
            longitude += (dlon / distance) * move_speed;
        }
    }

    coordinateSystem.wrap_coordinates(latitude, longitude);
    auto screen = get_position_xy();
    position.x = screen.first;
    position.y = screen.second;
    position.z = altitude;

    //std::cout << "target position: ";
    //target_position.print();
    //// Update missile (if exists)
    //update_missile(dt, target_position, Vector3(0, 0, 0));
}



// Move the aircraft in the direction of its current heading
void Aircraft::move(float distance) {
    float heading_radians = heading * M_PI / 180.0f;

    // Calculate new latitude and longitude based on heading
    float delta_latitude = distance * std::cos(heading_radians);
    float delta_longitude = distance * std::sin(heading_radians);

    latitude += delta_latitude;
    longitude += delta_longitude;

    // Wrap around the coordinate system if needed
    coordinateSystem.wrap_coordinates(latitude, longitude);
}

void Aircraft::attack(Aircraft& target) {
    if (!is_alive()) {
        std::cout << name << " cannot attack because it is destroyed.\n";
        return;
    }
    if (!target.is_alive()) {
        std::cout << target.get_name() << " is already destroyed.\n";
        return;
    }
    target.health -= 10; // Example damage value
    target.health = std::max(0, target.health); // Ensure health does not go below 0
    std::cout << name << " attacked " << target.get_name() << "!\n";
}

void Aircraft::defend() {
    if (!is_alive()) {
        std::cout << name << " cannot defend because it is destroyed.\n";
        return;
    }
    health += 5; // Example defense healing value
    health = std::min(100, health); // Assume 100 is max health
    std::cout << name << " is defending and restored health to " << health << ".\n";
}

bool Aircraft::is_alive() const {
    return health > 0;
}


void Aircraft::update(double dt) {
    if (!is_alive()) {
        std::cout << name << " is destroyed and cannot perform updates.\n";
        return;
    }

    if (!is_moving && !path_queue.empty()) {
        auto next = path_queue.front();
        path_queue.pop_front();
        move_to(next.first, next.second);
    }

    auto screen = get_position_xy();
    position.x = screen.first;
    position.y = screen.second;
    position.z = altitude;

    // Update the position if the aircraft is moving
    update_position(dt);

    // Update missile (if current target exists)
    if (current_target)
        update_missile(dt, current_target->get_position());

	RenderManager::get_instance().drawAircraft(this);

    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> delta = currentTime - lastTime;
    lastTime = currentTime;

    deltaTime = delta.count(); // Delta time in seconds

    // Call function after 1 second
    static float elapsedTime = 0.0f;
    elapsedTime += deltaTime;
    if (elapsedTime >= 1.0f) {
        // #### Radar scane
        // #### Radar scane
        elapsedTime = 0.0f;
    }
        perform_radar_scan();
}

void Aircraft::launch_missile() {
    if (!missile) {
        auto pos = get_position();
        missile = new Missile(heading, force, pos.first, pos.second, &coordinateSystem, 0.5, 10.0);
        std::cout << "Missile launched!\n";
    }
}

void Aircraft::update_missile(double dt, std::pair<float, float> targetPos) {
    if (missile) {
        if (missile->hit) {
            std::cout << "[MISSILE HIT] " << current_target->get_name() << " destroyed!\n";

            // Destroy the target
            Simulation::get_instance().remove_aircraft(current_target);

             //Clear missile and target reference
            current_target = nullptr;
            delete missile;
            missile = nullptr;
        } else
            missile->update(targetPos.first, targetPos.second, dt);  // Update missile trajectory and check for collision
    }
}

void Aircraft::perform_radar_scan() {
    int screen_x = get_position3().x;
    int screen_y = get_position3().y;

    auto& simulation = Simulation::get_instance();
    double baseScale = simulation.getCoordinateSystem().get_map_base_scale();
    if (baseScale <= 0.0001) baseScale = 1.0;
    float geoRadius = static_cast<float>(radar.getRadarRadius() / baseScale);
    auto& allAircrafts = simulation.get_aircrafts_mutable();

    std::vector<std::reference_wrapper<Aircraft>> detectedEntities =
        radar.getEntitiesInRadarCone(allAircrafts, latitude, longitude, get_heading(), geoRadius);

    std::unordered_set<int> engagedTargets; // Keeps track of already targeted entities

    for (const auto& entity : detectedEntities) {
        if (&entity.get() == this) // Skip self-detection
            continue;

        int target_id = entity.get().get_id();

        if (engagedTargets.find(target_id) == engagedTargets.end()) {
            // Target not engaged before, launch missile
            //std::cout << "[" << get_name() << " - " << get_id() << "]'s radar detected: ["
            //    << entity.get().get_name() << " - " << target_id << "]\n";

            RenderManager::get_instance().lockLine(&entity.get().position, &position, force);

            // Set the current target position and velocity
            current_target = &entity.get();

            //current_target_position = entity.get().get_position3();
            //current_target_velocity = entity.get().get_velocity();

            launch_missile();  // Fire missile
            engagedTargets.insert(target_id); // Mark this target as engaged
        }
    }
}
