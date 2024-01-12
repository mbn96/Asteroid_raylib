#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <utility>
#include <vector>

#include "store.hpp"

static constexpr int WIDTH = 1000;
static constexpr int WIDTH_HALF = WIDTH >> 1;
static constexpr int HEIGHT = 1000;
static constexpr int HEIGHT_HALF = HEIGHT >> 1;

static constexpr float bullet_speed = WIDTH / 2.0f;
static constexpr float rotationSpeed{4.0};
static constexpr float acc{100};
static constexpr int ROCK_SIDES = 36;

// static Vector2 wrapToScreen(const Vector2 &v) {
//   Vector2 pos = v;
//   pos.x = pos.x > WIDTH ? 0 : pos.x;
//   pos.x = pos.x < 0 ? WIDTH : pos.x;
//
//   pos.y = pos.y > HEIGHT ? 0 : pos.y;
//   pos.y = pos.y < 0 ? HEIGHT : pos.y;
//   return pos;
// }

struct GameState {
  bool isRunning = true;
  int score = 0;
  int bestScore = 0;
};

class IGameObject {

public:
  virtual ~IGameObject() = default;

  bool update(float dt);
  void draw() const = delete;
};

static GameStore store;

class Ship;

template <int N> class Rock : public IGameObject {
private:
  Vector2 pos;
  Vector2 vel;
  int angularSpeed;
  float angle = 0;
  std::shared_ptr<std::array<Vector2, N>> shape;
  uint8_t size;
  float scale = 10.0;
  float aliveTime = 0;

  friend class Bullet;

public:
  Rock(Vector2 pos, Vector2 vel, int angularSpeer, uint8_t size)
      : pos(pos), vel(vel), angularSpeed(angularSpeer),
        shape(std::make_shared<std::array<Vector2, N>>()), size(size) {
    Vector2 unitVec = {0, 1};

    for (size_t i = 0; i < N; i++) {
      (*shape)[i] = Vector2Scale(
          Vector2Rotate(unitVec, (2.0F * static_cast<float>(i) * PI) / N),
          static_cast<float>(GetRandomValue(85, 99) / 100.0));
    }
  }
  Rock() = default;
  Rock(const Rock &r) = default;

  bool update(float dt) {
    if (aliveTime > 6) {
      return false;
    }
    angle += (DEG2RAD * angularSpeed) * dt;
    pos = Vector2Add(pos, Vector2Scale(vel, dt));
    aliveTime += dt;
    // if (pos.x > WIDTH || pos.x < 0 || pos.y > HEIGHT || pos.y < 0) {
    //   float x_out = pos.x > WIDTH ? pos.x - WIDTH : 0;
    //   x_out = pos.x < 0 ? -pos.x : x_out;
    //
    //   float y_out = pos.y > HEIGHT ? pos.y - HEIGHT : 0;
    //   y_out = pos.y < 0 ? -pos.y : y_out;
    //
    //   float max_diff = std::max(x_out, y_out);
    //   if (max_diff >= getRadious()) {
    //     pos = wrapToScreen(pos);
    //   }
    // }
    return true;
  }

  inline float getRadious() const { return this->size * this->scale; }
  inline const Vector2 &getPos() const { return pos; }

  void draw() const {
    // Vector2 temp[N];
    std::array<Vector2, N> temp{};
    for (size_t i = 0; i < N; i++) {
      temp.at(i) = Vector2Add(
          pos,
          Vector2Rotate(Vector2Scale((*shape).at(i), scale * size), angle));
    }
    DrawLineStrip(temp.data(), N, PINK);
    DrawLineV(temp[N - 1], temp[0], PINK);
  }
};

static Rock<ROCK_SIDES> spawnRandom();

class Bullet : public IGameObject {
private:
  Vector2 pos;
  Vector2 vel;
  float aliveTime = 0;
  bool isCollision(const Rock<ROCK_SIDES> &r) const;

public:
  Bullet() { pos = vel = {0}; }
  Bullet(const Bullet &t) = default;
  Bullet(const Vector2 &pos, const Vector2 &vel) : pos(pos), vel(vel) {}

  bool update(const float dt) {
    if (aliveTime > 2) {
      return false;
    }

    auto &rocks = store.getObj<std::vector<Rock<ROCK_SIDES>>>();
    for (size_t i = 0; i < rocks.size(); i++) {
      auto &r = rocks[i];

      if (isCollision(r)) {
        if (r.size > 2) {
          auto c1 = r;
          auto c2 = r;

          c1.size >>= 1;
          c2.size >>= 1;

          c1.aliveTime = c2.aliveTime = 0.0f;

          c1.vel =
              Vector2Add(c1.vel, (Vector2){c1.vel.y / 2.0f, -c1.vel.x / 2.0f});
          c2.vel =
              Vector2Add(c2.vel, (Vector2){-c2.vel.y / 2.0f, c2.vel.x / 2.0f});
          rocks.push_back(c1);
          rocks.push_back(c2);
        }
        std::swap(rocks[i], rocks.back());
        rocks.pop_back();
        store.getObj<GameState>().score++;
        return false;
      }
    }

    pos = Vector2Add(pos, Vector2Scale(vel, dt));
    aliveTime += dt;
    return true;
  }

