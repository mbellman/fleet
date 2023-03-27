#include "Gamma.h"

#include "fleet_macros.h"

using namespace Gamma;

// @todo move to game constants
constexpr static float MAX_DT = 1.f / 30.f;
constexpr static float LEVEL_1_ALTITUDE = 5000.f;

internal void initializeGame(GmContext* context) {
  // Engine/scene configuration
  {
    auto& camera = get_camera();

    context->scene.zNear = 1.f;
    context->scene.zFar = 10000.f;

    context->scene.sky.sunDirection = Vec3f(0, 1.f, 0.5f).unit();
    context->scene.sky.sunColor = Vec3f(1.f, 0.8f, 0.5f);
    context->scene.sky.atmosphereColor = Vec3f(1.f);

    camera.position.y = LEVEL_1_ALTITUDE;
    camera.orientation.pitch = Gm_HALF_PI * 0.7f;
    camera.rotation = camera.orientation.toQuaternion();

    Gm_EnableFlags(GammaFlags::VSYNC);
  }

  // Global meshes/objects
  {
    add_mesh("ocean", 1, Mesh::Plane(2));
    add_mesh("ocean-floor", 1, Mesh::Plane(2));

    mesh("ocean")->type = MeshType::WATER;

    auto& ocean = create_object_from("ocean");
    auto& floor = create_object_from("ocean-floor");

    ocean.scale = Vec3f(10000.f, 1.f, 10000.f);

    floor.position = ocean.position - Vec3f(0, 500.f, 0);
    floor.scale = ocean.scale;
    floor.color = Vec3f(0.1f, 1.f, 1.f);

    commit(ocean);
    commit(floor);
  }
}

internal void updateGame(GmContext* context, float dt) {
  auto& camera = get_camera();

  camera.position.z += 2000.f * dt;

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
}

int main(int argc, char* argv[]) {
  using namespace Gamma;

  auto* context = Gm_CreateContext();
  // GameState state;

  Gm_OpenWindow(context, "Fleet", { 1200, 675 });
  Gm_SetRenderMode(context, GmRenderMode::OPENGL);

  initializeGame(context);

  auto& input = get_input();
  auto& camera = get_camera();

  input.on<MouseButtonEvent>("mousedown", [&](const MouseButtonEvent& event) {
    if (!Gm_IsWindowFocused()) {
      Gm_FocusWindow();
    }
  });

  input.on<Key>("keydown", [](Key key) {
    if (key == Key::ESCAPE) {
      Gm_UnfocusWindow();
    }
  });

  while (!context->window.closed) {
    float dt = Gm_GetDeltaTime(context);

    Gm_HandleFrameStart(context);

    // @todo handle this within the engine (?)
    if (dt > MAX_DT) {
      dt = MAX_DT;
    }

    updateGame(context, dt);

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