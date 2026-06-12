#include "raylib.h"
#include <cmath>
#include <vector>
#include <string>


struct StateVector {
    double x, y, z;
    double vx, vy, vz;
};

double SolveEccentricAnomaly(double M, double e) {
    double E = M;
    for (int iter = 0; iter < 15; iter++) {
        double f  = E - e * std::sin(E) - M;
        double fp = 1.0 - e * std::cos(E);
        E -= f / fp;
    }
    return E;
}

StateVector OrbitalElementsToStateVector(
    double a,     // km
    double e,
    double i,     // rad
    double Omega, // rad
    double omega, // rad
    double M,     // rad
    double mu     // km^3/s^2
) {
    double E = SolveEccentricAnomaly(M, e);

    double cosE = std::cos(E);
    double sinE = std::sin(E);
    double sqrt1me2 = std::sqrt(1.0 - e*e);

    double nu = std::atan2(sqrt1me2 * sinE, cosE - e);
    double r  = a * (1.0 - e * cosE);

    double x_orb = r * std::cos(nu);
    double y_orb = r * std::sin(nu);

    double n = std::sqrt(mu / (a*a*a)); // mean motion
    double rdot  = (a * n / r) * e * sinE;
    double rfdot = (a * n / r) * sqrt1me2 * cosE;

    double vx_orb = rdot * std::cos(nu) - rfdot * std::sin(nu);
    double vy_orb = rdot * std::sin(nu) + rfdot * std::cos(nu);

    double cO = std::cos(Omega), sO = std::sin(Omega);
    double co = std::cos(omega), so = std::sin(omega);
    double ci = std::cos(i),     si = std::sin(i);

    double R11 =  cO*co - sO*so*ci;
    double R12 = -cO*so - sO*co*ci;
    double R13 =  sO*si;

    double R21 =  sO*co + cO*so*ci;
    double R22 = -sO*so + cO*co*ci;
    double R23 = -cO*si;

    double R31 =  so*si;
    double R32 =  co*si;
    double R33 =  ci;

    StateVector sv;
    sv.x  = R11*x_orb + R12*y_orb + R13*0.0;
    sv.y  = R21*x_orb + R22*y_orb + R23*0.0;
    sv.z  = R31*x_orb + R32*y_orb + R33*0.0;

    sv.vx = R11*vx_orb + R12*vy_orb;
    sv.vy = R21*vx_orb + R22*vy_orb;
    sv.vz = R31*vx_orb + R32*vy_orb;

    return sv;
}

// ----------------- Solar system body definition -----------------

struct Body {
    std::string name;
    Color color;
    double radiusPixels;

    // Orbital elements (heliocentric, J2000, simplified)
    double a;      // km
    double e;
    double i;      // rad
    double Omega;  // rad
    double omega;  // rad
    double M0;     // mean anomaly at epoch (rad)
    double epoch;  // seconds (arbitrary reference)
    StateVector state;

    std::vector<Vector2> trail; // world-space trail
};

