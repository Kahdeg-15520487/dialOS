dialOS App Store (static)

This folder contains a small static app browser you can host with GitHub Pages. Edit `index.json` to add or remove applet entries. Each entry should contain:

- id: unique id
- name: display name
- version
- description
- author
- tags: array
- icon: URL to small image (optional)
- url: URL to the downloadable .dsb file

Publishing options (pick one):

1) Publish using the `gh-pages` branch (recommended, no tree moves):

```pwsh
# From repo root
# Push the contents of the `appstore/` folder to gh-pages branch
git subtree push --prefix appstore origin gh-pages
```

After running, enable GitHub Pages for branch `gh-pages` in repository Settings (source: gh-pages).

2) Publish from the `docs/` folder on `main`/`master` branch:

```pwsh
# copy the appstore files into docs/
mkdir -Force docs
Copy-Item -Path .\appstore\* -Destination .\docs\ -Recurse -Force
# commit and push
git add docs
git commit -m "Publish appstore to docs/ for GitHub Pages"
git push origin HEAD
```

3) Manual gh-pages branch commit (worktree approach):

```pwsh
# create a clean worktree for gh-pages and copy files
git worktree add -B gh-pages .gh-pages origin/gh-pages
Remove-Item -Recurse -Force .gh-pages/*
Copy-Item -Path .\appstore\* -Destination .\.gh-pages\ -Recurse -Force
cd .gh-pages
git add --all
git commit -m "Publish appstore"
git push origin gh-pages --force
cd ..
# then remove worktree when done
git worktree remove .gh-pages
```

Local testing: open `appstore/index.html` in a browser. For best local testing (CORS and fetch behavior), run a static server, e.g. with Python:

```pwsh
# if you have Python 3
python -m http.server 8000
# then open http://localhost:8000/appstore/
```

Notes:
- The site is intentionally tiny and static; update `index.json` manually when adding applets.
- If you want the site to live at the repository root on GitHub Pages, move or copy the files into the repository `docs/` folder or publish the `gh-pages` branch as shown above.

WASM compiler (CI-built)
-----------------------

This repository includes a workflow that can build the `compiler/` project with Emscripten and place any resulting `.wasm` (and optional JS glue) into `appstore/wasm/`.

- Workflow: `.github/workflows/build-compiler-wasm.yml` (runs on push to `compiler/**` or manually).
- The built artifact is committed back to `appstore/wasm/` so it will be served by GitHub Pages together with the rest of the appstore.

If you prefer to build locally, you can use Emscripten SDK and run:

```pwsh
# Example (PowerShell) - requires emsdk/emcmake on PATH
emcmake cmake -S compiler -B compiler/build-wasm -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-sSTANDALONE_WASM=1"
cmake --build compiler/build-wasm --config Release -- -j4

# copy outputs into appstore/wasm
mkdir -Force appstore\wasm
Copy-Item -Path compiler\build-wasm\**\*.wasm -Destination appstore\wasm -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item -Path compiler\build-wasm\**\*.js -Destination appstore\wasm -Recurse -Force -ErrorAction SilentlyContinue
```

After copying the wasm into `appstore/wasm/` update `appstore/index.json` if you need to change the `url` for the compiler entry.
