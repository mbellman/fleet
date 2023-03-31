#pragma once

#include "Gamma.h"

using namespace Gamma;

struct Bounds {
  Vec3f top;
  Vec3f bottom;
};

struct Bullet {
  Vec3f velocity = Vec3f(0.f);
  Vec3f position = Vec3f(0.f);
  Vec3f color = Vec3f(0.f);
  float scale = 0.f;
};

struct Enemy {
  u16 index = 0;
  Vec3f velocity = Vec3f(0.f);
  float lastBulletFireTime = 0.f;
  float health = 100.f;

  bool operator==(const Enemy& enemy) {
    return index == enemy.index;
  }
};

struct SpiralShip : Enemy {};

enum EnemyType {
  SPIRAL_SHIP
};

struct EnemySpawn {
  float time;
  Vec3f offset;
  EnemyType type;
};

struct GameState {
  Vec3f gameFieldCenter;

  Vec3f velocity;
  Vec3f offset;
  Bounds bounds;

  float levelStartTime = 0.f;

  u8 bulletTier = 2;
  std::vector<Bullet> playerBullets;
  std::vector<Bullet> enemyBullets;
  float lastPlayerBulletFireTime = 0.f;
  u16 nextPlayerBulletIndex = 0;
  u16 nextEnemyBulletIndex = 0;

  std::vector<EnemySpawn> remainingEnemySpawns;

  std::vector<SpiralShip> spiralShips;
};