# ADR 0004: Concepts and templates instead of interfaces

- **Status:** Accepted
- **Date:** 2026-10-13
- **RFC:** [0004](../rfc/0004-core-restructure.md)
- **Related:** nexus-dll-lib-cmake ADR 0011

## Context

The generator has one external boundary worth faking in tests: running git. A first version used
a virtual `IProcessRunner` and a gmock class.

## Decision

- No `I`-interfaces. `ProcessRunnerConcept` names the contract (`Result<ProcessResult>
  run(const Command &)`); git functions and `Generator<RunnerT = ReprocRunner>` are templates
  over it.
- `Generator` owns its runner by value, like `Nexus<BleOpsT>` in nexus-dll-lib-cmake.
- Tests pass `FakeRunner`, a plain struct that records commands and answers through a
  `std::function`. gmock is used only for matchers.
- Keep the core in three files: `core.hpp`, `core.cpp`, `generator.hpp`.

## Alternatives considered

- Virtual interface + gmock: a vtable and a mock type for one seam.
- `std::function` parameter: no compile-time contract or name.

## Consequences

- The generator is header-only and is compiled into both `dalang` and `dalang_tests`.
- A wrong fake is a compile error at the `static_assert`, not a runtime mock failure.
- The runner type is fixed at compile time, which is all dalang needs.
