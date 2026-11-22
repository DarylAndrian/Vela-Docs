# Contributing to Vela Docs

Thank you for your interest in contributing! 🎉

## Quick start

1. **Fork** the repo on GitHub.
2. **Clone** your fork: `git clone https://github.com/<you>/vela-docs.git`
3. **Set up** the build environment — see [docs/BUILD.md](BUILD.md).
4. **Create a branch**: `git checkout -b feat/your-feature`
5. **Build and test**:
   ```powershell
   cmake --build build --config Release --parallel
   cd build && ctest -C Release --output-on-failure
   ```
6. **Make your changes**, commit with a clear message.
7. **Push** to your fork and **open a Pull Request**.

---

## Coding style

We follow the [Qt Coding Style](https://wiki.qt.io/Qt_Coding_Style) with a
few additions:

- **C++20** is the floor. Use `auto`, structured bindings, `std::optional`,
  `std::variant` where they improve clarity.
- **`namespace vela { ... }`** for everything in `src/`.
- **Header guards**: `#pragma once`.
- **Class members**: `m_camelCase`. Pointer members use `m_` prefix.
- **Forward declarations** in headers; full includes in `.cpp` where possible.
- **Signals/slots**: use the new Qt 5+ syntax (`connect(sender, &Class::sig, ...)`).
- **Logging**: use `VE_LOG_DEBUG/INFO/WARNING/ERROR(msg)` macros.
- **Tr()** every user-facing string.

### File headers

Every source file should start with the standard header banner:

```cpp
// ════════════════════════════════════════════════════════════════════════════
//  MyFile.h — One-line description
// ════════════════════════════════════════════════════════════════════════════
#pragma once
...
```

---

## Commit messages

Use [Conventional Commits](https://www.conventionalcommits.org/):

```
feat(ribbon): add paragraph styles dropdown
fix(docx): preserve numbering on round-trip
docs(architecture): explain TipTap integration
chore(deps): bump cmark-gfm to 0.29.0.gfm.14
```

---

## Pull request checklist

Before submitting:

- [ ] Builds clean on Windows with `cmake --build build --config Release`
- [ ] All tests pass (`ctest -C Release`)
- [ ] New code has unit tests (if applicable)
- [ ] No new compiler warnings (`/W4` on MSVC, `-Wall -Wextra -Wpedantic` elsewhere)
- [ ] User-facing strings wrapped in `tr()`
- [ ] Updated `docs/` if behavior changed
- [ ] Linked the relevant issue (if any)

---

## Reporting bugs

Please open an issue with:

- Vela Docs version (`Help → About Vela Docs`)
- OS version (Windows 10/11, build number)
- Qt version (from log file)
- Steps to reproduce
- Expected vs actual behavior
- Log file (`%APPDATA%\Vela Docs\logs\vela-YYYYMMDD.log`) — attach if relevant

Use the bug report template (or open an issue directly).

---

## Areas that need help

Check the [GitHub Issues](../../issues) for `good-first-issue` and
`help-wanted` labels.

Currently:
- v0.5.1: Page Layout ribbon group + per-section UI controls
- v0.5.1: Endnote `<w:endnoteReference>` round-trip wiring
- v0.5.1: Document Protection "Restrict Editing → set password" UI
- v1.0: Real-time collaboration (Yjs + WebSocket)
- v1.0: Voice typing (Windows Speech API)
- Cross-platform: Mac / Linux ports

---

## Code of conduct

Be respectful. Be kind. Help others learn.

---

## License

By contributing, you agree that your contributions will be licensed under
the project's MIT license.
