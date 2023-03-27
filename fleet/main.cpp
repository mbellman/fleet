#include "Gamma.h"

#include "fleet_macros.h"

using namespace Gamma;

// @todo move to game constants
constexpr static float MAX_DT = 1.f / 30.f;
constexpr static float LEVEL_1_ALTITUDE = 5000.f;
constexpr static float MAX_VELOCITY = 400.f;

struct GameState {
  Vec3f velocity;
  Vec3f offset;
};

internal void initializeGame(GmContext* context, GameState& state) {
  // Engine/scene configuration
  {
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

    Gm_EnableFlags(GammaFlags::VSYNC);
  }

  // Global meshes/objects
  {
    add_mesh("ocean", 1, Mesh::Plane(2));
    add_mesh("ocean-floor", 1, Mesh::Plane(2));
    add_mesh("main-ship", 1, Mesh::Model("./fleet/assets/main-ship.obj"));

    mesh("ocean")->type = MeshType::WATER;
    mesh("main-ship")->roughness = 0.2f;

    auto& ocean = create_object_from("ocean");
    auto& floor = create_object_from("ocean-floor");
    auto& player = create_object_from("main-ship");

    ocean.scale = Vec3f(10000.f, 1.f, 10000.f);

    floor.position = ocean.position - Vec3f(0, 500.f, 0);
    floor.scale = ocean.scale;
    floor.color = Vec3f(0.1f, 1.f, 1.f);

    player.scale = Vec3f(30.f);
    player.color = Vec3f(0.8f, 0.9f, 1.f);

    commit(ocean);
    commit(floor);
    commit(player);
  }
}

internal void updateGame(GmContext* context, GameState& state, float dt) {
  auto& camera = get_camera();
  auto& input = get_input();

  camera.position.z += 2000.f * dt;

  // Handle input
  {
    Vec3f acceleration;
    const float rate = 3000.f;

    if (input.isKeyHeld(Key::ARROW_UP)) {
      acceleration.z += rate * dt;
    }

    if (input.isKeyHeld(Key::ARROW_DOWN)) {
      acceleration.z -= rate * dt;
    }

    if (input.isKeyHeld(Key::ARROW_LEFT)) {
      acceleration.x -= rate * dt;
    }

    if (input.isKeyHeld(Key::ARROW_RIGHT)) {
      acceleration.x += rate * dt;
    }

    state.velocity += acceleration;

    if (state.velocity.magnitude() > MAX_VELOCITY) {
      state.velocity = state.velocity.unit() * MAX_VELOCITY;
    }

    state.offset += state.velocity * dt;

    state.velocity *= (1.f - 7.f * dt);
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

  // Update ships
  {
    auto& player = get_player();
    float roll = -0.25f * (state.velocity.x / MAX_VELOCITY);
    float pitch = 0.25f * (state.velocity.z / MAX_VELOCITY);

    player.position = camera.position - Vec3f(0, 500.f, 0) + Vec3f(0, 0, 200.f) + state.offset;
    player.rotation = Quaternion::fromAxisAngle(Vec3f(0, 0, 1), roll) * Quaternion::fromAxisAngle(Vec3f(1, 0, 0), pitch);

    commit(player);
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