# Cleanup Plan — Items 1-9

Items from REVIEW.md section 20, excluding #10 (profiling) and #12 (arena allocator).
Item #5 (`CONFIGURE_DEPENDS`) is already done — `CMakeLists.txt:56,63` already use it.

---

## 1. Rename `umap` alias

**Problem:** `umap` is an alias for `std::map` (ordered), not `std::unordered_map`.

**Definition:** `include/numsim_cas/numsim_cas_type_traits.h:111-112`
```cpp
template <typename ExprType> using expr_ordered_map = std::map<ExprType, ExprType>;
template <typename ExprType> using umap = expr_ordered_map<ExprType>;
```

**Plan:** Delete the `umap` alias. Replace all usages with `expr_ordered_map`.

**Usages (5 total):**
| File | Line | Usage |
|------|------|-------|
| `core/n_ary_tree.h` | 22 | `typename umap<expr_t>::iterator` |
| `core/n_ary_tree.h` | 23 | `typename umap<expr_holder_t>::const_iterator` |
| `core/n_ary_tree.h` | 138 | `umap<expr_holder_t> m_symbol_map` |
| `core/n_ary_vector.h` | 18 | `typename umap<expr_holder_t>::iterator` |
| `core/n_ary_vector.h` | 19 | `typename umap<expr_holder_t>::const_iterator` |

**Changes:**
- `numsim_cas_type_traits.h:112` — delete the `umap` alias line
- `core/n_ary_tree.h:22,23,138` — `umap<T>` → `expr_ordered_map<T>` (3 replacements)
- `core/n_ary_vector.h:18,19` — `umap<T>` → `expr_ordered_map<T>` (2 replacements)

---

## 2. Delete all commented-out code

### 2a. `numsim_cas.h` — strip to active includes only

**File:** `include/numsim_cas/numsim_cas.h` (253 lines, 74% commented)

Delete all commented-out blocks:
- Lines 4-91: TODO/design notes block
- Lines 93-96: old general includes
- Lines 143-173: old tensor includes
- Lines 180-196: old t2s includes
- Lines 197-252: old namespace with dead functions

Keep only the `#ifndef` guard and active includes (lines 1-3, 99-133, 175-179, 253).
Result: ~40 lines.

### 2b. `tensor_to_scalar_operators.h` — delete old operator_overload structs

**File:** `include/numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h` (401 lines, 62% commented)

Delete:
- Lines 150-235: old `binary_*_simplify()` functions
- Lines 237-399: old `operator_overload` specializations

Keep lines 1-148 (active tag_invoke code) + closing guard.

### 2c. `tensor_operators.h` — delete old operator_overload structs

**File:** `include/numsim_cas/tensor/tensor_operators.h` (223 lines, 40% commented)

Delete:
- Lines 132-221: old `operator_overload` specializations

Keep lines 1-131 (active tag_invoke code) + closing guard.

### 2d. `numsim_cas_type_traits.h` — delete variant-era dead code

**File:** `include/numsim_cas/numsim_cas_type_traits.h` (629 lines, ~200 commented)

Delete these commented blocks:
- Lines 14-30: old compiler detection `SYMTM_USE_VARIANT`/`SYMTM_USE_POLY`
- Lines 58-62: old boost variant aliases
- Lines 68-72: old `get_visitor` stub
- Lines 76-92: old `variant_wrapper` class
- Lines 115-120: old `visit` wrapper
- Lines 151-155: old `make_expression` template
- Lines 161-176: old `tensor_node` variant typedef
- Lines 188: `// class scalar_div;` comment
- Lines 213-227: old `scalar_node` variant typedef
- Lines 239-245: old t2s class forward declarations (commented-out ones)
- Lines 253-267: old `tensor_to_scalar_node` variant typedef
- Lines 269-282: old poly visitor typedefs
- Lines 288-313: old `expression_holder_data_type` variant specializations
- Lines 315-320: old `get_hash_value` struct
- Lines 328-331: old `get_type` specialization for variant_wrapper
- Lines 378-416: old `variant_index` alternate implementations
- Lines 420-423: old `variant_index_inner_loop`
- Lines 432-439: old `variant_index` conditional implementation
- Lines 447-466: old `make_expression` / `make_trigonometric_expression`
- Lines 468-512: old `base_expression` promotion rules

Also delete the now-unnecessary includes: `<variant>` (if no longer used after cleanup), `<unordered_map>` (never used).

### 2e. `tensor_to_scalar_node_list.h` — delete commented nodes

**File:** `include/numsim_cas/tensor_to_scalar/tensor_to_scalar_node_list.h`

