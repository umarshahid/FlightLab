#include "RenderManager.h"
#include <unordered_map>
#include <cmath>
#include <array>
#include <cctype>
#include <sstream>
#include <iomanip>

// Initialize the static instance to nullptr
std::once_flag flag1;
std::unique_ptr<RenderManager> RenderManager::instance = nullptr;

RenderManager& RenderManager::get_instance() {
    std::call_once(flag1, []() {
        instance = std::unique_ptr<RenderManager>(new RenderManager());
    });
    return *instance;
}

RenderManager::RenderManager() : window(nullptr), renderer(nullptr), quit(false) {

    iconPathAircraft = FileLoader::getSimulationObjectTexture(SimulationObjectType::Aircraft);
    iconPathWaypoint = FileLoader::getSimulationObjectTexture(SimulationObjectType::Waypoint);
    iconPathMissile = FileLoader::getSimulationObjectTexture(SimulationObjectType::Missile); 

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(std::string("SDL_Init Error: ") + SDL_GetError());
    }

    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) != 0) {
        throw std::runtime_error(std::string("SDL_GetCurrentDisplayMode Error: ") + SDL_GetError());
    }

    int screenWidth = displayMode.w;
    int screenHeight = displayMode.h;

    // You can set the window size to a percentage of the screen size
    int windowWidth = static_cast<int>(screenWidth * 0.8);  // 80% of screen width
    int windowHeight = static_cast<int>(screenHeight * 0.8); // 80% of screen height

    window = SDL_CreateWindow(
        "Aircraft Simulation",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    std::cout <<"Window Width: "<< windowWidth << ", Window Height: " << windowHeight << "\n";

    if (!window) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateWindow Error: ") + SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateRenderer Error: ") + SDL_GetError());
    }


    int sidebarWidth = 260;
    int mapWidth = windowWidth - sidebarWidth;
	Simulation::get_instance().setCoordinateSystem(-90.0f, 90.0f, -180.0f, 180.0f, mapWidth, windowHeight);
    Simulation::get_instance().setZoom(zoom);

    buttons.push_back({ {1030, 10, 36, 36}, "Red", SimulationObjectType::Aircraft, "Red Aircraft", "Add a red aircraft",
        "assets/icons/button/aircraft.png", [this]() {
        Simulation::get_instance().onButtonclick("red");
    } });
    buttons.push_back({ {1030, 50, 36, 36}, "Blue", SimulationObjectType::Aircraft, "Blue Aircraft", "Add a blue aircraft",
        "assets/icons/button/aircraft.png", [this]() {
        Simulation::get_instance().onButtonclick("blue");
    } });

    buttons.push_back({ {1030, 95, 36, 36}, "Red", SimulationObjectType::Waypoint, "Red Waypoint", "Place a red waypoint",
        "assets/icons/button/waypoint.png", [this]() {
        Simulation::get_instance().onButtonclick("red-waypoint");
    } });
    buttons.push_back({ {1030, 135, 36, 36}, "Blue", SimulationObjectType::Waypoint, "Blue Waypoint", "Place a blue waypoint",
        "assets/icons/button/waypoint.png", [this]() {
        Simulation::get_instance().onButtonclick("blue-waypoint");
    } });

    buttons.push_back({ {1030, 190, 36, 36}, "Green", SimulationObjectType::Path, "Path Plan", "Select aircraft then destination",
        "assets/icons/button/plan.png", [this]() {
        Simulation::get_instance().setDeployMode(SimulationObjectType::Path);
    } });

    buttons.push_back({ {1030, 230, 36, 36}, "Green", SimulationObjectType::Unknown, "Airways", "Show/Hide airway routes",
        "assets/icons/button/arrow.png", [this]() {
        showAirways = !showAirways;
        if (showAirways) {
            Simulation::get_instance().build_airways_from_python();
        }
    } });

    buttons.push_back({ {1030, 275, 36, 36}, "Green", SimulationObjectType::Unknown, "Toggle Grid", "Show/Hide lat/lon grid",
        "assets/icons/circle.png", [this]() {
        showGrid = !showGrid;
    } });

    buttons.push_back({ {1030, 315, 36, 36}, "Green", SimulationObjectType::Unknown, "Toggle Map", "Show/Hide map render",
        "assets/icons/button/home.png", [this]() {
        showMap = !showMap;
    } });

    buttons.push_back({ {1030, 365, 36, 36}, "Green", SimulationObjectType::Unknown, "Run Script", "Run Python script hooks",
        "assets/icons/button/door_enter.png", [this]() { Simulation::get_instance().initialize(); } });

}

void RenderManager::onWindowResized(int newWidth, int newHeight) {
    int buttonSpacing = 8;
    int buttonWidth = 36;
    int buttonHeight = 36;
    int sidebarWidth = 260;

    int startX = newWidth - sidebarWidth + 20;
    int y = 80;
    auto setBtn = [&](size_t idx, int yPos) {
        if (idx < buttons.size()) {
            SDL_Rect newRect = { startX, yPos, buttonWidth, buttonHeight };
            buttons[idx].setRect(newRect);
        }
    };

    // SIM group
    setBtn(0, y); y += buttonHeight + buttonSpacing;
    setBtn(1, y); y += buttonHeight + buttonSpacing;
    setBtn(2, y); y += buttonHeight + buttonSpacing;
    setBtn(3, y); y += buttonHeight + buttonSpacing;

    // PATH/SCRIPT group
    y += 18;
    setBtn(4, y); y += buttonHeight + buttonSpacing;
    setBtn(5, y); y += buttonHeight + buttonSpacing;
    setBtn(8, y); y += buttonHeight + buttonSpacing;

    // TOGGLES group
    y += 18;
    setBtn(6, y); y += buttonHeight + buttonSpacing;
    setBtn(7, y); y += buttonHeight + buttonSpacing;

    int mapWidth = newWidth - sidebarWidth;
    Simulation::get_instance().setScreenSize(mapWidth, newHeight);
    Simulation::get_instance().setZoom(zoom);
}


