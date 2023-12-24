#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <utility>
#include <vector>

#include "store.hpp"

static constexpr int WIDTH = 1000;
static constexpr int HEIGHT = 800;
static constexpr float bullet_speed = WIDTH / 2.0f;
static constexpr float rotationSpeed{2.0};
static constexpr float acc{100};
static constexpr int ROCK_SIDES = 36;

static Vector2 wrapToScreen(const Vector2 &v) {
  Vector2 pos = v;
  pos.x = pos.x > WIDTH ? 0 : pos.x;
  pos.x = pos.x < 0 ? WIDTH : pos.x;

  pos.y = pos.y > HEIGHT ? 0 : pos.y;
  pos.y = pos.y < 0 ? HEIGHT : pos.y;
  return pos;
}

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
  Vector2 shape[N];
  uint8_t size;
  float scale = 10.0;

  friend class Bullet;

public:
  Rock(Vector2 pos, Vector2 vel, int angularSpeer, uint8_t size)
      : pos(pos), vel(vel), angularSpeed(angularSpeer), size(size) {
    Vector2 unitVec = {0, 1};
    for (size_t i = 0; i < N; i++) {
      shape[i] = Vector2Scale(Vector2Rotate(unitVec, (2 * i * PI) / N),
                              GetRandomValue(85, 99) / 100.0);
    }
  }
  Rock() = default;
  Rock(const Rock &r) = default;
  // Rock(Rock &&r) = default;

  Rock &operator=(const Rock &o) {
    this->pos = o.pos;
    this->vel = o.vel;
    this->angularSpeed = o.angularSpeed;
    this->angle = o.angle;
    this->size = o.size;
    this->scale = o.scale;
    memcpy(this->shape, o.shape, N * sizeof(Vector2));
    return *this;
  }

  bool update(float dt) {
    angle += (DEG2RAD * angularSpeed) * dt;
    pos = wrapToScreen(Vector2Add(pos, Vector2Scale(vel, dt)));
    return true;
  }

  inline float getRadious() const { return this->size * this->scale; }
  inline const Vector2 &getPos() const { return pos; }

  void draw() const {
    Vector2 temp[N];
    for (size_t i = 0; i < N; i++) {
      temp[i] = Vector2Add(
          pos, Vector2Rotate(Vector2Scale(shape[i], scale * size), angle));
    }
    DrawLineStrip(temp, N, PINK);
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

          c1.vel = Vector2Subtract(pos, c1.pos);
          c2.vel = Vector2Scale(c1.vel, -1);
          rocks.push_back(c1);
          rocks.push_back(c2);
        }
        std::swap(rocks[i], rocks.back());
        rocks.pop_back();
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
  Vector2 pos;
  Vector2 forward, left, right;
  Vector2 v;
  bool thrust;
  float scale;

private:
  bool isCollision(const Rock<ROCK_SIDES> &r) const {
    return (r.getRadious() + scale) >=
           Vector2Length(Vector2Subtract(r.getPos(), pos));
  }

public:
  Ship() {
    pos = {WIDTH >> 1, HEIGHT >> 1};
    forward = {0, -1};
    left = {0.5, 0.5};
    right = {-0.5, 0.5};
    v = {0};
    thrust = false;
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
    pos = wrapToScreen(Vector2Add(pos, Vector2Scale(v, dt)));

    // fire bullets
    if (IsKeyReleased(KEY_SPACE)) {
      store.getObj<std::vector<Bullet>>().push_back(
          Bullet(pos, Vector2Scale(forward, bullet_speed)));
    }

    // checking collisions
    auto &rocks = store.getObj<std::vector<Rock<ROCK_SIDES>>>();
    for (size_t i = 0; i < rocks.size(); i++) {
      auto &r = rocks[i];
      if (isCollision(r)) {
        auto &d = store.getObj<bool>();
        d = true;
        printf("collision detected...\n");
        return false;
      }
    }
    return true;
  }

  const Vector2 &getPos() const { return this->pos; }

  void draw() const {
    auto fv = Vector2Add(pos, Vector2Scale(forward, scale));
    auto rv = Vector2Add(pos, Vector2Scale(right, scale));
    auto lv = Vector2Add(pos, Vector2Scale(left, scale));
    DrawTriangle(fv, rv, pos, WHITE);
    DrawTriangle(lv, fv, pos, WHITE);

    if (thrust) {
      DrawTriangle(rv, Vector2Add(pos, Vector2Scale(forward, -scale * 1.5f)),
                   lv, YELLOW);

      DrawTriangle(pos, rv, lv, SKYBLUE);
      DrawTriangle(rv, Vector2Add(pos, Vector2Scale(forward, -scale * 0.7)), lv,
                   SKYBLUE);
    }
  }
};

static Rock<ROCK_SIDES> spawnRandom() {
  auto &playerPos = store.getObj<Ship>().getPos();
  Vector2 rockOffset = {
      static_cast<float>(GetRandomValue(WIDTH / 4, WIDTH / 2)),
      static_cast<float>(GetRandomValue(HEIGHT / 4, HEIGHT / 2))};
  Vector2 vel = {static_cast<float>(GetRandomValue(30, 150)),
                 static_cast<float>(GetRandomValue(30, 150))};

  return Rock<ROCK_SIDES>(Vector2Add(playerPos, rockOffset), vel,
                          GetRandomValue(30, 70), 8);
}

int main(int argc, char *argv[]) {
  std::cout << "Hello Asteroids!" << std::endl;
  std::cout << "Sizeof(Rock): " << sizeof(Rock<ROCK_SIDES>) << std::endl;

  InitWindow(WIDTH, HEIGHT, "Asteroids - Raylib");
  if (!IsWindowReady()) {
    fprintf(stderr, "Could not initiate window!\n");
    return 1;
  }
  SetTargetFPS(65);

  store.setObj(Ship());
  // FIXME: menu system need a lot of work...
  store.setObj(false);
  store.setObj(std::vector<Bullet>());
  store.setObj(std::vector<Rock<ROCK_SIDES>>());

  store.getObj<std::vector<Rock<ROCK_SIDES>>>().push_back(
      Rock<ROCK_SIDES>({150, 150}, {25, 25}, 30, 8));

  float last_spawn = 0;
  float spawn_interval = 5.0;

  while (!WindowShouldClose()) {
    if (store.getObj<bool>()) {
      ClearBackground(BLACK);
      BeginDrawing();
      DrawFPS(10, 10);

      DrawText("Game Over", 100, 100, 26, RED);
      EndDrawing();
      continue;
    }

    // update
    auto dt = GetFrameTime();
    // ship.update(dt);
    store.getObj<Ship>().update(dt);
    auto &bullets = store.getObj<std::vector<Bullet>>();
    auto &rocks = store.getObj<std::vector<Rock<ROCK_SIDES>>>();

    last_spawn += dt;
    if (last_spawn >= spawn_interval) {
      last_spawn = 0;
      spawn_interval = std::max(spawn_interval - 0.1, 3.0);
      rocks.push_back(spawnRandom());
    }

    for (auto b = bullets.rbegin(); b != bullets.rend(); b++) {
      if (!b->update(dt)) {
        std::swap(*b, bullets.back());
        bullets.pop_back();
      }
    }

    for (auto &r : rocks) {
      r.update(dt);
    }

    ClearBackground(BLACK);
    BeginDrawing();
    DrawFPS(10, 10);

    store.getObj<Ship>().draw();
    for (auto &b : bullets) {
      b.draw();
    }

    for (auto &r : rocks) {
      r.draw();
    }

    EndDrawing();
  }
  CloseWindow();
  return 0;
}
