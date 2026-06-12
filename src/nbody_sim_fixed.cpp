#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "raylib.h"

struct Body {
    Vector2 pos;
    Vector2 vel;
    Vector2 acc;
    double mass;
    float radius;
    Color color;
    std::string name;
    std::vector<Vector2> trail;
};

static const int SCREEN_WIDTH = 1000;
static const int SCREEN_HEIGHT = 800;
static const double G = 0.4; // Scaled gravitational constant for simulation units
static const double SOFTENING = 2.0;
static const double TIME_SCALE = 40.0;
static const int TRAIL_LENGTH = 180;

std::vector<Body> MakeSolarSystem() {
    const Vector2 center = {(float)SCREEN_WIDTH / 2.0f, (float)SCREEN_HEIGHT / 2.0f};

    return {
        { center, {0.0f, 0.0f}, {0.0f, 0.0f}, 12000.0, 22.0f, YELLOW, "Sun", {} },
        { {center.x +  90.0f, center.y}, {0.0f,  6.8f}, {0.0f, 0.0f},  10.0,  6.0f, LIGHTGRAY, "Mercury", {} },
        { {center.x + 135.0f, center.y}, {0.0f,  5.8f}, {0.0f, 0.0f},  24.0,  7.0f, ORANGE,   "Venus",   {} },
        { {center.x + 185.0f, center.y}, {0.0f,  5.0f}, {0.0f, 0.0f},  30.0,  8.2f, BLUE,     "Earth",   {} },
        { {center.x + 245.0f, center.y}, {0.0f,  4.3f}, {0.0f, 0.0f},  18.0,  6.8f, RED,      "Mars",    {} },
        { {center.x + 320.0f, center.y}, {0.0f,  3.5f}, {0.0f, 0.0f},  40.0, 12.0f, PURPLE,   "Jupiter", {} }
    };
}

void ResetTrails(std::vector<Body>& bodies) {
    for (auto& body : bodies) {
        body.trail.clear();
    }
}

// Ceres semi-major axis is hardcoded. An earlier version fetched it from
// NASA's SBDB API (https://ssd-api.jpl.nasa.gov/sbdb.api) with libcurl.
bool AddCeres(std::vector<Body>& bodies, std::string& status) {
    if (bodies.empty()) {
        status = "No solar system loaded.";
        return false;
    }

    const double semiMajorAxisAU = 2.77;

    const Vector2 center = {(float)SCREEN_WIDTH / 2.0f, (float)SCREEN_HEIGHT / 2.0f};
    const float scaleAu = 85.0f;
    const float distance = (float)(semiMajorAxisAU * scaleAu);
    const double orbitalSpeed = std::sqrt(G * bodies[0].mass / distance);

    Body ceres;
    ceres.pos = {center.x + distance, center.y};
    ceres.vel = {0.0f, (float)orbitalSpeed};
    ceres.acc = {0.0f, 0.0f};
    ceres.mass = 2.5;
    ceres.radius = 5.0f;
    ceres.color = GREEN;
    ceres.name = "Ceres";
    ceres.trail.clear();

    bodies.push_back(ceres);
    status = "Added Ceres (a=" + std::to_string(semiMajorAxisAU) + " AU)";
    return true;
}

void ComputeGravity(std::vector<Body>& bodies) {
    const size_t n = bodies.size();
    for (size_t i = 0; i < n; ++i) {
        bodies[i].acc = {0.0f, 0.0f};
    }

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            double dx = bodies[j].pos.x - bodies[i].pos.x;
            double dy = bodies[j].pos.y - bodies[i].pos.y;
            double dist2 = dx * dx + dy * dy + SOFTENING * SOFTENING;
            double invDist = 1.0 / std::sqrt(dist2);
            double force = G * invDist * invDist * invDist;

            double ax_i = force * bodies[j].mass * dx;
            double ay_i = force * bodies[j].mass * dy;
            double ax_j = -force * bodies[i].mass * dx;
            double ay_j = -force * bodies[i].mass * dy;

            bodies[i].acc.x += (float)ax_i;
            bodies[i].acc.y += (float)ay_i;
            bodies[j].acc.x += (float)ax_j;
            bodies[j].acc.y += (float)ay_j;
        }
    }
}

