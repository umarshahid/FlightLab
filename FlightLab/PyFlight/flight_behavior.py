import flight_lab
import math

# Global variable to store the simulation object
simulation = None

def set_simulation(sim):
    global simulation
    simulation = sim
    print("Simulation object set in Python (flight_behavior)!")

def call_once():
    print("call_once() is called (simple navigation)...")

    if simulation is None:
        print("Simulation is not set!")
        return

    aircrafts = simulation.get_aircrafts()
    waypoints = simulation.get_waypoints()

    red_waypoints = [wp for wp in waypoints if wp.get_force() == "Red"]
    blue_waypoints = [wp for wp in waypoints if wp.get_force() == "Blue"]

    for aircraft in aircrafts:
        aircraft_lat, aircraft_lon = aircraft.get_position()
        target_waypoint = None

        if aircraft.get_force() == "Red":
            if red_waypoints:
                target_waypoint = red_waypoints.pop(0)
        elif aircraft.get_force() == "Blue":
            if blue_waypoints:
                target_waypoint = blue_waypoints.pop(0)

        if target_waypoint:
            target_lat, target_lon = target_waypoint.get_position()
            aircraft.move_to(target_lat, target_lon)
            aircraft_name = aircraft.get_name()
            print(f"Moving {aircraft_name} -> ({target_lat}, {target_lon})")
        else:
            print(f"No waypoint for {aircraft.get_name()}")

    return {"status": "success", "data": [1, 2, 3]}

def build_airways():
    if simulation is None:
        print("Simulation is not set!")
        return

    print("build_airways() called...")
    simulation.clear_airways()

    n1 = simulation.add_airway_node("A1", 30.0, 70.0)
    n2 = simulation.add_airway_node("A2", 31.0, 72.0)
    n3 = simulation.add_airway_node("A3", 32.0, 74.0)
    n4 = simulation.add_airway_node("A4", 33.0, 76.0)

    simulation.add_airway_edge(n1, n2, 120)
    simulation.add_airway_edge(n2, n3, 160)
    simulation.add_airway_edge(n3, n4, 140)
    simulation.add_airway_edge(n1, n3, 220)
    simulation.add_airway_edge(n2, n4, 210)

def run_script():
    if simulation is None:
        print("Simulation is not set!")
        return
    call_once()

def plan_path(start_lat, start_lon, goal_lat, goal_lon,
              min_lat, max_lat, min_lon, max_lon,
              grid_w, grid_h):
    def clamp(value, min_v, max_v):
        return max(min_v, min(max_v, value))

    def latlon_to_cell(lat, lon):
        lat = clamp(lat, min_lat, max_lat)
        lon = clamp(lon, min_lon, max_lon)
        x = int((lon - min_lon) / (max_lon - min_lon) * (grid_w - 1))
        y = int((max_lat - lat) / (max_lat - min_lat) * (grid_h - 1))
        return (x, y)

    def cell_to_latlon(x, y):
        lon = min_lon + (x / max(1, grid_w - 1)) * (max_lon - min_lon)
        lat = max_lat - (y / max(1, grid_h - 1)) * (max_lat - min_lat)
        return (lat, lon)

    start = latlon_to_cell(start_lat, start_lon)
    goal = latlon_to_cell(goal_lat, goal_lon)

    def neighbors(cell):
        x, y = cell
        for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            nx, ny = x + dx, y + dy
            if 0 <= nx < grid_w and 0 <= ny < grid_h:
                yield (nx, ny)

    def heuristic(a, b):
        return abs(a[0] - b[0]) + abs(a[1] - b[1])

    open_set = [start]
    came_from = {}
    g_score = {start: 0}
    f_score = {start: heuristic(start, goal)}

    while open_set:
        current = min(open_set, key=lambda c: f_score.get(c, float("inf")))
        if current == goal:
            path = [current]
            while current in came_from:
                current = came_from[current]
                path.append(current)
            path.reverse()
            return [cell_to_latlon(x, y) for (x, y) in path]

        open_set.remove(current)
        for n in neighbors(current):
            tentative_g = g_score[current] + 1
            if tentative_g < g_score.get(n, float("inf")):
                came_from[n] = current
                g_score[n] = tentative_g
                f_score[n] = tentative_g + heuristic(n, goal)
                if n not in open_set:
                    open_set.append(n)

    return [(start_lat, start_lon), (goal_lat, goal_lon)]