RenderManager::~RenderManager() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}


void RenderManager::run() {
    SDL_Event e;
    Simulation::get_instance().set_running(true);
    int newWidth, newHeight;
    SDL_GetWindowSize(window, &newWidth, &newHeight);
    onWindowResized(newWidth, newHeight);

    int mouseX = 0, mouseY = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                handleMouseEvent(e);
                break;
            case SDL_MOUSEMOTION:
                SDL_GetMouseState(&mouseX, &mouseY);
                break;
            case SDL_MOUSEWHEEL:
                handleMouseWheel(e);
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    newWidth = e.window.data1;
                    newHeight = e.window.data2;
                    onWindowResized(newWidth, newHeight);
                }
                break;
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        int sidebarWidth = 260;
        int mapWidth = newWidth - sidebarWidth;
        int mapHeight = newHeight;

        if (showMap) {
            // Render shapefile within the map viewport
            std::string shapefilePathStr = FileLoader::getMapFile();
            const char* shapefilePath = shapefilePathStr.c_str();
            RenderShapefile(renderer, shapefilePath, mapWidth, mapHeight);
        }

        if (showGrid) {
            drawGrid(Simulation::get_instance().getCoordinateSystem());
        }

        // Render airways (nodes + edges)
        if (showAirways) {
            drawAirways();
        }

        // Sidebar background
        SDL_Rect sidebar = { mapWidth, 0, sidebarWidth, newHeight };
        SDL_SetRenderDrawColor(renderer, 37, 37, 38, 255);
        SDL_RenderFillRect(renderer, &sidebar);
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderDrawLine(renderer, mapWidth, 0, mapWidth, newHeight);

        SDL_Color titleColor{ 220, 220, 220, 255 };
        drawText("FLIGHTLAB", mapWidth + 20, 20, 2, titleColor);
        SDL_Color sectionColor{ 130, 130, 130, 255 };
        drawText("SIM", mapWidth + 20, 50, 1, sectionColor);

        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderDrawLine(renderer, mapWidth + 20, 70, mapWidth + sidebarWidth - 20, 70);

        drawText("PATH/SCRIPT", mapWidth + 20, 250, 1, sectionColor);
        SDL_RenderDrawLine(renderer, mapWidth + 20, 270, mapWidth + sidebarWidth - 20, 270);

        drawText("TOGGLES", mapWidth + 20, 410, 1, sectionColor);
        SDL_RenderDrawLine(renderer, mapWidth + 20, 430, mapWidth + sidebarWidth - 20, 430);

        // Render buttons on the remaining right side (full window space)
        int hoverIndex = -1;
        for (size_t i = 0; i < buttons.size(); ++i) {
            if (buttons[i].isClicked(mouseX, mouseY)) {
                hoverIndex = static_cast<int>(i);
                break;
            }
        }

        for (size_t i = 0; i < buttons.size(); ++i) {
            const auto& button = buttons[i];
            bool isActive = (Simulation::get_instance().getDeployMode() == button.getType());
            bool isHovered = static_cast<int>(i) == hoverIndex;
            SDL_Rect r = button.getRect();

            if (isActive) {
                SDL_SetRenderDrawColor(renderer, 14, 99, 156, 255);
            } else if (isHovered) {
                SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
            }
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
            SDL_RenderDrawRect(renderer, &r);

            button.render(renderer);

            SDL_Color labelColor{ 200, 200, 200, 255 };
            drawText(button.getLabel(), r.x + r.w + 12, r.y + 12, 1, labelColor);
        }

        if (hoverIndex >= 0) {
            const auto& button = buttons[hoverIndex];
            std::string tip = button.getTooltip();
            SDL_Color tipColor{ 230, 230, 230, 255 };
            int tipX = mouseX + 12;
            int tipY = mouseY + 12;

            int tipW = static_cast<int>(tip.size()) * 6 + 12;
            int tipH = 18;
            SDL_Rect tipRect{ tipX, tipY, tipW, tipH };
            SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
            SDL_RenderFillRect(renderer, &tipRect);
            SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            SDL_RenderDrawRect(renderer, &tipRect);
            drawText(tip, tipX + 6, tipY + 4, 1, tipColor);
        }

        // Render aircraft preview (mouse-following)
        if (Simulation::get_instance().getDeployMode() == SimulationObjectType::Aircraft &&
            !Simulation::get_instance().getSelectedAircraft().empty()) {
            render_aircraft_preview(Simulation::get_instance().getSelectedAircraft(), mouseX, mouseY);
        }

        // Simulation update
        drawPaths();
        Simulation::get_instance().simulation_update();

        SDL_RenderPresent(renderer);
        SDL_Delay(5);
        //SDL_Delay(16);
    }

    Simulation::get_instance().set_running(false);
}


