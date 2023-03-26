#include "Gamma.h"

#include "fleet_macros.h"

using namespace Gamma;

// @todo move to game constants
constexpr static float MAX_DT = 1.f / 30.f;

internal void initializeGame(GmContext* context) {
  // Engine/scene configuration
  {
    context->scene.zNear = 1.f;
    context->scene.zFar = 10000.f;

    context->scene.sky.sunDirection = Vec3f(0, 0.2f, 1.f);
    context->scene.sky.sunColor = Vec3f(1.f, 0.8f, 0.5f);
    context->scene.sky.atmosphereColor = Vec3f(1.f);
  }

  // Global meshes/objects
  {
    add_mesh("ocean", 1, Mesh::Plane(2));

    mesh("ocean")->type = MeshType::WATER;

    auto& ocean = create_object_from("ocean");

    ocean.position = Vec3f(0, -500.f, 0);
    ocean.scale = Vec3f(5000.f, 1.f, 5000.f);

    commit(ocean);
  }
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

    // updateGame(context, state, dt);

    if (Gm_IsWindowFocused()) {
      camera.orientation.pitch += input.getMouseDelta().y / 1000.f;
      camera.orientation.yaw += input.getMouseDelta().x / 1000.f;

      camera.rotation = camera.orientation.toQuaternion();
    }

    Gm_HandleFreeCameraMode(context, 5000.f, dt);

    context->scene.sceneTime += dt;

    Gm_RenderScene(context);
    Gm_HandleFrameEnd(context);
  }

  Gm_DestroyContext(context);

  return 0;
}