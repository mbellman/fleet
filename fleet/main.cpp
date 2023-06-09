#include <vector>

#include "Gamma.h"

#include "game_types.h"
#include "game_constants.h"
#include "fleet_macros.h"

using namespace Gamma;

static std::vector<EnemySpawn> LEVEL_1_ENEMY_SPAWNS = {
  {
    .time = 0.f,
    .offset = Vec3f(-200.f, 0, 500.f),
    .type = SPIRAL_SHIP
  },
  {
    .time = 2.f,
    .offset = Vec3f(200.f, 0, 500.f),
    .type = SPIRAL_SHIP
  },

  {
    .time = 4.f,
    .offset = Vec3f(-200.f, 0, 500.f),
    .type = SPIRAL_SHIP
  },
  {
    .time = 6.f,
    .offset = Vec3f(200.f, 0, 500.f),
    .type = SPIRAL_SHIP
  }
};

internal Vec3f calculateGameFieldCenter(const Vec3f& cameraPosition) {
  return cameraPosition - Vec3f(0, 500.f, 0) + Vec3f(0, 0, 200.f);
}

// @todo consider different level view orientations
internal bool isScrolledOutOfBounds(const GameState& state, const Vec3f& position) {
  return position.z < (state.gameFieldCenter.z + state.bounds.bottom.z);
}

internal bool isBulletColliding(const Bullet& bullet, const Vec3f& target, float padding) {
  if (bullet.scale == 0.f) {
    return false;
  }

  if (
    (bullet.position.x + bullet.scale < target.x - padding) ||
    (bullet.position.x - bullet.scale > target.x + padding) ||
    (bullet.position.z + bullet.scale < target.z - padding) ||
    (bullet.position.z - bullet.scale > target.z + padding)
  ) {
    return false;
  }

  return true;
}

internal void spawnPlayerBullet(GmContext* context, GameState& state, const Bullet& bullet) {
  state.playerBullets[state.nextPlayerBulletIndex] = bullet;

  if (++state.nextPlayerBulletIndex >= TOTAL_PLAYER_BULLETS) {
    state.nextPlayerBulletIndex = 0;
  }
}

internal void spawnEnemyBullet(GmContext* context, GameState& state, const Bullet& bullet) {
  state.enemyBullets[state.nextEnemyBulletIndex] = bullet;

  if (++state.nextEnemyBulletIndex >= TOTAL_ENEMY_BULLETS) {
    state.nextEnemyBulletIndex = 0;
  }
}

internal Object& requestEnemyObject(GmContext* context, const std::string& objectName, u32 totalActiveEntities) {
  auto& existingObjects = objects(objectName);

  if (existingObjects.totalActive() > totalActiveEntities) {
    // When there are more active objects than active entities,
    // reuse an existing object. Pick the first one with a scale
    // of 0, used to mark objects as eligible for reuse.
    for (auto& object : existingObjects) {
      if (object.scale.x == 0.f) {
        return object;
      }
    }
  }

  return create_object_from(objectName);
}

internal void spawnEnemy(GmContext* context, GameState& state, EnemyType type, const Vec3f offset) {
  switch (type) {
    case SPIRAL_SHIP:
      auto& ship = requestEnemyObject(context, "spiral-ship", state.spiralShips.size());

      ship.position = state.gameFieldCenter + offset;
      ship.scale = Vec3f(20.f);

      state.spiralShips.push_back({
        ship._record.id,
        Vec3f(0, 0, -100.f),
        0.f,
        150.f
      });

      break;
  }
}

