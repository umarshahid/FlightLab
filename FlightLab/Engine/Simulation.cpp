#include "Simulation.h"
#include <iostream>
#include <stdexcept>
#include <atomic> // For thread-safe running state
#include <cmath>
#include <fstream>

static void LogMessage(const std::string& msg) {
    std::ofstream log("flightlab.log", std::ios::app);
    if (log.is_open()) {
        log << msg << "\n";
    }
}


// Initialize the static instance to nullptr
std::once_flag flag;
std::unique_ptr<Simulation> Simulation::instance = nullptr;

Simulation::Simulation(): running(false) //coordSystem(-90.0f, 90.0f, -180.0f, 180.0f, 800, 600) { // Initialize running to false
{
    initialize_deploy();

    //################################ python ################################

    // Pass the simulation object to Python
    {
        py::gil_scoped_acquire gil;
        try {
            py::module sys = py::module::import("sys");
            py::list sys_path = sys.attr("path");
            sys_path.append("D:/repos/FlightLab/FlightLab/PyFlight");
            sys_path.insert(0, "D:/repos/FlightLab/FlightLab/Bind");

            py::dict modules = sys.attr("modules");
            if (modules.contains("flight_lab")) {
                modules.attr("pop")("flight_lab");
                LogMessage("[python] cleared cached module: flight_lab");
            }

            behavior_module = py::module::import("flight_behavior");
            LogMessage("[python] imported module: flight_behavior");
            if (pybind11::hasattr(behavior_module, "__file__")) {
                std::string mod_file = pybind11::str(behavior_module.attr("__file__"));
                LogMessage(std::string("[python] flight_behavior file: ") + mod_file);
            }

            py::module flight_lab_mod = py::module::import("flight_lab");
            if (pybind11::hasattr(flight_lab_mod, "__file__")) {
                std::string mod_file = pybind11::str(flight_lab_mod.attr("__file__"));
                LogMessage(std::string("[python] flight_lab file: ") + mod_file);
            }

            py::object py_sim = py::cast(this);
            behavior_module.attr("set_simulation")(py_sim);
            LogMessage("[python] set_simulation called");
        }
        catch (const pybind11::error_already_set& e) {
            LogMessage(std::string("[python] set_simulation error: ") + e.what());
        }
    }

    //################################ python ################################
}

Simulation::~Simulation() {

}

int Simulation::setCoordinateSystem(float min_lat, float max_lat, float min_lon, float max_lon,
    int screen_w, int screen_h) {
    coordSystem = CoordinateSystem(min_lat, max_lat, min_lon, max_lon, screen_w, screen_h);
    return 0;
}

CoordinateSystem Simulation::getCoordinateSystem() {
    return coordSystem;
}

void Simulation::setZoom(float zoom) {
    coordSystem.set_zoom(zoom);
}

void Simulation::setCoordinateBounds(float min_lat, float max_lat, float min_lon, float max_lon) {
    coordSystem.set_bounds(min_lat, max_lat, min_lon, max_lon);
}

void Simulation::setScreenSize(int screen_w, int screen_h) {
    coordSystem.set_screen_size(screen_w, screen_h);
}

void Simulation::setMapTransform(double min_x, double max_x, double min_y, double max_y, int screen_w, int screen_h, float zoom_level) {
    coordSystem.set_map_transform(min_x, max_x, min_y, max_y, screen_w, screen_h, zoom_level);
}

void Simulation::setDeployMode(SimulationObjectType dm) {
    deployMode = dm;
    if (deployMode != SimulationObjectType::Path) {
        selectedAircraftId = -1;
    }
}

SimulationObjectType Simulation::getDeployMode() {
    return deployMode;
}

std::string Simulation::getSelectedAircraft() {
    return selectedAircraft;
}

std::string Simulation::getSelectedWaypoint() {
    return selectedWaypoint;
}

Aircraft* Simulation::get_aircraft_by_id(int id) {
    for (auto& aircraft : aircrafts) {
        if (aircraft && aircraft->get_id() == id) {
            return aircraft.get();
        }
    }
    return nullptr;
}

