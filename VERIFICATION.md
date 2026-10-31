# Rational Studio — Verification Report

<!-- ISSUE50_VERIFICATION_CURRENT_BEGIN -->

**Project version:** 0.1.0  
**Verification date:** October 30, 2026
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS — Issue #50 (M4-05), matrix and vector operations.** Linux GUI Debug **59/59 CTests PASS** (18.37 s), GUI Release **59/59 PASS** (12.37 s), core-only ASan/UBSan **38/38 PASS** (9.11 s); **692 MatrixMath assertions PASS** in each configuration. 141 matching source/documentation provenance entries; zero GUI compiler warnings. Manual GUI not required (core-only API). Matrix solves reject singular and poorly conditioned systems using numerical diagnostics, not certified bounds. Native Windows, Qt 5, and Qt GUI sanitizers remain unverified.

This document records Issue #44 pointer tracing from completed geometry for Cartesian, parametric, polar, implicit and inequality boundaries. Optional approximate significant-point markers are tied to the existing Features action and invalidated with stale analysis; parameter and signed-radius readouts are approximate. This adds a Qt-independent nearest-segment helper and sampled parameter provenance, not new rendering geometry or a workspace version. Previous Issue #43 and earlier results remain historical.

This document records Issue #43 user-interface controls for five graph representations. The two-column creation grid, collapsible syntax guide, consistent type badges, keyboard focus and vertical interval fields preserve mathematical compilation, rendering, caching and version-3 persistence. Offscreen Qt tests cover mixed-type creation and Save/Open. The evidence does not prove all small-window sizes, accessibility toolchains or native Windows behavior. Issue #42 and earlier qualifications remain historical.

This document records Issue #42 sampled vertical Cartesian inequality shading using a Qt-independent comparison normalizer, clipped vertical spans and bounded/cached Qt region rasters. Existing Cartesian path geometry supplies the strict/dashed or inclusive/solid boundary. Mixed version-3 persistence and transactional validation are preserved, and Issue #41 and earlier results remain historical evidence. The automated results do not certify arbitrary implicit inequalities or mathematical continuity.

This document records Issue #41 bounded numerical feature inspection for explicit Cartesian functions. The Qt-free service reports roots, local minima/maxima and heuristic stationary inflection candidates through the existing MainWindow graphing workflow. The results have explicit tolerances, finite sampling and incompleteness conditions; other representations remain outside this action. Previous Issue #40 and earlier qualifications are retained as historical evidence.

This document records Issue #40 finite-domain numerical intersection detection for pairs of explicit Cartesian functions. The Qt-independent IntersectionDetection service uses bounded sign-change refinement and sampled tangency heuristics with configurable residual tolerances, callback limits, result caps, and partial/coincident status. MainWindow integrates an Intersections action for valid visible Cartesian pairs in the current X-domain. Parametric, polar, and implicit pair detection are outside this implementation. Issue #39 and all earlier qualifications remain historical, including the original Issue #25 benchmark measurements. Automated evidence and developer-reported manual GUI acceptance are distinguished.

## Issue #50 current matrix and vector operations qualification

Issue #50 adds a real-valued, Qt-independent `MatrixMath` service with bounded
matrix/vector construction, arithmetic, multiplication, transpose, determinant,
and a partially pivoted LU solver. The solver reports a reciprocal-condition
estimate and checks backward residual before publishing a finite solution.
Matrices, vectors, and complex scalars remain separate mathematical types;
this issue adds no GUI, general expression syntax, or persistence changes.

- GUI-enabled Linux Debug: **59/59 CTests PASS** — 18.37 s.
- GUI-enabled Linux Release: **59/59 CTests PASS** — 12.37 s.
- Core-only ASan/UBSan: **38/38 CTests PASS** — 9.11 s.
- Dedicated `RationalMatrixMathTests`: **692 assertions PASS** in each configuration.
- Recorded provenance: **141 matching paths**, manifest SHA-256 `975b2623638cc3d31b8ff63e7a78783a3fc4c75452ede4f3664868fc0c7477e3`.
- Recorded GUI compiler warnings: **zero**.
- Manual GUI inspection: not required for core-only M4-05.

Coverage includes dimension and overflow rejection, rectangular matrix products,
vector dot/norm operations, transpose, determinants, pivoted systems, singular
and ill-conditioned inputs, and 160 deterministic reference systems.

**CTest evidence SHA-256:** Debug `f2b923a70bfddc8b916fbc37f60fabfbd95cca948aca88d82530ea6d8f7f6865`; Release
`0318f640610460358b1a3667ba040d37de96390d070574bc9096e2283e392e6e`; ASan `23e368de6a18efdb720198a06d38207535f525d5737b21d68ad50ed3743722dc`.

**Recorded toolchain:** c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0; cmake version 4.2.3; 1.13.2; base HEAD `a4c5101f7423a5bcd2eae4a7ad5d21f73aa1a529`;
qualification timestamp `2026-10-30T20:53:52-04:00`.

All prior Issue #49 and older qualifications remain historical below.

## Issue #49 historical complex-number arithmetic qualification

Issue #49 adds a Qt-independent `ComplexMath` service, a separately compiled
`ComplexExpression` with imaginary literals and typed complex results, and a
real-to-complex compatibility bridge. Supported principal-branch arithmetic,
functions, formatting, and RAD/DEG behavior are validated without modifying
existing `Expression::evaluate(double)`, Qt graph rendering or persistence.
Complex-valued reusable user functions and full scalar/complex AST unification
remain outside this initial core-only feature.

- Linux GUI Debug: **58/58 CTests PASS** — 18.68 s.
- Linux GUI Release: **58/58 CTests PASS** — 12.21 s.
- Core-only ASan/UBSan: **37/37 CTests PASS** — 8.65 s.
- Dedicated ComplexMath: **539 assertions PASS** in each configuration.
- Provenance: **135 matching paths**; manifest SHA-256 `6c1d70b9f76173c376cd6bac3858e5243302a7e973c6f7cbbe1e4716c7f25125`.
- GUI compiler warnings: zero recorded.
- Manual GUI verification: not required (core-only feature).

The regression exercises arithmetic and principal branches, complex argument
and conjugation, trigonometric degree/radian semantics, rectangular formatting,
explicit nonfinite/domain failures, scalar binding snapshots, compiled real
function compatibility, real-expression promotion and 120 fixed-seed pairs of
exact integer-based arithmetic oracles.

**CTest log SHA-256:** Debug `2bdba662ea3e7d4ea477c0334ac8edf71f92cf9abd403bef3d6b7fbb2bcbbcd6`; Release
`a1a388a4a342387d411d695acd0f7a8713fc150c65e24a04fc6e7951ca6f39e8`; ASan `22d681ddc95bee6ea90dfc89ec0d1ce7c35337d79281e80b89e9463871fb8f07`.

**Recorded environment:** c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0; cmake version 4.2.3; 1.13.2; base HEAD `b3d3fb1118752750ba3e10baff08d485deab1329`;
qualification timestamp `2026-10-30T20:34:29-04:00`.

Issue #48 and earlier qualifications below remain historical.

## Issue #48 historical numerical equation and root-finding qualification

Issue #48 adds a Qt-independent bracket-preserving secant/bisection root solver,
a finite-domain sign-change scanner, and overloads for compiled expressions and
left/right equation solving. Refinement requires an actual residual-qualified
candidate: bracket width alone never certifies a root. Resource limits bound
iterations, evaluations and scan results; invalid domains and exhausted search
budgets are explicit statuses. Tangential roots away from sample points and
unobserved discontinuities can be missed. No parser/GUI/persistence changes.

- Linux GUI Debug: **57/57 CTests PASS** — 18.83 s.
- Linux GUI Release: **57/57 CTests PASS** — 12.33 s.
- Core-only ASan/UBSan: **36/36 CTests PASS** — 8.69 s.
- Dedicated NumericalRootFinding: **512 assertions PASS** in each configuration.
- Recorded matching source/documentation paths: **129**;
  manifest SHA-256 `cfb5850474514928625759f488c2f300102d8bd009fe93c95da28b5103073a4f`.
- GUI compiler warnings: **zero recorded**.
- Manual GUI: not required for core-only M4-03.
- Toolchain: c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0; cmake version 4.2.3; 1.13.2; base HEAD `96c70d757da4d04bf70a11e3a0cb951bae8b6d74`;
  qualification timestamp `2026-10-30T16:53:29-04:00`.

Coverage includes analytical roots, bracket endpoints, discontinuous poles,
resource-limit statuses, domain scans, compiled equations, RAD/DEG and `ans`,
scalar bindings, user-defined functions, and 200 fixed-seed affine targets.

**CTest log SHA-256:** Debug `ea997731719bae1391a05238aa86e1d24e7dc02423e2b731d21fcfedc27f35c3`;
Release `a73d60d8c042ad87532c0869539fe6b46bb32a6a9e6ce5cad81dde02014e3732`; ASan `c50dcde071b6442c7d90ab8596f305bad9117c7f2d922f8735dd5f6a3ff427da`.

Issue #47 and all earlier results below remain historical.

## Issue #47 historical numerical integration qualification

Issue #47 introduces a Qt-independent adaptive embedded Gauss–Kronrod 7/15
quadrature service with bounded evaluation and subdivision counts, explicit
convergence/invalid-domain statuses, and no modification of GUI, graph renderer,
expression grammar, or workspace persistence. The embedded error estimate is an
uncertified convergence heuristic; difficult singular or oscillatory functions
may fail or produce misleading estimates despite finite sampling.

- Linux GUI Debug: **56/56 CTests PASS** — 18.06 s.
- Linux GUI Release: **56/56 CTests PASS** — 12.08 s.
- Core-only ASan/UBSan: **35/35 CTests PASS** — 8.57 s.
- Dedicated NumericalIntegration: **331 assertions PASS** in each configuration.
- Provenance: **123 matching source/documentation paths**; manifest SHA-256 `4e3697d304c068bd01461dce99c160d880c0b3231deaa18a1004ad3f76e34b91`.
- Compiler warnings: **zero recorded in GUI Debug/Release builds**.
- Manual GUI: not required for the core-only M4-02 implementation.

Coverage includes analytical smooth integrals, reversed and zero-width limits,
integrable endpoint singularities, bounded oscillation and discontinuity cases,
invalid domains, callback exceptions, evaluation/subdivision/depth limits,
RAD/DEG evaluation, scalar bindings, reusable user functions, `ans`, and a
fixed-seed 240-case polynomial reference matrix.

**CTest log SHA-256:** Debug `d61f8c815693ac2bb381c3ed9c7ec0336a2d2a33f4d23f08886f925bd7cce49a`; Release
`3be0558e2b0bec4c6b6d6805c4914cd62ebf97c2d6f05bd420b3a5b9c4195155`; ASan `837731f5385b5442f7de2a7126b5244adb8dd081f199eb44c7ea0dcd82922a0e`.

**Recorded toolchain:** c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0; cmake version 4.2.3; 1.13.2; pre-qualification base HEAD `009d74e8484e98640eda4e838aace58920271436`.

Issue #46 and earlier qualifications below remain historical.

## Issue #46 historical numerical differentiation qualification

Issue #46 adds a Qt-independent adaptive finite-difference service with central Richardson extrapolation,
optional one-sided three-point derivatives, explicit convergence/error/evaluation diagnostics, and bounded
work. The one-sided result is not a claim that a two-sided derivative exists. Cusp/jump filtering is heuristic.
No expression grammar, GUI widget, graph renderer or workspace schema changed.

- Linux GUI Debug: **55/55 CTests PASS** — 18.31 s.
- Linux GUI Release: **55/55 CTests PASS** — 12.22 s.
- Qt-free ASan/UBSan: **34/34 CTests PASS** — 8.43 s.
- Dedicated derivative suite: **338 assertions PASS** each in Debug, Release, and ASan/UBSan.
- Provenance: **118 matching files**, manifest SHA-256 `9dcb97e8580d7e68b59f7cbeaf28d2cb43240e89d0832e0e0dfacdcb0daf359d`.
- Manual GUI: not required for core-only M4-01; no new GUI controls.

Regression coverage includes smooth analytic derivatives, representative finite-domain one-sided estimates,
non-differentiable candidates, missing/nonfinite inputs, strict callback budgets, RAD/DEG semantics,
variable binding snapshots, nested reusable functions, and a fixed-seed 240-case analytical matrix.

**CTest log SHA-256:** Debug `91ccb65edd4bb74d96fe64e5fc3ce6d7a6ceaf1aa6ce0384d28a391220994a93`; Release `74336c48bfbc281fa29978e4f666d3e3fb19a1820e3e3dd73e7a321da4e5dbb8`;
ASan `ea0efed39e9abd2441e895492fa9771a348bc10c4e7a82b12357816ee4fe52df`. **Dedicated test log SHA-256:** Debug
`7019144c16c26e41de36076f1a5cadccc812381c7a2117a33cc8857af9f0b344`; Release `7019144c16c26e41de36076f1a5cadccc812381c7a2117a33cc8857af9f0b344`; ASan `7019144c16c26e41de36076f1a5cadccc812381c7a2117a33cc8857af9f0b344`.

**Recorded toolchain:** c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2; pre-qualification base HEAD `529c7b9fc2a3aeaf85d69ef83570268b4ce0bdd5`.
Passing finite numerical reference tests does not certify differentiation for arbitrary functions.

Issue #45 and all earlier qualifications below remain historical.

## Issue #45 historical advanced-graph qualification

Issue #45 adds two deterministic CTests without modifying the mathematical grammar, rendering pipeline or workspace format. The reference matrix checks sampled Cartesian, parametric, signed-radius polar, implicit, vertical inequality, feature and trace results against explicit analytical oracles. Resource tests exercise callback ceilings, domain gaps and nonfinite inputs. The earlier GUI integration suites remain part of the complete 54-test Linux desktop matrix. These are finite reference cases, not a proof of mathematical completeness or certified topology.

- Debug: **54/54 CTests PASS**, 18.37 s.
- Release: **54/54 CTests PASS**, 12.18 s.
- Core-only ASan/UBSan: **33/33 CTests PASS**, 8.53 s.
- Dedicated accuracy matrix: **836 Debug, **836 Release, **836 ASan assertions PASS.
- Dedicated resource matrix: **21 Debug, **21 Release, **21 ASan assertions PASS.
- Source/documentation provenance: **111 matching paths**, SHA-256 `34f08dd4abfff4cec8f35604d0b86851426f8c81d13bebc57a577bccffe951aa`.
- Developer-reported interactive acceptance: **PASS — developer-reported Issue #45 cross-representation desktop inspection; not separately instrumented in Release.**

**Advisory Release benchmark observations** (milliseconds; warmups excluded):

| Workload | Median (ms) | p95 (ms) | Maximum (ms) | Runs |
|---|---:|---:|---:|---:|
| Qt-independent · `cartesian_sine` | 0.038671 | 0.039601 | 0.048161 | 21 |
| Qt-independent · `parametric_circle` | 0.026391 | 0.026481 | 0.026520 | 21 |
| Qt-independent · `polar_rose` | 0.081272 | 0.082301 | 0.099281 | 21 |
| Qt-independent · `implicit_circle` | 0.228434 | 0.360067 | 0.370797 | 21 |
| Qt-independent · `inequality_boundary_and_spans` | 0.008540 | 0.013560 | 0.014301 | 21 |
| Qt-independent · `trace_completed_geometry` | 0.143153 | 0.205144 | 0.205444 | 21 |
| Qt offscreen · `qt_cartesian_cold_to_ready` | 1.304376 | 2.272574 | 2.272574 | 7 |
| Qt offscreen · `qt_cartesian_warm_repaint` | 0.223974 | 0.272455 | 0.272455 | 7 |
| Qt offscreen · `qt_parametric_cold_to_ready` | 1.839155 | 1.941168 | 1.941168 | 7 |
| Qt offscreen · `qt_parametric_warm_repaint` | 0.220864 | 0.357027 | 0.357027 | 7 |
| Qt offscreen · `qt_polar_cold_to_ready` | 1.941547 | 2.472838 | 2.472838 | 7 |
| Qt offscreen · `qt_polar_warm_repaint` | 0.226024 | 0.343766 | 0.343766 | 7 |
| Qt offscreen · `qt_implicit_cold_to_ready` | 23.042405 | 23.463973 | 23.463973 | 7 |
| Qt offscreen · `qt_implicit_warm_repaint` | 0.328566 | 0.342476 | 0.342476 | 7 |
| Qt offscreen · `qt_inequality_cold_to_ready` | 3.714312 | 4.134970 | 4.134970 | 7 |
| Qt offscreen · `qt_inequality_warm_repaint` | 0.250034 | 0.299046 | 0.299046 | 7 |

The Qt-independent inequality workload measures boundary evaluation and vertical span classification; Qt offscreen workloads separately measure shaded raster build and warm repaint. Cold-to-ready timings include event-loop scheduling and GUI painting, while the mathematical sampling timings exclude on-screen display. Host temperature, CPU frequency, Qt backend, and scheduling can affect these results. No performance improvement or universal frame-rate promise is established.

Evidence timestamp: `2026-10-30T15:12:55-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; base HEAD `b5ae3338709697512a5ba01a630d80201e39f0cf` plus recorded source provenance.

Previous Issue #44 and earlier qualifications below are historical records.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Current Issue #45 application builds | Linux GCC GUI Debug **54/54** CTests, GUI Release **54/54** CTests, core ASan/UBSan **33/33** CTests; 111 matching source paths; zero GUI compiler warnings; optional Qt/core Release benchmark measurements recorded |
| Historical Issue #44 application builds | Linux GCC 15.2.0 GUI Debug **205** Ninja steps (desktop executable linked during recorded build), Release **225** (desktop executable linked during recorded build), core ASan/UBSan **78**; **52/52**, **52/52**, **31/31 CTests PASS**; **97** matching provenance paths; zero GUI warnings |
| Historical Issue #43 application builds | Linux GCC 15.2.0 GUI Debug **180** Ninja steps (desktop executable linked during recorded build), Release **217** (desktop executable linked during recorded build), core ASan/UBSan **75** Ninja steps; **50/50**, **50/50**, **30/30 CTests PASS**; **93** matching provenance paths; zero recorded GUI warnings |
| Historical Issue #42 application builds | Linux GCC 15.2.0 GUI Debug **178/178** Ninja steps (incremental build; existing Linux desktop executable verified, but its link was not repeated in the recorded build), Release **209/209** (desktop executable linked in recorded build), core ASan/UBSan **75/75** Ninja steps; **49/49**, **49/49**, **30/30 CTests PASS**; **92** matching provenance entries; zero GUI compiler warnings |
| Historical Issue #41 application builds | Linux GCC 15.2.0 GUI Debug **198/198**, Release **198/198**, core ASan/UBSan **72/72** Ninja steps; **47/47**, **47/47**, **29/29 CTests PASS**; **88** matching provenance entries; zero GUI compiler warnings |
| Historical Issue #40 application builds | GCC 15.2.0 Linux GUI Debug and Release **187/187** steps each; core-only ASan/UBSan **69/69** steps; **45/45**, **45/45**, **28/28 CTests PASS**; **84** matching provenance entries; zero recorded Debug/Release compiler warnings |
| Historical Issue #39 application builds | GCC 15.2.0 Linux GUI Debug and Release **176/176** steps each; core-only ASan/UBSan **66/66** steps; **43/43**, **43/43**, **27/27 CTests PASS**; **80** matching provenance entries; zero recorded Debug/Release compiler warnings |
| Historical Issue #38 application builds | GCC 15.2.0 Linux GUI Debug and Release **165/165** steps each; core-only ASan/UBSan **63/63** steps; **41/41**, **41/41**, **26/26 CTests PASS**; **76** matching provenance entries; zero recorded Debug/Release compiler warnings |
| Historical Issue #37 application builds | GCC 15.2.0 Linux GUI-enabled Debug and Release **155/155** steps each; core-only ASan/UBSan **61/61** steps; **39/39**, **39/39**, and **25/25 CTests PASS**; **73** matching provenance entries; zero recorded Debug/Release warnings |
| Historical Issue #36 application builds | Linux GCC 15.2.0 GUI Debug and Release **144/144** steps each; GUI-disabled ASan/UBSan **58/58** steps; CTests **37/37**, **37/37**, and **24/24 PASS**; **69** matching provenance entries |
| Historical Issue #35 application builds | Linux GCC 15.2.0 GUI-enabled Debug **140/140**, Release **140/140**; core-only ASan/UBSan **54/54**; CTests **35/35**, **35/35**, **22/22 PASS**; 67 matching provenance entries |
| Historical Issue #34 application builds | Linux GCC 15.2.0 GUI-enabled Debug and Release **132/132**, **132/132** Ninja steps; core-only ASan/UBSan **54/54**; CTests **34/34**, **34/34**, **22/22 PASS**; 65 matching provenance entries |
| Historical Issue #33 application builds | GCC 15.2.0 Linux GUI-enabled Debug (86/86 final Ninja progress), GUI-enabled Release (124/124), core-only ASan/UBSan (54/54); **33/33**, **33/33**, **22/22** CTests PASS; matching source provenance |
| Historical Issue #32 application builds | GCC 15.2.0 Linux Debug (incremental), Release (114/114), and core-only ASan/UBSan (52/52), with **31/31**, **31/31**, and **21/21 CTests PASS**; provenance identical during run |
| Historical Issue #31 application builds | Linux GCC 15.2.0; GUI-enabled Debug and Release full builds, **29/29** CTests each; core-only ASan/UBSan build, **20/20** CTests; identical before/after verification provenance |
| Historical Issue #30 application builds | GCC 15.2.0 Linux GUI-enabled Debug and Release configurations: 27/27 CTests each; core-only ASan/UBSan: 19/19 CTests (validated against local CTest logs) |
| Historical Issue #29 application builds | GCC 15.2.0 GUI-enabled Debug full build 25/25 CTests; GUI-enabled Release full build 25/25 CTests; core-only ASan/UBSan build 18/18 CTests |
| Historical Issue #28 application builds | GCC 15.2.0 GUI-enabled Debug 24/24 CTests (incremental build); GUI-enabled Release full build 24/24 CTests; core-only ASan/UBSan build 17/17 CTests |
| Historical Issue #27 application builds | GCC 15.2.0 Debug and Release GUI-enabled builds and 23/23 CTests each; core-only ASan/UBSan 16/16 CTests |
| Historical Issue #26 application builds | Debug GUI-enabled full build and tests; Release desktop application target rebuilt |
| Historical Issue #25 application builds | Debug and Release, GUI enabled; optional benchmark targets ON |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

The Issue #26 GUI builds selected Qt 6. Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

### Issue #44 historical Debug and Release build and CTest results

**Debug:** `build-issue44` — **205 Ninja steps**, **52/52 CTests PASS** (18.49 s); desktop executable linked during recorded build.

**Release:** `build-issue44-release` — **225 Ninja steps**, **52/52 CTests PASS** (12.30 s); desktop executable linked during recorded build.

ELF executable SHA-256 at documentation time: Debug `3c0af1c8fa48135555822f108a120495e7b0751243ac825fa1f24ab93203405d`, Release `59f0203eb36079d35e93c4db1f2ff54cbb4bb0a28e47f41ad062f2e9bfbb728f`; hashes do not establish link history.

`verify_issue44.sh` evidence timestamp `2026-10-30T14:36:46-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; base Git HEAD `3c187f3fe9448d549953f39068e93994af7fbe36` plus working tree provenance. CTest times are not benchmarks.

