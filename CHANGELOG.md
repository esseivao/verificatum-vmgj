# Changelog

All notable changes to VMGJ are documented here.
This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

See [README.md](README.md) for build and installation instructions.

---

## [Unreleased]

### Fixed

- Added Java-side size estimation and configurable byte budgets for
  `FpowmTab` precomputation to reject oversized fixed-base tables
  before entering native code.

- Tightened fixed-base precomputation safeguards to reject
  `blockWidth > 24` and use guardrail defaults that scale with host
  memory but are bounded: total cap defaults to
  `min(totalMemory / 2, 32 GiB)` and per-table cap defaults to
  `min(totalMemory / 4, 8 GiB)` (with system-property overrides and
  bounded fallbacks when runtime memory detection is unavailable).

- Made `FpowmTab` implement `AutoCloseable` and updated the internal
  benchmark/test callers to release native fixed-base tables promptly.

---

## [1.4.0] - 2026-04-06

### Added

- `VMG.modmul(a, b, modulus)` — modular multiplication via GMP `mpz_mul` +
  `mpz_mod`, exposed through new JNI binding `Java_com_verificatum_vmgj_VMG_modmul`
  in `com_verificatum_vmgj_VMG.c`.

### Fixed

- **JNI local reference leak in `spowm`**: `GetObjectArrayElement` was called
  inside two loops without a matching `DeleteLocalRef`, causing a local-reference
  table overflow for large inputs.  Both loops now call `DeleteLocalRef` after
  each element is consumed.

- **Missing NULL checks on native allocations**: `malloc` failures for
  `fpowm_tab`, `millerrabin_state`, and `millerrabin_safe_state` were
  undetected.  `GetByteArrayElements` and `NewByteArray` failures in
  `convert.c` were also unchecked.  All allocation sites now throw
  `java.lang.OutOfMemoryError` and return immediately on failure.

- **Empty `byte[]` passed to native conversion**: `jbyteArray_to_mpz_t` in
  `convert.c` accessed `cBytes[0]` without first checking that
  `GetArrayLength` returned a positive value.  Arrays of length ≤ 0 now
  throw `java.lang.IllegalArgumentException`.

- **Stale or zeroed native handles not detected**: JNI functions that
  dereference a `jlong` state pointer (`fpowm`, `millerrabin_*`) did not
  guard against a null/zero handle value.  Each such function now throws
  `java.lang.IllegalStateException` with a descriptive message before
  any pointer dereference occurs.

- **`MillerRabin.once(base)` dispatched to wrong native function in
  safe-primality mode**: when a `MillerRabin` object was constructed for
  safe-prime testing (`primality = false`), the `once(BigInteger)` method
  incorrectly called `VMG.millerrabin_once` (the ordinary-primality API)
  instead of `VMG.millerrabin_safe_once`.  The correct branch is now taken
  based on the `primality` field.

### Security

- Null/zero native-handle dereferences that could previously cause a JVM
  crash on corrupt or reused state objects are now surfaced as
  `IllegalStateException` rather than undefined behaviour.

- An empty `byte[]` input that could previously trigger an out-of-bounds
  read inside the GMP conversion layer is now rejected at the JNI boundary.

### Build system

- Replaced libtool `-release $(VERSION)` with `-version-info 1:0:0`
  (`SOVERSION = 1`).  This adopts the standard libtool ABI versioning
  scheme and changes the shared-library file names:

  | Platform | Before (1.3.x) | After (1.4.0) |
  |----------|----------------|---------------|
  | Linux    | `libvmgj-1.3.0.so` | `libvmgj.so.1.0.0` → `libvmgj.so.1` |
  | Windows  | `libvmgj-1-3-0.dll` | `libvmgj-1.dll` |

  Downstream consumers must update any hard-coded library paths or
  pkg-config references to reflect the new file names.

- Retained `-no-undefined` in `libvmgj_la_LDFLAGS` to keep Windows DLL
  builds working under MinGW-w64/libtool.

---

## [1.3.0] — earlier release

Refer to the upstream `ChangeLog` file for history prior to 1.4.0.