internal void updateSpiralShips(GmContext* context, GameState& state, float dt) {
  const float scrollDistance = 2000.f * dt;
  auto& spiralShipObjects = objects("spiral-ship");
  float t = get_scene_time();
  u32 index = 0;

  while (index < state.spiralShips.size()) {
    auto& entity = state.spiralShips[index];
    auto& object = spiralShipObjects[entity.index];

    object.position += entity.velocity * dt;
    object.position.z += scrollDistance;

    for (auto& bullet : state.playerBullets) {
      if (isBulletColliding(bullet, object.position, object.scale.x)) {
        entity.health -= 10.f;
        bullet.scale = 0.f;
      }
    }

    if (isScrolledOutOfBounds(state, object.position) || entity.health <= 0.f) {
      Gm_VectorRemove(state.spiralShips, entity);

      // @todo hide/animate out destroyed ships
      object.scale = Vec3f(0.f);

      commit(object);

      continue;
    }

    object.rotation = Quaternion::fromAxisAngle(Vec3f(0, 1, 0), t);

    commit(object);

    if (time_since(entity.lastBulletFireTime) > 0.2f) {
      Vec3f left = object.rotation.getLeftDirection();

      spawnEnemyBullet(context, state, {
        .velocity = left * 150.f + entity.velocity,
        .position = object.position + left * 20.f,
        .color = Vec3f(1.f, 0, 0),
        .scale = 10.f
      });

      spawnEnemyBullet(context, state, {
        .velocity = left.invert() * 150.f + entity.velocity,
        .position = object.position + left.invert() * 20.f,
        .color = Vec3f(1.f, 0, 0),
        .scale = 10.f
      });

      entity.lastBulletFireTime = get_scene_time();
    }

    index++;
  }
}

internal void initializeScene(GmContext* context, GameState& state) {
  auto& camera = get_camera();

  context->scene.zNear = 1.f;
  context->scene.zFar = 10000.f;

  context->scene.sky.sunDirection = Vec3f(0, 1.f, 0.5f).unit();
  context->scene.sky.sunColor = Vec3f(1.f, 0.8f, 0.5f);
  context->scene.sky.atmosphereColor = Vec3f(1.f);

  camera.position.y = LEVEL_1_ALTITUDE;
  camera.position.z = -250000.f;
  camera.orientation.pitch = Gm_HALF_PI * 0.7f;
  camera.rotation = camera.orientation.toQuaternion();

  // @todo calculate this dynamically
  state.bounds.top = Vec3f(450.f, 0, 350.f);
  state.bounds.bottom = Vec3f(300.f, 0, -120.f);

  auto& light = create_light(LightType::DIRECTIONAL);

  light.color = Vec3f(1.f, 0.9f, 0.8f);
  light.direction = Vec3f(0, -1.f, 1.f);

  auto& flash = create_light(LightType::POINT);

  flash.color = Vec3f(1.f);
  flash.radius = 100.f;
  flash.power = 0.f;

  save_light("muzzle-flash", &flash);
}

internal void initializeMeshes(GmContext* context, GameState& state) {
  auto& camera = get_camera();

  add_mesh("ocean", 1, Mesh::Plane(2));
  add_mesh("ocean-floor", 1, Mesh::Plane(2));
  add_mesh("main-ship", 1, Mesh::Model("./fleet/assets/main-ship.obj"));
  add_mesh("bullet", TOTAL_PLAYER_BULLETS, Mesh::Sphere(6));
  add_mesh("bullet-glow", TOTAL_PLAYER_BULLETS, Mesh::Particles());
  add_mesh("enemy-bullet", TOTAL_ENEMY_BULLETS, Mesh::Sphere(6));
  add_mesh("enemy-bullet-glow", TOTAL_ENEMY_BULLETS, Mesh::Particles());

  mesh("ocean")->type = MeshType::WATER;
  mesh("main-ship")->roughness = 0.1f;
  // mesh("bullet")->emissivity = 0.5f;
  // mesh("enemy-bullet")->emissivity = 0.5f;

  for (u16 i = 0; i < TOTAL_PLAYER_BULLETS; i++) {
    auto& bullet = create_object_from("bullet");
    auto& glow = create_object_from("bullet-glow");

    bullet.scale = Vec3f(0.f);
    glow.scale = Vec3f(0.f);
  }

  for (u16 i = 0; i < TOTAL_ENEMY_BULLETS; i++) {
    auto& bullet = create_object_from("enemy-bullet");
    auto& glow = create_object_from("enemy-bullet-glow");

    bullet.scale = Vec3f(0.f);
    glow.scale = Vec3f(0.);
  }

  auto& ocean = create_object_from("ocean");
  auto& floor = create_object_from("ocean-floor");
  auto& player = create_object_from("main-ship");

  ocean.scale = Vec3f(10000.f, 1.f, 10000.f);

  floor.position = ocean.position - Vec3f(0, 500.f, 0);
  floor.scale = ocean.scale;
  floor.color = Vec3f(0.1f, 0.75f, 0.75f);

  player.position = camera.position - Vec3f(0, 500.f, 0) + Vec3f(0, 0, 200.f) + state.offset;
  player.scale = Vec3f(30.f);
  player.color = Vec3f(1.f);

  commit(ocean);
  commit(floor);
  commit(player);
}

