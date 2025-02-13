#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>

#define MAX_PARTICLES 100 // Limit the number of active particles

struct Player {
    Vector2 position;
    Vector2 velocity;
    float speed;
    int jumpCount;
    const int maxJumps = 2;
    bool isDashing = false;
    float dashSpeed = 15.0f;
    float dashTime = 0.2f;
    float dashCooldown = 1.0f;
    float dashTimer = 0.0f;
    float dashCooldownTimer = 0.0f;
    bool isAlive = true;
};

struct Platform {
    Rectangle rect;
    Color color;
    Vector2 velocity; // For moving platforms
    Vector2 startPos; // Starting position for moving platforms
    float moveDistance; // Distance the platform moves
    bool isMoving; // Whether the platform moves
};

struct Spike {
    Vector2 position; // Position of the spike (bottom center of the triangle)
    float width;      // Width of the spike base
    float height;     // Height of the spike
};

struct Flag {
    Vector2 position;
    bool isReached;
};

struct Particle {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float size;
    float life;
    Color color;
};

void AddParticle(std::vector<Particle>& particles, Vector2 pos, Color color) {
    if (particles.size() < MAX_PARTICLES) {
        Particle p;
        p.position = pos;
        p.velocity = { (float)(rand() % 41 - 20) / 10.0f, (float)(rand() % 41 - 20) / 10.0f };
        p.rotation = (float)(rand() % 360);
        p.size = (float)(rand() % 5 + 2);
        p.life = 1.0f;
        p.color = color;
        particles.push_back(p);
    }
}