void RenderManager::handleMouseEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        if (Simulation::get_instance().getDeployMode() == SimulationObjectType::Path) {
            Simulation::get_instance().clear_selected_aircraft();
            Simulation::get_instance().setDeployMode(SimulationObjectType::Unknown);
            std::cout << "Path mode canceled.\n";
            return;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int x = e.button.x;
        int y = e.button.y;

        std::cout << "Left click detected at (" << x << ", " << y << ")\n";

        bool buttonClicked = false;

        // Check if any button is clicked
        for (const auto& button : buttons) {
            if (button.isClicked(x, y)) {
                button.onClick();
                buttonClicked = true;  // Mark that a button was clicked
                std::cout << "Button clicked at (" << x << ", " << y << ")\n";
                break;  // Exit loop since a button was clicked
            }
        }

        int newWidth, newHeight;
        SDL_GetWindowSize(window, &newWidth, &newHeight);
        int sidebarWidth = 260;
        int mapWidth = newWidth - sidebarWidth;
        if (!buttonClicked && x >= mapWidth) {
            return;
        }

        // If no button is clicked, check for deploying aircraft
        if (!buttonClicked) {
            std::cout << "Screen clicked at (" << x << ", " << y << ")\n";
            std::cout << "Deploy mode: " << simulationObjectTypeToString(Simulation::get_instance().getDeployMode())
                << ", Selected aircraft: " << Simulation::get_instance().getSelectedAircraft() << "\n";

            if (Simulation::get_instance().getDeployMode() == SimulationObjectType::Aircraft && !Simulation::get_instance().getSelectedAircraft().empty()) {
                // Deploy the selected aircraft at the mouse location
                std::pair<float, float> lat_lon = Simulation::get_instance().getCoordinateSystem().to_lat_lon(x, y);
                float lat = lat_lon.first;
                float lon = lat_lon.second;
                Simulation::get_instance().render_single_aircraft(Simulation::get_instance().getSelectedAircraft(), lat, lon, angle);
            }

            if (Simulation::get_instance().getDeployMode() == SimulationObjectType::Waypoint && !Simulation::get_instance().getSelectedWaypoint().empty()) {
                // Deploy the selected aircraft at the mouse location
                std::pair<float, float> lat_lon = Simulation::get_instance().getCoordinateSystem().to_lat_lon(x, y);
                float lat = lat_lon.first;
                float lon = lat_lon.second;
                Simulation::get_instance().render_waypoint(Simulation::get_instance().getSelectedWaypoint(), lat, lon);
            }

            if (Simulation::get_instance().getDeployMode() == SimulationObjectType::Path) {
                auto& sim = Simulation::get_instance();
                if (!sim.has_selected_aircraft()) {
                    if (sim.select_aircraft_at(x, y, 18.0f)) {
                        std::cout << "Aircraft selected for path planning.\n";
                    } else {
                        std::cout << "No aircraft found at click position.\n";
                    }
                } else {
                    std::pair<float, float> lat_lon = sim.getCoordinateSystem().to_lat_lon(x, y);
                    float lat = lat_lon.first;
                    float lon = lat_lon.second;
                    if (sim.plan_path_for_selected(static_cast<float>(lat), static_cast<float>(lon))) {
                        std::cout << "Path planned.\n";
                    } else {
                        std::cout << "Path planning failed.\n";
                    }
                    sim.clear_selected_aircraft();
                }
            }
        }

    }
    else {
        // Always reset deploy mode after handling the click
        if (Simulation::get_instance().getDeployMode() != SimulationObjectType::Unknown) {
            std::cout << "Deploy mode deactivated.\n";
            Simulation::get_instance().setDeployMode(SimulationObjectType::Unknown);
        }
    }
}


void RenderManager::handleMouseClick(int x, int y) {
    bool buttonClicked = false;

    // Check if any button is clicked
    for (const auto& button : buttons) {
        if (button.isClicked(x, y)) {
            button.onClick();
            buttonClicked = true;  // Mark that a button was clicked
            std::cout << "Button clicked at (" << x << ", " << y << ")\n";
            break;  // Exit loop since a button was clicked
        }
    }

    // If no button is clicked, check for deploying aircraft
    if (!buttonClicked) {
        std::cout << "Screen clicked at (" << x << ", " << y << ")\n";
        std::cout << "Deploy mode: " << simulationObjectTypeToString(Simulation::get_instance().getDeployMode())
            << ", Selected aircraft: " << Simulation::get_instance().getSelectedAircraft() << "\n";

        if (Simulation::get_instance().getDeployMode() == SimulationObjectType::Aircraft && !Simulation::get_instance().getSelectedAircraft().empty()) {
            // Deploy the selected aircraft at the mouse location
            std::pair<float, float> lat_lon = Simulation::get_instance().getCoordinateSystem().to_lat_lon(x, y);
            float lat = lat_lon.first;
            float lon = lat_lon.second;
            Simulation::get_instance().render_single_aircraft(Simulation::get_instance().getSelectedAircraft(), lat, lon, angle);
            //deployMode = false;  // Turn off deploy mode after deploying
        }
    }
}

void RenderManager::handleMouseWheel(SDL_Event& e) {
    if (e.type == SDL_MOUSEWHEEL) {
        angle += e.wheel.y * 5.0f; // Adjust rotation by 5 degrees per scroll step
        float zoom_delta = e.wheel.y * 0.1f;
        zoom += zoom_delta;
        if (zoom < 0.3f) zoom = 0.3f;
        if (zoom > 4.0f) zoom = 4.0f;
        Simulation::get_instance().setZoom(zoom);
    }
}

