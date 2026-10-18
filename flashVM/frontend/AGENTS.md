# Desktop frontend rules

- Keep the VM host in C89 and the VM implementation in C++98. Share parsing,
  scheduling and presentation code; OS directories contain real launch adapters.
- Format for human readers, comment every file/function, ownership field and
  non-obvious branch. Keep --help and English/Russian manuals consistent.
- Never silently skip unsupported SWF scene tags or call a successful dummy-video
  run native OS/window acceptance. Keep display tests explicit and bounded.
- Keep all SDL events/render and VM calls on main thread; pin managed owners and
  perform GC only at host safe points. Release resources on every error path.
- Preserve authored script transforms: viewport projection belongs to rendering.
- Desktop targets stay optional. Android requires an Activity/surface integration,
  not a desktop main stub. Core/other projects must build without SDL.
