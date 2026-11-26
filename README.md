# RenderProbe

An Unreal Engine editor plugin that renders meshes and materials under fixed, repeatable conditions in an isolated preview world, and returns the results as files: captured images, per-slot material shader statistics, and image comparisons with diagnostics.

## Who it is for

The primary user is an AI agent. An agent submits a render job, waits for it to finish, and reads the images and `result.json` from the output directory instead of looking at the editor viewport. The same functions are planned to be callable by people from Blueprint and Python, but there is no dedicated UI.

Planned entry points, all sharing one core:

- An MCP toolset: `RunJob` (job JSON string), `RunJobFile` (job JSON file path) and `GetJobSchema` (requires the engine's experimental `ToolsetRegistry` plugin; the rest of the plugin loads without it)
- An editor subsystem (Blueprint and Python): `RunJob` (job structure), `RunJobFile` (job JSON file path) and `GetJobSchema`
- A commandlet: `-run=RenderProbe` (job JSON file path, or writes the job schema to a file)

## Commandlet

Run a job:

```
UnrealEditor-Cmd.exe <Project>.uproject -run=RenderProbe -job=<job JSON file> [-out=<output folder>] -AllowCommandletRendering -unattended
```

Write the job schema (the same JSON that `GetJobSchema` returns) without running a job:

```
UnrealEditor-Cmd.exe <Project>.uproject -run=RenderProbe -schema=<schema file to write>
```

- `-job=<file>` reads the job from a JSON file and runs it. Quote paths that contain spaces: `-job="C:/My Jobs/job.json"`.
- `-out=<folder>` is optional and replaces the job's `outputDir`.
- `-schema=<file>` cannot be combined with `-job=` or `-out=`.
- `-AllowCommandletRendering` is required for any job that captures images. Without it the commandlet does not render; the result gets a `RenderingUnavailable` diagnostic and is not ok. A process without an RHI (`-nullrhi`) gets the same diagnostic. Items that only measure statistics (no mesh) do not need it.

The result is written to `<outputDir>/result.json` as with the other entry points, and is also printed to the log.

Exit codes:

| Code | Meaning |
|------|---------|
| 0 | The job ran and the result is ok, or the schema was written |
| 1 | The job ran but the result is not ok (diagnostics or dissimilar comparisons); this includes a job file that cannot be read or parsed |
| 2 | Nothing ran: missing or invalid arguments, or the schema file could not be written |

## Status

Under development. This version is a skeleton: the modules load but provide no functionality yet.

## License

MIT. See [LICENSE](LICENSE).