// Reset player state manually
void ResetPlayer(Player& player) {
    player.position = { 100, 100 }; // Spawn point
    player.velocity = { 0, 0 };
    player.speed = 5.0f;
    player.jumpCount = 0;
    player.isDashing = false;
    player.dashTimer = 0.0f;
    player.dashCooldownTimer = 0.0f;
    player.isAlive = true;
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Scrolling Platformer");

    // Initialize audio
    InitAudioDevice();
    Sound jumpSound = LoadSound("jump.mp3"); // Load jump sound effect

    Player player;
    ResetPlayer(player); // Initialize player state

    const float gravity = 0.5f;

    // Create a camera
    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Platforms for testing
    std::vector<Platform> platforms = {
        { {0, 400, 800, 20}, GRAY, {0, 0}, {0, 200}, 0, false }, // Ground platform
        { {200, 300, 200, 20}, GRAY, {2, 0}, {200, 300}, 100, true }, // Moving platform
        { {500, 200, 150, 20}, GRAY, {0, 2}, {500, 200}, 100, true }, // Moving platform
        { {800, 300, 200, 20}, GRAY, {0, 0}, {800, 300}, 0, false },
        { {1200, 200, 150, 20}, GRAY, {0, 0}, {1200, 200}, 0, false }
    };


    // Win flag
    Flag flag = { {1400, 150}, false };

    std::vector<Particle> particles;
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (player.isAlive && !flag.isReached) {
            // Dash logic
            if (player.dashCooldownTimer > 0) player.dashCooldownTimer -= dt;
            if (player.isDashing) {
                player.dashTimer -= dt;
                if (player.dashTimer <= 0) {
                    player.isDashing = false;
                    player.speed = 5.0f;
                }
            }

            // Dash input
            if (IsKeyPressed(KEY_Z) && player.dashCooldownTimer <= 0) {
                player.isDashing = true;
                player.dashTimer = player.dashTime;
                player.dashCooldownTimer = player.dashCooldown;
                player.speed = player.dashSpeed;
                for (int i = 0; i < 20; i++) AddParticle(particles, { player.position.x + 20, player.position.y + 20 }, WHITE);
            }

            // Movement
            if (!player.isDashing) {
                if (IsKeyDown(KEY_RIGHT)) player.position.x += player.speed;
                if (IsKeyDown(KEY_LEFT)) player.position.x -= player.speed;
            }
            else {
                if (IsKeyDown(KEY_RIGHT)) player.position.x += player.dashSpeed;
                if (IsKeyDown(KEY_LEFT)) player.position.x -= player.dashSpeed;
            }

            // Jumping
            if (IsKeyPressed(KEY_SPACE)) {
                if (player.jumpCount < player.maxJumps) {
                    player.velocity.y = -10.0f;
                    player.jumpCount++;
                    PlaySound(jumpSound); // Play jump sound effect
                    if (player.jumpCount == 2) {
                        for (int i = 0; i < 20; i++) AddParticle(particles, { player.position.x + 20, player.position.y + 20 }, WHITE);
                    }
                }
            }

            // Apply gravity
            player.velocity.y += gravity;
            player.position.y += player.velocity.y;

            // Platform collisions
            Rectangle playerRect = { player.position.x, player.position.y, 40, 40 };
            bool onGround = false; // Track if the player is on the ground

            for (auto& platform : platforms) {
                if (CheckCollisionRecs(playerRect, platform.rect)) {
                    // Calculate overlap on each side
                    float overlapLeft = playerRect.x + playerRect.width - platform.rect.x;
                    float overlapRight = platform.rect.x + platform.rect.width - playerRect.x;
                    float overlapTop = playerRect.y + playerRect.height - platform.rect.y;
                    float overlapBottom = platform.rect.y + platform.rect.height - playerRect.y;

                    // Find the minimum overlap
                    float minOverlap = fminf(fminf(overlapLeft, overlapRight), fminf(overlapTop, overlapBottom));

                    // Resolve collision based on the minimum overlap
                    if (minOverlap == overlapTop) {
                        player.position.y = platform.rect.y - playerRect.height;
                        player.velocity.y = 0; // Stop vertical movement
                        player.jumpCount = 0; // Reset jumps
                        onGround = true; // Player is on the ground
                    }
                    else if (minOverlap == overlapBottom) {
                        player.position.y = platform.rect.y + platform.rect.height;
                        player.velocity.y = 0; // Stop vertical movement
                    }
                    else if (minOverlap == overlapLeft) {
                        player.position.x = platform.rect.x - playerRect.width;
                    }
                    else if (minOverlap == overlapRight) {
                        player.position.x = platform.rect.x + platform.rect.width;
                    }
                }

                // Update moving platforms
                if (platform.isMoving) {
                    platform.rect.x += platform.velocity.x;
                    platform.rect.y += platform.velocity.y;

                    // Reverse direction if the platform reaches its movement limits
                    if (platform.rect.x > platform.startPos.x + platform.moveDistance || platform.rect.x < platform.startPos.x - platform.moveDistance) {
                        platform.velocity.x *= -1;
                    }
                    if (platform.rect.y > platform.startPos.y + platform.moveDistance || platform.rect.y < platform.startPos.y - platform.moveDistance) {
                        platform.velocity.y *= -1;
                    }
                }
            }

        

            // Update camera to follow the player
            camera.target = player.position;

            // Check if player falls into the abyss
            if (player.position.y > screenHeight + 100) {
                ResetPlayer(player); // Respawn player at spawn point
            }

            // Check if player touches the flag
            Rectangle flagRect = { flag.position.x, flag.position.y, 40, 80 };
            if (CheckCollisionRecs(playerRect, flagRect)) {
                flag.isReached = true; // Player wins
            }
        }

        // Update particles
        for (auto& p : particles) {
            p.position.x += p.velocity.x;
            p.position.y += p.velocity.y;
            p.life -= dt;
            p.color.a = (unsigned char)(p.life * 255);
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0; }), particles.end());

        // Draw
        BeginDrawing();
        ClearBackground(SKYBLUE);

        // Begin 2D mode with the camera
        BeginMode2D(camera);

        // Draw platforms
        for (const auto& platform : platforms) {
            DrawRectangleRec(platform.rect, platform.color);
        }


        // Draw flag
        DrawRectangle(flag.position.x, flag.position.y, 40, 80, GREEN);
        if (flag.isReached) {
            DrawText("You Win!", flag.position.x - 50, flag.position.y - 20, 20, BLACK);
        }

        // Draw player
        if (player.isAlive) {
            DrawRectangle(player.position.x, player.position.y, 40, 40, BLUE);
        }
        else {
            DrawText("You Died!", player.position.x - 50, player.position.y - 20, 20, RED);
            if (IsKeyPressed(KEY_ENTER)) {
                ResetPlayer(player); // Reset player state
            }
        }

        // Draw particles
        for (const auto& p : particles) {
            DrawRectanglePro({ p.position.x, p.position.y, p.size, p.size }, { p.size / 2, p.size / 2 }, p.rotation, Fade(p.color, p.life));
        }

        // End 2D mode
        EndMode2D();


        EndDrawing();
    }

    // Unload sound
    UnloadSound(jumpSound);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}