void RenderManager::render_aircraft_preview(const std::string& force, int x, int y) {

    SDL_Texture* aircraftTexture = loadTexture(renderer, iconPathAircraft);
    if (!aircraftTexture) return;

    int texture_width = 32, texture_height = 32;
    SDL_QueryTexture(aircraftTexture, nullptr, nullptr, &texture_width, &texture_height);

    SDL_Rect renderQuad = { x - texture_width / 2, y - texture_height / 2, texture_width, texture_height };

    // Set aircraft color (based on the force)
    applyColorMod(aircraftTexture, force);

    SDL_RenderCopyEx(renderer, aircraftTexture, nullptr, &renderQuad, angle, nullptr, SDL_FLIP_NONE);
    SDL_DestroyTexture(aircraftTexture);

}

// ******************* Aircraft Drawing *******************
void RenderManager::drawAircraft(Aircraft* aircraft) const {

    int screen_x = aircraft->get_position3().x;
    int screen_y = aircraft->get_position3().y;

    auto latlon = aircraft->get_position();
    drawRadarCone(aircraft->getRadar(), latlon.first, latlon.second, aircraft->get_heading(), aircraft->get_force());

    SDL_Texture* aircraftTexture = loadTexture(renderer, iconPathAircraft);
    if (!aircraftTexture) return;

    // Calculate the render rectangle for the image
    // Get the dimensions of the aircraft texture
    int texture_width = 32, texture_height = 32;
    SDL_QueryTexture(aircraftTexture, nullptr, nullptr, &texture_width, &texture_height);

    // Calculate the position for the aircraft image (centered around the coordinates)
    //SDL_Rect renderQuad = { screen_x - texture_width / 2, screen_y - texture_height / 2, texture_width, texture_height };
    SDL_FRect renderQuad = { screen_x - texture_width / 2, screen_y - texture_height / 2, texture_width, texture_height };

    // Set aircraft color (based on the force)
    applyColorMod(aircraftTexture, aircraft->get_force());

    // Rotate the aircraft image based on its heading (rotate around its center)
    SDL_RenderCopyExF(renderer, aircraftTexture, nullptr, &renderQuad, aircraft->get_heading(), nullptr, SDL_FLIP_NONE);
    //SDL_RenderCopyEx(renderer, aircraftTexture, nullptr, &renderQuad, aircraft->get_heading(), nullptr, SDL_FLIP_NONE);

    // Free the texture after rendering
    SDL_DestroyTexture(aircraftTexture);

    if (aircraft->get_isMoving()) {

        int target_x = aircraft->get_target_position_xy().first;
        int target_y = aircraft->get_target_position_xy().second;

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green for target line
        SDL_RenderDrawLine(renderer, screen_x, screen_y, target_x, target_y);
    }

    auto* selected = Simulation::get_instance().get_selected_aircraft();
    if (selected && selected->get_id() == aircraft->get_id()) {
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Gold highlight
        SDL_Rect outline = { screen_x - 20, screen_y - 20, 40, 40 };
        SDL_RenderDrawRect(renderer, &outline);
    }

}

void RenderManager::drawPaths() const {
    auto& sim = Simulation::get_instance();
    CoordinateSystem coord = sim.getCoordinateSystem();

    for (const auto& aircraft : sim.get_aircrafts()) {
        if (!aircraft) continue;
        const auto& path = aircraft->get_path();
        if (path.empty()) continue;

        applyLineColor("Green");

        auto start = aircraft->get_position();
        auto start_screen = coord.to_screen_coordinates(start.first, start.second);
        int prev_x = start_screen.first;
        int prev_y = start_screen.second;

        for (const auto& p : path) {
            auto screen = coord.to_screen_coordinates(p.first, p.second);
            SDL_RenderDrawLine(renderer, prev_x, prev_y, screen.first, screen.second);
            prev_x = screen.first;
            prev_y = screen.second;
        }
    }
}

void RenderManager::drawAirways() const {
    auto& sim = Simulation::get_instance();
    CoordinateSystem coord = sim.getCoordinateSystem();
    const auto& nodes = sim.get_airway_nodes();
    const auto& edges = sim.get_airway_edges();

    if (nodes.empty() && edges.empty()) return;

    std::unordered_map<int, Simulation::AirwayNode> node_map;
    for (const auto& n : nodes) {
        node_map[n.id] = n;
    }

    // Draw edges
    SDL_Color costColor{ 255, 255, 255, 255 };
    for (const auto& e : edges) {
        if (node_map.find(e.from_id) == node_map.end() || node_map.find(e.to_id) == node_map.end()) {
            continue;
        }
        const auto& a = node_map[e.from_id];
        const auto& b = node_map[e.to_id];

        auto a_screen = coord.to_screen_coordinates(a.lat, a.lon);
        auto b_screen = coord.to_screen_coordinates(b.lat, b.lon);

        applyLineColor("Green");
        SDL_RenderDrawLine(renderer, a_screen.first, a_screen.second, b_screen.first, b_screen.second);

        int mid_x = (a_screen.first + b_screen.first) / 2;
        int mid_y = (a_screen.second + b_screen.second) / 2;
        drawNumber(static_cast<int>(std::round(e.cost)), mid_x + 4, mid_y + 4, 2, costColor);
    }

    // Draw nodes
    SDL_Texture* nodeTexture = loadTexture(renderer, iconPathWaypoint);
    for (const auto& n : nodes) {
        auto screen = coord.to_screen_coordinates(n.lat, n.lon);

        if (nodeTexture) {
            int texture_width = 24, texture_height = 24;
            SDL_QueryTexture(nodeTexture, nullptr, nullptr, &texture_width, &texture_height);
            SDL_Rect renderQuad = { screen.first - texture_width / 2, screen.second - texture_height / 2, texture_width, texture_height };
            applyColorMod(nodeTexture, "Green");
            SDL_RenderCopyEx(renderer, nodeTexture, nullptr, &renderQuad, 0, nullptr, SDL_FLIP_NONE);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 200, 120, 255);
            SDL_Rect dot{ screen.first - 3, screen.second - 3, 6, 6 };
            SDL_RenderFillRect(renderer, &dot);
        }
    }
    if (nodeTexture) SDL_DestroyTexture(nodeTexture);
}

