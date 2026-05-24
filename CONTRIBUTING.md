# Contributing to NumSim-CAS

Thanks for considering a contribution. NumSim-CAS is a foundational library for symbolic tensor calculus — small, focused changes with strong test coverage are preferred over sweeping refactors.

## Building & testing

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build
```

See [README.md](README.md) for the full set of CMake options.

## Code style

- **C++23**. The repo builds with GCC 14+, Clang 18+, and MSVC.
- **Strict warnings**. CI enforces `-Wall -Wextra -Wpedantic -Werror` (GCC/Clang) and `/WX` (MSVC). Submissions must compile cleanly.
- **clang-format**. Run before committing — CI verifies. Project style is in `.clang-format`.
- **clang-tidy**. Runs in CI; new violations are blocking.
- **No comments that describe what the code does**. Comments should explain *why*, not *what*. Identifiers carry the *what*.

## Branch + PR workflow

- One branch per issue. Branch naming: `<issue#>-<short-description>` (e.g. `142-rational-overflow`).
- Stack dependent PRs on each other rather than waiting for upstream merges. State the dependency explicitly in the PR description ("Stacked on #N — merge order: …").
- Iterate locally — do a self-review pass before opening the PR so reviewers see the polished version, not a work-in-progress.
- Force-push within your own branch is fine; force-push to shared branches (main) is not.

## Test discipline

- **Lock-in tests before refactoring**. Pin the current contract first, then change the implementation. A passing test suite after a refactor that started with lock-ins is a meaningful signal.
- **Structural assertions over numeric ones**. `EXPECT_TRUE(is_same<scalar_one>(eq(x, x)))` is cheaper to diagnose than `EXPECT_NEAR(...)`. Use both when both apply.
- **Typed tests for tensor work**. Cover dims 1, 2, 3 via `TYPED_TEST` to catch dim-dependent bugs (e.g. eye<T,D,4> identity ordering).
- **Bilateral CONTRACT pattern**. When a behaviour depends on two files (like the t2s pow ↔ div operator pair from #147), add CONTRACT NOTE comments at both sites with cross-references and pin the shape with explicit tests.

## Commit messages

- Imperative mood: "Add" / "Fix" / "Remove", not "Added" / "Fixed".
- Subject line under 72 characters.
- Body explains *why* the change is needed when non-obvious.
- Reference issues with `Closes #N` so GitHub auto-closes on merge.
- **No Co-Authored-By trailers.** This is project policy.

## License & contributor rights

NumSim-CAS is offered under a dual-license model (see [LICENSE](LICENSE) and [COMMERCIAL.md](COMMERCIAL.md)):

- GPL-3.0 for open-source use.
- Commercial license for proprietary embedding (FEM solvers, etc.).

To support this model, contributions must be offered under terms compatible with both licenses. We use the **Developer Certificate of Origin (DCO)** — a lightweight alternative to a formal CLA. By adding a `Signed-off-by:` trailer to your commit messages, you certify that you have the right to submit the work under the project's license:

```bash
git commit -s -m "Your commit message"
```

The `-s` flag adds the trailer automatically. See the [DCO text](https://developercertificate.org/) for the full certification.

**This is enforced in CI** (`.github/workflows/dco-check.yml`). PRs whose commits lack a `Signed-off-by:` trailer matching the author will fail the DCO check and cannot be merged. If you forgot, fix the history with `git rebase --signoff <base>` and force-push.

If you're contributing under your employer's name, ensure they're aware and have authorized the contribution under DCO terms.

## Issue triage

When opening an issue:

- **Bug**: include a minimal reproducer, expected vs actual behaviour, environment (compiler, OS, build mode).
- **Feature**: describe the user-facing need first, then propose the implementation if relevant. Architectural design discussions are welcome.
- **Discussion / design question**: tag the issue clearly so reviewers know not to treat it as a bug report.

## Questions

Open an issue or start a GitHub Discussion. For commercial-license inquiries see [COMMERCIAL.md](COMMERCIAL.md).