  void draw() const { DrawCircleV(pos, 2, YELLOW); }
};

bool Bullet::isCollision(const Rock<ROCK_SIDES> &r) const {
  return Vector2Distance(pos, r.getPos()) <= r.getRadious();
}

class Ship : public IGameObject {
private:
  Vector2 pos{};
  Vector2 forward{}, left{}, right{};
  Vector2 v{};
  bool thrust{};
  float scale;

private:
  bool isCollision(const Rock<ROCK_SIDES> &r) const {
    return (r.getRadious() + scale) >=
           Vector2Length(Vector2Subtract(r.getPos(), pos));
  }

public:
  Ship() {
    pos = {WIDTH_HALF, HEIGHT_HALF};
    forward = {0, -1};
    left = {0.5, 0.5};
    right = {-0.5, 0.5};
    v = {0};

    scale = 20.0;
  }

  Ship(const Ship &s) = default;
  Ship(Ship &&s) = default;

  bool update(float dt) {
    thrust = false;
    float rotateAngle{0};
    if (IsKeyDown(KEY_RIGHT)) {
      rotateAngle = rotationSpeed * dt;
    } else if (IsKeyDown(KEY_LEFT)) {
      rotateAngle = -rotationSpeed * dt;
    }
    if (rotateAngle != 0) {
      forward = Vector2Rotate(forward, rotateAngle);
      right = Vector2Rotate(right, rotateAngle);
      left = Vector2Rotate(left, rotateAngle);
    }

    if (IsKeyDown(KEY_UP)) {
      v = Vector2Add(v, Vector2Scale(forward, dt * acc));
      thrust = true;
    }
    // pos = wrapToScreen(Vector2Add(pos, Vector2Scale(v, dt)));
    pos = Vector2Add(pos, Vector2Scale(v, dt));

    // fire bullets
    if (IsKeyReleased(KEY_SPACE)) {
      store.getObj<std::vector<Bullet>>().push_back(
          Bullet(Vector2Add(pos, Vector2Scale(forward, scale)),
                 Vector2Add(v, Vector2Scale(forward, bullet_speed))));
    }

    // checking collisions
    auto &rocks = store.getObj<std::vector<Rock<ROCK_SIDES>>>();
    for (size_t i = 0; i < rocks.size(); i++) {
      auto &r = rocks[i];
      if (isCollision(r)) {
        auto &state = store.getObj<GameState>();
        state.isRunning = false;
        state.bestScore = std::max(state.score, state.bestScore);
        printf("collision detected...\n");
        return false;
      }
    }
    return true;
  }

  inline const Vector2 &getPos() const { return this->pos; }
  inline const Vector2 &getVel() const { return this->v; }

  void draw() const {
    auto fv = Vector2Add(pos, Vector2Scale(forward, scale));
    auto rv = Vector2Add(pos, Vector2Scale(right, scale));
    auto lv = Vector2Add(pos, Vector2Scale(left, scale));
    DrawTriangle(fv, rv, pos, WHITE);
    DrawTriangle(lv, fv, pos, WHITE);

    if (thrust) {
      DrawTriangle(rv, Vector2Add(pos, Vector2Scale(forward, -scale * 1.5F)),
                   lv, YELLOW);

      DrawTriangle(pos, rv, lv, SKYBLUE);
      DrawTriangle(rv, Vector2Add(pos, Vector2Scale(forward, -scale * 0.7F)),
                   lv, SKYBLUE);
    }
  }
};

static Rock<ROCK_SIDES> spawnRandom() {
  auto &playerPos = store.getObj<Ship>().getPos();
  auto &playerVel = store.getObj<Ship>().getVel();
  Vector2 rockOffset = {std::max(WIDTH / 2, HEIGHT / 2), 0.0F};
  rockOffset =
      Vector2Rotate(rockOffset, ((float)rand() / (float)RAND_MAX) * PI * 2.0F);
  Vector2 vel =
      Vector2Add(playerVel, Vector2Scale(Vector2Normalize(rockOffset),
                                         -(float)GetRandomValue(150, 300)));

  return Rock<ROCK_SIDES>(Vector2Add(playerPos, rockOffset), vel,
                          GetRandomValue(-180, 180), 8);
}

