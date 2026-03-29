import flight_lab
import math

# Global variable to store the simulation object
simulation = None
airway_nodes = {}
airway_edges = []

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
    airway_nodes.clear()
    airway_edges.clear()

    # Dense airway mesh grid over Pakistan-ish bounds for testing
    min_lat, max_lat = 24.0, 37.0
    min_lon, max_lon = 61.0, 76.0
    rows = 8
    cols = 10

    lat_step = (max_lat - min_lat) / (rows - 1)
    lon_step = (max_lon - min_lon) / (cols - 1)

    node_ids = []
    for r in range(rows):
        row_ids = []
        for c in range(cols):
            lat = min_lat + r * lat_step
            lon = min_lon + c * lon_step
            nid = simulation.add_airway_node(f"N{r}_{c}", lat, lon)
            airway_nodes[nid] = (lat, lon)
            row_ids.append(nid)
        node_ids.append(row_ids)

    # Connect grid with 4-neighbor + diagonals
    def add_edge(u, v):
        a = airway_nodes[u]
        b = airway_nodes[v]
        cost = int(round(math.hypot(a[0] - b[0], a[1] - b[1]) * 100))
        simulation.add_airway_edge(u, v, cost)
        airway_edges.append((u, v, cost))

    for r in range(rows):
        for c in range(cols):
            u = node_ids[r][c]
            if c + 1 < cols:
                add_edge(u, node_ids[r][c + 1])
            if r + 1 < rows:
                add_edge(u, node_ids[r + 1][c])
            if r + 1 < rows and c + 1 < cols:
                add_edge(u, node_ids[r + 1][c + 1])
            if r + 1 < rows and c - 1 >= 0:
                add_edge(u, node_ids[r + 1][c - 1])

def run_script():
    if simulation is None:
        print("Simulation is not set!")
        return
    call_once()

def plan_path(start_lat, start_lon, goal_lat, goal_lon,
              min_lat, max_lat, min_lon, max_lon,
              grid_w, grid_h):
    if airway_nodes and airway_edges:
        def dist(a, b):
            return math.hypot(a[0] - b[0], a[1] - b[1])

        # pick nearest airway nodes to start/end
        start_node = min(airway_nodes.items(), key=lambda kv: dist(kv[1], (start_lat, start_lon)))[0]
        end_node = min(airway_nodes.items(), key=lambda kv: dist(kv[1], (goal_lat, goal_lon)))[0]

        # Dijkstra on airway graph
        graph = {}
        for u, v, w in airway_edges:
            graph.setdefault(u, []).append((v, w))
            graph.setdefault(v, []).append((u, w))

        import heapq
        pq = [(0, start_node)]
        dist_map = {start_node: 0}
        prev = {}

        while pq:
            d, u = heapq.heappop(pq)
            if u == end_node:
                break
            if d != dist_map.get(u, 0):
                continue
            for v, w in graph.get(u, []):
                nd = d + w
                if nd < dist_map.get(v, float("inf")):
                    dist_map[v] = nd
                    prev[v] = u
                    heapq.heappush(pq, (nd, v))

        # reconstruct
        path_nodes = []
        cur = end_node
        while cur in prev:
            path_nodes.append(cur)
            cur = prev[cur]
        path_nodes.append(cur)
        path_nodes.reverse()

        path = [(start_lat, start_lon)]
        for nid in path_nodes:
            path.append(airway_nodes[nid])
        path.append((goal_lat, goal_lon))
        return path

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