bool Simulation::select_aircraft_at(int screen_x, int screen_y, float radius) {
    float best_dist = radius;
    int best_id = -1;
    for (auto& aircraft : aircrafts) {
        if (!aircraft) continue;
        auto pos = aircraft->get_position_xy();
        float dx = static_cast<float>(pos.first - screen_x);
        float dy = static_cast<float>(pos.second - screen_y);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= best_dist) {
            best_dist = dist;
            best_id = aircraft->get_id();
        }
    }

    selectedAircraftId = best_id;
    return selectedAircraftId != -1;
}

bool Simulation::has_selected_aircraft() const {
    return selectedAircraftId != -1;
}

Aircraft* Simulation::get_selected_aircraft() {
    if (selectedAircraftId == -1) return nullptr;
    return get_aircraft_by_id(selectedAircraftId);
}

void Simulation::clear_selected_aircraft() {
    selectedAircraftId = -1;
}

bool Simulation::plan_path_for_selected(float dest_lat, float dest_lon) {
    Aircraft* aircraft = get_selected_aircraft();
    if (!aircraft) return false;

    auto start = aircraft->get_position();

    PyGILState_STATE gstate = PyGILState_Ensure();
    bool ok = false;

    try {
        LogMessage("[python] plan_path called");
        auto bounds = getCoordinateSystem();
        pybind11::object result = behavior_module.attr("plan_path")(
            start.first, start.second,
            dest_lat, dest_lon,
            bounds.get_min_latitude(), bounds.get_max_latitude(),
            bounds.get_min_longitude(), bounds.get_max_longitude(),
            60, 40
        );

        std::vector<std::pair<float, float>> path;
        if (pybind11::isinstance<pybind11::list>(result)) {
            pybind11::list list = result.cast<pybind11::list>();
            for (auto item : list) {
                auto tup = item.cast<pybind11::tuple>();
                if (tup.size() != 2) continue;
                float lat = tup[0].cast<float>();
                float lon = tup[1].cast<float>();
                path.emplace_back(lat, lon);
            }
        }

        if (!path.empty()) {
            aircraft->set_path(path);
            ok = true;
            LogMessage("[python] plan_path returned " + std::to_string(path.size()) + " points");
        }
    }
    catch (const pybind11::error_already_set& e) {
        std::cerr << "Python error: " << e.what() << "\n";
        LogMessage(std::string("[python] plan_path error: ") + e.what());
    }

    PyGILState_Release(gstate);
    return ok;
}

int Simulation::add_airway_node(const std::string& name, float lat, float lon) {
    AirwayNode node;
    node.id = next_airway_node_id++;
    node.name = name;
    node.lat = lat;
    node.lon = lon;
    airway_nodes.push_back(node);
    return node.id;
}

void Simulation::add_airway_edge(int from_id, int to_id, float cost) {
    AirwayEdge edge;
    edge.from_id = from_id;
    edge.to_id = to_id;
    edge.cost = cost;
    airway_edges.push_back(edge);
}

void Simulation::clear_airways() {
    airway_nodes.clear();
    airway_edges.clear();
    next_airway_node_id = 1;
}

const std::vector<Simulation::AirwayNode>& Simulation::get_airway_nodes() const {
    return airway_nodes;
}

const std::vector<Simulation::AirwayEdge>& Simulation::get_airway_edges() const {
    return airway_edges;
}

// Check if the simulation is running
bool Simulation::is_running() const {
    return running;
}

void Simulation::set_running(bool state) {
    running = state;
}

void Simulation::onButtonclick(std::string color) {
    if (color == "red") {
        selectedAircraft = "Red";
        deployMode = SimulationObjectType::Aircraft;
    }

    if (color == "blue") {
        selectedAircraft = "Blue";
        deployMode = SimulationObjectType::Aircraft;
    }

    if (color == "red-waypoint") {
        selectedWaypoint = "Red";
        deployMode = SimulationObjectType::Waypoint;
    }

    if (color == "blue-waypoint") {
        selectedWaypoint = "Blue";
        deployMode = SimulationObjectType::Waypoint;
    }
}

Simulation& Simulation::get_instance() {
    std::call_once(flag, []() {
        instance = std::unique_ptr<Simulation>(new Simulation());
        });
    return *instance;
}

