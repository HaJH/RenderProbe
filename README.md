# RenderProbe

An Unreal Engine editor plugin that renders meshes and materials under fixed, repeatable conditions in an isolated preview world, and returns the results as files: captured images, per-slot material shader statistics, and image comparisons with diagnostics.

## Who it is for

The primary user is an AI agent. An agent submits a render job, waits for it to finish, and reads the images and `result.json` from the output directory instead of looking at the editor viewport. The same functions are planned to be callable by people from Blueprint and Python, but there is no dedicated UI.

Planned entry points, all sharing one core:

- An MCP toolset (requires the engine's experimental `ToolsetRegistry` plugin; the rest of the plugin loads without it)
- An editor subsystem (Blueprint and Python)
- A commandlet

## Status

Under development. This version is a skeleton: the modules load but provide no functionality yet.

## License

MIT. See [LICENSE](LICENSE).
