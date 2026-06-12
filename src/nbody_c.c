#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "raylib.h"

#define G 500.0    // Scaled gravitational constant for visible motion
#define SOFTEN 2.0
#define DT 0.01

typedef struct Body {
    double x, y;
    double vx, vy;
    double ax, ay;
    double fx, fy; // net force (for visualization)
    double mass;
    Color color;
} Body;

static void zero_acc(Body *b, int n) {
    for (int i = 0; i < n; ++i) {
        b[i].ax = b[i].ay = 0.0;
        b[i].fx = b[i].fy = 0.0;
    }
}

static void compute_forces(Body *b, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = i+1; j < n; ++j) {
            double dx = b[j].x - b[i].x;
            double dy = b[j].y - b[i].y;
            double r2 = dx*dx + dy*dy + SOFTEN*SOFTEN;
            double r = sqrt(r2);
            double force = (G * b[i].mass * b[j].mass) / r2; // magnitude
            double fx = force * dx / r;
            double fy = force * dy / r;
            b[i].fx += fx; b[i].fy += fy;
            b[j].fx -= fx; b[j].fy -= fy;
            b[i].ax += fx / b[i].mass; b[i].ay += fy / b[i].mass;
            b[j].ax -= fx / b[j].mass; b[j].ay -= fy / b[j].mass;
        }
    }
}

static void integrate_euler(Body *b, int n) {
    for (int i = 0; i < n; ++i) {
        b[i].vx += b[i].ax * DT;
        b[i].vy += b[i].ay * DT;
        b[i].x += b[i].vx * DT;
        b[i].y += b[i].vy * DT;
    }
}

static double compute_energy(Body *b, int n) {
    double ke = 0.0, pe = 0.0;
    for (int i = 0; i < n; ++i) {
        ke += 0.5 * b[i].mass * (b[i].vx*b[i].vx + b[i].vy*b[i].vy);
        for (int j = i+1; j < n; ++j) {
            double dx = b[j].x - b[i].x;
            double dy = b[j].y - b[i].y;
            double r = sqrt(dx*dx + dy*dy + SOFTEN*SOFTEN);
            pe -= (G * b[i].mass * b[j].mass) / r;
        }
    }
    return ke + pe;
}

// Draw an arrow representing net force on body (scale controls length)
static void draw_force_arrow(const Body *bb, float scale) {
    Vector2 start = { (float)bb->x, (float)bb->y };
    Vector2 end = { (float)(bb->x + bb->fx * scale), (float)(bb->y + bb->fy * scale) };
    DrawLineEx(start, end, 2.0f, RED);
    float dx = end.x - start.x; float dy = end.y - start.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len > 0.0f) {
        float ux = dx / len; float uy = dy / len;
        Vector2 p1 = { end.x - ux*10.0f - uy*6.0f, end.y - uy*10.0f + ux*6.0f };
        Vector2 p2 = { end.x - ux*10.0f + uy*6.0f, end.y - uy*10.0f - ux*6.0f };
        DrawTriangle(end, p1, p2, RED);
    }
}

int main(void) {
    const int screenW = 800, screenH = 600;

#if defined(__linux__)
    // No X11/Wayland display means we can't open a window
    const char *display = getenv("DISPLAY");
    int headless = (display == NULL || display[0] == '\0');
#else
    int headless = 0;
#endif

    // simple three-body system
    int n = 3;
    Body *b = calloc(n, sizeof(Body));
    if (!b) return 1;
    // central heavy body
    b[0].x = 400.0; b[0].y = 300.0; b[0].vx = 0; b[0].vy = 0; b[0].mass = 5000.0; b[0].color = GOLD;
    b[1].x = 400.0; b[1].y = 150.0; b[1].vx = 120.0; b[1].vy = 0; b[1].mass = 5.0; b[1].color = BLUE;
    b[2].x = 400.0; b[2].y = 500.0; b[2].vx = -90.0; b[2].vy = 0; b[2].mass = 8.0; b[2].color = GREEN;

    if (headless) {
        printf("Display unavailable. Running headless...\n");
        int totalSteps = 20000;
        for (int step = 0; step < totalSteps; ++step) {
            zero_acc(b, n);
            compute_forces(b, n);
            integrate_euler(b, n);
            if ((step+1) % 1000 == 0) printf("Step %d energy=%.3f\n", step+1, compute_energy(b, n));
        }
        for (int i = 0; i < n; ++i) printf("Body %d: x=%.3f y=%.3f vx=%.3f vy=%.3f\n", i, b[i].x, b[i].y, b[i].vx, b[i].vy);
        free(b);
        return 0;
    }

    InitWindow(screenW, screenH, "N-Body C Simulation");
    if (!IsWindowReady()) { free(b); return 1; }
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        zero_acc(b, n);
        compute_forces(b, n);
        integrate_euler(b, n);

        BeginDrawing();
        ClearBackground(BLACK);

        for (int i = 0; i < n; ++i) {
            DrawCircle((int)b[i].x, (int)b[i].y, 8, b[i].color);
            draw_force_arrow(&b[i], 0.02f);
            float mag = (float)sqrt(b[i].fx*b[i].fx + b[i].fy*b[i].fy);
            DrawText(TextFormat("F=%.0f", mag), (int)b[i].x + 10, (int)b[i].y - 10, 12, YELLOW);
        }

        DrawText("Press R to reset", 10, 10, 14, GREEN);
        DrawText("Red arrows = net gravitational force", 10, 30, 12, LIGHTGRAY);
        DrawText("Equation: m_i a_i = sum_j G m_i m_j (r_j - r_i) / |r_j - r_i|^3", 10, 50, 12, LIGHTGRAY);

        if (IsKeyPressed(KEY_R)) {
            // reset
            b[0].x = 400.0; b[0].y = 300.0; b[0].vx = 0; b[0].vy = 0;
            b[1].x = 400.0; b[1].y = 150.0; b[1].vx = 120.0; b[1].vy = 0;
            b[2].x = 400.0; b[2].y = 500.0; b[2].vx = -90.0; b[2].vy = 0;
        }

        EndDrawing();
    }

    CloseWindow();
    free(b);
    return 0;
}