void Simulation::render_single_aircraft(std::string color) {
    if (color == "red") {
        add_aircraft("Fighter - Red", "Red", 100, 60.f, -120.0f, 90.0f, 0.25f, coordSystem);
    }

    if (color == "blue") {
        add_aircraft("Fihgter - Blue", "Blue", 100, -60.f, 120.0f, 270.0f, 0.25f, coordSystem);
    }
}

void Simulation::render_waypoint(std::string color, float x, float y) {
    if (color == "Red") {
        add_waypoint("Fighter - Red", "Red", x, y, coordSystem);
    }
    else if (color == "Blue") {
        add_waypoint("Fighter - Blue", "Blue", x, y, coordSystem);
    }
}

void Simulation::render_single_aircraft(std::string color, float x, float y, float angle) {
    if (color == "Red") {
        add_aircraft("Fighter - Red", "Red", 100, x, y, angle, 0.10f, coordSystem);
        //add_aircraft("Fighter - Red", "Red", 100, x, y, angle, 0.25f, coordSystem);
    }
    else if (color == "Blue") {
        add_aircraft("Fighter - Blue", "Blue", 100, x, y, angle, 0.10f, coordSystem);
        //add_aircraft("Fighter - Blue", "Blue", 100, x, y, angle, 0.25f, coordSystem);
    }
}

void Simulation::render_aircrafts(std::string color) {
    int grid_size = 5; // 7x7 grid
    float spacing = 15.0f; // Distance between aircraft in the grid

    if (color == "red") {
        // Red team: Top-left corner
        float red_start_lat = 90.0f - 20;  // Top of the map
        float red_start_lon = -180.0f + 30; // Left of the map

        // Render Red team
        for (int row = 0; row < grid_size; ++row) {
            for (int col = 0; col < grid_size; ++col) {
                float lat = red_start_lat - row * spacing; // Move downward
                float lon = red_start_lon + col * spacing; // Move rightward
                std::string name = "RedAircraft" + std::to_string(row * grid_size + col + 1);
                add_aircraft(name, "Red", 100, lat, lon, 90.0f, 0.25f, coordSystem);
            }
        }
    }
    
    if (color == "blue") {
        // Blue team: Bottom-right corner
        float blue_start_lat = -90.0f + 20; // Bottom of the map
        float blue_start_lon = 180.0f - 30; // Right of the map

        // Render Blue team
        for (int row = 0; row < grid_size; ++row) {
            for (int col = 0; col < grid_size; ++col) {
                float lat = blue_start_lat + row * spacing; // Move upward
                float lon = blue_start_lon - col * spacing; // Move leftward
                std::string name = "BlueAircraft" + std::to_string(row * grid_size + col + 1);
                add_aircraft(name, "Blue", 100, lat, lon, 270.0f, 0.25f, coordSystem);
            }
        }
    }
    
}

// Add an aircraft
void Simulation::add_aircraft(const std::string& name, const std::string& force, int health, float x, float y, float heading, float speed, CoordinateSystem& coordSystem) {
    // Create an aircraft
    //aircrafts.emplace_back(name, force, health, x, y, heading, speed, coordSystem);
    aircrafts.emplace_back(std::make_unique<Aircraft>(name, force, health, x, y, heading, speed, coordSystem));
}

void Simulation::add_waypoint(const std::string& name, const std::string& force, float x, float y, CoordinateSystem& coordSystem) {
    // Create an aircraft
    waypoints.emplace_back(name, force, x, y, coordSystem);
}

// Get aircrafts (const version)
const std::vector<std::unique_ptr<Aircraft>>& Simulation::get_aircrafts() const {
    return aircrafts;
}

const std::vector<Waypoint>& Simulation::get_waypoints() const {
    return waypoints;
}


std::vector<std::unique_ptr<Aircraft>>& Simulation::get_aircrafts_mutable() {
    return aircrafts;
}

// simulation Update
void Simulation::simulation_update() {
    for (auto& aircraft : aircrafts) {
		if (aircraft) {
			aircraft->update(_dt); // Update each aircraft's state
		}
        //aircraft->update(_dt); // Update each aircraft's state
    }

    for (auto& waypoint : waypoints) {
        waypoint.update(); // Update each aircraft's state
    }
    //initialize();
    //PyGILState_STATE gstate;
    //gstate = PyGILState_Ensure();

    //// Perform Python actions here.
     //behavior_module.attr("sim_update")();
     //behavior_module.attr("call_once")();

    //// Release the thread. No Python API allowed beyond this point.
    //PyGILState_Release(gstate);
}

