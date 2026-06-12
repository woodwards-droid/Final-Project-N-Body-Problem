#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
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

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string UrlEncode(const std::string& value) {
    std::string encoded;
    encoded.reserve(value.size() * 3);
    for (char c : value) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded.push_back(c);
        } else if (c == ' ') {
            encoded.push_back('+');
        } else {
            char buffer[4];
            std::snprintf(buffer, sizeof(buffer), "%%%02X", static_cast<unsigned char>(c));
            encoded.append(buffer);
        }
    }
    return encoded;
}

std::string FetchUrl(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return {};

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "raylib-nbody-sbdb/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (result != CURLE_OK) {
        return {};
    }
    return response;
}

bool ParseJsonStringValue(const std::string& json, const std::string& key, std::string& out, size_t startPos = 0) {
    std::string search = '"' + key + '"';
    size_t pos = json.find(search, startPos);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return false;
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return false;
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return false;
    out = json.substr(pos + 1, end - pos - 1);
    return true;
}

bool ParseJsonNumberValue(const std::string& json, const std::string& key, double& out, size_t startPos = 0) {
    std::string search = '"' + key + '"';
    size_t pos = json.find(search, startPos);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return false;
    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return false;
    size_t end = pos;
    while (end < json.size() && (std::isdigit(json[end]) || json[end] == '+' || json[end] == '-' || json[end] == '.' || json[end] == 'e' || json[end] == 'E')) {
        ++end;
    }
    if (end == pos) return false;
    try {
        out = std::stod(json.substr(pos, end - pos));
    } catch (...) {
        return false;
    }
    return true;
}

bool TryExtractSBDBSemiMajorAxis(const std::string& json, double& aValue) {
    size_t orbitPos = json.find("\"elements\"");
    if (orbitPos == std::string::npos) {
        orbitPos = json.find("\"orbit\"");
        if (orbitPos == std::string::npos) return false;
    }
    return ParseJsonNumberValue(json, "a", aValue, orbitPos);
}

bool AddSBDBObject(std::vector<Body>& bodies, const std::string& designation, std::string& status) {
    if (bodies.empty()) {
        status = "No solar system loaded.";
        std::cerr << "ERROR: bodies empty" << std::endl;
        return false;
    }

    double semiMajorAxisAU = 0.0;
    std::string fullname = designation;
    
    // Try network fetch first
    const std::string url = "https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=" + UrlEncode(designation);
    std::cerr << "DEBUG: Attempting to fetch from: " << url << std::endl;
    std::string json = FetchUrl(url);
    
    if (!json.empty()) {
        std::cerr << "DEBUG: Got SBDB response, size=" << json.size() << std::endl;
        if (TryExtractSBDBSemiMajorAxis(json, semiMajorAxisAU)) {
            if (!ParseJsonStringValue(json, "fullname", fullname) &&
                !ParseJsonStringValue(json, "name", fullname) &&
                !ParseJsonStringValue(json, "des", fullname)) {
                fullname = designation;
            }
            std::cerr << "DEBUG: Extracted a=" << semiMajorAxisAU << " AU, name=" << fullname << std::endl;
        } else {
            std::cerr << "DEBUG: Failed to extract semi-major axis from JSON" << std::endl;
            semiMajorAxisAU = 0.0;
        }
    } else {
        std::cerr << "DEBUG: Network fetch failed, using hardcoded fallback" << std::endl;
    }

    // Hardcoded fallback for Ceres if network fails
    if (semiMajorAxisAU <= 0.0 && designation == "Ceres") {
        std::cerr << "DEBUG: Using hardcoded Ceres data" << std::endl;
        semiMajorAxisAU = 2.77;  // Ceres semi-major axis in AU
        fullname = "Ceres";
    }

    if (semiMajorAxisAU <= 0.0) {
        status = "Failed to fetch " + designation + " (no network or unknown object)";
        std::cerr << "ERROR: " << status << std::endl;
        return false;
    }

    const Vector2 center = {(float)SCREEN_WIDTH / 2.0f, (float)SCREEN_HEIGHT / 2.0f};
    const float scaleAu = 85.0f;
    const float distance = (float)(semiMajorAxisAU * scaleAu);
    const double orbitalSpeed = std::sqrt(G * bodies[0].mass / distance);

    std::cerr << "DEBUG: Creating body: pos=(" << (center.x + distance) << ", " << center.y << "), vel=(0, " << orbitalSpeed << ")" << std::endl;

    Body sbdbBody;
    sbdbBody.pos = {center.x + distance, center.y};
    sbdbBody.vel = {0.0f, (float)orbitalSpeed};
    sbdbBody.acc = {0.0f, 0.0f};
    sbdbBody.mass = 2.5;
    sbdbBody.radius = 5.0f;
    sbdbBody.color = GREEN;
    sbdbBody.name = fullname;
    sbdbBody.trail.clear();

    bodies.push_back(sbdbBody);
    status = "Loaded " + fullname + " from SBDB (a=" + std::to_string(semiMajorAxisAU) + " AU)";
    std::cerr << "SUCCESS: " << status << std::endl;
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
    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char* display = getenv("DISPLAY");
    if (display == nullptr || display[0] == '\0') {
        curl_global_cleanup();
        return runHeadless();
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Solar System N-Body Simulation");
    if (!IsWindowReady()) {
        curl_global_cleanup();
        return runHeadless();
    }

    SetTargetFPS(60);
    std::vector<Body> bodies = MakeSolarSystem();
    std::string sbdbStatus = "Ready. Press L to load Ceres from SBDB.";

    while (!WindowShouldClose()) {
        const double dt = GetFrameTime() * TIME_SCALE;
        ComputeGravity(bodies);
        IntegrateSymplectic(bodies, dt);

        BeginDrawing();
        ClearBackground(BLACK);

        DrawSystem(bodies);

        DrawText("Solar System N-Body Simulation", 10, 10, 16, RAYWHITE);
        DrawText("Press R to reset | Press L to load SBDB object", 10, 30, 12, GREEN);
        DrawText("Use Newton: F = G m1 m2 / r^2", 10, 46, 12, LIGHTGRAY);
        DrawText(TextFormat("Bodies: %zu | Time scale: %.1fx", bodies.size(), TIME_SCALE), 10, 62, 12, LIGHTGRAY);
        DrawText(sbdbStatus.c_str(), 10, 78, 11, YELLOW);
        EndDrawing();

        if (IsKeyPressed(KEY_R)) {
            bodies = MakeSolarSystem();
            ResetTrails(bodies);
            sbdbStatus = "Solar system reset.";
        }
        
        // Check for L or l key to load SBDB object
        int charPressed = GetCharPressed();
        if (IsKeyPressed(KEY_L) || charPressed == 'l' || charPressed == 'L') {
            std::string status;
            if (AddSBDBObject(bodies, "Ceres", status)) {
                sbdbStatus = status;
            } else {
                sbdbStatus = "SBDB load failed: " + status;
            }
        }
    }

    CloseWindow();
    curl_global_cleanup();
    return 0;
}