internal void initializeEntities(GmContext* context, GameState& state) {
  add_mesh("spiral-ship", 10, Mesh::Cube());

  state.playerBullets.resize(TOTAL_PLAYER_BULLETS);
  state.enemyBullets.resize(TOTAL_ENEMY_BULLETS);
}

internal void initializeGame(GmContext* context, GameState& state) {
  initializeScene(context, state);
  initializeMeshes(context, state);
  initializeEntities(context, state);

  Gm_EnableFlags(GammaFlags::VSYNC);

  state.levelStartTime = get_scene_time();

  // @temporary
  for (s32 i = LEVEL_1_ENEMY_SPAWNS.size() - 1; i >= 0; i--) {
    auto& spawn = LEVEL_1_ENEMY_SPAWNS[i];

    state.remainingEnemySpawns.push_back(spawn);
  }
}

internal void updateScrollOffset(GmContext* context, GameState& state, float dt) {
  auto& camera = get_camera();
  const float scrollDistance = 2000.f * dt;

  camera.position.z += scrollDistance;

  state.gameFieldCenter = calculateGameFieldCenter(camera.position);
}

internal void handleInput(GmContext* context, GameState& state, float dt) {
  auto& input = get_input();
  Vec3f acceleration;

  if (input.isKeyHeld(Key::ARROW_UP)) {
    acceleration.z += PLAYER_ACCELERATION_RATE * dt;
  }

  if (input.isKeyHeld(Key::ARROW_DOWN)) {
    acceleration.z -= PLAYER_ACCELERATION_RATE * dt;
  }

  if (input.isKeyHeld(Key::ARROW_LEFT)) {
    acceleration.x -= PLAYER_ACCELERATION_RATE * dt;
  }

  if (input.isKeyHeld(Key::ARROW_RIGHT)) {
    acceleration.x += PLAYER_ACCELERATION_RATE * dt;
  }

  float verticalAlpha = (state.offset.z - state.bounds.bottom.z) / (state.bounds.top.z - state.bounds.bottom.z);
  float verticalLimitFactor = 2.f * Gm_Absf(verticalAlpha - 0.5f);
  if (verticalLimitFactor > 1.f) verticalLimitFactor = 1.f;
  verticalLimitFactor = 1.f - powf(verticalLimitFactor, 10.f);

  if (
    (acceleration.z < 0.f && state.offset.z < 0.f) ||
    (acceleration.z > 0.f && state.offset.z > 0.f)
  ) {
    acceleration.z *= verticalLimitFactor;
    state.velocity.z *= verticalLimitFactor;
  }

  float horizontalLimit = Gm_Lerpf(state.bounds.bottom.x, state.bounds.top.x, verticalAlpha);
  float horizontalAlpha = 1.f - (-1.f * (state.offset.x - horizontalLimit) / (horizontalLimit * 2.f));
  float horizontalLimitFactor = 2.f * Gm_Absf(horizontalAlpha - 0.5f);
  if (horizontalLimitFactor > 1.f) horizontalLimitFactor = 1.f;
  horizontalLimitFactor = 1.f - powf(horizontalLimitFactor, 10.f);

  if (
    (acceleration.x < 0.f && state.offset.x < 0.f) ||
    (acceleration.x > 0.f && state.offset.x > 0.f)
  ) {
    acceleration.x *= horizontalLimitFactor;
    state.velocity.x *= horizontalLimitFactor;
  }

  if (state.offset.x < -horizontalLimit) {
    float delta = -horizontalLimit - state.offset.x;

    acceleration.x += 50.f * delta * dt;
  } else if (state.offset.x > horizontalLimit) {
    float delta = state.offset.x - horizontalLimit;

    acceleration.x -= 50.f * delta * dt;
  }

  state.velocity += acceleration;

  if (state.velocity.magnitude() > MAX_VELOCITY) {
    state.velocity = state.velocity.unit() * MAX_VELOCITY;
  }

  state.offset += state.velocity * dt;

  state.velocity *= (1.f - 7.f * dt);
}