// Assuming necessary headers are included and Simulation, Aircraft, Waypoint classes exist

void Simulation::processSimulation() {
    // Use pointers to original waypoints
    std::deque<Waypoint*> red_waypoints;
    std::deque<Waypoint*> blue_waypoints;

    // Iterate through original waypoints by reference
    for (Waypoint& wp : waypoints) { // Assuming waypoints is std::vector<Waypoint>
        if (wp.get_force() == "Red") {
            red_waypoints.push_back(&wp);
        }
        else if (wp.get_force() == "Blue") {
            blue_waypoints.push_back(&wp);
        }
    }

    // Process each aircraft by reference to modify originals
    for (const auto& aircraft : aircrafts) { // Assuming aircrafts is std::vector<Aircraft>
        // Get current position
        double aircraft_lat, aircraft_lon;
        aircraft_lat = aircraft->get_position().first;
        aircraft_lon = aircraft->get_position().second;

        Waypoint* target_waypoint = nullptr;
        std::string force = aircraft->get_force();

        // Assign appropriate waypoint based on force
        if (force == "Red") {
            if (!red_waypoints.empty()) {
                target_waypoint = red_waypoints.front();
                red_waypoints.pop_front();
            }
            else {
                std::cout << "No red waypoints available for aircraft "
                    << aircraft->get_name() << std::endl;
            }
        }
        else if (force == "Blue") {
            if (!blue_waypoints.empty()) {
                target_waypoint = blue_waypoints.front();
                blue_waypoints.pop_front();
            }
            else {
                std::cout << "No blue waypoints available for aircraft "
                    << aircraft->get_name() << std::endl;
            }
        }

        // Move aircraft if valid waypoint exists
        if (target_waypoint) {
            double target_lat, target_lon;
            target_lat = target_waypoint->get_position().first;
            target_lon = target_waypoint->get_position().second;
            aircraft->move_to(target_lat, target_lon);

            std::cout << "Moving Aircraft " << aircraft->get_name()
                << " from (" << aircraft_lat << ", " << aircraft_lon
                << ") to (" << target_lat << ", " << target_lon
                << ") at waypoint " << target_waypoint->get_name()
                << std::endl;
        }
        else {
            std::cout << "Aircraft " << aircraft->get_name()
                << " has no valid waypoint to move to." << std::endl;
        }
        //aircraft.launch_missile();  // Launch the missile (when the condition is right)

    }
}

void Simulation::initialize() {
// C++ Process call
    //processSimulation();

// Python Process call
 
    try {
        py::gil_scoped_acquire gil;
        LogMessage("[python] initialize run_script()");

        if (pybind11::hasattr(behavior_module, "run_script")) {
            behavior_module.attr("run_script")();
            LogMessage("[python] run_script completed");
            return;
        }

        LogMessage("[python] initialize call_once()");
        pybind11::object result = behavior_module.attr("call_once")();

        // Process the returned value (example for a dictionary)
        if (pybind11::isinstance<pybind11::dict>(result)) {
            pybind11::dict result_dict = result.cast<pybind11::dict>();
            std::string status = pybind11::str(result_dict["status"]);
            pybind11::list data = result_dict["data"].cast<pybind11::list>();

            std::cout << "Status: " << status << "\n";
            std::cout << "Data: ";
            for (auto item : data) {
                std::cout << item.cast<int>() << " ";
            }
            std::cout << "\n";
        }
        else {
            std::cerr << "Unexpected return type from call_once\n";
        }

    }
    catch (const pybind11::error_already_set& e) {
        std::cerr << "Python error: " << e.what() << "\n";
        LogMessage(std::string("[python] call_once error: ") + e.what());
    }

}

void Simulation::remove_aircraft(Aircraft* target) {
    aircrafts.erase(
        std::remove_if(aircrafts.begin(), aircrafts.end(),
            [&](const std::unique_ptr<Aircraft>& a) { return a.get() == target; }),
        aircrafts.end()
    );
}