int main() {
    const int screenWidth  = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Raylib Solar System (elements -> state vectors)");

    // Sun GM (mu) in km^3/s^2
    const double muSun = 1.32712440018e11;

    // Epoch reference (arbitrary, seconds)
    const double t0 = 0.0;

    // Bodies (very approximate elements)
    std::vector<Body> bodies;
    //solar system bodies: name, color, radiusPixels, a (km), e, i (rad), Omega (rad), omega (rad), M0 (rad), epoch, state
    //mercury
    bodies.push_back({
        "Mercury", ORANGE, 4.0,
        57.9e6, 0.2056,
        7.0 * DEG2RAD,
        48.331 * DEG2RAD,
        29.124 * DEG2RAD,
        174.796 * DEG2RAD,
        t0, {}
    });
    //venus
    bodies.push_back({
        "Venus", GOLD, 5.0,
        108.2e6, 0.0068,
        3.39 * DEG2RAD,
        76.680 * DEG2RAD,
        54.884 * DEG2RAD,
        50.115 * DEG2RAD,
        t0, {}
    });
    //earth
    bodies.push_back({
        "Earth", BLUE, 6.0,
        149.6e6, 0.0167,
        0.00005 * DEG2RAD,
        -11.26064 * DEG2RAD,
        114.20783 * DEG2RAD,
        357.51716 * DEG2RAD,
        t0, {}
    });
    //mars
    bodies.push_back({
        "Mars", RED, 5.0,
        227.9e6, 0.0934,
        1.85 * DEG2RAD,
        49.558 * DEG2RAD,
        286.502 * DEG2RAD,
        19.373 * DEG2RAD,
        t0, {}
    });
    //jupiter
    bodies.push_back({
        "Jupiter", BROWN, 7.0,
        778.5e6, 0.0489,
        1.304 * DEG2RAD,
        100.464 * DEG2RAD,
        273.867 * DEG2RAD,
        20.020 * DEG2RAD,
        t0, {}
    });
    //saturn
    bodies.push_back({
        "Saturn", BEIGE, 7.0,
        1.433e9, 0.0565,
        2.485 * DEG2RAD,
        113.665 * DEG2RAD,
        339.392 * DEG2RAD,
        317.020 * DEG2RAD,
        t0, {}
    });
    //uranus
    bodies.push_back({
        "Uranus", SKYBLUE, 6.0,
        2.872e9, 0.0457,
        0.773 * DEG2RAD,
        74.006 * DEG2RAD,
        96.998 * DEG2RAD,
        142.238 * DEG2RAD,
        t0, {}
    });
    //neptune
    bodies.push_back({
        "Neptune", DARKBLUE, 6.0,
        4.495e9, 0.0113,
        1.77 * DEG2RAD,
        131.784 * DEG2RAD,
        273.187 * DEG2RAD,
        256.228 * DEG2RAD,
        t0, {}
    });
    //pluto
    bodies.push_back({
        "Pluto", PURPLE, 5.0,
        5.906e9, 0.2488,
        17.16 * DEG2RAD,
        110.299 * DEG2RAD,
        113.834 * DEG2RAD,
        14.53 * DEG2RAD,
        t0, {}
    });

    // World extents: ±6e9 km
    const double worldRadius = 6.0e9;
    const double worldMinX = -worldRadius;
    const double worldMaxX =  worldRadius;
    const double worldMinY = -worldRadius;
    const double worldMaxY =  worldRadius;

    double worldWidth  = worldMaxX - worldMinX;
    double worldHeight = worldMaxY - worldMinY;

    double scaleX = (double)screenWidth  / worldWidth;
    double scaleY = (double)screenHeight / worldHeight;
    double worldToScreenScale = (scaleX < scaleY) ? scaleX : scaleY;

    Camera2D camera = {0};
    camera.target = { 0.0f, 0.0f };
    camera.offset = { (float)screenWidth/2.0f, (float)screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = (float)worldToScreenScale;

    double simTime = 0.0;          // seconds
    double timeScale = 60.0 * 60.0 * 24.0 * 5.0; // 5 days per real second

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        double dt = GetFrameTime();
        simTime += dt * timeScale;

        // Update bodies: propagate mean anomaly and compute state
        for (auto &b : bodies) {
            double n = std::sqrt(muSun / (b.a*b.a*b.a)); // rad/s
            double M = b.M0 + n * (simTime - b.epoch);
            M = std::fmod(M, 2.0*PI);
            if (M < 0) M += 2.0*PI;

            b.state = OrbitalElementsToStateVector(
                b.a, b.e, b.i, b.Omega, b.omega, M, muSun
            );

            // Add to trail (world space)
            Vector2 p = { (float)b.state.x, (float)b.state.y };
            b.trail.push_back(p);
            if (b.trail.size() > 2000) {
                b.trail.erase(b.trail.begin(), b.trail.begin() + 100);
            }
        }

        // Camera controls (mouse wheel zoom, right-drag pan)
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            float zoomFactor = 1.0f + wheel * 0.1f;
            camera.zoom *= zoomFactor;
            if (camera.zoom < 1e-12f) camera.zoom = 1e-12f;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);

        // Draw Sun
        DrawCircleV({0.0f, 0.0f}, 12.0f, YELLOW);

        // Draw trails
        for (auto &b : bodies) {
            if (b.trail.size() > 1) {
                for (size_t i = 1; i < b.trail.size(); ++i) {
                    DrawLineV(b.trail[i-1], b.trail[i], Fade(b.color, 0.5f));
                }
            }
        }

        // Draw planets
        for (auto &b : bodies) {
            Vector2 pos = { (float)b.state.x, (float)b.state.y };
            DrawCircleV(pos, (float)b.radiusPixels, b.color);
        }

        EndMode2D();

        // Labels (draw in screen space so they stay readable)
        for (auto &b : bodies) {
            Vector2 worldPos = { (float)b.state.x, (float)b.state.y };
            Vector2 screenPos = GetWorldToScreen2D(worldPos, camera);
            DrawText(b.name.c_str(), (int)screenPos.x + 8, (int)screenPos.y - 8, 14, RAYWHITE);
        }

        DrawText("Right-drag to pan, mouse wheel to zoom", 10, 10, 18, GREEN);
        DrawText("Time scale: 5 days / second", 10, 32, 18, GREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

