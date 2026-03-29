#pragma once
#include <string>
#include "enums.h"
#include <cmath>
#include <vector>
#include <iostream>

static std::string simulationObjectTypeToString(SimulationObjectType type) {
    switch (type) {
    case SimulationObjectType::Aircraft: return "Aircraft";
    case SimulationObjectType::Waypoint: return "Waypoint";
    case SimulationObjectType::Missile: return "Missile";
    case SimulationObjectType::Path: return "Path";
    default: return "Unknown";
    }
}

// 3D Vector Class
struct Vector3 {
    double x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vector3 operator-(const Vector3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vector3 operator*(double scalar) const { return { x * scalar, y * scalar, z * scalar }; }

    double magnitude() const { return sqrt(x * x + y * y + z * z); }
    Vector3 normalize() const {
        double mag = magnitude();
        return (mag > 0) ? *this * (1.0 / mag) : Vector3(0, 0, 0);
    }
    // Print function (for debugging)
    void print() const {
        std::cout << "Position: (" << x << ", " << y << ", " << z << ")\n";
    }

    // Dot product
    double dot(const Vector3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    // Cross product
    Vector3 cross(const Vector3& v) const {
        return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
    }
};
