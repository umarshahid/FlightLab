#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "INIReader.h"
#include <functional>
#include <iostream>
#include "../Utils/enums.h"
#include <string>
#include "../Utils/FileLoader.cpp"
//
//class Button {
//    SDL_Rect rect;                      // Button rectangle (position and size)
//    std::string force;                  // Button color
//    SimulationObjectType buttonType;    // Type of the button (Aircraft, Waypoint, etc.)
//    std::string iconPath;               // Path to the button icon
//
//public:
//    std::function<void()> onClick;      // Callback for button click
//    Button(const SDL_Rect& rect, std::string force, SimulationObjectType buttonType, std::function<void()> onClick)
//        : rect(rect), force(std::move(force)), buttonType(buttonType), onClick(std::move(onClick)) {
//        iconPath = FileLoader::getButtonTexture(buttonType);
//    }
//
//    void render(SDL_Renderer* renderer) const {
//        SDL_Texture* buttonTexture = loadTexture(renderer, iconPath);
//        if (!buttonTexture) return;
//
//        // Calculate the render rectangle for the image
//        SDL_Rect renderQuad = {
//            rect.x + (rect.w - 32) / 2,  // Center horizontally
//            rect.y + (rect.h - 32) / 2,  // Center vertically
//            32, 32                       // Fixed size (adjust if needed)
//        };
//
//        // Apply color modulation
//        applyColorMod(buttonTexture);
//
//        // Render the texture
//        SDL_RenderCopy(renderer, buttonTexture, nullptr, &renderQuad);
//
//        // Draw button border
//        //SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White border
//        SDL_RenderDrawRect(renderer, &rect);
//
//        // Clean up texture
//        SDL_DestroyTexture(buttonTexture);
//    }
//
//    bool isClicked(int x, int y) const {
//        return x >= rect.x && x <= rect.x + rect.w &&
//            y >= rect.y && y <= rect.y + rect.h;
//    }
//
//private:
//    static SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
//        SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
//        if (!texture) {
//            std::cerr << "Error loading texture (" << path << "): " << SDL_GetError() << "\n";
//        }
//        return texture;
//    }
//
//    void applyColorMod(SDL_Texture* texture) const {
//        if (force == "Blue") {
//            SDL_SetTextureColorMod(texture, 30, 144, 255); // Blue
//        }
//        else if (force == "Red") {
//            SDL_SetTextureColorMod(texture, 220, 20, 60);  // Red
//        }
//        else if (force == "Green") {
//            SDL_SetTextureColorMod(texture, 80, 200, 120); // Green
//        }
//    }
//
//};


class Button {
    SDL_Rect rect;                      // Button rectangle (position and size)
    std::string force;                  // Button color
    SimulationObjectType buttonType;    // Type of the button (Aircraft, Waypoint, etc.)
    std::string iconPath;               // Path to the button icon
    std::string label;
    std::string tooltip;

public:
    std::function<void()> onClick;      // Callback for button click

    Button(const SDL_Rect& rect, std::string force, SimulationObjectType buttonType,
        std::string label, std::string tooltip, std::function<void()> onClick)
        : rect(rect), force(std::move(force)), buttonType(buttonType),
        label(std::move(label)), tooltip(std::move(tooltip)), onClick(std::move(onClick)) {
        iconPath = FileLoader::getButtonTexture(buttonType);
    }

    void render(SDL_Renderer* renderer) const {
        SDL_Texture* buttonTexture = loadTexture(renderer, iconPath);
        if (!buttonTexture) return;

        SDL_Rect renderQuad = {
            rect.x + (rect.w - 32) / 2,
            rect.y + (rect.h - 32) / 2,
            32, 32
        };

        applyColorMod(buttonTexture);
        SDL_RenderCopy(renderer, buttonTexture, nullptr, &renderQuad);
        //SDL_RenderDrawRect(renderer, &rect);
        SDL_DestroyTexture(buttonTexture);
    }

    bool isClicked(int x, int y) const {
        return x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h;
    }

    // Getter for rect
    const SDL_Rect& getRect() const { return rect; }
    const std::string& getLabel() const { return label; }
    const std::string& getTooltip() const { return tooltip; }
    SimulationObjectType getType() const { return buttonType; }

    // Setter for rect
    void setRect(const SDL_Rect& newRect) { rect = newRect; }

private:
    static SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
        SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
        if (!texture) {
            std::cerr << "Error loading texture (" << path << "): " << SDL_GetError() << "\n";
        }
        return texture;
    }

    void applyColorMod(SDL_Texture* texture) const {
        if (force == "Blue") {
            SDL_SetTextureColorMod(texture, 30, 144, 255);
        }
        else if (force == "Red") {
            SDL_SetTextureColorMod(texture, 220, 20, 60);
        }
        else if (force == "Green") {
            SDL_SetTextureColorMod(texture, 80, 200, 120);
        }
    }
};