Delete:
- Lines 4-7: commented forward declarations
- Lines 23-25: commented `NEXT()` entries

### 2f. `tensor_node_list.h` — delete commented nodes

**File:** `include/numsim_cas/tensor/tensor_node_list.h`

Delete:
- Lines 29-32: commented `NEXT()` entries (`tensor_scalar_div`, `tensor_to_scalar_with_tensor_div`)

Keep lines 4-9 (documentation comments about tensor fundamentals — these are useful).

### 2g. `scalar_binary_simplify_fwd.h` — delete commented declarations

**File:** `include/numsim_cas/scalar/scalar_binary_simplify_fwd.h`

Delete:
- Lines 7-21: four commented-out function declarations

Keep only the active `binary_scalar_pow_simplify` declaration.

### 2h. Delete orphaned `scalar_binary_simplify_imp.h`

**File:** `include/numsim_cas/scalar/scalar_binary_simplify_imp.h`

This file is never included anywhere. The definition it provided is now in
`src/numsim_cas/scalar/scalar_binary_simplify.cpp`. Delete the entire file.

---

## 3. Add warning flags to library target

**File:** `CMakeLists.txt`

Add after `target_compile_definitions` block (~line 108):

```cmake
# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wno-comment -Wno-overloaded-virtual)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()
```

Note: uses the same `-Wno-comment -Wno-overloaded-virtual` exceptions as the test
target (tests/CMakeLists.txt:53), since these are known in the codebase. Does NOT
add `-Werror` to the library target — that would break downstream builds on
compiler upgrades. `-Werror` stays tests-only.

**Verification:** `cmake --build build -j$(nproc)` must still compile cleanly.
If new warnings appear, fix them as part of this step.

---

## 4. Fix `push_back(&&)` to actually move

**File:** `include/numsim_cas/core/n_ary_tree.h:50-52`

Current:
```cpp
inline void push_back(expression_holder<expr_t> &&expr) {
    insert_hash(expr);  // passes lvalue — copies
}
```

Fix:
```cpp
inline void push_back(expression_holder<expr_t> &&expr) {
    insert_hash(std::move(expr));
}
```

Also fix `insert_hash` to move into the map. Current (line 141-148):
```cpp
template <typename T> void insert_hash(T const &expr) {
    if (m_symbol_map.find(expr) != m_symbol_map.end()) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map[expr] = expr;
    update_hash_value();
}
```

Need two overloads (or a forwarding reference):
```cpp
void insert_hash(expression_holder<expr_t> const &expr) {
    if (m_symbol_map.find(expr) != m_symbol_map.end()) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map[expr] = expr;
    update_hash_value();
}

void insert_hash(expression_holder<expr_t> &&expr) {
    if (m_symbol_map.find(expr) != m_symbol_map.end()) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    auto key = expr;  // copy key (map needs it for lookup)
    m_symbol_map[key] = std::move(expr);
    update_hash_value();
}
```

Note: `std::map::operator[]` needs a key, and after `std::move(expr)` the key may
be invalid, so we copy the key first, then move the value.

Actually, simpler approach — since `expression_holder` wraps `shared_ptr`, the move
is just nulling the source. The map key and value are both `expression_holder`, so:

```cpp
template <typename T> void insert_hash(T &&expr) {
    using bare = std::remove_cvref_t<T>;
    static_assert(std::is_same_v<bare, expression_holder<expr_t>>);
    if (m_symbol_map.find(expr) != m_symbol_map.end()) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    if constexpr (std::is_rvalue_reference_v<T&&>) {
        auto key = expr;  // copy for map key
        m_symbol_map[key] = std::move(expr);
    } else {
        m_symbol_map[expr] = expr;
    }
    update_hash_value();
}
```

Simplest correct approach: just keep the existing `T const&` overload and add a
move overload. The map stores `{expr, expr}` (key=value), so for the rvalue case
we need to keep a copy for the key before moving:

```cpp
void insert_hash(expression_holder<expr_t> const &expr) {
    if (m_symbol_map.find(expr) != m_symbol_map.end()) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map[expr] = expr;
    update_hash_value();
}

void insert_hash(expression_holder<expr_t> &&expr) {
    if (m_symbol_map.find(expr) != m_symbol_map.end()) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    auto key = expr;
    m_symbol_map[std::move(key)] = std::move(expr);
    update_hash_value();
}
```

Wait — after `std::move(expr)` into the map value, `expr` is moved-from. And `key`
is a separate copy. But `std::move(key)` into the map key also leaves `key`
moved-from. This is fine because both the key and value are now owned by the map.

