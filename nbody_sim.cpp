#include <iostream>
#include <vector>
#include <cmath>
#include "raylib.h"

struct Body {
    double x;
    double y;
    double vx;
    double vy;
    double ax;
    double ay;
    double mass;
};

void zeroAccelerations(std::vector<Body>& bodies) {
    for (auto& b : bodies) {
        b.ax = 0.0;
        b.ay = 0.0;
    }
}

void computeForces(std::vector<Body>& bodies, double G, double softening) {
    size_t n = bodies.size();
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            double dx = bodies[j].x - bodies[i].x;
            double dy = bodies[j].y - bodies[i].y;
            double r = std::sqrt(dx*dx + dy*dy + softening*softening);
            double force = G * bodies[i].mass * bodies[j].mass / (r * r);
            double fx = force * dx / r;
            double fy = force * dy / r;
            bodies[i].ax += fx / bodies[i].mass;
            bodies[i].ay += fy / bodies[i].mass;
            bodies[j].ax -= fx / bodies[j].mass;
            bodies[j].ay -= fy / bodies[j].mass;
        }
    }
}

void integrateEuler(std::vector<Body>& bodies, double dt) {
    for (auto& b : bodies) {
        b.vx += b.ax * dt;
        b.vy += b.ay * dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
    }
}

double computeEnergy(const std::vector<Body>& bodies, double G, double softening) {
    double ke = 0.0;
    double pe = 0.0;
    size_t n = bodies.size();
    for (size_t i = 0; i < n; i++) {
        double v2 = bodies[i].vx * bodies[i].vx + bodies[i].vy * bodies[i].vy;
        ke += 0.5 * bodies[i].mass * v2;
        for (size_t j = i + 1; j < n; j++) {
            double dx = bodies[j].x - bodies[i].x;
            double dy = bodies[j].y - bodies[i].y;
            double r = std::sqrt(dx*dx + dy*dy + softening*softening);
            pe -= G * bodies[i].mass * bodies[j].mass / r;
        }
    }
    return ke + pe;
}

std::vector<Body> makeSolarLikeSystem() {
    return {
        {400.0, 300.0, 0.0, 0.0, 0.0, 0.0, 5000.0},
        {400.0, 150.0, 120.0, 0.0, 0.0, 0.0, 5.0},
        {400.0, 500.0, -90.0, 0.0, 0.0, 0.0, 8.0}
    };
}

int runHeadless() {
    std::cout << "Display unavailable. Running headless n-body simulation..." << std::endl;
    double G = 50.0;
    double softening = 2.0;
    double dt = 0.01;
    std::vector<Body> bodies = makeSolarLikeSystem();
    int totalSteps = 20000;
    for (int step = 0; step < totalSteps; step++) {
        zeroAccelerations(bodies);
        computeForces(bodies, G, softening);
        integrateEuler(bodies, dt);
        if ((step + 1) % 1000 == 0) {
            double energy = computeEnergy(bodies, G, softening);
            std::cout << "Step " << (step + 1) << " energy=" << energy << std::endl;
        }
    }
    std::cout << "Headless run complete." << std::endl;
    for (size_t i = 0; i < bodies.size(); i++) {
        const auto& b = bodies[i];
        std::cout << "Body " << i << ": x=" << b.x << " y=" << b.y << " vx=" << b.vx << " vy=" << b.vy << std::endl;
    }
    return 0;
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    const double G = 50.0;
    const double softening = 2.0;
    const double dt = 0.01;

    const char* display = getenv("DISPLAY");
    if (display == nullptr || display[0] == '\0') {
        return runHeadless();
    }

    InitWindow(screenWidth, screenHeight, "N-Body Gravity");
    if (!IsWindowReady()) {
        return runHeadless();
    }

    SetTargetFPS(60);
    std::vector<Body> bodies = makeSolarLikeSystem();

    while (!WindowShouldClose()) {
        zeroAccelerations(bodies);
        computeForces(bodies, G, softening);
        integrateEuler(bodies, dt);

        BeginDrawing();
        ClearBackground(BLACK);

        for (auto& b : bodies) {
            DrawCircle((int)b.x, (int)b.y, 5, WHITE);
        }

        DrawText("Press R to reset", 10, 10, 14, GREEN);
        EndDrawing();

        if (IsKeyPressed(KEY_R)) {
            bodies = makeSolarLikeSystem();
        }
    }

    CloseWindow();
    return 0;
}
