#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <utility>

class IWrapper {
public:
  virtual ~IWrapper() = default;
};

template <typename T> class Wrapper : public IWrapper {
  // private:
public:
  T obj;

public:
  Wrapper(T &&t) : obj(std::move(t)) {}
};

class GameStore {
private:
  std::unordered_map<const char *, std::shared_ptr<IWrapper>> objects{};

public:
  template <typename T> T &getObj() {
    auto tname = typeid(T).name();
    assert(objects.find(tname) != objects.end() && "Object not added");
    return std::static_pointer_cast<Wrapper<T>>(objects[tname])->obj;
  }
  template <typename T> void setObj(T &&o) {
    auto tName = typeid(T).name();
    objects.insert_or_assign(tName, std::make_shared<Wrapper<T>>(std::forward<T>(o)));
  }
};