Actually even simpler: since the key and value in `umap<expr_holder_t>` (which is
`std::map<expr_holder_t, expr_holder_t>`) must both be valid, and we want the map to
own the expression, just do:

```cpp
void insert_hash(expression_holder<expr_t> const &expr) {
    if (m_symbol_map.contains(expr)) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map.emplace(expr, expr);
    update_hash_value();
}

void insert_hash(expression_holder<expr_t> &&expr) {
    if (m_symbol_map.contains(expr)) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    // expr wraps shared_ptr, so copy is cheap (one atomic inc).
    // We need the key to remain valid; move only the value.
    auto value = std::move(expr);
    m_symbol_map.emplace(value, std::move(value));
    update_hash_value();
}
```

Hmm, that moves `value` twice. Let's keep it simple — the `shared_ptr` copy is an
atomic increment. The improvement from moving is saving one atomic inc+dec on the
value side. Not worth over-engineering. Final approach:

```cpp
void insert_hash(expression_holder<expr_t> const &expr) {
    if (m_symbol_map.contains(expr)) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map[expr] = expr;
    update_hash_value();
}

void insert_hash(expression_holder<expr_t> &&expr) {
    if (m_symbol_map.contains(expr)) {
        throw internal_error("n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map[expr] = std::move(expr);
    update_hash_value();
}
```

The key `expr` is read before the move (as the map subscript), and only the value
assignment uses `std::move`. This saves one atomic ref-count pair per rvalue insert.
Also modernize `find() != end()` → `contains()` (C++20).

---

## 5. ~~Add `CONFIGURE_DEPENDS` to `GLOB_RECURSE`~~ — ALREADY DONE

`CMakeLists.txt:56` and `:63` already have `CONFIGURE_DEPENDS`. No action needed.

---

## 6. Delete stub simplifier files

Delete these files (confirmed: no includes anywhere in the codebase):

- `include/numsim_cas/tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_div.h`
- `include/numsim_cas/tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_mul.h`
- `include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_div.h` (203 lines, 100% commented out)

---

## 7. Establish header/source split

The only remaining `_imp.h` pattern issue is the orphaned file handled in 2h.

After deleting `scalar_binary_simplify_imp.h` (step 2h), the codebase has:
- `scalar_binary_simplify_fwd.h` — declaration (kept, cleaned in 2g)
- `scalar_binary_simplify.cpp` — definition (already exists, created last session)
- `tensor_data_make_imp.h` — legitimate template code, no change needed

No further header/source split work needed beyond what step 2 covers.

---

## 8. Constrain forwarding constructor on `scalar_expression`

**File:** `include/numsim_cas/scalar/scalar_expression.h:16-17`

Current:
```cpp
template <typename... Args>
scalar_expression(Args &&...args) : expression(std::forward<Args>(args)...) {}
```

Replace with:
```cpp
template <typename... Args>
requires(sizeof...(Args) == 0 ||
         (sizeof...(Args) != 1 ||
          !std::is_same_v<std::remove_cvref_t<Args>..., scalar_expression>))
scalar_expression(Args &&...args) : expression(std::forward<Args>(args)...) {}
```

This prevents the forwarding constructor from matching `scalar_expression` lvalues,
which should use the copy constructor instead.

Note: the `n_ary_tree` forwarding constructors (lines 27-42) always take additional
`n_ary_tree` or non-Args parameters, so they don't have the same ambiguity problem.
No change needed there.

---

## 9. Add sanitizer build option

**File:** `CMakeLists.txt`

Add after the `option(BUILD_BENCHMARK ...)` line (~line 36):

```cmake
option(NUMSIM_CAS_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
```

Add after the library warning flags block (after step 3's addition):

```cmake
if(NUMSIM_CAS_SANITIZERS)
    target_compile_options(${PROJECT_NAME} PUBLIC
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PUBLIC
        -fsanitize=address,undefined)
endif()
```

Uses `PUBLIC` so that tests and examples linked against the library also get
sanitizer flags. This is intentional — sanitizers must be consistent across all
TUs in a binary.

---

## Execution Order

1. **Item 4** — fix `push_back(&&)` move (2 files, small diff)
2. **Item 1** — rename `umap` (3 files, mechanical)
3. **Item 8** — constrain forwarding constructor (1 file, 1 change)
4. **Item 2** — delete commented-out code (8 files + 1 file deletion)
5. **Item 6** — delete stub files (3 file deletions)
6. **Item 3** — add warning flags (1 file) + fix any new warnings
7. **Item 9** — add sanitizer option (1 file)

Build and test after each step.

---

## Verification

After all changes:
```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Expected: all 406 tests pass, all 5 examples build and run.
