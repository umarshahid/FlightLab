#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <memory>


class Aircraft;

class Radar
{
    int radarRadius;  // Range of the radar in pixels
    float radarAngle; // Half-angle of the radar cone in degrees

public:
    Radar(int radius, float angle);
    Radar(); // Default constructor
    ~Radar() = default;

    // Disallow copying
    Radar(const Radar&) = delete;
    Radar& operator=(const Radar&) = delete;

    // Allow moving
    Radar(Radar&&) noexcept = default;
    Radar& operator=(Radar&&) noexcept = default;

    std::vector<std::reference_wrapper<Aircraft>> getEntitiesInRadarCone(std::vector<std::unique_ptr<Aircraft>>& entities, float centerLat, float centerLon, float heading, float geoRadius);
    float normalizeAngle(float angle);

	inline int getRadarRadius() const { return radarRadius; }
	inline float getRadarAngle() const { return radarAngle; }
    void update();
};