static double nice_step(double value) {
    if (value <= 0.0) return 1.0;
    double exp = std::floor(std::log10(value));
    double base = value / std::pow(10.0, exp);
    double nice;
    if (base < 1.5) nice = 1.0;
    else if (base < 3.0) nice = 2.0;
    else if (base < 7.0) nice = 5.0;
    else nice = 10.0;
    return nice * std::pow(10.0, exp);
}

void RenderManager::drawGrid(const CoordinateSystem& coord) const {
    if (!coord.has_transform()) return;

    double minX, maxX, minY, maxY;
    coord.get_map_bounds(minX, maxX, minY, maxY);
    double scale = coord.get_map_scale();
    int screenW = coord.get_screen_width();
    int screenH = coord.get_screen_height();

    double targetPx = 90.0;
    double step = nice_step(targetPx / std::max(0.0001, scale));

    int precision = 0;
    if (step < 1.0) precision = 1;
    if (step < 0.1) precision = 2;

    SDL_SetRenderDrawColor(renderer, 55, 55, 55, 255);

    double lonStart = std::floor(minX / step) * step;
    for (double lon = lonStart; lon <= maxX; lon += step) {
        auto top = coord.to_screen_coordinates(static_cast<float>(maxY), static_cast<float>(lon));
        auto bottom = coord.to_screen_coordinates(static_cast<float>(minY), static_cast<float>(lon));
        SDL_RenderDrawLine(renderer, top.first, top.second, bottom.first, bottom.second);

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << lon;
        SDL_Color labelColor{ 140, 140, 140, 255 };
        drawText(ss.str(), top.first + 4, 6, 1, labelColor);
    }

    double latStart = std::floor(minY / step) * step;
    for (double lat = latStart; lat <= maxY; lat += step) {
        auto left = coord.to_screen_coordinates(static_cast<float>(lat), static_cast<float>(minX));
        auto right = coord.to_screen_coordinates(static_cast<float>(lat), static_cast<float>(maxX));
        SDL_RenderDrawLine(renderer, left.first, left.second, right.first, right.second);

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << lat;
        SDL_Color labelColor{ 140, 140, 140, 255 };
        drawText(ss.str(), 6, left.second + 4, 1, labelColor);
    }
}

