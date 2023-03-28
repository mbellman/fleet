#include <vector>

#include "Gamma.h"

#include "fleet_macros.h"

using namespace Gamma;

// @todo move to game constants
constexpr static float MAX_DT = 1.f / 30.f;
constexpr static float LEVEL_1_ALTITUDE = 5000.f;
constexpr static float PLAYER_ACCELERATION_RATE = 5000.f;
constexpr static float MAX_VELOCITY = 500.f;
constexpr static u16 TOTAL_PLAYER_BULLETS = 100;
constexpr static u16 TOTAL_ENEMY_BULLETS = 500;

struct Bullet {
  Vec3f velocity = Vec3f(0.f);
  Vec3f position = Vec3f(0.f);
  Vec3f color = Vec3f(0.f);
  float scale = 0.f;
};

struct Enemy {
  u16 index = 0;
  Vec3f velocity = Vec3f(0.f);
  float health = 100.f;
};

struct SpiralShip : Enemy {};

enum EnemyType {
  SPIRAL_SHIP
};

struct GameState {
  Vec3f velocity;
  Vec3f offset;

  u8 bulletTier = 2;
  std::vector<Bullet> playerBullets;
  std::vector<Bullet> enemyBullets;
  float lastPlayerBulletFireTime = 0.f;
  u16 nextPlayerBulletIndex = 0;
  u16 nextEnemyBulletIndex = 0;

  std::vector<SpiralShip> spiralShips;
};

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

internal void spawnEnemy(GmContext* context, GameState& state, EnemyType type, const Vec3f offset) {
  auto& player = get_player();

  switch (type) {
    case SPIRAL_SHIP:
      if (objects("spiral-ship").totalActive() == 10) {
        return;
      }

      auto& ship = create_object_from("spiral-ship");

      ship.position = player.position + offset;
      ship.scale = Vec3f(20.f);

      state.spiralShips.push_back({
        ship._record.id,
        Vec3f(0, 0, -100.f),
        150.f
      });

      break;
  }
}

internal void updateSpiralShips(GmContext* context, GameState& state, float dt) {
  const float scrollDistance = 2000.f * dt;
  auto& spiralShipObjects = objects("spiral-ship");
  float t = get_scene_time();

  for (auto& ship : state.spiralShips) {
    ship.velocity.x = sinf(t * 2.f) * 50.f;

    auto& object = spiralShipObjects[ship.index];

    object.position += ship.velocity * dt;
    object.position.z += scrollDistance;
    object.rotation = Quaternion::fromAxisAngle(Vec3f(0, 1, 0), t);

    commit(object);
  }
}

internal void updateEnemyShips(GmContext* context, GameState& state, float dt) {
  updateSpiralShips(context, state, dt);
}

internal void initializeGame(GmContext* context, GameState& state) {
  auto& camera = get_camera();

  // Engine/scene configuration
  {
    context->scene.zNear = 1.f;
    context->scene.zFar = 10000.f;

    context->scene.sky.sunDirection = Vec3f(0, 1.f, 0.5f).unit();
    context->scene.sky.sunColor = Vec3f(1.f, 0.8f, 0.5f);
    context->scene.sky.atmosphereColor = Vec3f(1.f);

    camera.position.y = LEVEL_1_ALTITUDE;
    camera.position.z = -250000.f;
    camera.orientation.pitch = Gm_HALF_PI * 0.7f;
    camera.rotation = camera.orientation.toQuaternion();

    auto& light = create_light(LightType::DIRECTIONAL);

    light.color = Vec3f(1.f, 0.9f, 0.8f);
    light.direction = Vec3f(0, -1.f, 1.f);

    auto& flash = create_light(LightType::POINT);

    flash.color = Vec3f(1.f);
    flash.radius = 100.f;
    flash.power = 0.f;

    save_light("muzzle-flash", &flash);

    Gm_EnableFlags(GammaFlags::VSYNC);
  }

  // Global meshes/objects
  {
    add_mesh("ocean", 1, Mesh::Plane(2));
    add_mesh("ocean-floor", 1, Mesh::Plane(2));
    add_mesh("main-ship", 1, Mesh::Model("./fleet/assets/main-ship.obj"));
    add_mesh("bullet", TOTAL_PLAYER_BULLETS, Mesh::Sphere(6));
    add_mesh("bullet-glow", TOTAL_PLAYER_BULLETS, Mesh::Particles());

    mesh("ocean")->type = MeshType::WATER;
    mesh("main-ship")->roughness = 0.1f;
    mesh("bullet")->emissivity = 0.5f;

    for (u16 i = 0; i < TOTAL_PLAYER_BULLETS; i++) {
      auto& bullet = create_object_from("bullet");
      auto& glow = create_object_from("bullet-glow");

      bullet.scale = Vec3f(10.f);
      bullet.color = Vec3f(1.f, 0.5f, 0.25f);

      glow.scale = bullet.scale * 2.f;
      glow.color = Vec3f(1.f, 0.8f, 0.5f);
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

  // Entity initialization
  {
    add_mesh("spiral-ship", 10, Mesh::Cube());

    state.playerBullets.reserve(TOTAL_PLAYER_BULLETS);
    state.enemyBullets.reserve(TOTAL_ENEMY_BULLETS);
  }

  // @temporary
  spawnEnemy(context, state, SPIRAL_SHIP, Vec3f(-200.f, 0, 500.f));
  spawnEnemy(context, state, SPIRAL_SHIP, Vec3f(200.f, 0, 500.f));
}

internal void updateGame(GmContext* context, GameState& state, float dt) {
  auto& camera = get_camera();
  auto& input = get_input();
  const float scrollDistance = 2000.f * dt;

  // Scroll level
  {
    camera.position.z += scrollDistance;
  }

  // Handle directional input
  {
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

    state.velocity += acceleration;

    if (state.velocity.magnitude() > MAX_VELOCITY) {
      state.velocity = state.velocity.unit() * MAX_VELOCITY;
    }

    state.offset += state.velocity * dt;

    state.velocity *= (1.f - 7.f * dt);
  }

  // Update player ships
  {
    auto& player = get_player();
    float roll = -0.25f * (state.velocity.x / MAX_VELOCITY);
    float pitch = 0.25f * (state.velocity.z / MAX_VELOCITY);

    player.position = camera.position - Vec3f(0, 500.f, 0) + Vec3f(0, 0, 200.f) + state.offset;
    player.rotation = Quaternion::fromAxisAngle(Vec3f(0, 0, 1), roll) * Quaternion::fromAxisAngle(Vec3f(1, 0, 0), pitch);

    commit(player);
  }

  // Update enemy ships
  {
    updateEnemyShips(context, state, dt);
  }

  // Handle player bullets
  {
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

  // Handle lights
  {
    auto& player = get_player();
    auto& flash = get_light("muzzle-flash");

    flash.position = player.position + Vec3f(0, player.scale.y, player.scale.z * 2.f);

    if (time_since(state.lastPlayerBulletFireTime) > 0.04f) {
      flash.power = 0.f;
    }
  }

  // Sync ocean plane position to camera
  {
    auto& ocean = objects("ocean")[0];
    auto& floor = objects("ocean-floor")[0];

    ocean.position.x = camera.position.x;
    ocean.position.z = camera.position.z;
    floor.position = ocean.position - Vec3f(0, 500.f, 0);

    commit(ocean);
    commit(floor);
  }

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