int main(int argc, char *argv[]) {
  std::cout << "Hello Asteroids!" << '\n';
  std::cout << "Sizeof(Rock): " << sizeof(Rock<ROCK_SIDES>) << '\n';
  const static char *game_over_text = "     Game Over\n"
                                      "Press R to restart.";

  srand(time(0));
  InitWindow(WIDTH, HEIGHT, "Asteroids - Raylib");
  if (!IsWindowReady()) {
    fprintf(stderr, "Could not initiate window!\n");
    return 1;
  }
  SetTargetFPS(65);

  // Loading BG texture and shader
  // Texture2D bg = LoadTexture("assets/bg_grid_5.png");
  Texture2D bg = LoadTexture("assets/bg.png");
  Shader bg_shder = LoadShader(nullptr, "assets/bg.fs.glsl");

  int bg_shader_offset = GetShaderLocation(bg_shder, "offset");

  store.setObj(GameState());
  store.setObj(Ship());
  // FIXME: menu system need a lot of work...
  // store.setObj(false);
  store.setObj(std::vector<Bullet>());
  store.setObj(std::vector<Rock<ROCK_SIDES>>());

  store.getObj<std::vector<Rock<ROCK_SIDES>>>().push_back(
      Rock<ROCK_SIDES>({150, 150}, {25, 25}, 30, 8));

  Camera2D c = {0};
  c.zoom = 1.0f;
  c.offset = {WIDTH / 2.0f, HEIGHT / 2.0f};
  store.setObj(std::move(c));

  float last_spawn = 0;
  float spawn_interval = 5.0;
  char score_text[32];

  SetTextLineSpacing(26 + 14);
  auto game_over_width = MeasureText(game_over_text, 26);

  while (!WindowShouldClose()) {
    auto &camera = store.getObj<Camera2D>();
    auto &state = store.getObj<GameState>();

    if (!state.isRunning) {
      if (IsKeyReleased(KEY_R)) {
        store.setObj(Ship());
        // store.setObj(false);
        store.setObj(std::vector<Bullet>());
        store.setObj(std::vector<Rock<ROCK_SIDES>>());

        state.isRunning = true;
        state.score = 0;
      }
      ClearBackground(BLACK);
      BeginDrawing();
      DrawFPS(10, 10);
      sprintf(score_text, "Score: %d", state.score);
      DrawText(score_text, 150, 10, 22, WHITE);

      sprintf(score_text, "Highscore: %d", state.bestScore);
      DrawText(score_text, 150, 50, 26, WHITE);

      DrawText(game_over_text, (WIDTH - game_over_width) / 2, (HEIGHT - 26) / 2,
               26, RED);
      EndDrawing();
      continue;
    }

    // update
    auto dt = GetFrameTime();
    // ship.update(dt);
    store.getObj<Ship>().update(dt);
    camera.target = store.getObj<Ship>().getPos();
    auto &bullets = store.getObj<std::vector<Bullet>>();
    auto &rocks = store.getObj<std::vector<Rock<ROCK_SIDES>>>();

    last_spawn += dt;
    if (last_spawn >= spawn_interval) {
      last_spawn = 0;
      spawn_interval = std::max(spawn_interval - 0.1F, 1.0F);
      rocks.push_back(spawnRandom());
    }

    // NOTE: this might cause undefined behavior, I don't know
    for (auto b = bullets.rbegin(); b != bullets.rend(); b++) {
      if (!b->update(dt)) {
        std::swap(*b, bullets.back());
        bullets.pop_back();
      }
    }

    for (auto r = rocks.rbegin(); r != rocks.rend(); r++) {
      if (!r->update(dt)) {
        std::swap(*r, rocks.back());
        rocks.pop_back();
      }
    }

    // ClearBackground({0x30, 0x40, 0x50, 0xff});
    ClearBackground(BLACK);
    BeginDrawing();

    Vector2 bg_offset = Vector2Scale(camera.target, 1.0f / WIDTH);
    BeginShaderMode(bg_shder);
    SetShaderValue(bg_shder, bg_shader_offset, &bg_offset, SHADER_UNIFORM_VEC2);
    DrawTextureEx(bg, {0, 0}, 0, (float)WIDTH / (float)bg.width, SKYBLUE);
    EndShaderMode();

    DrawFPS(10, 10);

    sprintf(score_text, "Score: %d", state.score);
    DrawText(score_text, 150, 10, 18, WHITE);
    BeginMode2D(camera);

    store.getObj<Ship>().draw();
    for (auto &b : bullets) {
      b.draw();
    }

    for (auto &r : rocks) {
      r.draw();
    }

    EndMode2D();
    EndDrawing();
  }
  UnloadShader(bg_shder);
  UnloadTexture(bg);
  CloseWindow();
  return 0;
}
