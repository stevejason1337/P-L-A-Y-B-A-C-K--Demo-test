#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <limits>
#include <functional>
#include <cmath>

// Engine headers (от корня проекта)
#include "Engine/Core/Settings.h"
#include "Engine/Physics/AABB.h"

struct Character {
    glm::vec3 pos = glm::vec3(0.f, 20.f, 0.f);
    glm::vec3 vel = glm::vec3(0.f);
    float eyeH = STAND_H;
    bool onGround = false;
    bool crouching = false;
    bool sprinting = false;
};

extern Character player;

#ifndef GUN_STATE_DEFINED
#define GUN_STATE_DEFINED
struct GunState {
    int   ammo = 12;
    float shootCooldown = 0.f;
    float recoilOffset = 0.f;
    float recoilTimer = 0.f;
    bool  reloading = false;
    bool  reloadFull = false;
    float bobTimer = 0.f;
    float adsProgress = 0.f;
};
#endif

extern GunState gun;
extern float    flashTimer;
extern int      fireAnimCounter;

#ifndef BULLET_HOLE_DEFINED
#define BULLET_HOLE_DEFINED
struct BulletHole { glm::vec3 pos; float life; };
#endif
extern std::vector<BulletHole> bulletHoles;

inline float playerHP = 100.f;
inline float playerMaxHP = 100.f;

// ─── Физика игрока ───────────────────────────────────────
inline void updatePlayer(float dt)
{
    // Таймеры оружия
    if (gun.shootCooldown > 0) gun.shootCooldown -= dt;
    if (flashTimer > 0)        flashTimer -= dt;

    // Recoil
    if (gun.recoilOffset > 0.f) {
        gun.recoilOffset -= dt * 0.25f;
        if (gun.recoilOffset < 0.f) gun.recoilOffset = 0.f;
    }

    // Bullet holes lifetime
    for (auto& bh : bulletHoles) bh.life -= dt;
    bulletHoles.erase(
        std::remove_if(bulletHoles.begin(), bulletHoles.end(),
            [](const BulletHole& b) { return b.life <= 0.f; }),
        bulletHoles.end());

    extern bool noclip;
    if (noclip) {
        player.vel = glm::vec3(0.f);
        player.onGround = false;
        return;
    }

    // Гравитация
    player.vel.y += GRAVITY * dt;

    // Желаемая высота приседания
    float targetEyeH = player.crouching ? CROUCH_H : STAND_H;
    player.eyeH += (targetEyeH - player.eyeH) * std::min(dt * 10.f, 1.f);

    // Движение
    player.pos += player.vel * dt;

    // Коллизия со стенами
    wallCollide(player.pos);

    // Коллизия с полом
    float gy = getGroundY(player.pos, 50.f);
    float floorY = (gy != std::numeric_limits<float>::lowest()) ? gy : -9999.f;

    if (player.pos.y <= floorY) {
        player.pos.y = floorY;
        player.vel.y = 0.f;
        player.onGround = true;
    }
    else {
        player.onGround = false;
    }

    // Смерть / респаун
    if (player.pos.y < -100.f) {
        player.pos = glm::vec3(0.f, 20.f, 0.f);
        player.vel = glm::vec3(0.f);
        playerHP = playerMaxHP;
    }
}

// ─── Вспомогательное: повернуть вектор на случайный угол (спред) ─────
inline glm::vec3 _spreadDir(const glm::vec3& front, float spreadDeg)
{
    if (spreadDeg <= 0.f) return front;

    glm::vec3 up = glm::abs(front.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    glm::vec3 realUp = glm::cross(right, front);

    float rad = glm::radians(spreadDeg);
    float angle = ((float)rand() / RAND_MAX) * 2.f * 3.14159f;
    float r = ((float)rand() / RAND_MAX) * rad;

    return glm::normalize(
        front
        + right * (sinf(r) * cosf(angle))
        + realUp * (sinf(r) * sinf(angle))
    );
}

// ─── Выстрел ─────────────────────────────────────────────
// gShootEnemyFn - колбэк из main.cpp, вызывает enemyManager.rayHit
// Это нужно чтобы Player.h не зависел от Enemy.h (порядок include)
using ShootEnemyFn = std::function<int(const glm::vec3&, const glm::vec3&, float*)>;
inline ShootEnemyFn gShootEnemyFn; // инициализируется в main.cpp после загрузки

inline void doShoot(const glm::vec3& camPos, const glm::vec3& camFront,
    float fireRate, float recoilKick,
    int pellets = 1, float spreadDeg = 0.f)
{
    if (gun.shootCooldown > 0 || gun.ammo <= 0 || gun.reloading) return;

    gun.ammo--;
    gun.shootCooldown = fireRate;
    gun.recoilOffset = recoilKick;
    flashTimer = 0.1f;
    fireAnimCounter++;

    for (int p = 0; p < pellets; p++)
    {
        glm::vec3 dir = (pellets > 1)
            ? _spreadDir(camFront, spreadDeg)
            : camFront;

        float enemyDist = 200.f;
        int   hitIdx = -1;
        if (gShootEnemyFn)
            hitIdx = gShootEnemyFn(camPos, dir, &enemyDist);

        glm::vec3 hitPos;
        if (shootRay(camPos, dir, hitPos)) {
            float wallDist = glm::length(hitPos - camPos);
            if (hitIdx < 0 || wallDist < enemyDist)
                bulletHoles.push_back({ hitPos, 5.f });
        }
    }
    // Звук вызывается снаружи (в Input.h mouse_button_callback)
}