void RenderManager::drawDigit(int digit, int x, int y, int scale, SDL_Color color) const {
    static const int font[10][15] = {
        {1,1,1, 1,0,1, 1,0,1, 1,0,1, 1,1,1}, // 0
        {0,1,0, 1,1,0, 0,1,0, 0,1,0, 1,1,1}, // 1
        {1,1,1, 0,0,1, 1,1,1, 1,0,0, 1,1,1}, // 2
        {1,1,1, 0,0,1, 0,1,1, 0,0,1, 1,1,1}, // 3
        {1,0,1, 1,0,1, 1,1,1, 0,0,1, 0,0,1}, // 4
        {1,1,1, 1,0,0, 1,1,1, 0,0,1, 1,1,1}, // 5
        {1,1,1, 1,0,0, 1,1,1, 1,0,1, 1,1,1}, // 6
        {1,1,1, 0,0,1, 0,1,0, 1,0,0, 1,0,0}, // 7
        {1,1,1, 1,0,1, 1,1,1, 1,0,1, 1,1,1}, // 8
        {1,1,1, 1,0,1, 1,1,1, 0,0,1, 1,1,1}  // 9
    };

    if (digit < 0 || digit > 9) return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (font[digit][row * 3 + col] == 1) {
                SDL_Rect pixel{ x + col * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

void RenderManager::drawNumber(int value, int x, int y, int scale, SDL_Color color) const {
    if (value < 0) value = 0;
    std::string text = std::to_string(value);
    int offset = 0;
    for (char c : text) {
        int digit = c - '0';
        drawDigit(digit, x + offset, y, scale, color);
        offset += (3 * scale) + scale;
    }
}

void RenderManager::drawChar(char c, int x, int y, int scale, SDL_Color color) const {
    static const std::unordered_map<char, std::array<const char*, 7>> font = {
        {'A', {"01110","10001","10001","11111","10001","10001","10001"}},
        {'B', {"11110","10001","10001","11110","10001","10001","11110"}},
        {'C', {"01110","10001","10000","10000","10000","10001","01110"}},
        {'D', {"11110","10001","10001","10001","10001","10001","11110"}},
        {'E', {"11111","10000","10000","11110","10000","10000","11111"}},
        {'F', {"11111","10000","10000","11110","10000","10000","10000"}},
        {'G', {"01110","10001","10000","10111","10001","10001","01110"}},
        {'H', {"10001","10001","10001","11111","10001","10001","10001"}},
        {'I', {"11111","00100","00100","00100","00100","00100","11111"}},
        {'J', {"00111","00010","00010","00010","00010","10010","01100"}},
        {'K', {"10001","10010","10100","11000","10100","10010","10001"}},
        {'L', {"10000","10000","10000","10000","10000","10000","11111"}},
        {'M', {"10001","11011","10101","10101","10001","10001","10001"}},
        {'N', {"10001","11001","10101","10011","10001","10001","10001"}},
        {'O', {"01110","10001","10001","10001","10001","10001","01110"}},
        {'P', {"11110","10001","10001","11110","10000","10000","10000"}},
        {'Q', {"01110","10001","10001","10001","10101","10010","01101"}},
        {'R', {"11110","10001","10001","11110","10100","10010","10001"}},
        {'S', {"01111","10000","10000","01110","00001","00001","11110"}},
        {'T', {"11111","00100","00100","00100","00100","00100","00100"}},
        {'U', {"10001","10001","10001","10001","10001","10001","01110"}},
        {'V', {"10001","10001","10001","10001","10001","01010","00100"}},
        {'W', {"10001","10001","10001","10101","10101","10101","01010"}},
        {'X', {"10001","10001","01010","00100","01010","10001","10001"}},
        {'Y', {"10001","10001","01010","00100","00100","00100","00100"}},
        {'Z', {"11111","00001","00010","00100","01000","10000","11111"}},
        {'0', {"01110","10001","10011","10101","11001","10001","01110"}},
        {'1', {"00100","01100","00100","00100","00100","00100","01110"}},
        {'2', {"01110","10001","00001","00010","00100","01000","11111"}},
        {'3', {"11110","00001","00001","01110","00001","00001","11110"}},
        {'4', {"00010","00110","01010","10010","11111","00010","00010"}},
        {'5', {"11111","10000","10000","11110","00001","00001","11110"}},
        {'6', {"01110","10000","10000","11110","10001","10001","01110"}},
        {'7', {"11111","00001","00010","00100","01000","01000","01000"}},
        {'8', {"01110","10001","10001","01110","10001","10001","01110"}},
        {'9', {"01110","10001","10001","01111","00001","00001","01110"}},
        {'-', {"00000","00000","00000","11111","00000","00000","00000"}},
        {' ', {"00000","00000","00000","00000","00000","00000","00000"}}
    };

    char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    auto it = font.find(up);
    if (it == font.end()) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const auto& glyph = it->second;
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (glyph[row][col] == '1') {
                SDL_Rect px{ x + col * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

void RenderManager::drawText(const std::string& text, int x, int y, int scale, SDL_Color color) const {
    int offset = 0;
    for (char c : text) {
        drawChar(c, x + offset, y, scale, color);
        offset += (5 * scale) + scale;
    }
}

void RenderManager::lockLine(Vector3* target, Vector3* self, std::string force) {
    applyLineColor(force);
	SDL_RenderDrawLine(renderer, self->x, self->y, target->x, target->y);
}

// ******************* Waypoint Drawing *******************
void RenderManager::drawWaypoint(Waypoint* waypoint) const {

    // Get screen coordinates of the waypoint
    int screen_x = waypoint->get_position_xy().first;
    int screen_y = waypoint->get_position_xy().second;

    SDL_Texture* waypointTexture = loadTexture(renderer, iconPathWaypoint);
    if (!waypointTexture) return;

    int texture_width = 32, texture_height = 32;
    SDL_QueryTexture(waypointTexture, nullptr, nullptr, &texture_width, &texture_height);

    // Calculate the position for the waypoint image (centered around the coordinates)
    SDL_Rect renderQuad = { screen_x - texture_width / 2, screen_y - texture_height / 2, texture_width, texture_height };

    // Set waypoint color (based on the force)
    applyColorMod(waypointTexture, waypoint->get_force());

	// Waypoints do not rotate; passing zero as angle
    SDL_RenderCopyEx(renderer, waypointTexture, nullptr, &renderQuad, 0, nullptr, SDL_FLIP_NONE);
    SDL_DestroyTexture(waypointTexture);

}



void RenderManager::drawRadarCone(Radar* radar, float centerLat, float centerLon, float heading, std::string force) const {


    // Convert heading and angles to radians
    float headingRad = heading * M_PI / 180.0f;
    float leftEdgeRad = headingRad - (radar->getRadarAngle() * M_PI / 180.0f);
    float rightEdgeRad = headingRad + (radar->getRadarAngle() * M_PI / 180.0f);

    // Calculate the end points of the cone edges
    const CoordinateSystem coord = Simulation::get_instance().getCoordinateSystem();
    double baseScale = coord.get_map_base_scale();
    if (baseScale <= 0.0001) baseScale = 1.0;
    float geoRadius = static_cast<float>(radar->getRadarRadius() / baseScale);

    auto centerScreen = coord.to_screen_coordinates(centerLat, centerLon);
    int centerX = centerScreen.first;
    int centerY = centerScreen.second;

    float leftLat = centerLat + geoRadius * std::cos(leftEdgeRad);
    float leftLon = centerLon + geoRadius * std::sin(leftEdgeRad);
    float rightLat = centerLat + geoRadius * std::cos(rightEdgeRad);
    float rightLon = centerLon + geoRadius * std::sin(rightEdgeRad);

    auto leftScreen = coord.to_screen_coordinates(leftLat, leftLon);
    auto rightScreen = coord.to_screen_coordinates(rightLat, rightLon);
    int leftX = leftScreen.first;
    int leftY = leftScreen.second;
    int rightX = rightScreen.first;
    int rightY = rightScreen.second;

    // Set radar boundary color (yellow)
    //SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    applyLineColor(force);

    // Draw the cone's boundary edges
    SDL_RenderDrawLine(renderer, centerX, centerY, leftX, leftY); // Left edge
    SDL_RenderDrawLine(renderer, centerX, centerY, rightX, rightY); // Right edge

    // Optionally, draw the arc connecting the edges to complete the cone boundary
    const int segments = 50; // Number of segments for smoothness
    for (int i = 0; i < segments; ++i) {
        float t1 = leftEdgeRad + (i / static_cast<float>(segments)) * (rightEdgeRad - leftEdgeRad);
        float t2 = leftEdgeRad + ((i + 1) / static_cast<float>(segments)) * (rightEdgeRad - leftEdgeRad);
        float arcLat1 = centerLat + geoRadius * std::cos(t1);
        float arcLon1 = centerLon + geoRadius * std::sin(t1);
        float arcLat2 = centerLat + geoRadius * std::cos(t2);
        float arcLon2 = centerLon + geoRadius * std::sin(t2);
        auto arc1 = coord.to_screen_coordinates(arcLat1, arcLon1);
        auto arc2 = coord.to_screen_coordinates(arcLat2, arcLon2);
        int arcX1 = arc1.first;
        int arcY1 = arc1.second;
        int arcX2 = arc2.first;
        int arcY2 = arc2.second;
        SDL_RenderDrawLine(renderer, arcX1, arcY1, arcX2, arcY2);
    }
}

SDL_Surface* RenderManager::ResizeSurface(SDL_Surface* source, int newWidth, int newHeight) {
    SDL_Surface* resized = SDL_CreateRGBSurface(0, newWidth, newHeight, 32,
        source->format->Rmask, source->format->Gmask, source->format->Bmask, source->format->Amask);

    if (!resized) {
        std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_Rect srcRect = { 0, 0, source->w, source->h };
    SDL_Rect dstRect = { 0, 0, newWidth, newHeight };

    SDL_BlitScaled(source, &srcRect, resized, &dstRect);
    return resized;
}

void RenderManager::drawMissile(Missile* missile) const {

    int screen_x = missile->get_position3().x;
    int screen_y = missile->get_position3().y;

    SDL_Texture* missileTexture = loadTexture(renderer, iconPathMissile);
    if (!missileTexture) return;

    // Calculate the render rectangle for the image
    // Get the dimensions of the aircraft texture
    int texture_width = 32, texture_height = 32;
    SDL_QueryTexture(missileTexture, nullptr, nullptr, &texture_width, &texture_height);

    // Calculate the position for the aircraft image (centered around the coordinates)
    SDL_Rect renderQuad = { screen_x - texture_width / 2, screen_y - texture_height / 2, texture_width, texture_height };

    // Set aircraft color (based on the force)
    applyColorMod(missileTexture, missile->force);

    // Rotate the aircraft image based on its heading (rotate around its center)
    SDL_RenderCopyEx(renderer, missileTexture, nullptr, &renderQuad, missile->get_heading(), nullptr, SDL_FLIP_NONE);

    // Free the texture after rendering
    SDL_DestroyTexture(missileTexture);

    if (missile->active) {

        int target_x = missile->get_target_position_xy().first;
        int target_y = missile->get_target_position_xy().second;

        //SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green for target line
        applyLineColor(missile->force);
        SDL_RenderDrawLine(renderer, screen_x, screen_y, target_x, target_y);
    }

}

// common for all render manager

SDL_Texture* RenderManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
    if (!texture) {
        std::cerr << "Error loading texture (" << path << "): " << SDL_GetError() << "\n";
    }
    return texture;
}

void RenderManager::applyColorMod(SDL_Texture* texture, std::string force) const {
    if (force == "Blue") {
        SDL_SetTextureColorMod(texture, 30, 144, 255); // Blue
    }
    else if (force == "Red") {
        SDL_SetTextureColorMod(texture, 220, 20, 60);  // Red
    }
    else if (force == "Green") {
        SDL_SetTextureColorMod(texture, 80, 200, 120); // Green
    }
    else {
        SDL_SetTextureColorMod(texture, 255, 255, 255);
    }
}

void RenderManager::applyLineColor(std::string force) const {
    if (force == "Blue") {
        SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255); // Blue
    }
    else if (force == "Red") {
        SDL_SetRenderDrawColor(renderer, 220, 20, 60, 255);  // Red
    }
    else if (force == "Green") {
        SDL_SetRenderDrawColor(renderer, 80, 200, 120, 255); // Green
    }
    else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
}

void RenderManager::RenderPoint(SDL_Renderer* renderer, OGRPoint* point, double minX, double minY, double maxX, double maxY, int screenWidth, int screenHeight) {
    int screenX = (int)((point->getX() - minX) / (maxX - minX) * screenWidth);
    int screenY = (int)((maxY - point->getY()) / (maxY - minY) * screenHeight);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawPoint(renderer, screenX, screenY);
}

void RenderManager::RenderLine(SDL_Renderer* renderer, OGRLineString* line, double minX, double minY, double maxX, double maxY, int screenWidth, int screenHeight) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    for (int i = 0; i < line->getNumPoints() - 1; i++) {
        int x1 = (int)((line->getX(i) - minX) / (maxX - minX) * screenWidth);
        int y1 = (int)((maxY - line->getY(i)) / (maxY - minY) * screenHeight);
        int x2 = (int)((line->getX(i + 1) - minX) / (maxX - minX) * screenWidth);
        int y2 = (int)((maxY - line->getY(i + 1)) / (maxY - minY) * screenHeight);

        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

void RenderManager::RenderPolygon(SDL_Renderer* renderer, OGRPolygon* polygon, double minX, double minY, double maxX, double maxY, int screenWidth, int screenHeight) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

    OGRLinearRing* ring = polygon->getExteriorRing();
    for (int i = 0; i < ring->getNumPoints() - 1; i++) {
        int x1 = (int)((ring->getX(i) - minX) / (maxX - minX) * screenWidth);
        int y1 = (int)((maxY - ring->getY(i)) / (maxY - minY) * screenHeight);
        int x2 = (int)((ring->getX(i + 1) - minX) / (maxX - minX) * screenWidth);
        int y2 = (int)((maxY - ring->getY(i + 1)) / (maxY - minY) * screenHeight);

        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

void RenderManager::RenderShapefile(SDL_Renderer* renderer, const char* shapefilePath, int renderWidth, int renderHeight) {
    GDALAllRegister();

    GDALDataset* dataset = (GDALDataset*)GDALOpenEx(shapefilePath, GDAL_OF_VECTOR, NULL, NULL, NULL);
    if (!dataset) {
        std::cerr << "Failed to open shapefile!" << std::endl;
        return;
    }

    OGRLayer* layer = dataset->GetLayer(0);
    OGRFeature* feature;

    OGREnvelope envelope;
    layer->GetExtent(&envelope);

    double minX = envelope.MinX;
    double maxX = envelope.MaxX;
    double minY = envelope.MinY;
    double maxY = envelope.MaxY;

    Simulation::get_instance().setCoordinateBounds(
        static_cast<float>(minY),
        static_cast<float>(maxY),
        static_cast<float>(minX),
        static_cast<float>(maxX)
    );
    Simulation::get_instance().setMapTransform(minX, maxX, minY, maxY, renderWidth, renderHeight, zoom);

    double shapeWidth = maxX - minX;
    double shapeHeight = maxY - minY;

    double scaleX = renderWidth / shapeWidth;
    double scaleY = renderHeight / shapeHeight;
    double scale = std::min(scaleX, scaleY);
    scale *= zoom;

    double shapeMidX = (minX + maxX) / 2.0;
    double shapeMidY = (minY + maxY) / 2.0;

    double screenMidX = renderWidth / 2.0;
    double screenMidY = renderHeight / 2.0;

    auto TransformToPixel = [&](double x, double y) -> SDL_Point {
        int px = static_cast<int>(screenMidX + (x - shapeMidX) * scale);
        int py = static_cast<int>(screenMidY - (y - shapeMidY) * scale); // Y is flipped

#ifdef DEBUG
        if (px < 0 || px >= renderWidth || py < 0 || py >= renderHeight) {
            std::cerr << "Point out of bounds: (" << px << ", " << py << ")" << std::endl;
        }
#endif

        return ClampToScreen(px, py, renderWidth, renderHeight);
    };

    while ((feature = layer->GetNextFeature()) != nullptr) {
        OGRGeometry* geometry = feature->GetGeometryRef();
        if (!geometry) continue;

        OGRwkbGeometryType geomType = wkbFlatten(geometry->getGeometryType());

        if (geomType == wkbPolygon || geomType == wkbMultiPolygon) {
            auto DrawPolygon = [&](OGRPolygon* poly) {
                OGRLinearRing* ring = poly->getExteriorRing();
                int numPoints = ring->getNumPoints();

                if (numPoints < 3) return; // Ignore degenerate polygons

                std::vector<SDL_Point> points(numPoints + 1);

                for (int i = 0; i < numPoints; ++i) {
                    points[i] = TransformToPixel(ring->getX(i), ring->getY(i));
                }
                points[numPoints] = points[0];

                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderDrawLines(renderer, points.data(), numPoints + 1);
            };

            if (geomType == wkbPolygon) {
                DrawPolygon((OGRPolygon*)geometry);
            }
            else if (geomType == wkbMultiPolygon) {
                OGRMultiPolygon* multiPolygon = (OGRMultiPolygon*)geometry;
                for (int i = 0; i < multiPolygon->getNumGeometries(); ++i) {
                    OGRPolygon* subPolygon = (OGRPolygon*)multiPolygon->getGeometryRef(i);
                    DrawPolygon(subPolygon);
                }
            }
        }

        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(dataset);
}

SDL_Point RenderManager::ClampToScreen(int x, int y, int screenWidth, int screenHeight) {
    SDL_Point p;
    p.x = std::min(std::max(x, 0), screenWidth - 1);
    p.y = std::min(std::max(y, 0), screenHeight - 1);
    return p;
}
