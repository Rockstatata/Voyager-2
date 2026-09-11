Finish Phase 1 — Application/Window/Input/Time/Camera + two-object test
Understanding
Turn the starter into a clean reusable app foundation while preserving exact visual output. Main.cpp becomes a thin shell over a new Application. Add Window, Input, Time, Camera; move camera state out of Renderer; render two SceneObjects with different transforms. Reuse starter Shader/VAO/VBO/EBO. OpenGL 3.3 Core / GLFW / GLAD unchanged.

Assumptions
Workspace root = project folder (no .sln; folder-opened). src/ and assets/ live in project root.
Include path covers project root, so src files can include root headers via ../../ or via added project-root include dir (already present from prior fix).
Keep default.vert/default.frag; shader uses model/view/proj/scale uniforms.
Approach
Fix ownership (RAII wrappers + by-value Mesh members), then add Time, Input, Camera, Window, Application; slim Main.cpp; update Renderer to take camera+aspect; add two objects; register files; build.

Key Files
src/rendering/Mesh.{h,cpp} — RAII, by-value wrappers
VAO/VBO/EBO .h/.cpp — move-only RAII
src/core/{Time,Input,Window,Application}.{h,cpp} — new
src/rendering/Camera.{h,cpp} — new
src/rendering/Renderer.{h,cpp} — drop camera state, beginFrame(camera,aspect)
src/scene/SceneObject.h — add shared mesh support
Main.cpp — slim to Application shell
Project1.vcxproj(.filters) — register new files
Risks & Open Questions
Changing VAO/VBO/EBO to RAII destructors: must ensure no double-delete with existing manual Delete() calls (Mesh will stop calling Delete()).
Shared mesh: use shared_ptr so both objects share one geometry safely.
Steps
Fix ownership: RAII + move-only VAO/VBO/EBO; Mesh holds by value (no new/delete)
Add SceneObject shared-mesh support (shared_ptr)
Create src/core/Time (frame delta, clamp first frame)
Create src/core/Input (key down/pressed/released + mouse pos/delta)
Create src/rendering/Camera (position/yaw/pitch, view/proj, free-fly update from Input)
Create src/core/Window (GLFW window lifecycle, shouldClose, swap, framebuffer-size → glViewport, aspect)
Create src/core/Application (owns Window/Input/Time/Camera/Scene/Renderer + loop)
Refactor Renderer: remove camera state; beginFrame(camera, aspect)
Slim Main.cpp to Application shell; add two SceneObjects with different transforms
Register new files in vcxproj + filters
Build Debug x64, fix warnings from our changes