#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../Engine/Simulation.h"

namespace py = pybind11;

PYBIND11_MODULE(flight_lab, m) {
    py::class_<Aircraft>(m, "Aircraft")
        .def("get_name", &Aircraft::get_name)
        .def("get_force", &Aircraft::get_force)
        .def("get_health", &Aircraft::get_health)
        .def("get_position", &Aircraft::get_position)
        .def("move_to", &Aircraft::move_to)
        .def("attack", &Aircraft::attack)
        .def("defend", &Aircraft::defend)
        .def("set_heading", &Aircraft::set_heading)
        .def("get_heading", &Aircraft::get_heading)
        .def("is_alive", &Aircraft::is_alive);

    py::class_<Waypoint>(m, "Waypoint")
        .def("get_name", &Waypoint::get_name)
        .def("get_force", &Waypoint::get_force)
        .def("get_position", &Waypoint::get_position);

    py::class_<Simulation>(m, "Simulation")
        .def("add_aircraft", &Simulation::add_aircraft)
        //.def("get_aircrafts", &Simulation::get_aircrafts, py::return_value_policy::reference_internal)
        .def("get_aircrafts", [](Simulation& self) {
        std::vector<Aircraft*> aircrafts_raw;
        for (auto& aircraft : self.get_aircrafts()) {
            aircrafts_raw.push_back(aircraft.get());  // Convert unique_ptr to raw pointer
        }
        return aircrafts_raw;
        }, py::return_value_policy::reference_internal)
        .def("get_waypoints", &Simulation::get_waypoints, py::return_value_policy::reference_internal)
        .def("add_airway_node", &Simulation::add_airway_node)
        .def("add_airway_edge", &Simulation::add_airway_edge)
        .def("clear_airways", &Simulation::clear_airways);
        //.def("is_quit", &Simulation::is_quit);

    m.def("get_simulation_instance", []() -> Simulation& {
        return Simulation::get_instance();  // Return a reference to avoid re-initializing
        }, py::return_value_policy::reference);// Ensure Python receives the same reference
}