internal void updatePlayerShips(GmContext* context, GameState& state, float dt) {
  auto& camera = get_camera();
  auto& player = get_player();
  float roll = -0.25f * (state.velocity.x / MAX_VELOCITY);
  float pitch = 0.25f * (state.velocity.z / MAX_VELOCITY);

  player.position = camera.position - Vec3f(0, 500.f, 0) + Vec3f(0, 0, 200.f) + state.offset;
  player.rotation = Quaternion::fromAxisAngle(Vec3f(0, 0, 1), roll) * Quaternion::fromAxisAngle(Vec3f(1, 0, 0), pitch);

  commit(player);
}

internal void updateEnemyShips(GmContext* context, GameState& state, float dt) {
  updateSpiralShips(context, state, dt);
}

internal void handleNewEnemySpawns(GmContext* context, GameState& state, float dt) {
  float runningTime = time_since(state.levelStartTime);

  while (1) {
    if (state.remainingEnemySpawns.size() == 0) {
      break;
    }

    auto& nextSpawn = state.remainingEnemySpawns.back();

    if (runningTime >= nextSpawn.time) {
      spawnEnemy(context, state, nextSpawn.type, nextSpawn.offset);

      state.remainingEnemySpawns.pop_back();
    } else {
      break;
    }
  }
}

internal void updatePlayerBullets(GmContext* context, GameState& state, float dt) {
  auto& input = get_input();
  const float scrollDistance = 2000.f * dt;

  if (
    input.isKeyHeld(Key::SPACE) &&
    time_since(state.lastPlayerBulletFireTime) >= 0.05f
  ) {
    auto& player = get_player();

    // Primary bullet
    spawnPlayerBullet(context, state, {
      .velocity = Vec3f(0, 0, 1000.f),
      .position = player.position,
      .color = Vec3f(1.f, 0.5f, 0.25f),
      .scale = 10.f
    });

    // Tier-1 bullets
    if (state.bulletTier >= 1) {
      spawnPlayerBullet(context, state, {
        .velocity = Vec3f(-200.f, 0, 900.f),
        .position = player.position,
        .color = Vec3f(1.f, 0.25f, 0.1f),
        .scale = 10.f
      });

      spawnPlayerBullet(context, state, {
        .velocity = Vec3f(200.f, 0, 900.f),
        .position = player.position,
        .color = Vec3f(1.f, 0.25f, 0.1f),
        .scale = 10.f
      });
    }

    // Tier-2 bullets
    if (state.bulletTier >= 2) {
      spawnPlayerBullet(context, state, {
        .velocity = Vec3f(0, 0, 1000.f),
        .position = player.position - Vec3f(30.f, 0, 0),
        .color = Vec3f(0.2f, 0.4f, 1.f),
        .scale = 6.f
      });

      spawnPlayerBullet(context, state, {
        .velocity = Vec3f(0, 0, 1000.f),
        .position = player.position + Vec3f(30.f, 0, 0),
        .color = Vec3f(0.2f, 0.4f, 1.f),
        .scale = 6.f
      });
    }

    get_light("muzzle-flash").power = 5.f;

    state.lastPlayerBulletFireTime = get_scene_time();
  }

  // Update bullet/glow position
  auto& bullets = objects("bullet");
  auto& glows = objects("bullet-glow");

  for (u16 i = 0; i < TOTAL_PLAYER_BULLETS; i++) {
    auto& playerBullet = state.playerBullets[i];
    auto& bullet = bullets[i];
    auto& glow = glows[i];

    playerBullet.position += playerBullet.velocity * dt;
    playerBullet.position.z += scrollDistance;

    bullet.position = glow.position = playerBullet.position;
    bullet.color = playerBullet.color;
    bullet.scale = Vec3f(playerBullet.scale);

    glow.position = bullet.position;
    glow.color = Vec3f(playerBullet.color);
    glow.scale = Vec3f(playerBullet.scale * 2.f);

    commit(bullet);
    commit(glow);
  }
}

