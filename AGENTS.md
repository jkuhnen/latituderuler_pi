# Agent guidance

`.devkit` (`jkuhnen/opencpn-plugin-devkit`) is the shared development baseline for this repository. Before changing OpenCPN API behavior, architecture, Windows builds, packaging, CI, Git workflow, maritime HMI, or design-system work, read `.devkit/AGENTS.md` and the relevant `.devkit/docs/*` files.

Use this precedence:

```text
verified upstream OpenCPN/API behavior
        ↓
issue/task-specific requirements
        ↓
Latitude Ruler local documented rules
        ↓
shared DevKit conventions
        ↓
agent assumptions
```

Verified upstream OpenCPN/API behavior is authoritative over local and DevKit conventions. Issue/task-specific requirements take precedence over general local defaults. Latitude Ruler's documented rules and actual implementation take precedence over generic DevKit conventions where they intentionally differ.

## Latitude Ruler defaults

- Keep C++11 unless an explicit project-wide task changes it.
- Target OpenCPN plugin API 1.18 and the existing `opencpn-libs/api-18` setup unless a dedicated compatibility/API task changes it.
- Preserve both wxDC and OpenGL rendering paths when changing canvas rendering behavior.
- Use OpenCPN viewport transforms; do not invent a separate chart projection or coordinate model.
- Keep render and mouse/canvas paths bounded and lightweight. Avoid blocking I/O, network access, unbounded work, and unnecessary allocation churn in frame-sensitive callbacks.
- Preserve OpenCPN DAY/DUSK/NIGHT behavior and follow the DevKit maritime HMI and color guidance.
- Treat the ruler as an auxiliary navigation aid. It must not imitate official chart symbology, hide critical chart information, or imply ECDIS or type-approval compliance.
- Prefer OpenCPN APIs and existing project patterns over new dependencies or frameworks.
- After relevant C++ or CMake changes, build with the repository's actual Windows/MSVC workflow and identify any runtime test still required in OpenCPN.
- Work on a dedicated branch, review the diff before committing, open a PR against `main`, and do not merge without explicit maintainer instruction.

Do not silently update `.devkit` or `opencpn-libs` submodule pointers during unrelated work. If submodules are uninitialized, use the normal recursive initialization workflow (`git submodule update --init --recursive`) rather than fabricating missing guidance.
