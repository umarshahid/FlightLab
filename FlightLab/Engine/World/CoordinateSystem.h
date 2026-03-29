#pragma once
#include <utility> // For std::pair

class CoordinateSystem {
private:
    float min_latitude;   // Minimum latitude value (e.g., 0.0)
    float max_latitude;   // Maximum latitude value (e.g., 100.0)
    float min_longitude;  // Minimum longitude value (e.g., 0.0)
    float max_longitude;  // Maximum longitude value (e.g., 100.0)
    int screen_width;     // Screen width in pixels
    int screen_height;    // Screen height in pixels
    float zoom = 1.0f;
    bool has_map_transform = false;
    double map_min_x = 0.0;
    double map_max_x = 0.0;
    double map_min_y = 0.0;
    double map_max_y = 0.0;
    double map_scale = 1.0;
    double map_base_scale = 1.0;
    double map_mid_x = 0.0;
    double map_mid_y = 0.0;
    double screen_mid_x = 0.0;
    double screen_mid_y = 0.0;

public:
    // Constructor
	CoordinateSystem() = default;
    CoordinateSystem(float min_lat, float max_lat, float min_lon, float max_lon,
        int screen_w, int screen_h);

    // Convert latitude/longitude to screen coordinates
    std::pair<int, int> to_screen_coordinates(float latitude, float longitude) const;

    // Convert screen coordinates to latitude/longitude
    std::pair<float, float> to_lat_lon(int screen_x, int screen_y) const;

    // Wrap latitude and longitude to stay within bounds
    void wrap_coordinates(float& latitude, float& longitude) const;

    void set_zoom(float z);
    float get_zoom() const;

    // Getters
    float get_min_latitude() const;
    float get_max_latitude() const;
    float get_min_longitude() const;
    float get_max_longitude() const;

    void set_bounds(float min_lat, float max_lat, float min_lon, float max_lon);
    void set_screen_size(int screen_w, int screen_h);
    void set_map_transform(double min_x, double max_x, double min_y, double max_y, int screen_w, int screen_h, float zoom_level);
    bool has_transform() const;
    void get_map_bounds(double& min_x, double& max_x, double& min_y, double& max_y) const;
    double get_map_scale() const;
    double get_map_base_scale() const;
    int get_screen_width() const;
    int get_screen_height() const;
};
