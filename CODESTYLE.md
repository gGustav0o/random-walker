# C++ Code Style Guide

This style guide defines conventions used in this repository for C++20 development with Qt 6 and Eigen. It emphasizes functional purity, immutability, composability, and clarity.

---

## 1. General Principles

- Favor **immutability** and **pure functions**.
- Prefer **expressions** over statements.
- Avoid **side effects** wherever possible.
- Use **C++20 features**: `ranges`, `concepts`, `constexpr`, `modules`.
- Use **Doxygen** format for public APIs.
- All numerical output must be printed with fixed-point precision (default: `4` digits).

---

## 2. Naming Conventions

| Element            | Style                                    | Example                        |
| ------------------ | ---------------------------------------- | ------------------------------ |
| Classes            | `PascalCase`                             | `ImageFilter`, `SeedSelector`  |
| Functions          | `snake_case`                             | `compute_laplacian`, `solve()` |
| Function Variables | `snake_case`                             | `normalize`, `create_mask`     |
| Constants          | `kPascalCase`                            | `kMaxIterations`               |
| Regular Variables  | `snake_case`                             | `label_matrix`, `input_image`  |
| Namespaces         | `snake_case`                             | `math_utils`, `label_ops`      |
| Template Types     | `PascalCase`                             | `Transformer`, `Mapper`        |
| Files              | Based on main class or namespace in file | `GrayImage.hpp`, `segmenter.cpp` |
| Class Members      | `snake_case`, suffix `_` if mutable      | `buffer_`, `width`             |

---

## 3. Functions

- Prefer pure functions where they make dependencies and transformations
  explicit.
- Express each function property independently with standard C++ syntax:
  ```cpp
  [[nodiscard]] constexpr int add(int a, int b) noexcept {
      return a + b;
  }
  ```
- Apply `constexpr`, `noexcept`, `static`, and `[[nodiscard]]` only when each
  property is semantically correct for the function.
- Functional objects and stateless lambdas are encouraged:
  ```cpp
  constexpr auto square = [](int x) noexcept { return x * x; };
  ```

- Use `concepts` to constrain templates:
  ```cpp
  template <typename T>
  concept Arithmetic = std::is_arithmetic_v<T>;

  template <Arithmetic T>
  constexpr T double_value(T value) { return value * 2; }
  ```

---

## 4. Variables

- Use `const` by default.
- Avoid mutations; prefer value transformations.
- Use `auto` when the type is obvious or irrelevant semantically.

---

## 5. Composition and Ranges

Use `std::ranges` for functional-style transformations:

```cpp
auto result = input
            | std::views::filter(is_valid)
            | std::views::transform(normalize);
```

Compose custom utilities where needed:

```cpp
template <
    std::ranges::range R
    , typename Pred
>
auto filter(
    R&& r
    , Pred&& pred
) {
    return r | std::views::filter(std::forward<Pred>(pred));
}
```

---

## 6. Qt Integration

- Use `QStringView` instead of `QString` where possible.
- Keep Qt objects out of pure logic.
- Prefer `std::vector` over `QVector` unless Qt requires otherwise.
- Use signals/slots declaratively, avoid heavy logic inside slots.

---

## 7. Eigen Integration

- Treat Eigen expressions as lazy and pure.
- Avoid in-place modifications; return new expressions.
- Use `Map<const MatrixXd>` for efficient reference without copy.
- Combine with `std::ranges`:

```cpp
std::ranges::for_each(
    std::views::iota(0, vec.size())
    , [&](int i) {
        result[i] = transform(vec[i]);
    }
);
```

---

## 8. Error Handling

- Use `std::optional`, `std::variant`, or `std::expected`.
- Avoid exceptions for control flow.

---

## 9. Function and Type Annotations

Use standard C++ keywords and attributes directly:

```cpp
class Worker {
public:
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
};

[[nodiscard]] constexpr int answer() noexcept {
    return 42;
}
```

Do not introduce macros that combine unrelated properties such as linkage,
constant evaluation, exception guarantees, or optimization hints.

---

## 10. Directory Structure

- `model/` contains the mathematical domain, algorithms, and execution ports.
- `application/` contains application use cases and services.
- `infrastructure/` contains implementations of external persistence and
  platform ports.
- `presentation/` contains Qt adapters, image providers, and QML integration.
- `viewmodel/` contains MVVM presentation state and commands.
- `bootstrap/` contains the composition root and dependency wiring.
- Dependencies must point toward domain and application abstractions.

---

## 11. Formatting

- Indent with **4 spaces**.
- Opening braces on the **same line**.
- Use **leading commas** in all multiline lists: function parameters, template arguments, function calls, enums, and initializer lists.

**Example:**

```cpp
enum class Mode {
    Fast
    , Safe
    , Debug
};
```

- Group includes in the order:
    
    - related header (`.hpp`),
        
    - standard headers,
        
    - 3rd-party libraries (e.g. Eigen, Qt),
        
    - project headers.
        
- Indent with **4 spaces**.
    
- Opening braces on the **same line**.
    
- Use **leading commas** in all multiline lists: function parameters, template arguments, function calls, enums, and initializer lists.
    
- Group includes in the order:
    
    - related header (`.hpp`),
        
    - standard headers,
        
    - 3rd-party libraries (e.g. Eigen, Qt),
        
    - project headers.
---

## 12. Precision and Output

- Use `std::ostringstream` with `std::fixed` and `std::setprecision` when
  stream-based numeric formatting is appropriate.
- Keep formatting policy local to the responsible component:

```cpp
inline constexpr int kDefaultPrecision = 4;
```

---

## Example

```cpp
template <typename T>
[[nodiscard]] std::string toString(
    const Eigen::MatrixBase<T>& obj
    , int precision = kDefaultPrecision
) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision);
    oss << obj;
    return oss.str();
}
```

---