### Issue #43 historical Debug and Release build and CTest results

**Debug:** `build-issue43` — **180 Ninja steps**, **50/50 CTests PASS** (17.64 s); desktop executable linked during recorded build.

**Release:** `build-issue43-release` — **217 Ninja steps**, **50/50 CTests PASS** (12.07 s); desktop executable linked during recorded build.

ELF executable SHA-256 at documentation update time: Debug `843d447726550f2a203c40984276a240f1b136ca22c83ca280272ebfd7fa1953`, Release `f36a80838716136ab95668f9c5fb9fae87deb384ad431548aed810670f12bd4d`. These hashes identify the binaries present now, not the original link timestamps.

`verify_issue43.sh` recorded evidence at `2026-10-30T14:01:27-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; base Git HEAD `b3b1c4cac6412064856817a02b1c15defad7dafe` plus source provenance. CTest times are not rendering benchmarks.

### Issue #42 historical Debug and Release build and CTest results

**Debug:** `build-issue42` — **178/178 Ninja steps PASS**, **49/49 CTests PASS** (18.17 s wall-clock), zero GUI warnings; incremental build; existing Linux desktop executable verified, but its link was not repeated in the recorded build.

**Release:** `build-issue42-release` — **209/209 Ninja steps PASS**, **49/49 CTests PASS** (12.19 s wall-clock), zero GUI warnings; desktop executable linked in recorded build.

Desktop executable SHA-256 at verification-document update time: Debug `1f5b93fc8516d4e48cc237ab16d85a5a761cbca70a40cd3e5ee9b3115795b889`; Release `8df49916777138d897d7f7ca6a22af5f5d3a2553f5fbd60501502933a0163f5b`. These hashes establish the binaries present when the report was generated, not an independently authenticated link history.

`verify_issue42.sh` evidence timestamp `2026-10-30T10:48:53-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; baseline HEAD `7a44f6ce0276d39ee97953a33bdd7cb8d7b06ab1` plus working-tree source provenance. CTest durations are not controlled rendering benchmarks.

### Issue #41 historical Debug and Release build and CTest results

**Debug:** `build-issue41` — **198/198 Ninja steps PASS**, **47/47 CTests PASS** (17.12 s wall-clock); zero recorded compiler warnings.

**Release:** `build-issue41-release` — **198/198 Ninja steps PASS**, **47/47 CTests PASS** (11.19 s wall-clock); zero recorded compiler warnings.

`verify_issue41.sh` evidence timestamp `2026-10-30T00:44:13-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; baseline Git HEAD `ead19efe63ed3da3aac4474982b5697c3a5264fe` plus captured source provenance. CTest wall-clock durations are not controlled performance benchmarks.

### Issue #40 historical Debug and Release build and CTest results

**Debug:** `build-issue40` — **187/187 Ninja steps PASS**, **45/45 CTests PASS** (15.39 s wall-clock); zero recorded compiler warnings.

**Release:** `build-issue40-release` — **187/187 Ninja steps PASS**, **45/45 CTests PASS** (9.52 s wall-clock); zero recorded compiler warnings.

`verify_issue40.sh` evidence timestamp `2026-10-29T23:29:10-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; baseline Git HEAD `a000cfec662ee8ea1c87f364bd4a1362de18f5f6` plus recorded working-tree changes. Both GUI builds linked RationalStudio. CTest wall-clock timings are not controlled numerical or rendering benchmarks.

### Issue #39 historical Debug and Release build and CTest results

**Debug:** `build-issue39` — **176/176 Ninja steps PASS**, **43/43 CTests PASS** (14.16 s wall-clock); zero recorded compiler warnings.

**Release:** `build-issue39-release` — **176/176 Ninja steps PASS**, **43/43 CTests PASS** (8.20 s wall-clock); zero recorded compiler warnings.

`verify_issue39.sh` evidence timestamp `2026-10-29T22:54:23-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; baseline Git HEAD `0a9283f17f01e867539c632c0df74103adbf4fa7` plus recorded working-tree changes. Both GUI configurations linked RationalStudio. CTest timings are not controlled rendering benchmarks.

### Issue #38 historical Debug and Release build and CTest results

**Debug:** `build-issue38` — **165/165 Ninja steps PASS**, **41/41 CTests PASS** (13.33 s wall-clock); zero recorded compiler warnings.

**Release:** `build-issue38-release` — **165/165 Ninja steps PASS**, **41/41 CTests PASS** (7.84 s wall-clock); zero recorded compiler warnings.

`verify_issue38.sh` evidence timestamp `2026-10-28T05:12:32-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; baseline Git HEAD `fadb27116272d79c6f7943e46d56728b54c04e64` plus recorded working-tree changes. Both builds linked the Qt desktop application; timings are CTest wall-clock observations, **not** rendering-performance benchmarks.

### Issue #37 historical Debug and Release build and CTest results

**Debug:** `build-issue37` — **155/155 Ninja steps PASS**, **39/39 CTests PASS** (13.25 s CTest wall-clock); no compiler warnings in build log.

**Release:** `build-issue37-release` — **155/155 Ninja steps PASS**, **39/39 CTests PASS** (7.30 s CTest wall-clock); no compiler warnings in build log.

`verify_issue37.sh` evidence timestamp `2026-10-28T04:40:07-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`, `cmake version 4.2.3`, Ninja `1.13.2`; Git HEAD `2f10d341884a0b934bb7bf27867bb077f2958720` plus recorded working-tree changes. Both configurations linked the actual desktop executable. CTest durations are not rendering benchmarks.

### Issue #36 historical Debug and Release build and CTest results

**Debug:** `build-issue36` — **144/144 Ninja steps PASS**; **37/37 CTests PASS** (12.07 s CTest wall-clock).

**Release:** `build-issue36-release` — **144/144 Ninja steps PASS**; **37/37 CTests PASS** (6.97 s CTest wall-clock).

`verify_issue36.sh` evidence timestamp `2026-10-28T03:35:45-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; baseline Git HEAD `df7855fc089c85844d245581cd28d2d05044d9b9` plus recorded working-tree changes. Both GUI builds linked `RationalStudio`. These timings are CTest host observations, not benchmarks.

### Issue #35 historical Debug and Release build and CTest results

**Debug:** `build-issue35` — **140/140 Ninja steps PASS**; **35/35 CTests PASS** (12.76 s CTest wall-clock).

**Release:** `build-issue35-release` — **140/140 Ninja steps PASS**; **35/35 CTests PASS** (7.27 s CTest wall-clock).

`verify_issue35.sh` evidence timestamp `2026-10-28T03:09:41-04:00`; compiler `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`, `cmake version 4.2.3`, Ninja `1.13.2`; base Git HEAD `98586a203d93dc2144655df418fe2dcbf10e9a03` plus working-tree changes. Both GUI-enabled configurations linked the RationalStudio desktop executable. These timings are host observations, not performance comparisons.

### Issue #34 historical Debug and Release build and CTest results

**Debug:** `build-issue34` — **132/132 Ninja build steps PASS**; GUI-enabled **34/34 CTests PASS** (12.00 s CTest wall-clock).

**Release:** `build-issue34-release` — **132/132 Ninja build steps PASS**; GUI-enabled **34/34 CTests PASS** (6.45 s CTest wall-clock).

`verify_issue34.sh` recorded evidence at `2026-10-28T02:39:56-04:00` with `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`, `cmake version 4.2.3`, Ninja `1.13.2` and Git HEAD `1d28c782368b979abc77862007b5f2ceb5cbf369` plus recorded working-tree changes. Both configurations linked the Qt desktop application. The build steps and test durations are host observations, not performance measurements.

### Issue #33 historical Debug and Release build and CTest results

**Debug:** `build-issue33` — successful GUI-enabled Ninja build; final progress **86/86**, **33/33 CTests PASS**, **11.57 s** CTest wall-clock. This is not attested as a fresh-from-empty-tree build; the build log contains regeneration activity before its final progress sequence.

**Release:** `build-issue33-release` — successful GUI-enabled build **124/124**, **33/33 CTests PASS**, **6.41 s** CTest wall-clock.

`verify_issue33.sh` timestamp `2026-10-27T19:26:47-04:00`; compiler `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`; `cmake version 4.2.3`, Ninja `1.13.2`, base Git HEAD `2de81c7aafca6aa055b45282c6212d09bec5f4a8` plus working-tree changes. These are recorded Linux build/CTest outcomes, not Windows or Qt 5 results.

### Issue #32 historical Debug and Release build and CTest results

**Debug:** `build-issue32` — successful **incremental 6/6 Ninja build**, **31/31 CTests PASS** (11.19 s CTest wall-clock).

**Release:** `build-issue32-release` — successful **114/114 Ninja build**, **31/31 CTests PASS** (5.99 s CTest wall-clock).

`verify_issue32.sh` evidence timestamp `2026-10-27T18:43:12-04:00`; toolchain `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3, 1.13.2`; Git HEAD `38c1da65e967779a13d886283cf618d6f8228ded` plus recorded working-tree edits. The builds linked the desktop executable.

### Issue #31 historical Debug and Release build and CTest results

**Debug:** `build-issue31` — CMake configured successfully; **101/101 build steps completed**; **29/29 CTests PASS**, total CTest wall-clock **10.91 s**.

**Release:** `build-issue31-release` — CMake configured successfully; **101/101 build steps completed**; **29/29 CTests PASS**, total CTest wall-clock **5.62 s**.

The consolidated `verify_issue31.sh` runner recorded all configure/build/CTest outputs under `issue31-evidence/`. Environment: `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`, `cmake version 4.2.3`, Ninja `1.13.2`; evidence timestamp `2026-10-26T16:38:58-04:00`. Source state at the time was based on Git HEAD `36a16b3ad4c802e42ebf2207c6f6567452337fd6` plus recorded working-tree changes. Both application builds completed through linking `RationalStudio`.

### Issue #30 historical Debug and Release CTest results

**GUI-enabled Debug:** `build-issue30`, **27/27 PASS**; 10.66 s total individual test durations (not CTest wall-clock).

**GUI-enabled Release:** `build-issue30-release`, **27/27 PASS**; 5.00 s total individual test durations (not CTest wall-clock).

Test evidence: `Testing/Temporary/LastTest.log` or recorded CTest console output. The CMakeCache configurations were checked; these automated outcomes do not by themselves establish a fresh-from-empty-tree build, nor a manual application launch.

### Issue #29 historical Debug and Release build results

**Debug (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue29 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue29 --parallel
ctest --test-dir build-issue29 --output-on-failure
```

```text
[81/81] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 25
Total Test time (real) = 10.68 sec
```

