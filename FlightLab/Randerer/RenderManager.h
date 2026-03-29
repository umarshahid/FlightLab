#pragma once
#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>
#include <vector>
#include "Button.h"
#include "../Engine/Simulation.h"
#include "../Utils/utils.cpp"
#include "../Objects/Aircraft.h"
#include "../Utils/FileLoader.cpp"
#include "string"
#include <SDL2/SDL_render.h>

#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include <iostream>


class RenderManager
{
	RenderManager();
	static std::unique_ptr<RenderManager> instance; // Use unique_ptr for automatic memory management

	static SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
	void applyColorMod(SDL_Texture* texture, std::string force) const;
	void drawDigit(int digit, int x, int y, int scale, SDL_Color color) const;
	void drawNumber(int value, int x, int y, int scale, SDL_Color color) const;
	void drawChar(char c, int x, int y, int scale, SDL_Color color) const;
	void drawText(const std::string& text, int x, int y, int scale, SDL_Color color) const;

	std::string iconPathAircraft;
	std::string iconPathWaypoint;
	std::string iconPathMissile;


	SDL_Window* window;
	SDL_Renderer* renderer;
	std::vector<Button> buttons;
	bool quit;
    float angle = 0.0f;
    float zoom = 1.0f;
    bool showGrid = true;
    bool showMap = true;
    bool showAirways = true;

public:
	RenderManager(const RenderManager&) = delete;
	RenderManager& operator=(const RenderManager&) = delete;
	static RenderManager& get_instance(); // Returns a reference to the singleton


	~RenderManager();
	void run();
	void handleMouseEvent(const SDL_Event& e);
	void handleMouseClick(int x, int y);
	void handleMouseWheel(SDL_Event& e);
	void render_aircraft_preview(const std::string& force, int x, int y);
	void drawAircraft(Aircraft* aircraft) const;
	void drawWaypoint(Waypoint* waypoint) const;
	void drawRadarCone(Radar* radar, float centerLat, float centerLon, float heading, std::string force) const;
	void drawMissile(Missile* aircraft) const;
	void drawPaths() const;
	void drawAirways() const;
    void drawGrid(const CoordinateSystem& coord) const;
	SDL_Surface* ResizeSurface(SDL_Surface* source, int newWidth, int newHeight);
	void applyLineColor(std::string force) const;
	void lockLine(Vector3* target, Vector3* self, std::string force);
	void RenderPolygon(SDL_Renderer* renderer, OGRPolygon* polygon, double minX, double minY, double maxX, double maxY, int screenWidth, int screenHeight);
	void RenderLine(SDL_Renderer* renderer, OGRLineString* line, double minX, double minY, double maxX, double maxY, int screenWidth, int screenHeight);
	void RenderPoint(SDL_Renderer* renderer, OGRPoint* point, double minX, double minY, double maxX, double maxY, int screenWidth, int screenHeight);
	void RenderShapefile(SDL_Renderer* renderer, const char* shapefilePath, int renderWidth, int renderHeight);
	void onWindowResized(int newWidth, int newHeight);
	SDL_Point ClampToScreen(int x, int y, int screenWidth, int screenHeight);
};