internal void updateEnemyBullets(GmContext* context, GameState& state, float dt) {
  auto& bullets = objects("enemy-bullet");
  auto& glows = objects("enemy-bullet-glow");
  const float scrollDistance = 2000.f * dt;

  for (u16 i = 0; i < TOTAL_ENEMY_BULLETS; i++) {
    auto& enemyBullet = state.enemyBullets[i];
    auto& bullet = bullets[i];
    auto& glow = glows[i];

    enemyBullet.position += enemyBullet.velocity * dt;
    enemyBullet.position.z += scrollDistance;

    bullet.position = glow.position = enemyBullet.position;
    bullet.color = enemyBullet.color;
    bullet.scale = Vec3f(enemyBullet.scale);

    glow.position = bullet.position;
    glow.color = Vec3f(enemyBullet.color);
    glow.scale = Vec3f(enemyBullet.scale * 2.f);

    commit(bullet);
    commit(glow);
  }
}

internal void updateLights(GmContext* context, GameState& state, float dt) {
  auto& player = get_player();
  auto& flash = get_light("muzzle-flash");

  flash.position = player.position + Vec3f(0, player.scale.y, player.scale.z * 2.f);

  if (time_since(state.lastPlayerBulletFireTime) > 0.04f) {
    flash.power = 0.f;
  }
}

internal void updateOcean(GmContext* context, GameState& state, float dt) {
  auto& camera = get_camera();
  auto& ocean = objects("ocean")[0];
  auto& floor = objects("ocean-floor")[0];

  ocean.position.x = camera.position.x;
  ocean.position.z = camera.position.z;
  floor.position = ocean.position - Vec3f(0, 500.f, 0);

  commit(ocean);
  commit(floor);
}

internal void updateGame(GmContext* context, GameState& state, float dt) {
  auto& camera = get_camera();
  auto& input = get_input();
  const float scrollDistance = 2000.f * dt;

  updateScrollOffset(context, state, dt);
  handleInput(context, state, dt);
  updatePlayerShips(context, state, dt);
  updateEnemyShips(context, state, dt);
  handleNewEnemySpawns(context, state, dt);
  updatePlayerBullets(context, state, dt);
  updateEnemyBullets(context, state, dt);
  updateLights(context, state, dt);
  updateOcean(context, state, dt);

  context->scene.sceneTime += dt;

  // Debug messages
  #if GAMMA_DEVELOPER_MODE == 1
    add_debug_message("Camera: " + Gm_ToString(camera.position));
    add_debug_message("Velocity: " + Gm_ToString(state.velocity));
    add_debug_message("Position: " + Gm_ToString(state.offset));
  #endif
}

int main(int argc, char* argv[]) {
  using namespace Gamma;

  auto* context = Gm_CreateContext();
  GameState state;

  Gm_OpenWindow(context, "Fleet", { 1200, 675 });
  Gm_SetRenderMode(context, GmRenderMode::OPENGL);

  initializeGame(context, state);

  auto& input = get_input();
  auto& camera = get_camera();

  input.on<MouseButtonEvent>("mousedown", [&](const MouseButtonEvent& event) {
    if (!Gm_IsWindowFocused()) {
      Gm_FocusWindow();
    }
  });

  bool fullscreen = false;

  input.on<Key>("keydown", [context, &fullscreen](Key key) {
    if (key == Key::ESCAPE) {
      Gm_UnfocusWindow();
    }

    if (key == Key::F) {
      fullscreen = !fullscreen;

      Gm_SetFullScreen(context, fullscreen);
    }

    if (key == Key::T) {
      Gm_ToggleFlag(GammaFlags::ENABLE_DEV_TOOLS);
    }
  });

  while (!context->window.closed) {
    float dt = Gm_GetDeltaTime(context);

    Gm_HandleFrameStart(context);

    // @todo handle this within the engine (?)
    if (dt > MAX_DT) {
      dt = MAX_DT;
    }

    updateGame(context, state, dt);

    // if (Gm_IsWindowFocused()) {
    //   camera.orientation.pitch += input.getMouseDelta().y / 1000.f;
    //   camera.orientation.yaw += input.getMouseDelta().x / 1000.f;

    //   camera.rotation = camera.orientation.toQuaternion();
    // }

    // Gm_HandleFreeCameraMode(context, 5000.f, dt);

    // context->scene.sceneTime += dt;

    Gm_RenderScene(context);
    Gm_HandleFrameEnd(context);
  }

  Gm_DestroyContext(context);

  return 0;
}