**Release (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue29-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue29-release --parallel
ctest --test-dir build-issue29-release --output-on-failure
```

```text
[81/81] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 25
Total Test time (real) = 5.09 sec
```

Both GUI-enabled configurations were freshly configured and built, and all registered tests passed. Existing scientific, CLI-linked core, viewport, and graph-rendering tests remained green. The GUI source was not extended with function-definition editing in this issue.

### Issue #28 historical Debug and Release build results

**Debug (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue28 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue28 --parallel
ctest --test-dir build-issue28 --output-on-failure
```

```text
[9/9] Linking CXX executable RationalVariableDefinitionsTests
100% tests passed, 0 tests failed out of 24
Total Test time (real) = 10.94 sec
```

The Debug build was incremental in the supplied terminal log; this result
confirms successful configuration, linking, and full CTest execution, not a
fresh-from-empty-tree Debug compilation.

**Release (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue28-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue28-release --parallel
ctest --test-dir build-issue28-release --output-on-failure
```

```text
[78/78] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 24
Total Test time (real) = 5.03 sec
```

The Release build completed successfully; all previously registered GUI,
viewport, graphing, and mathematical regressions passed alongside the new
Qt-independent definition-store suite.

### Issue #27 historical Debug and Release build results

**Debug (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue27 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue27 --parallel
ctest --test-dir build-issue27 --output-on-failure
```

```text
[75/75] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 23
Total Test time (real) = 10.35 sec
```

**Release (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue27-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue27-release --parallel
ctest --test-dir build-issue27-release --output-on-failure
```

```text
[75/75] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 23
Total Test time (real) = 5.10 sec
```

Both full GUI-enabled builds succeeded and all registered tests passed. The
previously available graphics/cache/budget/scheduling regressions remain part
of these suites. No new native Windows or Qt 5 verification was performed.

### Issue #26 historical build results

**Commands executed during implementation:**

```bash
cmake --build build-issue26 -j 4
ctest --test-dir build-issue26 --output-on-failure -j 4
cmake --build build-issue26-release --target RationalStudio -j 4
```

The existing GCC 15.2.0 Debug configuration rebuilt RationalStudio, RationalCLI, and the regression executables. The full GUI-enabled Debug suite passed **22/22** tests. The existing Release configuration rebuilt the RationalStudio desktop target successfully; a complete current Release CTest run was not recorded. Both configurations use C++20 and Qt 6 Widgets. Configuration details are retained locally in their `CMakeCache.txt` files.

### Issue #25 historical Debug build

**Commands executed:**

```bash
cmake -S . -B build-issue25 -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=ON
cmake --build build-issue25 --parallel
```

GCC 15.2.0 successfully configured and built RationalStudio, RationalCLI, the test executables, and both optional benchmarking executables.

**Final recorded Debug build output:**

```text
[71/71] Linking CXX executable RationalStudio
```

No compiler warnings or errors appeared in the submitted final Issue #25 Debug build output; this does not constitute a `-Werror` verification.

### Issue #25 historical Release build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue25-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=ON
cmake --build build-issue25-release --parallel
ctest --test-dir build-issue25-release --output-on-failure
```

**Final recorded Release build output:**

```text
[71/71] Linking CXX executable RationalStudio
```

**Recorded Release CTest summary:**

```text
100% tests passed, 0 tests failed out of 18
Total Test time (real) = 0.68 sec
```

No compiler warnings or errors appeared in the submitted final Issue #25 Release build output. The interactive GUI was manually checked in Debug, not separately in Release.

## 3. Automated regression tests — PASS

### Issue #44 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI Debug CTest | **52/52 PASS** — 18.49 s |
| GUI Release CTest | **52/52 PASS** — 12.30 s |
| Core-only ASan/UBSan CTest | **31/31 PASS** — 8.46 s |
| `RationalGraphTraceTests` (Debug/Release/ASan) | **25 assertions PASS each** |
| `RationalGraphTracingTests` (Debug/Release) | **26 assertions PASS each** |
| GUI compiler warnings | **0** |

Core tests cover nearest visible sampled segments, discontinuities, bounded scans, parameter interpolation, and real sampler metadata. Offscreen GUI tests cover five representations, negative polar radii, analysis markers, and cache-preserving hover. Neither is a certified inversion solver or an exhaustive numerical topology test.

**CTest SHA-256:** Debug `81aeb899bab9fcdb05d60aedc8345fcc614c72f2a87bc617587ed56311fcf9b6`, Release `15456fbbb0d57438083a30261ecb9122a8321e711e0951cb897f1a17b05654d5`, ASan `762a5be194e54bf08befe7fd80acc8e7fde0be3b2e6f3dff5327c4098eaf3738`; GUI direct Debug `5bf9481c96c46d6553e39acd6b5656e8a80b96a83abdad6b659e720446250910`, Release `5bf9481c96c46d6553e39acd6b5656e8a80b96a83abdad6b659e720446250910`; core direct Debug `2acf25f128e0c0bd35f3cfa12c6ea2b0aec8d58540151c2a0b32a961556014d6`.

**Provenance:** 97 matching paths; manifest SHA-256 `372c9ef9d4769696881f4a7a9162e3ab0f473d45100b5b58a7b783e2cfb246a3`.

### Issue #43 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI Debug CTest | **50/50 PASS** — 17.64 s |
| GUI Release CTest | **50/50 PASS** — 12.07 s |
| Core-only ASan/UBSan CTest | **30/30 PASS** — 8.32 s |
| `RationalGraphRepresentationControlsTests` direct Qt (Debug/Release) | **44 assertions PASS each** |
| GUI compiler warnings | **0** |

This GUI-only issue adds one offscreen CTest. The core suite remains unchanged. The regression tests inspect five creation actions, narrow sidebar button dimensions, help expansion, focus, parametric/polar intervals and mixed version-3 workspace persistence.

**CTest log SHA-256:** Debug `906d1a45e8e02bfb203b69ee7dbb6e6247251e8630c59d0556694caccace9d61`, Release `56be8c2a5151802d3c0df392b9d7b76e73eff47cf00a0eeea6cafa347c100a02`, ASan `291a8c77911bf2d9f0f6cb5f5c4370352ecbc264d7639fbba40f16c1229e3886`; GUI direct Debug `a11940d71c20f0733632f9d9bdc6f2154119c868aeebf88776f2ec04a719e545`, Release `a11940d71c20f0733632f9d9bdc6f2154119c868aeebf88776f2ec04a719e545`.

**Provenance:** 93 matching paths; manifest SHA-256 `19bad31083f411bf4e727eb74f8bba6c8ad587e7a66f7a2f2fbb27b1fcdab755`.

### Issue #42 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI Debug CTest | **49/49 PASS** — 18.17 s |
| GUI Release CTest | **49/49 PASS** — 12.19 s |
| Core-only ASan/UBSan CTest | **30/30 PASS** — 8.46 s |
| `RationalInequalityRegionTests` (Debug/Release/ASan) | **89 assertions PASS each** |
| `RationalInequalityGraphTests` (Debug/Release) | **41 assertions PASS each** |
| GUI compiler warnings | **0** |

Core regressions cover comparison normalization, reversed/Unicode operators, finite pixel clipping and invalid values. Offscreen GUI tests cover row editing, cached shading, viewport changes and mixed workspace persistence.

**Log SHA-256:** Debug CTest `61d8bfdf038f9044a33ff39acf6369f01d5faaa3e59552b3c586be7d9b08f0a2`; Release CTest `257e482b84821c30fe10e98d91ffac3e10e69cb3751e54c48bcb27cbcf7f16c4`; ASan CTest `f04431d3478696157bc1ea72d293272bdbee756790ba9e0c32aea710af66b312`; direct core `05cbba39029438fddd0f5f0f29fe37e02ac64ed5166baadd49f6208c21e3e960`; direct GUI `d3b16819cbc6eed7931b0bc32dcce041f3c7193429fdda9c7f13c92d0be5d3a2`.

**Provenance:** 92 matching paths; manifest SHA-256 `291d51cca0d9a3286136f986d75211e95c537ee9a196a7bf28a41b8433329d16`.

### Issue #41 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **47/47 PASS** — 17.12 s |
| GUI-enabled Release CTest | **47/47 PASS** — 11.19 s |
| Core-only ASan/UBSan CTest | **29/29 PASS** — 8.25 s |
| `RationalGraphFeatureDetectionTests` (Debug/Release/ASan) | **66 assertions PASS each** |
| `RationalGraphFeatureGraphTests` (Debug/Release) | **31 assertions PASS each** |
| GUI compiler warnings | **0 recorded** |

Tests use analytical roots, shifted/local extrema, stationary inflections, discontinuities, finite callback limits, dynamic bindings, real MainWindow interactions and stale-report recovery. Finite sampling is not a certificate of complete extrema/root detection.

**Log SHA-256:** Debug CTest `99a78f9bbd3619a747460e850f78cc4fced32b32fb9c49e91dddf75c17a063a3`; Release CTest `a38d1316739320eee5d51b57583e8ad0b84d8282cde90d053875a52155b2501c`; ASan CTest `877f0e2228ba53f43ec20c3bffddb3e83466dec0d2e143ae1b0bddd8a3a48df1`; direct core `14c25b02c5684b310003bd4198682df180d63fc1dd7527b63c0deb2c7f6041bc`; direct GUI `d704a7d8448ce5c52259ea401f1f2c3dac859f0d8cdc649e37073132c99dbacd`.

**Provenance:** 88 matching before/after paths; SHA-256 `8c48e69187fb8ef51c6021f271ed83c4f3da16ced66f5c25e88e51974b3f6689`.

### Issue #40 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **45/45 PASS** — 15.39 s |
| GUI-enabled Release CTest | **45/45 PASS** — 9.52 s |
| Core-only ASan/UBSan CTest | **28/28 PASS** — 8.29 s |
| `RationalIntersectionDetectionTests` direct (Debug/Release/ASan) | **155 assertions PASS each** |
| `RationalIntersectionGraphTests` direct offscreen (Debug/Release) | **16 assertions PASS each** |
| Compiler warnings (Debug/Release) | **0 / 0 recorded** |

One new Qt-independent CTest and one offscreen Qt CTest increase GUI-enabled registrations from 43 to 45 and the core-only suite from 27 to 28. The core suite covers sign crossings, sampled tangencies, duplicates, coincident functions, undefined/discontinuous evaluations, bounds, callback/result limits, and fresh variable bindings. The offscreen GUI suite tests the real MainWindow Intersections action, eligible curve selection, and numerical results. This is a finite regression matrix, not a complete or certified intersection oracle.

**Evidence SHA-256:** Debug CTest `23e5159acb6bfa60c0898092f03368b5b0ec7027ffdd9e47f21fdee2d8114033`; Release CTest `260dc687bd27b3a51a74a1a5f55ba219ea960384ea3a2690b357c8cc166470a2`; ASan/UBSan CTest `38e0e07cdf362a5eed9848924063a8cbf2cb9fe4e9f3c456814d8730e98fbea9`. Direct intersection-core Debug/Release/ASan: `649a7d98512b5db1df5b51bb5b8814e20f645cfdc3326135124ac3a70f12a473` / `649a7d98512b5db1df5b51bb5b8814e20f645cfdc3326135124ac3a70f12a473` / `649a7d98512b5db1df5b51bb5b8814e20f645cfdc3326135124ac3a70f12a473`. Direct intersection-GUI Debug/Release: `0281ed0b42766a89bd994c77f1c7bb5e7fb2a134da147ed2641e6d6d8c36f2ea` / `0281ed0b42766a89bd994c77f1c7bb5e7fb2a134da147ed2641e6d6d8c36f2ea`.

**Provenance:** identical before/after 84-entry manifests, manifest SHA-256 `e4555dfb2d6835a7f8020f6cde49b0fcba1298a78361c3e55739fbc7e003af50`. Equality establishes stability **during the verification run** only; later README and verification updates are separate.

### Issue #39 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **43/43 PASS** — 14.16 s |
| GUI-enabled Release CTest | **43/43 PASS** — 8.20 s |
| Core-only ASan/UBSan CTest | **27/27 PASS** — 8.21 s |
| `RationalImplicitContourTests` direct (Debug/Release/ASan) | **796 assertions PASS each** |
| `RationalImplicitGraphTests` direct offscreen (Debug/Release) | **33 assertions PASS each** |
| Compiler warnings (Debug/Release) | **0 / 0 recorded** |

One new Qt-independent CTest and one new offscreen Qt CTest raise GUI-enabled registrations from 41 to 43 and core-only from 26 to 27. The contour test covers analytical circles, invalid/non-finite fields, deterministic incremental continuation and callback limits; the Qt test exercises mixed graph rows, cache changes, invalid input, and workspace save/load. This finite matrix does not prove that marching squares detects every zero set or discontinuity.

**Evidence SHA-256:** Debug CTest `ad357a6390cb86129a9c63a8534d7ec8e576c337f20a6bd4cbfc58743a9a7c58`; Release CTest `4ae8ba0e40fb5a9aa10aa471d5a8fc4757c88f2fdfdf0106ba6ca0900ebb4f36`; ASan/UBSan CTest `6f14de50d23cbe62a0c4153b34ca3ae6f8eb94fcc12cba1366a0138e1b55f52d`. Direct contour Debug/Release/ASan: `9a91642210108c7c76a03c964276e990beb966a971924e53766933ac409a92dc` / `9a91642210108c7c76a03c964276e990beb966a971924e53766933ac409a92dc` / `9a91642210108c7c76a03c964276e990beb966a971924e53766933ac409a92dc`. Direct Qt Debug/Release: `21ab4392829fe9cc50e39d1adf2c186198d947744f7ef0750b66d3246f53b3f1` / `21ab4392829fe9cc50e39d1adf2c186198d947744f7ef0750b66d3246f53b3f1`.

**Provenance:** identical before/after 80-entry manifests, manifest SHA-256 `19eac873a432ad548f12ce2f2fda1a2a099f93955f21f16d0108fc954efe1612`. Equality establishes stability **during the verification run** only; later documentation edits are separate.

### Issue #38 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **41/41 PASS** — 13.33 s |
| GUI-enabled Release CTest | **41/41 PASS** — 7.84 s |
| Core-only ASan/UBSan CTest | **26/26 PASS** — 8.02 s |
| `RationalPolarExpressionTests` direct (Debug/Release/ASan) | **511 assertions PASS each** |
| `RationalPolarGraphTests` direct offscreen (Debug/Release) | **34 assertions PASS each** |
| Recorded compiler warnings (Debug/Release) | **0 / 0** |

Compared with Issue #37, two new registered tests raise the GUI-enabled count from 39 to 41 and core-only from 25 to 26. The Qt-independent adapter test covers constant radius, shifted polar circles, rose/negative radii, θ/theta normalization, degree conversion, slider bindings, invalid ranges and shared budget limits. The Qt test covers real MainWindow controls, shared GraphCanvas path caching, radius/interval invalidation, mixed workspaces, validation and recovery. These are finite regression cases, not an exhaustive mathematical guarantee.

**Evidence SHA-256:** Debug CTest `e56e433e49355e5029aaa4487193fb848173c6adfced693403b2f3efa9ce3a28`; Release CTest `703696ed1eb2d957b816d0f2bd040e6d94537ff7c2691c80684aaf1dacb2cf65`; ASan/UBSan CTest `d9b07810fae9650a075bc7c632cb5c2b4ff2b2e0bfe9d24982d3c27a234ff925`. Core direct Debug/Release/ASan `eedb964ec52b747f6e2cc60678addf99b86b4165c854e471ad5cd70362fed697` / `eedb964ec52b747f6e2cc60678addf99b86b4165c854e471ad5cd70362fed697` / `eedb964ec52b747f6e2cc60678addf99b86b4165c854e471ad5cd70362fed697`; Qt direct Debug/Release `6b26961000d0855e837cf87c1814360812501268f3604645edf4f499d2407deb` / `6b26961000d0855e837cf87c1814360812501268f3604645edf4f499d2407deb`.

**Provenance:** identical before/after 76-entry manifests, SHA-256 `611e8c199ea4d431c81ea76339556b2d9a73f245d6a9a93960fe23ebcf31fd1b`; this establishes stability **during** automated verification, not after documentation edits.

### Issue #37 historical automated regression results

| Verification | Recorded result |
|---|---|
| Linux GUI-enabled Debug CTest | **39/39 PASS**, 13.25 s |
| Linux GUI-enabled Release CTest | **39/39 PASS**, 7.30 s |
| Linux core-only ASan/UBSan CTest | **25/25 PASS**, 8.41 s |
| `RationalParametricSamplerTests` direct (Debug/Release/ASan) | **291 assertions PASS each** |
| `RationalParametricGraphTests` direct Qt offscreen (Debug/Release) | **32 assertions PASS each** |
| Compiler warnings (Debug/Release) | **0 recorded** |

Two CTests extend the Issue #36 baseline (37 GUI-enabled and 24 core-only) to 39 GUI-enabled and 25 core-only. The Qt-free sampler tests exercise parametric reference cases, finite intervals, invalid/disconnected coordinates, bounded work and incremental continuation. The Qt test exercises real graph-row controls, shared renderer integration, and mixed-workspace version-3 persistence. These passing cases are finite regressions, not a guarantee for every curve.

**Evidence SHA-256:** Debug CTest `d71668d608afcce21f1bb22f6e2c00e12691101efb6d6db05fa958f10f261670`; Release CTest `b2418ac37c078dcf48c222f24a7b4675461a2613bf8e9912817d757daa353fc3`; ASan/UBSan CTest `6642259b90b593337161ef9ce60805a6729965100e1c4fefc0844539b22ba802`. Core direct runs (Debug/Release/ASan): `4c551dc5629c1ed221d6c88f4d96bf1fcb5401bdb0e9ed626faa05e626c70ae5` / `4c551dc5629c1ed221d6c88f4d96bf1fcb5401bdb0e9ed626faa05e626c70ae5` / `4c551dc5629c1ed221d6c88f4d96bf1fcb5401bdb0e9ed626faa05e626c70ae5`. Qt direct runs (Debug/Release): `d78100996790be8ce7e7835f3ef60022a8cb68b18743f8727a92ec1deeb871da` / `d78100996790be8ce7e7835f3ef60022a8cb68b18743f8727a92ec1deeb871da`.

**Provenance:** 73 entries, matching before/after SHA-256 manifests; manifest digest `44101530943a549c36fd013263b15a1d05a992d8c9c8ed270eaf596533a1e0d5`. Equality establishes stability **during testing** only; the subsequently updated README and this verification report are not part of that equality.

### Issue #36 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **37/37 PASS** — 12.07 s |
| GUI-enabled Release CTest | **37/37 PASS** — 6.97 s |
| Core-only ASan/UBSan CTest | **24/24 PASS** — 8.13 s |
| `RationalVariableFunctionInteractionTests` (direct Debug/Release/ASan) | **80 assertions PASS in each** |
| `RationalDynamicEvaluationMatrixTests` (direct Debug/Release/ASan) | **15,940 assertions PASS in each** |

Two new Qt-independent executables increase GUI-enabled CTests from 35 to 37 and core-only sanitizer CTests from 22 to 24. Cross-owner tests cover case normalization, direct/transitive dependencies, nested functions, scope snapshots, invalid/cyclic definition rejection and rollback, undefined propagation/recovery, and graph-variable isolation. The deterministic numeric matrix checks 240 updates across 17 positions per update using independent mathematical expectations; these are targeted regressions, not a proof of every mathematical input.

**Evidence SHA-256:** Debug CTest `e0234aeeeb601cd8f69ea06c676ab06e3cbf0a8248843f36dbf5b0960ede6459`, Release CTest `ae106d4d550d8ac94d2a012301728821c94e8542a03d2e1cccc70e3c54b89bd6`, ASan/UBSan CTest `c6449b17c5132753bc842a6f6a4c6d079cd5688af44beb7f2072fd8aba9afac4`. Direct interaction (Debug/Release/ASan) `891c92fd3b498c49cdd2a1c4a88a413fa3e497b33ad28790e6b6e2d5ba4a15c6` / `891c92fd3b498c49cdd2a1c4a88a413fa3e497b33ad28790e6b6e2d5ba4a15c6` / `891c92fd3b498c49cdd2a1c4a88a413fa3e497b33ad28790e6b6e2d5ba4a15c6`; direct dynamic (Debug/Release/ASan) `db8b84d2e6bbd9daf88a4729c303bac88e2e04cdcb55530a7bd1ab6e29bc026f` / `db8b84d2e6bbd9daf88a4729c303bac88e2e04cdcb55530a7bd1ab6e29bc026f` / `db8b84d2e6bbd9daf88a4729c303bac88e2e04cdcb55530a7bd1ab6e29bc026f`.

**Provenance:** before/after 69-entry manifests match byte-for-byte; manifest SHA-256 `263c82355beb475949a9dd9c64d9708233d8729caf4cd8fc56bde831ce5e7dbd`. This establishes file stability **during** the verification run; later README/verification-document edits are not included.

### Issue #35 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **35/35 PASS** — 12.76 s |
| GUI-enabled Release CTest | **35/35 PASS** — 7.27 s |
| Core-only ASan/UBSan CTest | **22/22 PASS** — 7.94 s |
| `RationalGraphWorkspacePersistenceTests` direct Qt offscreen Debug | **55 assertions PASS** |

Targeted coverage includes version-1 compatibility and version-2 round trips, slider values/ranges/steps, non-slider scalar assignments, nested reusable functions and transitive dependencies, rejected invalid JSON/definitions, and actual MainWindow save/load and rollback. This does not prove all possible filesystem failures or desktop configurations.

**Evidence SHA-256:** Debug CTest `73d64af92bfc851b4095e98a6f79d7bd7031cf3afb773a0c6819f67d7e497a04`; Release CTest `c43eba79be316d3eee16ef3d0345f0e9dd6a02ffde3bf91e3beef97047d6c056`; core ASan/UBSan CTest `49e92d4a82de90fc36cb055bc102d44f6b68df279aff6d5d3ebf6805cfbfd26e`; direct Qt persistence regression `cc9ba95f2cdf9ae58ad6854fec96afe5982dccab1f903632ca2844ffb87c9229`.

**Provenance:** identical before/after 67-entry SHA-256 manifests; manifest digest `f487a8437d5a9b25fcccbc658952235e7b0fdc2b256a5c71319de3366a4dce07`. The comparison establishes file stability **during this test run**. README and VERIFICATION updates made afterward are not included in that equality.

### Issue #34 historical automated regression results

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **34/34 PASS** — 12.00 s |
| GUI-enabled Release CTest | **34/34 PASS** — 6.45 s |
| Core-only ASan/UBSan CTest | **22/22 PASS** — 7.11 s |
| `RationalMathKeyboardShortcutTests` direct offscreen Debug | **35 assertions PASS** |

The Issue #34 GUI-specific test exercises mathematical symbols, trigonometric templates, selection/caret semantics, ordinary Qt undo/select-all behavior, MainWindow graph-row focus navigation and new-row creation. The unchanged Qt-independent sanitizer suite does not execute these Qt-only shortcuts.

**Evidence SHA-256:** Debug CTest `b3d78a1b6a42b4295ebf70e986a0026c6b0a02cb9226f8ee37ce05380120c6a2`; Release CTest `699ca25b16a7d37a99f6ed3a4a8a241d8bf7d407981dbb5e087174097bb96e28`; core ASan/UBSan CTest `b5d7a79b826e0e100a173fe920be628c2226e06515773019b575f6eeb58f4c6a`; direct offscreen shortcut regression `73e371b9d738292f0347e6ce0e706ce7f8bb67a97d38cb2bb38de0ef05e293f3`.

**Provenance:** identical before/after 65-entry SHA-256 manifests, manifest digest `2e387e5af9bda6553ad9866549ad9459421d543399fd853675cd89055d7d694f`. The equality establishes recorded file stability **during the automated run** only. Later README and VERIFICATION edits are separate.

### Issue #33 historical automated regression results

| Verification | Recorded result |
|---|---|
| Debug GUI-enabled CTest | **33/33 PASS**, 11.57 s |
| Release GUI-enabled CTest | **33/33 PASS**, 6.41 s |
| Core-only ASan/UBSan CTest | **22/22 PASS**, 7.14 s |
| `RationalExpressionDiagnosticTests` direct Debug | **323 assertions PASS** |
| `RationalExpressionInlineFeedbackTests` direct offscreen Qt Debug | **19 assertions PASS** |

The Qt-independent diagnostic suite checks error categories, source byte offsets, malformed inputs, identifier/function errors and Unicode column mapping. The real MainWindow offscreen suite checks inline messages, valid/invalid row isolation, recovery, and symbol-aware locations. The tests do not prove every input case or runtime-domain error.

**Evidence SHA-256:** Debug CTest `dcf58729dbe71d0832ee841a30e3a146d00d8b08218e84d274f1211669233079`; Release CTest `d505a3c57f406df88dcfc0977e7e28378d0f9ccf54c0f5ca64b01a052b1231fe`; ASan/UBSan CTest `3a9b47fd6cccd51fc3976a50d19a21577b65643745fa4d0a8745a73de44ad423`; diagnostic direct run `ba169fc4b81cbebcb414434d71f6b3c2b65e76c8ac82aafcebb90d9154944bed`; inline direct run `a4807d6d10ca614c43caaf03e853e09bb0fe254b3ea7efe57649ed9a4312f334`.

**Provenance:** 64 matching before/after SHA-256 entries; manifest SHA-256 `c863667c4ee7e117cad97309e9f4fc2824feb56f0168c0e883780570bf511706`. This establishes stability **during** verification. The subsequent README and VERIFICATION documentation updates are not covered by that comparison.

### Issue #32 historical automated regression results

| Verification | Recorded result |
|---|---|
| Debug GUI-enabled CTest | **31/31 PASS** — 11.19 s |
| Release GUI-enabled CTest | **31/31 PASS** — 5.99 s |
| Core-only ASan/UBSan CTest | **21/21 PASS** — 7.15 s |
| `RationalMathExpressionInputTests` direct Debug | **4,032 assertions PASS** |
| `RationalMathExpressionEditorTests` direct Qt offscreen Debug | **19 assertions PASS** |

The new Qt-free test covers Unicode symbol normalization, parser-compatible evaluation, incomplete-versus-malformed input classification, and deterministic expression sweeps. The new Qt test checks insertion/selection/caret semantics, paired parentheses, scientific input, graph-row feedback, and symbol-toolbar integration. These are targeted regressions, not exhaustive Unicode or manual desktop tests.

**Evidence SHA-256:** Debug `63961975f31430fe878cf1e597b1ae0df36bdd54dfe285139197db323f9d2143`; Release `fbc63f3bdba62fc540a627f045c6f1ca480b72aa224bf2a0f0ad547e01f76ac8`; ASan/UBSan `4417b702d48be6f2f5a454703302b7d516d2bd4d8b8ebaae4234012ca730dc4b`; direct input `428c04ddab56ba99b6c86845dc02e7aef43ab533ba12a3ae99322dcd71d79431`; direct editor `0f77cb9d6161719277e4eb0b978180024d720d5bf87991c208af56d79cf75a6e`.

**Provenance:** matching before/after 62-entry SHA-256 manifests; manifest digest `249c5a9627fb7fd4d5e7c029d9b5dea92e61193ac377c8c6fcc49578dbbc6ffe`. This establishes stability **during the verification run**. README and VERIFICATION documentation changes made afterward are not covered by that equality.

### Issue #31 historical automated regression results

| Verification | Recorded result |
|---|---|
| Linux GCC 15.2.0 GUI-enabled Debug CTest | **29/29 PASS**, 10.91 s CTest wall-clock |
| Linux GCC 15.2.0 GUI-enabled Release CTest | **29/29 PASS**, 5.62 s CTest wall-clock |
| Linux GCC 15.2.0 core-only ASan/UBSan CTest | **20/20 PASS**, 6.93 s CTest wall-clock |
| `RationalGraphVariableDependencyTests` direct Debug run | **PASS — 1,223 assertions** |
| `RationalGraphVariableRefreshTests` direct offscreen Qt Debug run | **PASS — 40 assertions** |

Issue #31 adds one Qt-independent CTest and one Qt-offscreen CTest: **29 CTest invocations** in GUI-enabled configurations and **20 CTests** in the GUI-disabled sanitizer configuration. The tests cover variable declaration repair, selected direct and transitive dependency invalidation, per-evaluation binding snapshots, and actual `MainWindow`/`GraphCanvas` integration. They are targeted regressions, not an exhaustive mathematical or rendering proof.

**Saved evidence hashes:** Debug CTest SHA-256 `2c08db18def7c334c2090054b1dd5e98c48005b229e258d236221d8b3471027d`; Release CTest SHA-256 `4471721f2b5d819e887838c109dc55d747d67ead4e9c8cd30b3c12dcb607b314`; ASan/UBSan CTest SHA-256 `9267e534f94225e8959b7793a3b5b32e0ee2f8125b8cbc166dca11ad98211f77`. Direct dependency-test SHA-256 `a99b0ae32dc6cc766d9e8961eb49aea1319a63cb06902a9485df360a2e4f8dde`; direct Qt-refresh-test SHA-256 `5d2dbcf3d8a6da697a32e0498eb4fe9ad5ebcc95e4592a7a796a101dcebc14b6`.

**Provenance:** `57` recorded paths; before and after manifests are identical; manifest SHA-256 `82dd6d4473b9d5dda31d73d4129195714c016469ad1c7da384a2843c3d27067c`. These comparisons establish file stability **during this test run**; README/verification edits made afterward change documentation and are not covered by that before/after equality.

### Issue #30 historical automated regression results

**Evidence references:** `debug` CTest log SHA-256 `5bce716f784f70a1789c2473cc0204d44965f7d6a50e177701d9516cb0edeb2c`; `release` CTest log SHA-256 `795b9edf77ecd3f0f060a8d879043d8888b80d90e8f67bd2b138a0128b319d22`; `asan` CTest log SHA-256 `7f6f6074b8056580a2d484f454c14b775e89fa3d47bb57f64d247c62c609550a`. These are hashes of test logs, not source-revision provenance.

| Verification | Recorded result |
|---|---|
| GUI-enabled Debug CTest | **27/27 PASS**; 10.66 s total individual test durations (not CTest wall-clock) |
| GUI-enabled Release CTest | **27/27 PASS**; 5.00 s total individual test durations (not CTest wall-clock) |
| Core-only ASan/UBSan CTest | **19/19 PASS**; 7.05 s total individual test durations (not CTest wall-clock) |
| `RationalVariableSliderScaleTests` | PASS — 14,785 assertions |
| `RationalVariableSliderPanelTests` | PASS — 47 assertions |

Coverage includes bounded integer tick mapping, finite range/step validation, exact terminal endpoint reachability, Qt control synchronization, invalid edit handling, and variable-definition integration. The two Issue #30 tests add **27 CTest registrations from 26 executables** (19 Qt-independent and 8 offscreen Qt invocations from 7 widget executables). The core-only sanitizer suite runs **19 tests**.

### Issue #29 historical automated regression results

| Verification | Recorded result |
|---|---|
| GCC 15.2.0 GUI-enabled Debug CTest | **25/25 PASS**, 10.68 s total |
| GCC 15.2.0 GUI-enabled Release CTest | **25/25 PASS**, 5.09 s total |
| GCC 15.2.0 core-only ASan/UBSan CTest | **18/18 PASS**, 7.22 s total |
| `RationalFunctionDefinitionsTests` (direct Debug run) | **PASS — 1,143 assertions** |

Direct test invocation and output:

```bash
./build-issue29/RationalFunctionDefinitionsTests
```

```text
PASS: 1143 FunctionDefinitions assertions
```

The new Qt-independent test covers function definition parsing; positional parameters and checked arity; references to previously defined functions; lexical parameter scope and scalar variable captures; nested calls; invalid definition and recursion/cycle rejection; preservation of prior published definitions after rejected edits; accepted redefinition and immutable compiled-scope snapshots; and repeated evaluation with changing scalar bindings. The submitted Debug build archive independently contains the 1,143-assertion result in `Testing/Temporary/LastTest.log`.

Issue #29 adds one Qt-independent test executable: **25 CTest registrations from 24 executables** (18 Qt-independent tests and seven offscreen Qt invocations from six widget test executables). The GUI-disabled sanitizer configuration runs **18 Qt-independent CTests**, excluding Qt widgets.

### Issue #28 historical automated regression results

| Verification | Recorded result |
|---|---|
| GCC 15.2.0 GUI-enabled Debug CTest | **24/24 PASS**, 10.94 s total |
| GCC 15.2.0 GUI-enabled Release CTest | **24/24 PASS**, 5.03 s total |
| GCC 15.2.0 core-only ASan/UBSan CTest | **17/17 PASS**, 6.66 s total |
| `RationalVariableDefinitionsTests` (direct Debug run) | **PASS — 951 assertions** |

Direct test invocation and output:

```bash
./build-issue28/RationalVariableDefinitionsTests
```

```text
PASS: 951 VariableDefinitions assertions
```

The new core suite covers scalar assignment parsing; name/reference validation;
direct and transitive dependency ordering; selective recomputation and changed
values; rejected cycles or malformed edits with transactional preservation;
undefined-value propagation and recovery; revision/binding snapshots; and
reuse of compiled graph expressions with newly resolved bindings. These are
regression checks of the Qt-independent APIs, not an interactive GUI assignment
workflow. The Debug build archive's `LastTest.log` separately confirms the
951-assertion pass and GUI-enabled CTest execution.

Issue #28 adds one Qt-independent test executable: **24 CTest registrations
from 23 executables** (17 core tests and 7 offscreen Qt invocations from
6 widget executables). The GUI-disabled ASan/UBSan suite contains
**17 core-only CTests**; it does not exercise Qt widget code.

### Issue #27 historical automated regression results

| Verification | Recorded result |
|---|---|
| GCC 15.2.0 GUI-enabled Debug CTest | **23/23 PASS**, 10.35 s total |
| GCC 15.2.0 GUI-enabled Release CTest | **23/23 PASS**, 5.10 s total |
| GCC 15.2.0 core-only ASan/UBSan CTest | **16/16 PASS**, 6.25 s total |
| `RationalNamedVariableTests` (direct Debug run) | **PASS — 3,149 assertions** |

Direct test invocation and recorded output:

```bash
./build-issue27/RationalNamedVariableTests
```

```text
PASS: 3149 NamedVariable assertions
```

**New regression coverage:** ASCII identifier validation (1–64 characters),
case-insensitive declarations and bindings, builtin-name reservation, duplicate
and undeclared identifiers, scope isolation, missing/non-finite values, unique
sorted referenced-variable discovery, arithmetic with builtins and `x`/`ans`,
move semantics, and changes to per-evaluation bindings without reparsing.

The new regression adds one Qt-independent executable. The complete GUI-enabled
suite contains **23 CTest registrations from 22 executables** (16 core tests
and 7 offscreen Qt invocations of 6 widget executables, including the 2× DPI
invocation). The sanitizer build runs **16 core-only tests** and excludes Qt.

### Issue #26 historical automated results

**Recorded Debug suite result:**

```text
100% tests passed, 0 tests failed out of 22
Total Test time (real) = 7.93 sec
```

This implementation run used `ctest --test-dir build-issue26 --output-on-failure -j 4`. The subsequently inspected local `build-issue26/Testing/Temporary/LastTest.log` also records all 22 passing tests; the dedicated canvas coverage test took 7.99 seconds in that log. Timings are host observations, not performance acceptance thresholds.

| Executable / CTest | Recorded result |
|---|---|
| `RationalCurveCoverageRegressionTests` | PASS — 4,626 assertions |
| `RationalCurveSamplingJobTests` | PASS — 896,291 assertions |
| `RationalGraphCanvasCoverageTests` | PASS — 33,628 assertions |
| `RationalGraphCanvasCoverageHiDpiTests` | PASS — 493 assertions; `QT_SCALE_FACTOR=2` |
| `RationalGraphCanvasBudgetTests` | PASS — 332 assertions with resumable completion under small budgets |
| `RationalGraphCanvasSchedulingTests` | PASS — 64 assertions |

The configured suite contains **15 Qt-independent tests** and **7 offscreen Qt invocations from 6 widget executables**, totaling **22 CTests from 21 executables**. The high-DPI test invokes the coverage executable a second time. Optional benchmarks and Python recorder tests are not CTests.

**Manual evidence recorder unit tests:**

```bash
python3 -m unittest discover -s tests -p 'test_issue26_gui_verification.py' -v
```

```text
Ran 12 tests in 0.234s

OK
```

These test recorder verdicts, required/optional cases, schema rejection, save/resume behavior, report escaping, and check exit status. They do not operate the desktop GUI or attest that the final rendering fix passed manual testing.

### Issue #25 historical automated results

**Command executed:**

```bash
ctest --test-dir build-issue25 --output-on-failure
```

**Recorded Issue #25 Debug CTest results:**

```text
1/18 Test #1: RationalCoreTests ... Passed 0.00 sec
2/18 Test #2: RationalViewportTests ... Passed 0.00 sec
3/18 Test #3: RationalCurveSamplerTests ... Passed 0.00 sec
4/18 Test #4: RationalCurveDomainTests ... Passed 0.01 sec
5/18 Test #5: RationalCurveDiscontinuityTests ... Passed 0.02 sec
6/18 Test #6: RationalCurveSteepTests ... Passed 0.02 sec
7/18 Test #7: RationalViewportPrecisionTests ... Passed 0.00 sec
8/18 Test #8: RationalCurveGeometryCacheTests ... Passed 0.00 sec
9/18 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.01 sec
10/18 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.00 sec
11/18 Test #11: RationalGraphViewportBoundaryTests ... Passed 0.00 sec
12/18 Test #12: RationalGraphRegressionMatrixTests ... Passed 0.01 sec
13/18 Test #13: RationalGraphBenchmarkStatisticsTests ... Passed 0.00 sec
14/18 Test #14: RationalGraphCanvasCacheTests ... Passed 0.07 sec
15/18 Test #15: RationalGraphCanvasBudgetTests ... Passed 0.11 sec
16/18 Test #16: RationalGraphCanvasSchedulingTests ... Passed 0.29 sec
17/18 Test #17: RationalGraphCanvasBoundaryTests ... Passed 0.07 sec
18/18 Test #18: RationalGraphCanvasRegressionMatrixTests ... Passed 0.12 sec

100% tests passed, 0 tests failed out of 18
Total Test time (real) = 0.74 sec
```

**Recorded Issue #25 Release CTest:** 18/18 passed (0.68 seconds total).

The Issue #25 GUI-enabled suite includes **thirteen Qt-independent executables** (including the benchmark-statistics test) and **five offscreen Qt widget regression executables**: eighteen total. The optional **RationalGraphSamplingBench** and **RationalGraphCanvasBench** programs are separate measurement tools, not CTests. The core-only sanitizer build runs thirteen tests and excludes the Qt renderer.

### Test coverage by executable

| Test executable | Verified areas |
|---|---|
| `RationalCoreTests` | Fraction arithmetic and normalization, exact integer handling, scientific parsing/evaluation, operator precedence, functions, malformed inputs, and undefined results. |
| `RationalViewportTests` | Coordinate conversions, inverse transformations, viewport dimensions, pan and cursor-anchored zoom, invalid inputs, and state preservation. |
| `RationalCurveSamplerTests` | Adaptive screen-space subdivision, line/curve approximation, oscillatory probes, finite drawing geometry, and evaluation limits. |
| `RationalCurveDomainTests` | Open/closed function domains, valid regions separated by undefined intervals, exception/non-finite handling, and bounded sampling. |
| `RationalCurveDiscontinuityTests` | Reciprocal and tangent poles, finite step jumps, `floor(x)` boundaries, offset viewport cases, sampled-position sweeps, and continuous controls. |
| `RationalCurveSteepTests` | Steep continuous straight lines, hyperbolic tangent and inverse-tangent transitions, bounded terminal refinement, approximation consistency, sampling-position sweeps, and discontinuity controls. |
| `RationalViewportPrecisionTests` | Coordinate round trips, repeated pan/zoom stability, zoom-limit behavior, restored viewport scales, extreme-coordinate validation, and state preservation after rejected navigation. |
| `RationalCurveGeometryCacheTests` | Qt-independent cache identity, reuse, invalidation, reordering, and lazy geometry regeneration. |
| `RationalGraphCanvasCacheTests` | Offscreen Qt widget integration for `QPainterPath` cache reuse and invalidation during paint operations. |
| `RationalCurveEvaluationBudgetTests` | Exact callback accounting, per-curve and shared limits, zero-budget and invalid-result handling, and deterministic multi-curve budget sweeps. |
| `RationalGraphCanvasBudgetTests` | Offscreen Qt integration for per-paint budget enforcement, deferred uncached curves, sampling diagnostics, and invalidation after budget changes. |
| `RationalGraphRepaintSchedulerTests` | Qt-independent navigation event coalescing, generation tracking, obsolete deferred-work cancellation, and screen-space preview transform regressions. |
| `RationalGraphCanvasSchedulingTests` | Offscreen Qt wheel/drag scheduling integration, path preview reuse, delayed resampling, and superseded navigation cancellation. |
| `RationalGraphViewportBoundaryTests` | Invalid viewport input rejection and state preservation, extreme finite centers, bounded grid geometry, and resize-dimension regressions. |
| `RationalGraphCanvasBoundaryTests` | Offscreen Qt resize and boundary checks, extreme zoom/center rendering, cancellation of stale refreshes, and invalid-view rollback. |
| `RationalGraphRegressionMatrixTests` | Qt-independent cross-subsystem checks for viewport transformations, cursor-anchored zoom, sampled segment accuracy, restricted/invalid domains, discontinuities, and evaluation ceilings. |
| `RationalGraphCanvasRegressionMatrixTests` | Offscreen Qt integration checks for cache reuse, navigation, resizing, invalid views, evaluation budgets, and discontinuity/domain rendering. |
| `RationalGraphBenchmarkStatisticsTests` | Qt-independent checks of percentile summaries, report escaping, benchmark output files, and argument validation; no host-speed thresholds. |
| `RationalCurveCoverageRegressionTests` | Reproduce left-side budget starvation, check distributed strip coverage, shared ceilings, and smooth/restricted-domain controls for the single-call sampler. |
| `RationalCurveSamplingJobTests` | Complete tangent branches across widths and zoom levels, exact callback accounting, budget-independent geometry, pole/step separation, and steep continuous controls. |
| `RationalNamedVariableTests` | Qt-independent identifier validation, scoped named-variable resolution, case-folded bindings, reserved names, missing/non-finite values, reference introspection, and backward-compatible evaluation. |
| `RationalVariableDefinitionsTests` | Qt-independent scalar assignments, dependency ordering and change propagation, circular/malformed redefinition rollback, undefined-value recovery, selective recomputation, revision tracking, and binding snapshots. |
| `RationalFunctionDefinitionsTests` | Qt-independent reusable functions, positional parameters and arity, lexical scoping, captured scalar bindings, nested calls, recursion rejection, transactional redefinition, published snapshots, and repeated evaluation. |
| `RationalVariableSliderScaleTests` | Qt-independent validated slider ranges, bounded tick mapping, snapped values, numeric stability, and terminal endpoint. |
| `RationalVariableSliderPanelTests` | Offscreen Qt widget integration for creating sliders, editing values/ranges/steps, and updating scalar variables and dependents. |
| `RationalGraphVariableDependencyTests` | Qt-independent graph-to-variable dependency selection, transitive scalar dependencies, and function-captured variable references. |
| `RationalGraphVariableRefreshTests` | Qt offscreen MainWindow and GraphCanvas integration, recognizing slider variables in expressions, selective refresh, and invalid/repaired row transitions. |
| `RationalExpressionDiagnosticTests` | Qt-free structured diagnostics: error kinds, readable messages, source positions, function/identifier errors and Unicode-visible column translation. |
| `RationalExpressionInlineFeedbackTests` | Offscreen Qt MainWindow integration for scientific/graph error text, row isolation, recovery and Unicode columns. |
| `RationalMathKeyboardShortcutTests` | Offscreen Qt mathematical symbol and function shortcuts, selection/caret handling, native editing regressions, and actual MainWindow graph-row navigation and creation. |
| `RationalGraphWorkspacePersistenceTests` | Offscreen Qt version-1/2 JSON round trips, variable and function dependency restoration, slider settings, invalid-load rejection, and real MainWindow Save/Open integration. |
| `RationalVariableFunctionInteractionTests` | Qt-independent cross-owner variables, nested functions, captured scalar values, scope/binding snapshots, rollback, recursion rejection, domain recovery, and selective dependency matching. |
| `RationalDynamicEvaluationMatrixTests` | Qt-independent deterministic 240-update by 17-position matrix, independent numeric oracles, transitive dependency changes and preserved unrelated expressions. |
| `RationalParametricSamplerTests` | Qt-free adaptive parametric sampling, circle reference, interval validation, disconnected geometry, numerical budgets and incremental continuation. |
| `RationalParametricGraphTests` | Qt offscreen real graph-row integration, X(t)/Y(t) controls, interval edits, cache updates and version-3 mixed-workspace round trips. |
| `RationalPolarExpressionTests` | Qt-independent polar coordinate adapter: θ/theta normalization, Cartesian numerical reference cases, negative radius, RAD/DEG handling, variable bindings and sampling budgets. |
| `RationalPolarGraphTests` | Qt offscreen polar editor, cache identity, angular bounds, mixed workspace persistence, real MainWindow integration and error recovery. |
| `RationalImplicitContourTests` | Qt-independent bounded incremental marching squares, analytical contour reference, deterministic cell processing, non-finite cell rejection and shared evaluation-budget accounting. |
| `RationalImplicitGraphTests` | Offscreen Qt MainWindow implicit rows, mixed rendering, cache invalidation, equation feedback, JSON version-3 round trips and invalid-load protection. |
| `RationalIntersectionDetectionTests` | Qt-independent bounded explicit Cartesian function-pair intersection search, residual and X tolerances, sampled tangencies, coincident functions, numerical budgets, domain limits, and variable binding updates. |
| `RationalIntersectionGraphTests` | Offscreen Qt MainWindow intersection action, supported Cartesian row selection, numerical result reporting, and preservation of unrelated graph types and user state. |
| `RationalMathExpressionInputTests` | Unicode-to-parser translation, mathematical evaluation and incomplete/invalid operator classification (Qt-independent). |
| `RationalMathExpressionEditorTests` | Offscreen Qt caret and selection handling, math symbol insertion, immediate syntax feedback, scientific and graph MainWindow integration. |
| `RationalGraphCanvasCoverageTests` | Rendered tangent-tail coverage, identical repeated repaints, retained previews through interrupted refreshes, toolbar zoom, resizing, and exhausted shared budgets. |
| `RationalGraphCanvasCoverageHiDpiTests` | Second invocation of the coverage executable with `--dpi-smoke` and `QT_SCALE_FACTOR=2`. |

### Discontinuity regression results

The new test executable was also run directly:

```bash
./build-issue17/RationalCurveDiscontinuityTests
```

**Recorded output:**

```text
PASS: 8663 CurveDiscontinuity assertions
```

The discontinuity suite exercises:

- Selected `1/(x-a)` vertical asymptotes, including shifted/non-grid-aligned positions.
- `tan(x)` near the tested asymptotes at `±pi/2` in radian mode.
- Finite step discontinuities and `floor(x)` integer boundaries.
- A deterministic sweep of 257 sample-phase locations for reciprocal poles and step jumps.
- A translated/scaled viewport case.
- A steep continuous straight line and a smooth parabola, guarding against indiscriminate rejection of legitimate geometry.

A finite-step case was previously observed to bridge a discontinuity with the preceding sampler and passed after the Issue #17 revision. That is targeted regression evidence; it is not a proof of complete discontinuity detection.

### Steep continuous-curve regression results

Issue #18 introduces bounded secondary refinement for finite curve intervals that remain unresolved at the normal subdivision limit.

The new implementation provides additional refinement opportunities for steep continuous functions rather than immediately disconnecting their drawing geometry.

The dedicated regression executable is:

`RationalCurveSteepTests`

It was executed directly using:

```bash
./build-issue18/RationalCurveSteepTests
```

**Recorded output:**

```text
PASS: 6516 CurveSteep assertions
```

### Viewport precision and navigation stability regression results

Issue #19 improves numerical precision and stability in the Qt-independent `GraphViewport` component.

The implementation uses precision-aware navigation calculations, preserves viewport state when zoom limits are reached, and rejects operations that would introduce excessive screen-space error due to floating-point limitations.

The dedicated regression executable is:

`RationalViewportPrecisionTests`

It was executed directly using:

```bash
./build-issue19/RationalViewportPrecisionTests
```

Recorded result:

```text
PASS: 3474 GraphViewportPrecision assertions
```

The regression suite verifies:

- Coordinate round trips across multiple viewport centers and scales.
- Coordinate behavior with odd viewport dimensions and half-pixel centers.
- Repeated cursor-anchored zoom-in and zoom-out cycles.
- Repeated zoom attempts at minimum and maximum wheel-scale limits.
- Restored viewport states outside ordinary mouse-wheel zoom limits.
- Repeated forward and reverse panning operations.
- Invalid navigation input rejection.
- Preservation of viewport state following rejected operations.
- Safe handling of extreme world-coordinate values.
- Rejection of navigation that cannot be represented within the configured screen-space error tolerance.

The tests include a regression case for previously incorrect zoom-direction behavior when a restored viewport scale exceeded the ordinary mouse-wheel limits.

The viewport continues to use IEEE-754 double-precision coordinates. These improvements do not establish arbitrary-precision navigation or guarantee that all extreme coordinate transformations can be represented accurately.

### Curve geometry cache regression results

Issue #20 introduces a Qt-independent geometry-cache policy and caches generated `QPainterPath` geometry in `GraphCanvas`. Valid paths can be reused during ordinary repaints and presentation-only changes, while changes to mathematical inputs or the viewport require regeneration.

The Qt-independent regression executable was run directly:

```bash
./build-issue20/RationalCurveGeometryCacheTests
```

Recorded output:

```text
PASS: 55 CurveGeometryCache assertions
```

The offscreen Qt widget regression executable was also run directly:

```bash
QT_QPA_PLATFORM=offscreen ./build-issue20/RationalGraphCanvasCacheTests
```

Recorded output:

```text
PASS: 49 GraphCanvasCache assertions
```

These are focused regression tests for cache management and Qt widget cache integration. They do not constitute complete end-to-end GUI interaction tests, pixel-level rendering comparisons, or controlled performance benchmarks. No frames-per-second improvement is claimed.

### Bounded curve evaluation and rendering-budget regression results

Issue #21 introduces configurable per-curve and shared per-paint callback ceilings for rebuilding curve geometry. The Qt-independent `CurveEvaluationBudget` accounts for calls across multiple sampling operations; the Qt renderer reports exhausted or truncated work and defers curves not processed in the current paint. This bounds callback counts, **not** the duration of each callback or total frame time.

The Qt-independent budget executable was run directly:

```bash
./build-issue21/RationalCurveEvaluationBudgetTests
```

Recorded output:

```text
PASS: 5944 CurveEvaluationBudget assertions
```

The offscreen Qt widget budget executable was also run directly:

```bash
QT_QPA_PLATFORM=offscreen ./build-issue21/RationalGraphCanvasBudgetTests
```

Recorded output:

```text
PASS: 20 GraphCanvasBudget assertions
```

The focused tests exercise callback accounting, per-curve/shared limits, deterministic multi-curve budget sweeps, deferred cache builds, sampling diagnostics, and cache invalidation after sampling limits change. They do not establish wall-clock execution limits, completeness of budget-truncated geometry, or measured UI performance improvements.

### Graph repaint scheduling regression results

Issue #22 adds `GraphRepaintScheduler` to coalesce rapid navigation updates and cancel obsolete deferred work. While a refresh is pending, `GraphCanvas` can transform existing cached paths as an interim display; the renderer then samples the latest viewport on its scheduled refresh. The target interval is 16 ms. This is not a real-time frame deadline.

The Qt-independent scheduler test was run directly:

```bash
./build-issue22/RationalGraphRepaintSchedulerTests
```

Recorded result:

```text
PASS: 5947 GraphRepaintScheduler assertions
```

The offscreen Qt integration test was also run directly:

```bash
QT_QPA_PLATFORM=offscreen ./build-issue22/RationalGraphCanvasSchedulingTests
```

Recorded result:

```text
PASS: 57 GraphCanvasScheduling assertions
```

The suites cover navigation coalescing, generation tracking, cancellation of superseded deferred requests, transformed geometry previews, and scheduled rebuilding. Passing tests do not establish visually flicker-free drag rendering or measurable FPS improvements.

### Issue #23 viewport boundary and resize regression results

Issue #23 improves graph resizing and viewport boundary handling. `GraphGridLayout` validates grid spacing and constructs safe pixel-space positions; `GraphCanvas` avoids stale rendering on resize and handles extreme but finite viewport centers without unsafe grid-label coordinates. These checks preserve the prior precision-aware viewport and Issue #22 repaint scheduling behavior.

**Qt-independent direct regression command:**

```bash
./build-issue23/RationalGraphViewportBoundaryTests
```

Recorded result:

```text
PASS: 1884 GraphViewportBoundary assertions
```

**Qt offscreen direct regression command:**

```bash
QT_QPA_PLATFORM=offscreen ./build-issue23/RationalGraphCanvasBoundaryTests
```

Recorded result:

```text
PASS: 80 GraphCanvasBoundary assertions
```

The targeted suites cover invalid view changes and state rollback, extreme finite centers and scales, grid-position safety, repeated dimensions/resize transitions, and renderer invalidation. They do **not** prove arbitrary-precision coordinate behavior, exhaustive geometry correctness, or the absence of every GUI defect. Manual verification of resizing and interaction was also reported successful (Section 5).

### Issue #24 graph sampling and viewport regression matrix results

Issue #24 adds two complementary automated regression matrices without modifying production graph-rendering algorithms. The Qt-independent matrix exercises transformations, cursor-anchored zoom, emitted-segment approximation accuracy, restricted domains, selected finite and infinite discontinuities, and bounded evaluation work. The offscreen Qt matrix checks the combined behavior of viewport navigation, resizing, cached geometry, invalid-view rollback, and rendering controls.

**Qt-independent regression matrix:**

```bash
./build-issue24/RationalGraphRegressionMatrixTests
```

Recorded output:

```text
PASS: 5021 GraphRegressionMatrix assertions
```

**Offscreen Qt widget regression matrix:**

```bash
QT_QPA_PLATFORM=offscreen ./build-issue24/RationalGraphCanvasRegressionMatrixTests
```

Recorded output:

```text
PASS: 92 GraphCanvasRegressionMatrix assertions
```

These suites complement the existing focused tests. The reported assertions cover representative mathematical functions, defined tolerances, selected invalid-domain and discontinuity cases, and numerical callback limits; they do not establish exhaustive mathematical correctness, pixel-identical output, end-to-end GUI automation, or a frame-time guarantee. The developer separately reports successful manual GUI verification (Section 5).

### Issue #25 graph rendering and navigation benchmark results

The Issue #25 Release runner completed **150 mathematical-sampling measurements** and **750 Qt offscreen canvas/navigation measurements**. These are genuine host measurements imported from the benchmark JSON/CSV files when the documentation updater ran; they are **not CTest success thresholds**.

**Recorded benchmark setup:** 960 × 600 logical-pixel viewport, 1/4/8 active curves, 25 recorded trials per scenario, 5 warmup trials, 12 synthetic events per navigation burst, GCC 15.2.0 Release build, `QT_QPA_PLATFORM=offscreen` for Qt.

| Environment field | Recorded value |
|---|---|
| Date (UTC) | 2026-10-23T17:35:26+00:00 |
| Git commit | e0c19bf140905e7f5c1fde08bca578a2711c81b8 |
| Git worktree | M CMakeLists.txt |
| OS | Linux-7.0.0-38-generic-x86_64-with-glibc2.43 |
| CPU | AMD Ryzen 7 5800X 8-Core Processor |
| Available hardware threads | 16 |
| Governor | powersave |
| Display session | wayland |
| Sampling compiler | GCC 15.2.0 |
| Sampling build type | Release (NDEBUG) |
| Qt benchmark build type | Release (NDEBUG) |
| Trials per group | 25 |
| Events per navigation burst | 12 |

**graph_sampling — measured scenario aggregates**

| Measurement | Scenario | Curves | N | Median (ms) | p95 (ms) | Total callbacks | Total builds | Truncated runs |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| sampling_callbacks_plus_geometry | default_view | 1 | 25 | 0.0050 | 0.0065 | 6425 | 25 | 0 |
| sampling_callbacks_plus_geometry | default_view | 4 | 25 | 0.0235 | 0.0255 | 31050 | 100 | 0 |
| sampling_callbacks_plus_geometry | default_view | 8 | 25 | 0.0940 | 0.0985 | 156350 | 200 | 0 |
| sampling_callbacks_plus_geometry | zoomed_view | 1 | 25 | 0.0025 | 0.0025 | 4025 | 25 | 0 |
| sampling_callbacks_plus_geometry | zoomed_view | 4 | 25 | 0.0146 | 0.0146 | 21350 | 100 | 0 |
| sampling_callbacks_plus_geometry | zoomed_view | 8 | 25 | 0.1175 | 0.1604 | 176700 | 200 | 0 |

- Source CSV: `benchmark-results/issue25/graph_sampling.csv` (SHA-256 `1a78ed43754d2a87191ecc5c9bb56a617995d06769cbf5e33992c5036abd02e6`)
- Source JSON: `benchmark-results/issue25/graph_sampling.json` (SHA-256 `305be7cbba06b2c6b0626a767775f72b089806f06726d9a42d7b886431c282f3`)

**graph_canvas — measured scenario aggregates**

| Measurement | Scenario | Curves | N | Median (ms) | p95 (ms) | Total callbacks | Total builds | Truncated runs |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| cold_invalidated_render | 960x600 | 1 | 25 | 0.6166 | 0.8056 | 6301 | 25 | 0 |
| warm_cached_render | 960x600 | 1 | 25 | 0.5868 | 0.7175 | 0 | 0 | 0 |
| input_to_first_preview | wheel_zoom_burst | 1 | 25 | 0.6322 | 0.7729 | 0 | 0 | 0 |
| navigation_event_dispatch_total | wheel_zoom_burst | 1 | 25 | 0.0255 | 0.0315 | 0 | 0 | 0 |
| preview_render_total | wheel_zoom_burst | 1 | 25 | 8.0470 | 9.2648 | 0 | 0 | 0 |
| last_event_to_geometry_settled | wheel_zoom_burst | 1 | 25 | 9.9845 | 10.3864 | 4025 | 25 | 0 |
| input_to_first_preview | mouse_drag_burst | 1 | 25 | 0.6465 | 0.7213 | 0 | 0 | 0 |
| navigation_event_dispatch_total | mouse_drag_burst | 1 | 25 | 0.0268 | 0.0377 | 0 | 0 | 0 |
| preview_render_total | mouse_drag_burst | 1 | 25 | 7.1867 | 8.7479 | 0 | 0 | 0 |
| last_event_to_geometry_settled | mouse_drag_burst | 1 | 25 | 10.6316 | 10.9026 | 6425 | 25 | 0 |
| cold_invalidated_render | 960x600 | 4 | 25 | 2.0677 | 2.4290 | 30998 | 100 | 0 |
| warm_cached_render | 960x600 | 4 | 25 | 1.9911 | 2.4238 | 0 | 0 | 0 |
| input_to_first_preview | wheel_zoom_burst | 4 | 25 | 2.0518 | 2.1282 | 0 | 0 | 0 |
| navigation_event_dispatch_total | wheel_zoom_burst | 4 | 25 | 0.0273 | 0.0324 | 0 | 0 | 0 |
| preview_render_total | wheel_zoom_burst | 4 | 25 | 29.2878 | 32.9129 | 0 | 0 | 0 |
| last_event_to_geometry_settled | wheel_zoom_burst | 4 | 25 | 3.8000 | 4.5081 | 22350 | 100 | 0 |
| input_to_first_preview | mouse_drag_burst | 4 | 25 | 1.9915 | 2.0824 | 0 | 0 | 0 |
| navigation_event_dispatch_total | mouse_drag_burst | 4 | 25 | 0.0302 | 0.0372 | 0 | 0 | 0 |
| preview_render_total | mouse_drag_burst | 4 | 25 | 23.5251 | 24.7977 | 0 | 0 | 0 |
| last_event_to_geometry_settled | mouse_drag_burst | 4 | 25 | 6.2567 | 6.6141 | 31050 | 100 | 0 |
| cold_invalidated_render | 960x600 | 8 | 25 | 3.0269 | 3.0933 | 147651 | 200 | 0 |
| warm_cached_render | 960x600 | 8 | 25 | 2.6691 | 2.7858 | 0 | 0 | 0 |
| input_to_first_preview | wheel_zoom_burst | 8 | 25 | 2.7690 | 2.8100 | 0 | 0 | 0 |
| navigation_event_dispatch_total | wheel_zoom_burst | 8 | 25 | 0.0300 | 0.0340 | 0 | 0 | 0 |
| preview_render_total | wheel_zoom_burst | 8 | 25 | 38.5885 | 39.0943 | 0 | 0 | 0 |
| last_event_to_geometry_settled | wheel_zoom_burst | 8 | 25 | 6.3814 | 6.6292 | 142950 | 200 | 0 |
| input_to_first_preview | mouse_drag_burst | 8 | 25 | 2.6894 | 2.7225 | 0 | 0 | 0 |
| navigation_event_dispatch_total | mouse_drag_burst | 8 | 25 | 0.0338 | 0.0383 | 0 | 0 | 0 |
| preview_render_total | mouse_drag_burst | 8 | 25 | 31.6884 | 32.3934 | 0 | 0 | 0 |
| last_event_to_geometry_settled | mouse_drag_burst | 8 | 25 | 8.6816 | 8.9484 | 157475 | 200 | 0 |

- Source CSV: `benchmark-results/issue25/graph_canvas.csv` (SHA-256 `d6150bf4c9ff4852daf4474860ecd4a0b54313d0143b23e4129a3c7b77590e22`)
- Source JSON: `benchmark-results/issue25/graph_canvas.json` (SHA-256 `b66efe965b07f389da426542310480c2ac6e96ff22a83af55d406fa89ea08a43`)

- Generated detailed Markdown baseline: `benchmark-results/issue25/RESULTS.md` (SHA-256 `415d570762776131013cde73ba40ff03969331ff927fd6b8c0d27f1a54359d73`).
- These paths identify locally generated files; whether raw files are committed is a separate repository decision. Scenario aggregates above remain in this verification report even if raw files are not tracked.

**Interpretation boundaries:** Sampling totals count mathematical callbacks, not instructions or CPU cycles. Cold Qt rendering includes sampling and rasterization; warm rendering is cached path rasterization. Offscreen `QWidget::render()` does not measure desktop compositor presentation, actual mouse-to-monitor latency, or FPS. The navigation settlement metric includes timer and event-loop waiting. Measurements are machine-, load-, and build-dependent; no before/after speedup can be established without a comparably instrumented earlier run.

### Issue #26 wide-view sampling and rendering regression results

The original GUI failure was caused by evaluation-budget exhaustion during left-to-right adaptive sampling of repeated tangent poles. An intermediate strip-based workaround spread geometry across the viewport but could still exhaust a strip partway through a branch and cache missing tails. The final renderer uses resumable `CurveSamplingJob` builds, retaining completed geometry until a complete replacement is ready.

The current implementation provides:

- Bounded callbacks per curve per paint and across each paint, with unfinished sampling resumed on paced later paints; cached probes prevent duplicate callback work in a retried interval.
- World-space coarse interval limits, monotonicity-aware offscreen pruning, geometric error checks for steep chords, additional refinement around invalid probes, and clipped drawing coordinates.
- Surrounding sampled geometry covering twice the widget width and height for nearby navigation previews, retained through wheel input, toolbar zoom, resize, and repeated interruption of an unfinished refresh.
- Separate branch stroking and device-scale-aware raster caches for unchanged completed curves and pending previews; later curves retain their previews even when earlier curves consume the shared paint allowance.

**Coverage:** Core tests use widths 350, 900, and 1600, height 617, a translated view, and scales 58, 15, 1.38, 0.5, and 0.1 pixels per unit. They check analytically expected tangent intersections with both viewport edges, finite ordered geometry, no sampled chords across tangent poles, exact callback accounting, and identical completed vertices under tiny versus large work allowances. Separate controls check shifted reciprocal poles, finite jumps, and connected steep `tanh` transitions.

Offscreen widget tests render `tan(x)`, `1/(x-0.133)`, and `floor(x)` together at widths 350, 900, and 1600 and scales 58, 9, 1.38, 0.5, and 0.1. They check expected tail pixels near the top and bottom, identical repeated cached images, zero redundant geometry builds, interrupted refresh previews, toolbar zoom, resizing, and a four-callback shared budget. A separate 2× display-scale invocation checks widths/scales selected by `--dpi-smoke`.

The rendered checks are targeted pixel-presence and same-run image comparisons, not a reference-image oracle for every curve or evidence of physical-display latency. The initial dense-view test exceeded its 60-second timeout before raster caching; the final implementation passed the same test in approximately eight seconds. This development observation is not a controlled benchmark or an FPS claim. Issue #25 benchmark data above have not been regenerated for the new renderer.

## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

### Issue #44 historical complete core-only sanitizer suite — PASS

`build-issue44-asan`: **78 Ninja steps**, **31/31 CTests PASS** (8.46 s). Direct GraphTrace core: 25 assertions PASS. Qt widget tests were not sanitizer-qualified.

### Issue #43 historical complete core-only sanitizer suite — PASS

`build-issue43-asan`: **75 Ninja steps**, **30/30 CTests PASS** (8.32 s). There are no new Qt-free tests in Issue #43. Qt GUI sanitizers were not run.

### Issue #42 historical complete core-only sanitizer suite — PASS

`build-issue42-asan`: **75/75 Ninja steps PASS**, **30/30 CTests PASS** (8.46 s wall-clock). Direct inequality core regression passed **89 assertions**. Qt GUI tests were not sanitizer-qualified.

### Issue #41 historical complete core-only sanitizer suite — PASS

`build-issue41-asan`: **72/72 Ninja steps PASS**, **29/29 CTests PASS** (8.25 s wall-clock). `RationalGraphFeatureDetectionTests` passed **66 assertions** in a direct ASan/UBSan run. Qt GUI was disabled and is not sanitizer-qualified.

### Issue #40 historical complete core-only sanitizer suite — PASS

`build-issue40-asan`: **69/69 Ninja steps PASS**, **28/28 CTests PASS** (8.29 s wall-clock). `RationalIntersectionDetectionTests` passed **155 assertions** in a direct ASan/UBSan run. Qt GUI targets were disabled; this does not establish Qt Widgets sanitizer qualification.

### Issue #39 historical complete core-only sanitizer suite — PASS

`build-issue39-asan`: **66/66 Ninja steps PASS**, **27/27 CTests PASS** (8.21 s wall-clock). `RationalImplicitContourTests` passed **796 assertions** in a direct ASan/UBSan run. The Qt GUI was disabled; this does not qualify Qt Widgets under sanitizers.

### Issue #38 historical complete core-only sanitizer suite — PASS

`build-issue38-asan`: **63/63 Ninja steps PASS**, **26/26 CTests PASS** (8.02 s wall-clock). The Qt-independent `RationalPolarExpressionTests` also passed 511 assertions in a direct ASan/UBSan run. The GUI was disabled; Qt Widgets are **not** sanitizer-qualified by these results.

### Issue #37 historical complete core-only sanitizer suite — PASS

`build-issue37-asan`: **61/61 Ninja steps PASS**, **25/25 CTests PASS**, 8.41 s CTest wall-clock. The new Qt-independent ParametricSampler test passed 291 assertions directly under ASan/UBSan. GUI and optional benchmarks were disabled; Qt Widgets were **not** sanitizer-tested.

### Issue #36 historical complete core-only sanitizer suite — PASS

`build-issue36-asan`: **58/58 Ninja steps PASS**, **24/24 CTests PASS**, 8.13 s CTest wall-clock. Both new core regression executables passed under ASan/UBSan and each also passed direct sanitizer invocation. GUI and benchmarks were disabled; no Qt application sanitizer coverage is claimed.

### Issue #35 historical complete core-only sanitizer suite — PASS

`build-issue35-asan` completed **54/54 Ninja steps** and **22/22 core-only CTests PASS** (7.94 s CTest wall-clock). GUI and optional benchmarking were disabled. The new JSON/Qt persistence suite is excluded, so the full Qt application has **not** been sanitizer-qualified.

### Issue #34 historical complete core-only sanitizer suite — PASS

`build-issue34-asan` completed **54/54 Ninja steps**, and **22/22 core-only CTests PASS** (7.11 s CTest wall-clock). The Qt-dependent keyboard regression is deliberately excluded; this does not qualify the full Qt application under sanitizers.

### Issue #33 historical complete core-only sanitizer suite — PASS

`build-issue33-asan` completed **54/54 Ninja steps**, **22/22 CTests PASS**, CTest wall-clock **7.14 s**. The Qt-independent ExpressionDiagnostic suite is included; the GUI inline-feedback suite is excluded. The Qt application has not been tested under sanitizers in this qualification.

### Issue #32 historical complete core-only sanitizer suite — PASS

`build-issue32-asan`: **52/52 Ninja build steps**, **21/21 CTests PASS**, CTest wall-clock 7.15 s; GUI and benchmark targets disabled. The Qt-independent Unicode-input test is included. Qt Widgets and the full desktop application are **not** sanitizer-qualified.

### Issue #31 historical complete core-only sanitizer suite — PASS

`build-issue31-asan` completed **50/50 build steps completed** and **20/20 CTests PASS** (CTest wall-clock **6.93 s**). The consolidated script configured Debug, `RATIONALSTUDIO_BUILD_GUI=OFF`, benchmarks OFF, and compiler/linker flags `-fsanitize=address,undefined -fno-omit-frame-pointer`. The new Qt-independent dependency test is included; the Qt refresh test is excluded. The complete Qt application has **not** been sanitizer-qualified.

### Issue #30 historical complete core-only sanitizer suite — PASS

`build-issue30-asan` was configured with GUI disabled and both `-fsanitize=address,undefined` flags; its CTest log records **19/19 PASS**. Total timing evidence: 7.05 s total individual test durations (not CTest wall-clock). The Qt slider panel is excluded, so the complete graphical application does not have ASan/UBSan coverage.

### Issue #29 historical complete core-only sanitizer suite — PASS

```bash
cmake -S . -B build-issue29-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue29-asan --parallel
ctest --test-dir build-issue29-asan --output-on-failure
```

```text
[45/45] Linking CXX executable RationalFunctionDefinitionsTests
100% tests passed, 0 tests failed out of 18
Total Test time (real) = 7.22 sec
```

The first pasted sanitizer commands contained accidental shell control characters and an extra trailing `~`, producing command-entry errors before configuration. The corrected configuration, build, and complete **18/18 PASS** run supersede those failed invocations. No sanitizer diagnostics were reported in the successful run. GUI/Qt rendering was excluded.

### Issue #28 historical complete core-only sanitizer suite — PASS

```bash
cmake -S . -B build-issue28-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue28-asan --parallel
ctest --test-dir build-issue28-asan --output-on-failure
```

```text
[42/42] Linking CXX executable RationalNamedVariableTests
100% tests passed, 0 tests failed out of 17
Total Test time (real) = 6.66 sec
```

All 17 registered Qt-independent tests passed in the ASan/UBSan configuration.
No sanitizer diagnostics appeared in the supplied run. Qt widgets and the
full GUI renderer were deliberately excluded from the sanitizer build.

### Issue #27 historical complete core-only sanitizer suite — PASS

```bash
cmake -S . -B build-issue27-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue27-asan --parallel
ctest --test-dir build-issue27-asan --output-on-failure
```

```text
[39/39] Linking CXX executable RationalCurveSamplerTests
100% tests passed, 0 tests failed out of 16
Total Test time (real) = 6.25 sec
```

All 16 registered Qt-independent tests passed with AddressSanitizer and
UndefinedBehaviorSanitizer instrumentation. No sanitizer diagnostics were
reported in the supplied terminal output. This does **not** establish
sanitizer coverage of Qt widgets or the GUI render pipeline.

### Historical sanitizer evidence (Issues #25–#26)

**Configuration and build:**

```bash
cmake -S . -B build-issue25-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue25-asan --parallel
```

**Final recorded build output:**

```text
[33/33] Linking CXX executable RationalCurveGeometryCacheTests
```

**Test command:**

```bash
ctest --test-dir build-issue25-asan --output-on-failure
```

**Recorded output:**

```text
1/13 Test #1: RationalCoreTests ... Passed 0.02 sec
2/13 Test #2: RationalViewportTests ... Passed 0.01 sec
3/13 Test #3: RationalCurveSamplerTests ... Passed 0.02 sec
4/13 Test #4: RationalCurveDomainTests ... Passed 0.04 sec
5/13 Test #5: RationalCurveDiscontinuityTests ... Passed 0.09 sec
6/13 Test #6: RationalCurveSteepTests ... Passed 0.07 sec
7/13 Test #7: RationalViewportPrecisionTests ... Passed 0.01 sec
8/13 Test #8: RationalCurveGeometryCacheTests ... Passed 0.01 sec
9/13 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.03 sec
10/13 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.01 sec
11/13 Test #11: RationalGraphViewportBoundaryTests ... Passed 0.01 sec
12/13 Test #12: RationalGraphRegressionMatrixTests ... Passed 0.05 sec
13/13 Test #13: RationalGraphBenchmarkStatisticsTests ... Passed 0.03 sec

100% tests passed, 0 tests failed out of 13
Total Test time (real) = 0.41 sec
```

All thirteen Qt-independent CTests passed in the sanitizer-enabled configuration, including the new benchmark-statistics test. No ASan or UBSan failures were reported.

**Scope:** The Qt desktop application, all five Qt widget regression executables, and the two optional non-CTest benchmark tools are excluded from this sanitizer build. The results do not establish Qt renderer or interactive GUI sanitizer coverage.

### Issue #26 incremental sampling sanitizer result

**Commands executed during implementation:**

```bash
cmake --build build-issue26-asan --target RationalCurveSamplingJobTests -j 4
ASAN_OPTIONS=detect_leaks=0 ./build-issue26-asan/RationalCurveSamplingJobTests
```

```text
PASS: 896291 sampling job assertions
```

The core-only Debug configuration used `-fsanitize=address,undefined -fno-omit-frame-pointer`. No ASan or UBSan failure was reported for this direct executable run. Leak detection was disabled for this invocation. This is a focused current result, separate from the historical **13/13** Issue #25 sanitizer suite above; the full current **15-test** core-only suite and Qt renderer sanitizer tests were not recorded. The original Issue #26 sanitizer build directory is no longer present locally at documentation-update time; the command/result above are retained from the implementation session.

## 5. Desktop GUI — current acceptance and historical results

### Issue #44 historical manual desktop GUI acceptance

PASS — developer-reported Issue #44 advanced graph-tracing desktop acceptance; not independently instrumented case by case or separately attested in Release. Automated offscreen tests do not substitute for interactive desktop inspection.

### Issue #43 historical manual desktop GUI acceptance

PASS — developer-reported Issue #43 multi-representation graph-controls desktop acceptance; not independently instrumented case by case or separately attested in Release. Qt offscreen regressions do not replace human desktop verification.

### Issue #42 historical manual desktop GUI acceptance

PASS — developer-reported Issue #42 inequality shading desktop acceptance; not independently instrumented case by case or separately attested in Release. Qt offscreen coverage does not replace interactive desktop acceptance.

### Issue #41 historical manual desktop GUI acceptance

PASS — developer-reported Issue #41 roots/extrema/critical-point desktop acceptance; not independently instrumented case by case or separately attested in Release. Offscreen GUI CTests are automated coverage, not an interactive desktop-session transcript.

### Issue #40 historical manual desktop GUI acceptance

PASS — developer-reported Issue #40 intersection-analysis desktop acceptance; not independently instrumented case by case or separately attested in Release. The developer reports checking the numerical intersection workflow and existing graphing functionality after the automated verification completed. Both Debug and Release offscreen Qt regression runs passed. An independently instrumented manual case-by-case transcript or interactive Release acceptance was not supplied.

### Issue #39 historical manual desktop GUI acceptance

PASS — developer-reported Issue #39 implicit-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. The developer reports that the implicit-equation workflow and GUI regression checks appeared functional after verification. Offscreen Qt integration passed in Debug and Release; no independently recorded manual test transcript or interactive Release acceptance was supplied.

### Issue #38 historical manual desktop GUI acceptance

PASS — developer-reported Issue #38 polar-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. The developer reports that the polar-graphing GUI and related regressions behaved correctly in a manual desktop pass. The automated offscreen Qt regressions passed in both Debug and Release; a separately instrumented manual test matrix and interactive Release launch were not supplied.

### Issue #37 historical manual desktop GUI acceptance

PASS — developer-reported Issue #37 interactive desktop acceptance; not independently instrumented case by case or separately attested in Release. The developer reports that the parametric graphing workflow, GUI, and regression checks appeared functional after the warning fixes. No independently captured step-by-step desktop checklist or separate interactive Release run was provided. Offscreen Qt regressions passed in both Debug and Release.

### Issue #36 manual desktop GUI acceptance

PASS — developer-reported Issue #36 desktop regression smoke check; not independently recorded step by step or separately verified in Release. The developer reported completing a smoke check after adding the core-only tests; the test-only change does not add new GUI functionality. The existing offscreen Qt regressions ran in the full Debug and Release CTest suites. No separate instrumented Release desktop acceptance is reported.

### Issue #35 historical manual desktop GUI acceptance

PASS — developer-reported Issue #35 desktop workspace save/open, restored variables/functions/sliders and GUI regression check; not an independently instrumented case-by-case acceptance run. The developer reported that workspace save/open restored the expected state and other GUI checks looked functional. This is not an independently instrumented test transcript, nor a separate interactive Release run.

**Desktop observation:** Qt platform reported mouse-grab warnings and KDE KIO directory-model warnings while the file dialog browsed `build-issue35/` (generated build files). Save/open and the restored workspace were reported functional. No associated data loss or GUI failure was observed; this does not prove the warnings can never indicate a GUI problem in other circumstances.

### Issue #34 historical manual desktop GUI acceptance

PASS — developer-reported manual Issue #34 desktop keyboard and graph interaction check; not an independently recorded case-by-case acceptance session. The developer reported checking the mathematical shortcuts and desktop GUI successfully. No separately instrumented checklist, specific per-shortcut desktop log, or interactive Release acceptance was supplied. Offscreen Qt regression coverage independently passed 35 assertions.

### Issue #33 historical manual desktop GUI acceptance

PASS — developer-reported manual Issue #33 desktop GUI check; not independently instrumented. The developer reported that inline scientific and graph diagnostics, correction/recovery, and other existing GUI behavior looked correct. This is an informal report, not an independently captured case-by-case checklist or separate Release desktop run.

### Issue #32 historical manual desktop GUI acceptance

PASS — developer-attested Issue #32 manual GUI checklist; not an automated desktop test. The recommended checklist covers symbol entry, selection wrapping, automatic parenthesis pairing, cursor placement, valid/incomplete/invalid feedback, scientific result/history, graph rendering, sliders, save/reopen and navigation. Offscreen Qt tests passing alone do not attest these manual steps.

### Issue #31 historical manual desktop GUI acceptance

PASS — developer-reported manual desktop GUI acceptance: slider variables are accepted in graph expressions and changing sliders updates plotted curves. The developer confirmed that session sliders are visible, that named scalar variables are accepted in graph expressions, and that changing the sliders updates their graphs. This is a developer attestation, not an independently recorded step-by-step GUI script; interactive Release desktop verification was not separately reported. The offscreen integration test additionally passed 40 direct assertions.

### Issue #30 historical interactive GUI regression status

Not separately attested in this documentation update; offscreen Qt tests passed. Graph-curve response to variable changes is not yet implemented; slider settings are not yet serialized with graph workspaces.

### Issue #29 historical interactive GUI regression status

**PASS — developer-reported informal manual check.** The developer launched the existing GUI after integrating the Issue #29 Qt-independent function-definition system and reported that the usual calculator, graphing, navigation, and presentation behaviors remained normal. This was not a scripted GUI acceptance session, and an interactive Release GUI launch was not separately documented. The offscreen Qt tests passed in both 25-test GUI-enabled configurations. No function-definition editing rows or UI controls were added in Issue #29.

### Issue #28 historical interactive GUI regression status

**PASS — developer-reported informal manual check.** The developer launched
and inspected the existing GUI after the Issue #28 core changes, revisiting
usual graphing and calculator regression scenarios, and reported no visible
behavior changes or failures. No formal Issue #28 checklist, automated desktop
interaction session, or separate interactive Release GUI run was supplied.
The Qt offscreen regression tests passed in both 24-test GUI-enabled suites.
Issue #28 does **not** add variable-assignment controls to the GUI.

### Issue #27 historical interactive GUI status

Issue #27 interactive GUI smoke test: PASS — developer-attested after verifying existing graphing/navigation and calculator behavior; not an automated GUI acceptance test.

The named-variable interface remains a **Qt-independent core API** in this
issue. The passing offscreen `GraphCanvas` tests validate existing widget
regressions, but do not demonstrate GUI assignment syntax or slider support.
The historical Issue #26 failed GRA-03 JSON/Markdown record is retained. The
developer separately reported a successful visual retest of that fix during
Issue #26 closeout; an updated machine-readable post-fix session is not in this
attached verification document. Neither observation is attributed to new
Issue #27 interactive testing.

The historical Issue #25 Debug application was launched using:

```bash
./build-issue25/RationalStudio
```

The developer reports successful completion of the prescribed Issue #19 graphical verification and regression checks. The inspected graphs displayed the expected behavior, with no functional failures reported.

The previously completed graphical regression checks included:

| Expression / behavior | Verification goal |
|---|---|
| `y = 1/x` | Separate reciprocal branches near zero. |
| `y = 1/(x-0.133)` and `y = 1/(x+0.327)` | Avoid false joins across shifted poles. |
| `y = tan(x)` | Inspect branch separation near tangent poles (radian mode). |
| `y = floor(x)` and `y = sign(x)` | Inspect finite jump behavior. |
| `y = 500*x` | Preserve a steep continuous straight line. |
| `y = -2500*x` | Preserve a steep continuous line with negative slope. |
| `y = tanh(1000*x)` | Preserve continuity through a steep transition at the origin. |
| `y = atan(1000*x)` | Preserve continuity through a steep inverse-tangent transition. |
| `y = tanh(1000*(x-0.137))` | Preserve a steep continuous transition shifted away from the origin. |
| `y = x^5` and `y = exp(40*x)` | Inspect smooth but strongly varying continuous functions. |
| `y = x^2` and `y = sin(x)` | Preserve ordinary smooth curves. |
| `y = sqrt(x)` | Preserve restricted-domain behavior. |

The manual regression review also covered graph panning, cursor-anchored zoom, resizing, multiple functions, visibility toggles, coordinate tracing, workspace save/load, PNG export, and the non-graph calculator workspaces. No failures were reported.

### Issue #19 navigation verification

The Issue #19 Debug application was launched on Linux.

The prescribed navigation checks include:

- Cursor-anchored zoom at off-center positions.
- Repeated zoom-in and zoom-out operations.
- Zoom-limit behavior without viewport drift.
- Horizontal and vertical mouse panning.
- Repeated panning and zooming.
- Viewport reset and resizing.
- Saved-workspace viewport restoration.
- Existing graph rendering and PNG export regression checks.

The developer reports completing all prescribed Issue #19 navigation and graphical regression checks successfully, with no functional failures observed.

### Issue #20 cached-geometry manual verification

The developer reports completing the prescribed Issue #20 interactive GUI verification successfully after launching:

```bash
./build-issue20/RationalStudio
```

The verification focused on correct graph output and invalidation behavior around ordinary repaints, expression changes, visibility controls, panning/zooming, viewport resizing, angle-mode and previous-answer changes, workspace persistence, and PNG export. No functional failures were reported. This is a reported manual result, not an automated performance measurement.

Separately, `RationalGraphCanvasCacheTests` passed as an offscreen Qt widget test (49 assertions); it exercises cache behavior but is not an end-to-end GUI automation suite.

### Issue #21 sampling-budget verification

The Issue #21 Debug application was launched using `./build-issue21/RationalStudio`.

The developer explicitly confirms completing the prescribed Issue #21 interactive budget and graphing regression checks successfully, with no functional failures reported.

The recommended manual coverage comprises normal functions, steep continuous transitions, discontinuity regressions, multiple demanding functions, expression/visibility changes, pan/zoom, resizing, persistence, and PNG export. An application launch alone does not verify every interaction or demonstrate a measured frame-time improvement.

Separately, `RationalGraphCanvasBudgetTests` passed as an offscreen Qt widget regression suite with 20 assertions; it is not complete end-to-end GUI automation.

### Issue #22 navigation scheduling manual verification

The developer initially observed visible line flashing during graph panning. After a subsequent fix, the developer explicitly confirmed the panning flicker was resolved and repeated the Issue #22 interactive GUI checks successfully. The fix and retest are operator-attested and are not inferred from the original CTest run.

The original Qt offscreen scheduling suite passed 57 assertions, but it does not replace manual continuous-drag visual verification. Temporary previews may omit newly exposed graph regions until resampling completes; sustained flashing or blank-path transitions should be investigated.

### Issue #23 viewport boundary and resize manual verification

The developer reports completing Issue #23 interactive GUI verification successfully, including resizing, extreme zoom values, invalid viewport behavior, and regression checks for previously implemented graphing features. No functional failures or corrupted geometry were reported. The previous Issue #22 panning flicker was not reported to recur.

During the Debug GUI launch, the console printed four `kf.kio.widgets.kdirmodel: No node found for item that was just removed` messages referring to `CMakeLists.txt`, `LICENSE.MD`, `README.md`, and `VERIFICATION.md`. These are KDE KIO file-model runtime diagnostics; the report includes no accompanying crash or failed user-facing workflow. Their root cause was not established, and their appearance is not evidence that graph geometry was corrupted. They are recorded for traceability, not marked as a failed regression test.

The direct `RationalGraphCanvasBoundaryTests` offscreen suite passed 80 assertions. The full GUI was **not** subjected to GUI-enabled ASan/UBSan or pixel-diff visual testing; manual checks are developer-reported observations.

### Issue #24 manual GUI regression verification

The developer reports completing the Issue #24 manual GUI regression pass successfully after launching the Debug application. No visible graphing, panning, zooming, resizing, or curve-rendering regression was reported. This is a developer-attested interactive check, not a pixel-comparison or automated end-to-end GUI test.

Separately, the new `RationalGraphCanvasRegressionMatrixTests` offscreen suite passed 92 assertions. The combined results provide focused integration coverage but do not guarantee all possible Qt event orders, graph functions, or window-system environments.

### Issue #25 manual GUI and benchmark verification

The developer reports successfully completing the Issue #25 manual GUI regression pass after running the application. No failures were reported for the existing graph-rendering, panning, zooming, caching, or resizing functionality. This report distinguishes the developer-attested GUI check from synthetic offscreen benchmark events.

The Release benchmark runner completed successfully with 150 sampling measurements and 750 canvas/navigation measurements and generated five local output files (`graph_sampling.csv`, `graph_sampling.json`, `graph_canvas.csv`, `graph_canvas.json`, and `RESULTS.md`). Aggregate median/p95 values are recorded in the Issue #25 benchmark section above, using the files present at documentation-update time. These measurements are a reproducible **initial baseline**, not evidence of a performance improvement relative to Issue #24.

**Scope:** Manual functional checks and offscreen widget tests are not complete end-to-end GUI automation, pixel-comparison testing, accessibility validation, or controlled display/compositor performance benchmarking. The recorded results do not establish mathematically perfect graph rendering.

### Issue #26 manual GUI evidence and final-fix retest status

The local recorder report was updated at **2026-10-24T01:17:42+00:00** (October 23 in the development timezone), for commit `a4e6150063c0ce2cf3dbdebeb2b0f93448c5ecb5` with the recorder files untracked. It identifies Linux, KDE/Wayland, and `build-issue26/RationalStudio`.

**Recorded manual assessment: FAIL — 24 passed, 1 failed, 0 skipped, 0 not run.** Of 22 required checks, 21 passed and `GRA-03` failed; all three optional checks passed. The failed observation describes tangent branches clustered on the left when zoomed out, with screenshot references `GRA-03-Zoom-Out-Fail.png` and `GRA-03-Zoom-Out-Fail-Zoomed_In.png` in `build-issue26/`.

The other recorded passes cover launch, ordinary/restricted-domain/steep curves, tracing, panning and prior flicker, zoom, navigation, resizing, editing, eight demanding curves, persistence, PNG export, other calculator workspaces, and settled-view checks. Optional passes include malformed workspace handling, an interactive Release build, and an additional display scale/monitor. These observations predate the final rendering fix and are not evidence of a post-fix manual PASS.

The final fix has the passing automated and focused rendered-pixel coverage in Section 3. **A fresh operator-attested manual retest of the final implementation has not been recorded.** Keep the original failed session as historical evidence and use a new recorder output file for a retest, as described in `docs/Issue26GraphingManualQA.md`.

**Local evidence fingerprints:**

- `verification-results/issue26/manual_gui.json` — SHA-256 `29b50cb4edd60105fbbf87ee0e1fd7824813efa43a19af3b2e09aa784d3170bd`.
- `verification-results/issue26/manual_gui.md` — SHA-256 `802603754c20a5426c2004b19eb0ad337911ab5c659ae0923471e9450023ca52`.

Benchmark sources/results and verification reports remain local and excluded from Git by the current `.gitignore`. The Issue #25 source fingerprints above match the inspected local files. Their historical aggregates are retained here; neither those measurements nor the original manual evidence have been overwritten for this update.

## 6. Terminal application

The `RationalCLI` target built successfully in the historical Issue #25 Debug, Release, and core-only sanitizer configurations and the current Issue #26 full Debug build. The current Release rebuild targeted the desktop application only. A historical manual CLI smoke test produced:

```text
Input:     5/2 7/4
Operation: +
Result:    5/2 + 7/4 = 17/4
```

No separate interactive CLI execution was included in the supplied Issue #25 terminal record. The CLI compiled in the Issue #25 Debug, Release, and sanitizer-enabled configurations; the historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled (Issue #40) | PASS — 187/187 steps, 0 warnings | **45/45 PASS** | Intersection Qt offscreen PASS; manual: PASS — developer-reported Issue #40 intersection-analysis desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Linux Release, Qt GUI enabled (Issue #40) | PASS — 187/187 steps, 0 warnings | **45/45 PASS** | Intersection Qt offscreen PASS; interactive Release not separately attested |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #40) | PASS — 69/69 steps | **28/28 PASS** | Qt-free intersection ASan/UBSan PASS; Qt excluded |
| Linux Debug, Qt GUI enabled (Issue #39) | PASS — 176/176 steps, 0 warnings | **43/43 PASS** | Implicit offscreen Qt PASS; manual: PASS — developer-reported Issue #39 implicit-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Linux Release, Qt GUI enabled (Issue #39) | PASS — 176/176 steps, 0 warnings | **43/43 PASS** | Implicit Qt offscreen PASS; interactive Release not separately attested |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #39) | PASS — 66/66 steps | **27/27 PASS** | Qt-independent implicit contour ASan/UBSan PASS; Qt excluded |
| Linux Debug, Qt GUI enabled (Issue #38) | PASS — 165/165 steps, 0 warnings | **41/41 PASS** | Polar offscreen Qt PASS; manual: PASS — developer-reported Issue #38 polar-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Linux Release, Qt GUI enabled (Issue #38) | PASS — 165/165 steps, 0 warnings | **41/41 PASS** | Polar offscreen Qt PASS; interactive Release not separately attested |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #38) | PASS — 63/63 steps | **26/26 PASS** | Qt-free polar ASan/UBSan PASS; Qt excluded |
| Linux Debug, Qt GUI enabled (Issue #37) | PASS — 155/155 build, 0 warnings | **39/39 PASS** | Offscreen parametric GUI PASS; desktop: PASS — developer-reported Issue #37 interactive desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Linux Release, Qt GUI enabled (Issue #37) | PASS — 155/155 build, 0 warnings | **39/39 PASS** | Offscreen parametric GUI PASS; interactive Release not separately attested |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #37) | PASS — 61/61 build | **25/25 PASS** | Qt-free sampler sanitizer PASS; Qt Widgets excluded |
| Linux Debug, Qt GUI enabled (Issue #36) | PASS — 144/144 build | **37/37 PASS** | Existing offscreen GUI regressions PASS; manual desktop: PASS — developer-reported Issue #36 desktop regression smoke check; not independently recorded step by step or separately verified in Release. |
| Linux Release, Qt GUI enabled (Issue #36) | PASS — 144/144 build | **37/37 PASS** | Existing Qt offscreen CTests PASS; separate interactive Release launch unverified |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #36) | PASS — 58/58 build | **24/24 PASS** | Both new direct suites PASS under ASan/UBSan; Qt excluded |
| Linux Debug, Qt GUI enabled (Issue #35) | PASS — 140/140 build | **35/35 PASS** | Offscreen workspace regression PASS; PASS — developer-reported Issue #35 desktop workspace save/open, restored variables/functions/sliders and GUI regression check; not an independently instrumented case-by-case acceptance run. File-dialog warnings observed. |
| Linux Release, Qt GUI enabled (Issue #35) | PASS — 140/140 build | **35/35 PASS** | Qt offscreen regression PASS; separate Release desktop launch not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #35) | PASS — 54/54 build | **22/22 PASS** | Qt-dependent persistence excluded from sanitizer configuration |
| Linux Debug, Qt GUI enabled (Issue #34) | PASS — 132/132 build | **34/34 PASS** | Offscreen keyboard regression PASS; PASS — developer-reported manual Issue #34 desktop keyboard and graph interaction check; not an independently recorded case-by-case acceptance session. |
| Linux Release, Qt GUI enabled (Issue #34) | PASS — 132/132 build | **34/34 PASS** | Offscreen Qt suites PASS; interactive Release desktop run not separately reported |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #34) | PASS — 54/54 build | **22/22 PASS** | Core-only testing; Qt keyboard shortcuts excluded |
| Linux Debug, Qt GUI enabled (Issue #33) | PASS — build final 86/86 | **33/33 PASS** | Offscreen inline test PASS; PASS — developer-reported manual Issue #33 desktop GUI check; not independently instrumented. |
| Linux Release, Qt GUI enabled (Issue #33) | PASS — 124/124 build | **33/33 PASS** | Offscreen GUI tests PASS; interactive Release launch not separately attested |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #33) | PASS — 54/54 build | **22/22 PASS** | Qt-free diagnostics PASS; Qt Widgets excluded |
| Linux Debug, Qt GUI enabled (Issue #32) | PASS — incremental 6/6 build | **31/31 PASS** | Qt editor offscreen PASS; PASS — developer-attested Issue #32 manual GUI checklist; not an automated desktop test. |
| Linux Release, Qt GUI enabled (Issue #32) | PASS — 114/114 build | **31/31 PASS** | Qt editor offscreen PASS; interactive Release launch not separately recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #32) | PASS — 52/52 build | **21/21 PASS** | Qt-free input checked; Qt excluded |
| Linux Debug, Qt GUI enabled (Issue #31) | PASS — full build (101/101 build steps completed) | **29/29 PASS** | Qt offscreen refresh test PASS; PASS — developer-reported manual desktop GUI acceptance: slider variables are accepted in graph expressions and changing sliders updates plotted curves. |
| Linux Release, Qt GUI enabled (Issue #31) | PASS — full build (101/101 build steps completed) | **29/29 PASS** | Qt offscreen tests PASS; separate interactive Release desktop run not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #31) | PASS — full core-only build (50/50 build steps completed) | **20/20 PASS** | Qt-independent dependencies tested; Qt Widgets excluded |
| Linux Debug, Qt GUI enabled (Issue #30) | CMake/CTest PASS | **27/27 PASS** | Offscreen slider panel and earlier Qt suites PASS; interactive GUI not separately attested |
| Linux Release, Qt GUI enabled (Issue #30) | CMake/CTest PASS | **27/27 PASS** | Offscreen Qt suites PASS; interactive Release run not separately recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #30) | CMake/CTest PASS | **19/19 PASS** | Qt-independent suite PASS; Qt Widgets excluded |
| Linux Debug, Qt GUI enabled (Issue #29) | PASS (full build) | **25/25 PASS** | Qt offscreen regressions PASS; informal manual GUI check PASS (developer-reported) |
| Linux Release, Qt GUI enabled (Issue #29) | PASS (full build) | **25/25 PASS** | Qt offscreen regressions PASS; interactive Release GUI launch not separately recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #29) | PASS | **18/18 PASS** | Qt-independent suite PASS; Qt GUI renderer excluded |
| Linux Debug, Qt GUI enabled (Issue #28) | PASS (incremental build) | **24/24 PASS** | Qt offscreen regressions PASS; informal manual GUI check PASS (developer-reported) |
| Linux Release, Qt GUI enabled (Issue #28) | PASS | **24/24 PASS** | Qt offscreen regressions PASS; interactive Release launch not separately recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #28) | PASS | **17/17 PASS** | Entire Qt-independent suite passed; GUI renderer excluded |
| Linux Debug, Qt GUI enabled (Issue #27) | PASS | **23/23 PASS** | Qt offscreen regression tests passed; Issue #27 manual smoke-test status recorded in Section 5 |
| Linux Release, Qt GUI enabled (Issue #27) | PASS | **23/23 PASS** | Qt offscreen regression tests passed; interactive Release launch not separately recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #27) | PASS | **16/16 PASS** | Entire Qt-independent suite passed; Qt GUI excluded |
| Linux Debug, Qt GUI enabled (Issue #25) | PASS | 18/18 PASS | Manual GUI regression verification PASS (developer-reported); Qt widget tests PASS |
| Linux Release, Qt GUI enabled (Issue #25) | PASS | 18/18 PASS | Offscreen Qt widget tests PASS; 150 core sampling and 750 Qt benchmark observations recorded; interactive Release GUI launch not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #25) | PASS | 13/13 PASS | Core regression and statistics test execution PASS; Qt GUI excluded |
| Linux Debug, Qt GUI enabled (Issue #26) | PASS | 22/22 PASS | Offscreen pixel/preview/repaint regressions and 2× display-scale test PASS; final manual retest not recorded |
| Linux Release desktop target (Issue #26) | PASS | Full current suite not run | Final interactive Release retest not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #26) | Target PASS | Direct sampling job: 896,291 assertions PASS | Leak detection disabled; full current core suite and Qt GUI excluded from this result |
| Python manual evidence recorder (Issue #26) | Not applicable | 12/12 unit tests PASS | Historical manual report: 24 PASS / 1 FAIL; final manual retest not recorded |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Issues #27–#31 provide scoped variables, dependent scalar assignments, compiled functions, interactive sliders, and selective graph refresh. **Issue #35 persists slider settings, scalar assignment sources and reusable function definitions in version-2 JSON workspaces**, with transactional validation and legacy version-1 loading. GUI authoring of function/assignment definitions is not yet implemented; scalar assignments still cannot depend on `x` or `ans`.
- Issue #32 provides a **single-line** editor, limited supported Unicode glyph normalization, selection/caret editing, and parser-backed syntax state. It does not offer a two-dimensional typesetting engine or exhaustive syntax classification. Symbol text remains visible; only parsed copies are normalized. GUI function-definition authoring remains pending; slider persistence was added in Issue #35.
- Issue #33 provides parser error categories, positions and readable inline feedback; it does not guarantee complete predictive syntax diagnostics, token-level underlining, or detection of all runtime mathematical domain failures. Invalid rows do not replace unrelated valid graph curves.
- Issue #34 adds editor-local Alt shortcuts and graph-row navigation/creation; OS/window-manager shortcuts may intercept key combinations, and no global shortcut registration or separate platform-specific keymap is claimed. The single-line editor and current 40-row workspace limit remain unchanged.
- Issue #35 saves version-2 JSON workspaces and loads version-1 files, but cannot guarantee successful writes on full/read-only filesystems or recover from arbitrary process/hardware failures. GUI function-definition editing remains unavailable; loading external definitions is supported. KDE file-dialog model and Qt mouse-grab warnings occurred during manual testing without an observed save/load failure.
- Issue #36 adds Qt-independent cross-component and deterministic numerical regression coverage only. It does not change runtime parser/evaluation behavior, resolve every possible floating-point case, or add new GUI or persistence functionality. New tests do not constitute Qt sanitizer coverage.
- Issue #38 supports polar radius curves over finite angular intervals through the existing parametric sampling pipeline. Angular bounds use the selected angle mode and are not automatically converted when switching RAD/DEG. Finite sampling may miss narrow features or singularities; polar parameter-aware tracing remains future work. Polar, parametric and Cartesian workspace entries use the existing version-3 mixed format; implicit/inequality rendering remains pending.
- Issue #37 supports explicit Cartesian and paired parametric X(t)/Y(t) curves with finite bounded parameter intervals, but does not guarantee recovery of every singularity, tightly folded segment, high-frequency feature or extremely narrow domain gap. Polar/implicit/inequality graphing and parameter-aware tracing remain future Milestone 3 work. Version-3 mixed workspaces preserve parametric rows; earlier version-1/2 formats remain readable. Core-only sanitizers exclude Qt.
- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Bounded terminal refinement improves steep continuous curves, but sufficiently narrow or extreme nonlinear features may still be omitted or approximated imperfectly.
- Highly oscillatory functions may be undersampled.
- Per-curve and shared per-paint callback-count limits bound evaluation counts, not the time spent in any callback or the total frame time. The single-call sampler can return truncated geometry. Current GUI sampling jobs resume unfinished work; complete replacement paths may take multiple paints.
- Valid `QPainterPath` geometry is cached; rapid navigation uses timer-coalesced refreshes and transformed cached-path previews. Resizes invalidate stale geometry and outdated refresh requests, while bounded pixel-space grid positions avoid unrepresentable label coordinates. At extreme finite centers, grid markers may be suppressed when floating-point precision is insufficient. Sampling remains on the GUI thread under callback budgets; no frame-time, input-latency, or FPS guarantee is claimed.
- Six offscreen Qt `GraphCanvas` executables cover cache, budget, scheduling, viewport boundaries, cross-feature behavior, and selected rendered-pixel/repaint comparisons, with a second coverage invocation at 2× display scale. Full desktop interaction automation and exhaustive rendered-pixel correctness are not established. Issue #25 timing measurements remain historical; benchmark tools are not CTests.
- A panning-flicker regression was reported during initial Issue #22 manual verification; the developer subsequently attested that it was corrected and retested.
- Retained preview geometry has finite surrounding coverage; sufficiently large navigation jumps can expose regions requiring fresh sampling. No arbitrary-zoom or universal flicker-free rendering guarantee is established. The original Issue #26 failed manual check still requires a recorded post-fix retest.
- The complete Qt application has not undergone recorded sanitizer testing.
- Native Windows builds, Qt 5 compatibility, and a post-fix interactive Release GUI retest remain unverified. The original Issue #26 optional Release check passed before the final fix.

## 9. Reproduction

On Linux with the documented dependencies installed, use fresh build directories:

```bash
# Full Debug build and tests
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
ctest --test-dir build-debug --output-on-failure
./build-debug/RationalStudio

# Full Release build and tests
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure

# Core-only ASan/UBSan build and tests
cmake -S . -B build-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

These generic commands describe a reproducible procedure. Current Issue #41 validation uses `verify_issue41.sh`, `build-issue41*` and `issue41-evidence/`. Historical Issue #40 validation used `build-issue40`, `build-issue40-release`, and `build-issue40-asan` via `verify_issue40.sh` with evidence in `issue40-evidence/`. Historical Issue #39 validation used `build-issue39`, `build-issue39-release`, and `build-issue39-asan` via `verify_issue39.sh` with evidence in `issue39-evidence/`. Historical Issue #38 validation used `build-issue38`, `build-issue38-release`, and `build-issue38-asan` through `verify_issue38.sh` with `issue38-evidence/`. Historical Issue #37 validation used `build-issue37`, `build-issue37-release`, and `build-issue37-asan` via `verify_issue37.sh` with logs in `issue37-evidence/`. Historical Issue #36 validation used `build-issue36`, `build-issue36-release` and `build-issue36-asan` via `verify_issue36.sh` with evidence under `issue36-evidence/`. Historical Issue #35 validation used `build-issue35`, `build-issue35-release`, and `build-issue35-asan` with `verify_issue35.sh` and `issue35-evidence/`. Historical Issue #34 validation used `build-issue34`, `build-issue34-release`, and `build-issue34-asan` with `verify_issue34.sh` and `issue34-evidence/`. Historical Issue #33 validation used `build-issue33`, `build-issue33-release`, and `build-issue33-asan` with `verify_issue33.sh` and `issue33-evidence/`. Historical Issue #32 validation used `build-issue32`, `build-issue32-release`, and `build-issue32-asan` with `verify_issue32.sh` and `issue32-evidence/`. Historical Issue #31 validation used `build-issue31`, `build-issue31-release`, and `build-issue31-asan` with `verify_issue31.sh` and recorded evidence in `issue31-evidence/`. Historical Issue #30 validation used `build-issue30`, `build-issue30-release`, and `build-issue30-asan` for GUI-enabled Debug, Release, and core-only ASan/UBSan tests, respectively. Historical Issue #29 validation used `build-issue29`, `build-issue29-release`, and `build-issue29-asan` for GUI-enabled Debug, Release, and core-only sanitizer CTests, respectively. Historical Issue #28 validation used `build-issue28`, `build-issue28-release`, and `build-issue28-asan` for GUI-enabled Debug, Release, and core-only sanitizer CTest suites, respectively. Historical Issue #27 validation used `build-issue27`, `build-issue27-release`, and `build-issue27-asan` for the prior complete Debug, Release, and core-only sanitizer suites, respectively. Historical Issue #26 implementation validation used `build-issue26`, `build-issue26-release` (desktop target), and `build-issue26-asan` (direct sampling-job sanitizer target). The latter directory was subsequently removed locally. Historical Issue #25 validation used `build-issue25`, `build-issue25-release`, and `build-issue25-asan`. The performance baseline was generated separately with `bash scripts/run_issue25_benchmarks.sh` against the Release benchmark targets.

## 10. Verification summary

**Issue #44 addendum:** GUI Debug/Release **52/52 CTests PASS** (18.49 / 12.30 s); core-only ASan/UBSan **31/31 PASS** (8.46 s); 25 direct core assertions per configuration; 26 GUI assertions per GUI configuration; 97 unchanged paths during verification. Manual acceptance: PASS — developer-reported Issue #44 advanced graph-tracing desktop acceptance; not independently instrumented case by case or separately attested in Release.


**Issue #43 addendum:** GUI Debug/Release **50/50 CTests PASS** (17.64 / 12.07 s), core-only ASan/UBSan **30/30 PASS** (8.32 s), 44 direct GUI assertions per configuration, 93 unchanged provenance entries during verification. Manual acceptance: PASS — developer-reported Issue #43 multi-representation graph-controls desktop acceptance; not independently instrumented case by case or separately attested in Release.


**Issue #42 addendum:** Debug/Release **49/49 CTests PASS** (18.17 / 12.19 s); core-only ASan/UBSan **30/30 PASS** (8.46 s); 89 direct core assertions, 41 Qt assertions per GUI build, and 92 unchanged provenance files during verification. Manual acceptance: PASS — developer-reported Issue #42 inequality shading desktop acceptance; not independently instrumented case by case or separately attested in Release.


**Issue #41 addendum:** GUI Debug and Release **47/47 CTests PASS** (17.12 / 11.19 s), core-only sanitizer **29/29 PASS** (8.25 s); direct core **66** assertions and GUI **31** assertions per supported configuration; 88 matching provenance paths. Manual GUI: PASS — developer-reported Issue #41 roots/extrema/critical-point desktop acceptance; not independently instrumented case by case or separately attested in Release. The historical tables below remain the Issue #40 record.


| Area | Result |
|---|---|
| Issue #40 Debug GUI-enabled build | **187/187 PASS; zero warnings** |
| Issue #40 Release GUI-enabled build | **187/187 PASS; zero warnings** |
| Issue #40 core-only ASan/UBSan build | **69/69 PASS** |
| Issue #40 Debug GUI-enabled CTest | **45/45 PASS** |
| Issue #40 Release GUI-enabled CTest | **45/45 PASS** |
| Issue #40 core-only ASan/UBSan CTest | **28/28 PASS** |
| Issue #40 Qt-free intersection detection (each of 3 configurations) | **155 assertions PASS** |
| Issue #40 Qt intersection integration (Debug and Release) | **16 assertions PASS** |
| Issue #40 before/after provenance | **84 identical entries** |
| Issue #40 manual desktop acceptance | PASS — developer-reported Issue #40 intersection-analysis desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Issue #39 Debug GUI-enabled build | **176/176 PASS; zero warnings** |
| Issue #39 Release GUI-enabled build | **176/176 PASS; zero warnings** |
| Issue #39 core-only ASan/UBSan build | **66/66 PASS** |
| Issue #39 Debug GUI-enabled CTest | **43/43 PASS** |
| Issue #39 Release GUI-enabled CTest | **43/43 PASS** |
| Issue #39 core-only ASan/UBSan CTest | **27/27 PASS** |
| Issue #39 Qt-free implicit contour (each of 3 configurations) | **796 assertions PASS** |
| Issue #39 Qt implicit integration (Debug and Release) | **33 assertions PASS** |
| Issue #39 before/after provenance | **80 identical entries** |
| Issue #39 manual desktop acceptance | PASS — developer-reported Issue #39 implicit-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Issue #38 Debug GUI-enabled build | **165/165 PASS; zero warnings** |
| Issue #38 Release GUI-enabled build | **165/165 PASS; zero warnings** |
| Issue #38 core-only ASan/UBSan build | **63/63 PASS** |
| Issue #38 Debug GUI-enabled CTest | **41/41 PASS** |
| Issue #38 Release GUI-enabled CTest | **41/41 PASS** |
| Issue #38 core-only ASan/UBSan CTest | **26/26 PASS** |
| Issue #38 Qt-free polar adapter (each of 3 configurations) | **511 assertions PASS** |
| Issue #38 Qt polar integration (Debug and Release) | **34 assertions PASS** |
| Issue #38 before/after provenance | **76 identical entries** |
| Issue #38 manual desktop acceptance | PASS — developer-reported Issue #38 polar-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Issue #37 Debug GUI-enabled build | **155/155 PASS; zero warnings** |
| Issue #37 Release GUI-enabled build | **155/155 PASS; zero warnings** |
| Issue #37 core-only ASan/UBSan build | **61/61 PASS** |
| Issue #37 Debug GUI-enabled CTest | **39/39 PASS** |
| Issue #37 Release GUI-enabled CTest | **39/39 PASS** |
| Issue #37 core-only ASan/UBSan CTest | **25/25 PASS** |
| Issue #37 Qt-free parametric sampler (each of 3 configurations) | **291 assertions PASS** |
| Issue #37 Qt parametric integration (Debug and Release) | **32 assertions PASS** |
| Issue #37 before/after provenance | **73 identical entries** |
| Issue #37 manual desktop acceptance | PASS — developer-reported Issue #37 interactive desktop acceptance; not independently instrumented case by case or separately attested in Release. |
| Issue #36 Debug GUI-enabled build | **144/144 PASS** |
| Issue #36 Release GUI-enabled build | **144/144 PASS** |
| Issue #36 core-only ASan/UBSan build | **58/58 PASS** |
| Issue #36 Debug GUI-enabled CTest | **37/37 PASS** |
| Issue #36 Release GUI-enabled CTest | **37/37 PASS** |
| Issue #36 core-only ASan/UBSan CTest | **24/24 PASS** |
| Issue #36 variable/function interaction assertions (each of 3 configs) | **80 PASS** |
| Issue #36 dynamic-evaluation matrix assertions (each of 3 configs) | **15,940 PASS** |
| Issue #36 before/after source provenance | **69 matching entries** |
| Issue #36 manual GUI smoke acceptance | PASS — developer-reported Issue #36 desktop regression smoke check; not independently recorded step by step or separately verified in Release. |
| Issue #35 Debug GUI-enabled build | **140/140 PASS** |
| Issue #35 Release GUI-enabled build | **140/140 PASS** |
| Issue #35 core-only ASan/UBSan build | **54/54 PASS** |
| Issue #35 Debug GUI-enabled CTest | **35/35 PASS** |
| Issue #35 Release GUI-enabled CTest | **35/35 PASS** |
| Issue #35 core-only ASan/UBSan CTest | **22/22 PASS** |
| Issue #35 direct Qt persistence assertions | **55 PASS** |
| Issue #35 before/after provenance | **67 matching entries** |
| Issue #35 manual desktop GUI acceptance | PASS — developer-reported Issue #35 desktop workspace save/open, restored variables/functions/sliders and GUI regression check; not an independently instrumented case-by-case acceptance run. |
| Issue #35 KDE/Qt dialog diagnostics | Observed, non-blocking in reported desktop session |
| Issue #34 Debug GUI-enabled build | **132/132 PASS** |
| Issue #34 Release GUI-enabled build | **132/132 PASS** |
| Issue #34 core-only ASan/UBSan build | **54/54 PASS** |
| Issue #34 Debug GUI-enabled CTest | **34/34 PASS** |
| Issue #34 Release GUI-enabled CTest | **34/34 PASS** |
| Issue #34 core-only ASan/UBSan CTest | **22/22 PASS** |
| Issue #34 direct Qt keyboard shortcut assertions | **35 PASS** |
| Issue #34 before/after provenance | **65 matching entries** |
| Issue #34 manual desktop GUI acceptance | PASS — developer-reported manual Issue #34 desktop keyboard and graph interaction check; not an independently recorded case-by-case acceptance session. |
| Issue #33 Debug GUI-enabled build (final progress) | **86/86 PASS** |
| Issue #33 Release GUI-enabled build | **124/124 PASS** |
| Issue #33 core-only ASan/UBSan build | **54/54 PASS** |
| Issue #33 Debug GUI-enabled CTest | **33/33 PASS** |
| Issue #33 Release GUI-enabled CTest | **33/33 PASS** |
| Issue #33 core-only ASan/UBSan CTest | **22/22 PASS** |
| Issue #33 diagnostic direct assertions | **323 PASS** |
| Issue #33 Qt inline-feedback direct assertions | **19 PASS** |
| Issue #33 provenance manifests | **64 matching paths** |
| Issue #33 manual desktop GUI check | PASS — developer-reported manual Issue #33 desktop GUI check; not independently instrumented. |
| Issue #32 Debug GUI-enabled incremental build | **6/6 steps PASS** |
| Issue #32 Release GUI-enabled build | **114/114 steps PASS** |
| Issue #32 core-only ASan/UBSan build | **52/52 steps PASS** |
| Issue #32 Debug GUI-enabled CTest | **31/31 PASS** |
| Issue #32 Release GUI-enabled CTest | **31/31 PASS** |
| Issue #32 core-only ASan/UBSan CTest | **21/21 PASS** |
| Issue #32 mathematical input assertions | **4,032 PASS** |
| Issue #32 Qt editor assertions | **19 PASS** |
| Issue #32 matching provenance manifests | **62 paths, identical during verification** |
| Issue #32 desktop GUI acceptance | PASS — developer-attested Issue #32 manual GUI checklist; not an automated desktop test. |
| Issue #31 Debug GUI/CLI/core full build | **101/101 build steps PASS** |
| Issue #31 Release GUI/CLI/core full build | **101/101 build steps PASS** |
| Issue #31 core-only ASan/UBSan build | **50/50 build steps PASS** |
| Issue #31 GUI-enabled Debug CTest suite | **29/29 PASS** |
| Issue #31 GUI-enabled Release CTest suite | **29/29 PASS** |
| Issue #31 core-only ASan/UBSan CTest suite | **20/20 PASS** |
| Issue #31 variable-dependency regression assertions | **1,223 PASS** |
| Issue #31 Qt graph-refresh regression assertions | **40 PASS** |
| Issue #31 before/after SHA-256 provenance manifest | **IDENTICAL** during run |
| Issue #31 manual desktop GUI acceptance | PASS — developer-reported manual desktop GUI acceptance: slider variables are accepted in graph expressions and changing sliders updates plotted curves. |
| Issue #30 GUI-enabled Debug CTest suite | **27/27 PASS** |
| Issue #30 GUI-enabled Release CTest suite | **27/27 PASS** |
| Issue #30 core-only ASan/UBSan CTest suite | **19/19 PASS** |
| Issue #30 Qt-independent slider assertions | 14,785 PASS |
| Issue #30 Qt slider-panel assertions | 47 PASS |
| Issue #30 interactive GUI acceptance | Not separately attested in this documentation update; offscreen Qt tests passed. |
| GCC 15.2.0 Debug GUI/CLI/core full build — Issue #29 | **PASS** |
| GCC 15.2.0 Release GUI/CLI/core full build — Issue #29 | **PASS** |
| GUI-enabled Debug CTest suite — Issue #29 | **25/25 PASS** |
| GUI-enabled Release CTest suite — Issue #29 | **25/25 PASS** |
| Core-only ASan/UBSan CTest suite — Issue #29 | **18/18 PASS** |
| Function-definitions regression assertions — Issue #29 | **1,143 PASS** |
| Issue #29 interactive GUI regression check | PASS — informal developer-attested smoke check, not scripted end-to-end QA |
| GCC 15.2.0 Debug GUI/CLI/core incremental build — Issue #28 | **PASS** |
| GCC 15.2.0 Release GUI/CLI/core full build — Issue #28 | **PASS** |
| GUI-enabled Debug CTest suite — Issue #28 | **24/24 PASS** |
| GUI-enabled Release CTest suite — Issue #28 | **24/24 PASS** |
| Core-only ASan/UBSan CTest suite — Issue #28 | **17/17 PASS** |
| Variable-definitions regression assertions — Issue #28 | **951 PASS** |
| Issue #28 interactive GUI regression check | PASS — informal developer-attested smoke check, not scripted end-to-end QA |
| GCC 15.2.0 Debug GUI/CLI/core build — Issue #27 | **PASS** |
| GCC 15.2.0 Release GUI/CLI/core build — Issue #27 | **PASS** |
| GUI-enabled Debug CTest suite — Issue #27 | **23/23 PASS** |
| GUI-enabled Release CTest suite — Issue #27 | **23/23 PASS** |
| Core-only ASan/UBSan CTest suite — Issue #27 | **16/16 PASS** |
| Named-variable regression assertions — Issue #27 | **3,149 PASS** |
| Issue #27 interactive GUI smoke test | PASS — developer-attested; not independently automated |
| GCC 15.2.0 Debug GUI/CLI/core build — Issue #25 | PASS |
| GCC 15.2.0 Release GUI/CLI/core build — Issue #25 | PASS |
| GUI-enabled Debug CTest suite — Issue #26 | **22/22 PASS** |
| GUI-enabled Release CTest suite — Issue #25 historical | **18/18 PASS**; full Issue #26 suite not recorded |
| Core-only ASan/UBSan CTest suite — Issue #25 historical | **13/13 PASS** |
| Issue #26 Debug full build / Release desktop rebuild | PASS / PASS |
| Issue #26 incremental sampling assertions | **896,291 PASS**, including direct ASan/UBSan run with leak detection disabled |
| Issue #26 canvas / 2× display-scale assertions | **33,628 PASS / 493 PASS** |
| Manual evidence recorder unit tests | **12/12 PASS** |
| Issue #26 original manual GUI session | **24 PASS / 1 FAIL** (`GRA-03`); final-fix manual retest not recorded |
| Benchmark statistics regression assertions | **16 PASS** |
| Qt-independent sampling measurements | **150 recorded** (Release benchmark; six groups) |
| Offscreen Qt rendering and navigation measurements | **750 recorded** (Release benchmark; thirty groups) |
| Sampling median/p95 and evaluation/build totals | Recorded above from benchmark JSON; host-specific |
| Cold/warm render and wheel/drag preview/settlement median/p95 | Recorded above from benchmark JSON; offscreen synthetic events |
| Existing graphing/viewport/cache/budget/scheduling/boundary regressions | PASS — included in current CTest runs |
| Issue #25 Debug GUI launch | Executed |
| Issue #25 manual GUI regression verification | PASS — developer-reported |
| Issue #25 compiler warnings | None displayed in supplied final build output; not a `-Werror` verification |
| Interactive Release GUI verification | Original Issue #26 optional check PASS; final-fix retest not recorded |
| Previous Issue #23 KDE KIO diagnostics | Historical observation; not reported in Issue #25 supplied output |
| Prior CLI smoke test | PASS — previous baseline |
| Full GUI sanitizer coverage | Not performed |
| Targeted rendered-pixel and identical-repaint tests | PASS — Issue #26; full desktop end-to-end automation not performed |
| Native Windows verification | Not performed |

**Current assessment — Issue #40:** Linux GCC 15.2.0 GUI-enabled Debug **45/45 CTests PASS** (15.39 s), Release **45/45 CTests PASS** (9.52 s), and core-only ASan/UBSan **28/28 CTests PASS** (8.29 s). Qt-independent intersection detection passed **155** direct assertions in Debug/Release/ASan; Qt offscreen intersection integration passed **16** direct assertions in Debug/Release. The **84** recorded provenance paths matched before/after testing; no GUI build compiler warnings were recorded. Manual GUI: PASS — developer-reported Issue #40 intersection-analysis desktop acceptance; not independently instrumented case by case or separately attested in Release. The implementation handles bounded pairwise explicit Cartesian curves only; tangency detection is heuristic and finite sampling may miss roots or report spurious ones near discontinuities. Qt sanitizers, native Windows, Qt 5 and controlled speedup claims remain unverified. Historical Issue #25 benchmark measurements remain unchanged.

**Historical assessment — Issue #39:** Linux GCC 15.2.0 GUI-enabled Debug **43/43 CTests PASS** (14.16 s), Release **43/43 CTests PASS** (8.20 s), and core-only ASan/UBSan **27/27 CTests PASS** (8.21 s). Qt-free contour extraction passed **796** direct assertions in Debug/Release/ASan; Qt offscreen implicit integration passed **33** direct assertions in Debug/Release. The **80** recorded provenance paths matched before/after testing; no GUI build compiler warnings were recorded. Manual GUI: PASS — developer-reported Issue #39 implicit-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. Marching squares has finite-grid and discontinuity limitations; full Qt sanitizers, Windows, Qt 5 and controlled performance improvement remain unverified. The Issue #25 benchmark is retained as historical evidence.

**Historical assessment — Issue #38:** Linux GCC 15.2.0 GUI-enabled Debug **41/41 CTests PASS** (13.33 s), Release **41/41 CTests PASS** (7.84 s), and core-only ASan/UBSan **26/26 CTests PASS** (8.02 s). Qt-free polar conversion passed **511** direct assertions in Debug, Release and ASan/UBSan; Qt offscreen polar integration passed **34** assertions in Debug and Release. **76** recorded provenance paths matched before/after automated testing; zero Debug/Release compiler warnings were recorded. Manual GUI: PASS — developer-reported Issue #38 polar-graphing desktop acceptance; not independently instrumented case by case or separately attested in Release. Full Qt sanitizer coverage, native Windows, Qt 5 and controlled performance improvements remain unverified. Issue #25 benchmark measurements are retained as historical evidence.

**Historical assessment — Issue #37:** Linux GCC 15.2.0 Debug and Release GUI-enabled **39/39 CTests PASS** each (13.25 s / 7.30 s), core-only ASan/UBSan **25/25 PASS** (8.41 s). **291** Qt-free sampler assertions passed in Debug, Release and ASan/UBSan; **32** Qt offscreen parametric integration assertions passed in Debug and Release. Before/after provenance matches across 73 files. Both GUI builds recorded zero compiler warnings after the targeted correction. Manual GUI: PASS — developer-reported Issue #37 interactive desktop acceptance; not independently instrumented case by case or separately attested in Release. Qt sanitizer, native Windows, Qt 5 and controlled performance benchmarks remain unverified; Issue #25 measurements remain historical.

**Historical assessment — Issue #36:** Linux GCC 15.2.0 GUI-enabled Debug **37/37 CTests PASS** (12.07 s), Release **37/37 CTests PASS** (6.97 s), and core-only ASan/UBSan **24/24 CTests PASS** (8.13 s). Both new direct suites passed **80** and **15,940** assertions respectively in all three configurations. The 69-entry source/documentation provenance manifests match during testing. Manual GUI: PASS — developer-reported Issue #36 desktop regression smoke check; not independently recorded step by step or separately verified in Release. This is test-only hardening, not a new product capability or controlled performance measurement; Qt application sanitizer coverage, Windows, and Qt 5 remain unverified. Historical Issue #25 benchmark results are unchanged.

**Historical assessment — Issue #35:** Linux GCC 15.2.0 GUI-enabled **35/35 Debug** and **35/35 Release CTests PASS**, core-only ASan/UBSan **22/22 CTests PASS**. The direct offscreen Qt workspace regression passed **55 assertions**; 67 recorded SHA-256 source/documentation entries match before/after testing. Manual GUI: PASS — developer-reported Issue #35 desktop workspace save/open, restored variables/functions/sliders and GUI regression check; not an independently instrumented case-by-case acceptance run. KDE/Qt directory-model and mouse-grab warnings were observed, but no workspace save/open malfunction was reported. The Qt application lacks full sanitizer coverage; Windows, Qt 5 and controlled speedup claims are unverified. Issue #25 benchmark measurements remain historical.

**Historical assessment — Issue #34:** Linux GCC 15.2.0 GUI-enabled Debug and Release **34/34 CTests PASS** each, core-only ASan/UBSan **22/22 CTests PASS**. Offscreen Qt shortcut regression **35 assertions PASS**; 65-entry recorded source manifests match during verification. Manual GUI: PASS — developer-reported manual Issue #34 desktop keyboard and graph interaction check; not an independently recorded case-by-case acceptance session. The Qt-only behavior has no sanitizer coverage, and Windows/Qt 5 compatibility and any speedup remain unverified. Issue #25 benchmark measurements remain historical.

**Historical assessment — Issue #33:** Linux GCC 15.2.0 GUI-enabled **33/33 Debug and 33/33 Release CTests PASS**, core-only ASan/UBSan **22/22 CTests PASS**. Direct suites report **323 structured diagnostic** and **19 offscreen GUI feedback** assertions. The 64-entry provenance manifests match during testing. Manual GUI: PASS — developer-reported manual Issue #33 desktop GUI check; not independently instrumented. No Windows, Qt 5, complete Qt sanitizer, or controlled speedup claims are made; Issue #25 performance data remain historical.

**Historical assessment — Issue #32:** GCC 15.2.0 Linux GUI-enabled Debug and Release **31/31 CTests PASS** each, core-only ASan/UBSan **21/21 CTests PASS**. **4,032** Qt-independent mathematical-input and **19** offscreen Qt-editor assertions passed. Before/after source provenance matches across 62 paths. Manual GUI: PASS — developer-attested Issue #32 manual GUI checklist; not an automated desktop test. No native Windows/Qt 5, full Qt sanitizer or controlled performance improvement is claimed; the Issue #25 benchmark is historical.

**Historical assessment — Issue #31:** Linux GCC 15.2.0 GUI-enabled Debug and Release **29/29 CTests PASS** each, core-only ASan/UBSan **20/20 CTests PASS**. New direct suites report **1,223** dependency assertions and **40** Qt refresh assertions. Before/after manifests covering 57 paths are identical during testing. Manual GUI: PASS — developer-reported manual desktop GUI acceptance: slider variables are accepted in graph expressions and changing sliders updates plotted curves. Selective mathematical invalidation is covered by tests, but no new benchmark comparison or speedup is claimed. Slider persistence and GUI function authoring remain pending. Full Qt sanitizers, Windows, and Qt 5 remain unverified.

**Historical assessment — Issue #30:** The user-supplied local CTest records establish **27/27 Debug, 27/27 Release, and 19/19 core-only ASan/UBSan PASS**. `RationalVariableSliderScaleTests` passed **14,785 assertions** and `RationalVariableSliderPanelTests` passed **47 assertions** in the Debug CTest evidence. The result supports tested slider scale mathematics and offscreen Qt widget behavior. Not separately attested in this documentation update; offscreen Qt tests passed. Automatic graph binding/refresh and slider persistence remain planned for Issues #31 and #35. The Issue #25 benchmark baseline is historical; no new performance improvement is inferred.

**Historical assessment — Issue #29:** GCC 15.2.0 GUI-enabled Debug **25/25** and Release **25/25** CTests passed; the full core-only ASan/UBSan suite passed **18/18**. `RationalFunctionDefinitionsTests` passed **1,143 assertions**. These results support Qt-independent reusable functions, validated arity and parameter scoping, calls to previously defined functions, scalar binding captures, recursion/cycle rejection, and safe published scope snapshots. The developer reports an informal manual regression check of the existing GUI with no problems observed. Function-definition editing, automatic graph invalidation, sliders, and persistence remain future GUI integration work. No new performance measurements were supplied; the Issue #25 baseline remains historical.

**Historical assessment — Issue #28:** GCC 15.2.0 GUI-enabled Debug **24/24** and Release **24/24** CTests passed; the full core-only ASan/UBSan suite passed **17/17**. The new `VariableDefinitions` regression executable passed **951 assertions**. These results support Qt-independent scalar assignments, explicit dependencies, selective updates, undefined propagation, and transactional rejection of malformed or circular redefinitions. Existing GUI behavior was checked informally by the developer with no reported regressions. The GUI does not yet offer interactive assignment rows or sliders; dependency-driven graph updates, function definitions, and variable persistence remain future work. No new performance claims are made; Issue #25 benchmark measurements are preserved as historical data.

**Historical assessment — Issue #27:** GCC 15.2.0 Debug and Release GUI-enabled builds and all 23 CTests passed in each configuration; all 16 core-only ASan/UBSan CTests passed; the named-variable suite passed 3,149 assertions. Explicit named-variable declaration, scope validation, binding resolution, and legacy expression compatibility are supported by this verification. No GUI assignment, slider, dependency tracking, or function-definition implementation is claimed. The original Issue #26 failed manual session and Issue #25 benchmark measurements remain historical evidence.

**Historical assessment — Issue #26:** The Issue #26 fix is supported by a successful Linux Debug full build, **22/22 passing GUI-enabled CTests**, **896,291 passing incremental sampling assertions** including a direct ASan/UBSan run, **12 passing recorder unit tests**, and a successful Release desktop rebuild. Targeted core and rendered-pixel checks cover tangent tails across tested zooms and widths, callback accounting, deterministic completion, retained previews, same-run image stability, resizing, and 2× display scaling. The original manual report still records the tangent failure; no post-fix manual PASS is inferred. Historical Issue #25 test results and 150 sampling / 750 offscreen benchmark measurements remain intact and are not current-renderer speedup evidence. Full current Release and sanitizer suites, full Qt sanitizer coverage, native Windows/Qt 5 behavior, and physical display latency are outside the recorded current verification.
