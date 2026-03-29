#include "CoordinateSystem.h"
#include <cmath> // For floor and fmod
#include <iostream>

// Constructor

CoordinateSystem::CoordinateSystem(float minLat, float maxLat, float minLon, float maxLon, int screenWidth, int screenHeight)
    : min_latitude(minLat), max_latitude(maxLat),
    min_longitude(minLon), max_longitude(maxLon),
    screen_width(screenWidth), screen_height(screenHeight) {
    std::cout << "CoordinateSystem initialized with bounds: "
        << "Lat[" << minLat << ", " << maxLat << "], "
        << "Lon[" << minLon << ", " << maxLon << "]\n";
}

std::pair<int, int> CoordinateSystem::to_screen_coordinates(float latitude, float longitude) const {
    if (has_map_transform) {
        double px = screen_mid_x + (longitude - map_mid_x) * map_scale;
        double py = screen_mid_y - (latitude - map_mid_y) * map_scale;
        return { static_cast<int>(px), static_cast<int>(py) };
    }

    // Clamp latitude and longitude
    float clamped_lat = std::max(min_latitude, std::min(latitude, max_latitude));
    float clamped_lon = std::max(min_longitude, std::min(longitude, max_longitude));

    // Normalize latitude and longitude
    float normalized_lat = (clamped_lat - min_latitude) / (max_latitude - min_latitude);
    float normalized_lon = (clamped_lon - min_longitude) / (max_longitude - min_longitude);

    // Invert Y-axis for latitude
    normalized_lat = 1.0f - normalized_lat;

    // Map to screen space
    float screen_x = normalized_lon * screen_width;
    float screen_y = normalized_lat * screen_height;

    // Apply zoom around screen center
    float cx = screen_width / 2.0f;
    float cy = screen_height / 2.0f;
    screen_x = cx + (screen_x - cx) * zoom;
    screen_y = cy + (screen_y - cy) * zoom;

    return { static_cast<int>(screen_x), static_cast<int>(screen_y) };
}


std::pair<float, float> CoordinateSystem::to_lat_lon(int screen_x, int screen_y) const {
    if (has_map_transform) {
        double lon = map_mid_x + (screen_x - screen_mid_x) / map_scale;
        double lat = map_mid_y - (screen_y - screen_mid_y) / map_scale;
        return { static_cast<float>(lat), static_cast<float>(lon) };
    }

    float cx = screen_width / 2.0f;
    float cy = screen_height / 2.0f;
    float unzoom_x = cx + (screen_x - cx) / zoom;
    float unzoom_y = cy + (screen_y - cy) / zoom;

    // Normalize screen coordinates to 0-1 range
    float normalized_lon = unzoom_x / screen_width;
    float normalized_lat = 1.0f - (unzoom_y / screen_height); // Invert y-axis

    // Convert to latitude and longitude
    float latitude = min_latitude + normalized_lat * (max_latitude - min_latitude);
    float longitude = min_longitude + normalized_lon * (max_longitude - min_longitude);

    return { latitude, longitude };
}

void CoordinateSystem::set_zoom(float z) {
    if (z < 0.2f) z = 0.2f;
    if (z > 5.0f) z = 5.0f;
    zoom = z;
}

float CoordinateSystem::get_zoom() const {
    return zoom;
}

// Wrap latitude and longitude to stay within bounds
void CoordinateSystem::wrap_coordinates(float& latitude, float& longitude) const {
    // Wrap latitude
    if (latitude < min_latitude) {
        latitude = max_latitude - (min_latitude - latitude);
    }
    else if (latitude > max_latitude) {
        latitude = min_latitude + (latitude - max_latitude);
    }

    // Wrap longitude
    if (longitude < min_longitude) {
        longitude = max_longitude - (min_longitude - longitude);
    }
    else if (longitude > max_longitude) {
        longitude = min_longitude + (longitude - max_longitude);
    }
}

// Getters
float CoordinateSystem::get_min_latitude() const {
    return min_latitude;
}

float CoordinateSystem::get_max_latitude() const {
    return max_latitude;
}

float CoordinateSystem::get_min_longitude() const {
    return min_longitude;
}

float CoordinateSystem::get_max_longitude() const {
    return max_longitude;
}

void CoordinateSystem::set_bounds(float min_lat, float max_lat, float min_lon, float max_lon) {
    min_latitude = min_lat;
    max_latitude = max_lat;
    min_longitude = min_lon;
    max_longitude = max_lon;
}

void CoordinateSystem::set_screen_size(int screen_w, int screen_h) {
    screen_width = screen_w;
    screen_height = screen_h;
}

void CoordinateSystem::set_map_transform(double min_x, double max_x, double min_y, double max_y, int screen_w, int screen_h, float zoom_level) {
    map_min_x = min_x;
    map_max_x = max_x;
    map_min_y = min_y;
    map_max_y = max_y;

    screen_width = screen_w;
    screen_height = screen_h;

    double shapeWidth = map_max_x - map_min_x;
    double shapeHeight = map_max_y - map_min_y;
    double scaleX = screen_width / shapeWidth;
    double scaleY = screen_height / shapeHeight;
    map_base_scale = std::min(scaleX, scaleY);
    map_scale = map_base_scale * zoom_level;

    map_mid_x = (map_min_x + map_max_x) / 2.0;
    map_mid_y = (map_min_y + map_max_y) / 2.0;
    screen_mid_x = screen_width / 2.0;
    screen_mid_y = screen_height / 2.0;

    has_map_transform = true;
}

bool CoordinateSystem::has_transform() const {
    return has_map_transform;
}

void CoordinateSystem::get_map_bounds(double& min_x, double& max_x, double& min_y, double& max_y) const {
    min_x = map_min_x;
    max_x = map_max_x;
    min_y = map_min_y;
    max_y = map_max_y;
}

double CoordinateSystem::get_map_scale() const {
    return map_scale;
}

double CoordinateSystem::get_map_base_scale() const {
    return map_base_scale;
}

int CoordinateSystem::get_screen_width() const {
    return screen_width;
}

int CoordinateSystem::get_screen_height() const {
    return screen_height;
}