void IntegrateSymplectic(std::vector<Body>& bodies, double dt) {
    const float halfDt = (float)(dt * 0.5);

    // First half-step for velocity
    for (auto& body : bodies) {
        body.vel.x += body.acc.x * halfDt;
        body.vel.y += body.acc.y * halfDt;
    }

    // Full-step for position
    for (auto& body : bodies) {
        body.pos.x += body.vel.x * (float)dt;
        body.pos.y += body.vel.y * (float)dt;
    }

    // Recompute accelerations at new positions
    ComputeGravity(bodies);

    // Second half-step for velocity and trail update
    for (auto& body : bodies) {
        body.vel.x += body.acc.x * halfDt;
        body.vel.y += body.acc.y * halfDt;
        body.trail.push_back(body.pos);
        if ((int)body.trail.size() > TRAIL_LENGTH) {
            body.trail.erase(body.trail.begin());
        }
    }
}

void DrawSystem(const std::vector<Body>& bodies) {
    for (const auto& body : bodies) {
        if (body.trail.size() > 1) {
            DrawLineStrip(body.trail.data(), (int)body.trail.size(), Fade(body.color, 0.35f));
        }
        DrawCircleV(body.pos, body.radius, body.color);
        DrawText(body.name.c_str(), (int)body.pos.x + (int)body.radius + 4, (int)body.pos.y - 8, 12, WHITE);
    }
}

int runHeadless() {
    std::cout << "Display unavailable. Running headless solar-system simulation..." << std::endl;
    std::vector<Body> bodies = MakeSolarSystem();
    const double dt = 0.01;
    const int steps = 10000;
    for (int step = 0; step < steps; ++step) {
        ComputeGravity(bodies);
        IntegrateSymplectic(bodies, dt);
        if ((step + 1) % 2000 == 0) {
            std::cout << "Step " << (step + 1) << " complete." << std::endl;
        }
    }
    std::cout << "Headless run complete." << std::endl;
    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& b = bodies[i];
        std::cout << b.name << ": pos=" << b.pos.x << "," << b.pos.y << " vel=" << b.vel.x << "," << b.vel.y << std::endl;
    }
    return 0;
}

int main() {
#if defined(__linux__)
    // No X11/Wayland display means we can't open a window
    const char* display = std::getenv("DISPLAY");
    if (display == nullptr || display[0] == '\0') {
        return runHeadless();
    }
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Solar System N-Body Simulation");
    if (!IsWindowReady()) {
        return runHeadless();
    }

    SetTargetFPS(60);
    std::vector<Body> bodies = MakeSolarSystem();
    std::string statusLine = "Ready. Press L to add Ceres.";

    while (!WindowShouldClose()) {
        const double dt = GetFrameTime() * TIME_SCALE;
        ComputeGravity(bodies);
        IntegrateSymplectic(bodies, dt);

        BeginDrawing();
        ClearBackground(BLACK);

        DrawSystem(bodies);

        DrawText("Solar System N-Body Simulation", 10, 10, 16, RAYWHITE);
        DrawText("Press R to reset | Press L to add Ceres", 10, 30, 12, GREEN);
        DrawText("Use Newton: F = G m1 m2 / r^2", 10, 46, 12, LIGHTGRAY);
        DrawText(TextFormat("Bodies: %zu | Time scale: %.1fx", bodies.size(), TIME_SCALE), 10, 62, 12, LIGHTGRAY);
        DrawText(statusLine.c_str(), 10, 78, 11, YELLOW);
        EndDrawing();

        if (IsKeyPressed(KEY_R)) {
            bodies = MakeSolarSystem();
            ResetTrails(bodies);
            statusLine = "Solar system reset.";
        }

        if (IsKeyPressed(KEY_L)) {
            std::string status;
            if (AddCeres(bodies, status)) {
                statusLine = status;
            } else {
                statusLine = "Could not add Ceres: " + status;
            }
        }
    }

    CloseWindow();
    return 0;
}
