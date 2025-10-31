// Simple appstore script: fetches index.json and renders cards
const grid = document.getElementById('grid');
const meta = document.getElementById('meta');
const empty = document.getElementById('empty');
const searchInput = document.getElementById('search');
let apps = [];
let compilerModulePromise = null;
// Tab elements (initialized after DOM sections exist)
const tabBrowseBtn = document.getElementById('tab-browse');
const tabCompileBtn = document.getElementById('tab-compile');
const browseTab = document.getElementById('browse-tab');
const compileTab = document.getElementById('compile-tab');

function showTab(name){
  if (name === 'compile'){
    browseTab.hidden = true;
    compileTab.hidden = false;
    tabBrowseBtn.setAttribute('aria-selected','false');
    tabCompileBtn.setAttribute('aria-selected','true');
  } else {
    browseTab.hidden = false;
    compileTab.hidden = true;
    tabBrowseBtn.setAttribute('aria-selected','true');
    tabCompileBtn.setAttribute('aria-selected','false');
  }
}

// Wire tab buttons
if (tabBrowseBtn && tabCompileBtn){
  tabBrowseBtn.addEventListener('click', ()=> showTab('browse'));
  tabCompileBtn.addEventListener('click', ()=> showTab('compile'));
}

// Load the compiled compiler module (compile.js) from appstore/wasm.
// Returns a promise resolving to the Emscripten Module instance.
async function loadCompilerModule() {
  if (compilerModulePromise) return compilerModulePromise;

  // Try dynamic import first (ES module build)
  compilerModulePromise = (async () => {
      try {
        const spec = await import('./wasm/compile.js');
        const factory = spec && (spec.default || spec.createCompilerModule || spec.createModule || spec);
        if (typeof factory === 'function') {
          // Ensure the runtime does not auto-run main. Pass noInitialRun so
          // we can safely call cwrap/_malloc and other exports manually.
          const mod = await factory({ noInitialRun: true });
          return mod;
        }
    } catch (e) {
      // dynamic import may fail if compile.js is not an ES module; fall through to script tag approach
      console.warn('Dynamic import of compile.js failed, falling back to script tag:', e);
    }

    // Fallback: inject script tag and use global createCompilerModule
    return new Promise((resolve, reject) => {
      if (window.createCompilerModule) {
        try {
          window.createCompilerModule({ noInitialRun: true }).then(resolve).catch(reject);
          return;
        } catch (err) {
          reject(err);
          return;
        }
      }
      const s = document.createElement('script');
      s.src = './wasm/compile.js';
      s.async = true;
      s.onload = () => {
        if (window.createCompilerModule) {
          window.createCompilerModule({ noInitialRun: true }).then(resolve).catch(reject);
        } else if (window.createModule) {
          window.createModule({ noInitialRun: true }).then(resolve).catch(reject);
        } else {
          // As a last resort, if the script used classic (non-modularized)
          // output, set a Module hint to prevent auto-run before rejecting.
          if (!window.createCompilerModule && !window.createModule) {
            window.Module = Object.assign(window.Module || {}, { noInitialRun: true });
          }
          reject(new Error('compile.js loaded but no factory function found (createCompilerModule/createModule)'));
        }
      };
      s.onerror = (ev) => reject(new Error('Failed to load compile.js'));
      document.head.appendChild(s);
    });
  })();

  return compilerModulePromise;
}

async function compileWithWasm(sourceText) {
  const Module = await loadCompilerModule();
  if (!Module) throw new Error('WASM module not available');

  // cwrap wrapper for compile_source and free
  const compile_source = Module.cwrap ? Module.cwrap('compile_source', 'number', ['string', 'number']) : null;
  const free_buf = Module.cwrap ? Module.cwrap('free_compiled_buffer', 'void', ['number']) : null;
  if (!compile_source || !free_buf) {
    throw new Error('Required WASM exports (compile_source/free_compiled_buffer) not found');
  }

  // allocate 4-byte int on wasm heap to receive length
  const outLenPtr = Module._malloc(4);
  try {
    const ptr = compile_source(sourceText, outLenPtr);
    const len = Module.getValue(outLenPtr, 'i32');
    if (!ptr || !len) {
      throw new Error('Compilation returned no output (len=' + len + ')');
    }
    // copy bytes out of wasm heap
    const compiled = new Uint8Array(Module.HEAPU8.buffer, ptr, len).slice();
    // free native buffer
    free_buf(ptr);
    return compiled;
  } finally {
    Module._free(outLenPtr);
  }
}

// Compile-tab UI wiring
const compileSourceEl = document.getElementById('compile-source');
const compileRunBtn = document.getElementById('compile-run');
const compileStatus = document.getElementById('compile-status');
const compileResult = document.getElementById('compile-result');

if (compileRunBtn && compileSourceEl) {
  compileRunBtn.addEventListener('click', async () => {
    const src = (compileSourceEl.value || '').trim();
    if (!src) {
      compileStatus.textContent = 'Please enter source to compile.';
      return;
    }
    compileRunBtn.disabled = true;
    compileStatus.textContent = 'Compiling...';
    compileResult.innerHTML = '';
    try {
      const compiled = await compileWithWasm(src);
      compileStatus.textContent = `Compiled ${compiled.length} bytes`;
      const blob = new Blob([compiled], { type: 'application/octet-stream' });
      const url = URL.createObjectURL(blob);
      const link = document.createElement('a');
      link.href = url;
      link.download = 'output.dsb';
      link.className = 'btn';
      link.textContent = `Download compiled (${compiled.length} bytes)`;
      compileResult.appendChild(link);
    } catch (err) {
      console.error(err);
      compileStatus.textContent = 'Compile error: ' + (err && err.message ? err.message : String(err));
    } finally {
      compileRunBtn.disabled = false;
    }
  });
}

async function load() {
  try {
    const res = await fetch('./index.json', {cache: 'no-store'});
    if (!res.ok) throw new Error('Failed to load index.json: ' + res.status);
    const data = await res.json();
    apps = data.applets || [];
    renderMeta(data);
    renderGrid(apps);
  } catch (e) {
    meta.textContent = 'Error loading app index.';
    console.error(e);
    empty.hidden = false;
  }
}

function renderMeta(data){
  const updated = data.lastUpdated || data.generatedAt || '';
  meta.innerHTML = `Total applets: <strong>${(data.applets||[]).length}</strong>${updated? ' — last updated: ' + escapeHtml(updated):''}`;
}

function renderGrid(list){
  grid.innerHTML = '';
  if (!list.length){ empty.hidden = false; return } else { empty.hidden = true }
  for (const a of list){
    const card = document.createElement('article'); card.className = 'card';

    const title = document.createElement('div'); title.className = 'title';
    const img = document.createElement('img');
    // Prefer provided icon, then a local logo.png inside the appstore folder, then an embedded SVG fallback
    const embeddedFallback = 'data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="48" height="48"><rect width="48" height="48" fill="%230e1720"/></svg>';
    img.src = a.icon || 'logo.png';
    // If the chosen src fails to load (missing file or invalid URL), fall back to embedded SVG
    img.onerror = () => {
      if (img.src !== embeddedFallback) {
        img.src = embeddedFallback;
      }
      img.onerror = null;
    };
    img.alt = (a.name || 'Applet') + ' icon';
    title.appendChild(img);

    const h = document.createElement('h3'); h.textContent = a.name || 'Unnamed';
    title.appendChild(h);

    const p = document.createElement('p'); p.textContent = a.description || '';

    const mm = document.createElement('div'); mm.className = 'meta';
    mm.innerHTML = `<span>${escapeHtml(a.author || '')}</span><span>•</span><span>v${escapeHtml(a.version || '1.0')}</span>`;

    const actions = document.createElement('div'); actions.className = 'actions';
    const dl = document.createElement('a'); dl.className = 'btn'; dl.textContent = 'Download'; dl.href = a.url || '#'; dl.target = '_blank';
    actions.appendChild(dl);

    card.appendChild(title);
    card.appendChild(p);
    card.appendChild(mm);
    card.appendChild(actions);

    grid.appendChild(card);
  }
}

function escapeHtml(s){ if(!s) return ''; return s.replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;') }

searchInput.addEventListener('input', ()=>{
  const q = (searchInput.value||'').trim().toLowerCase();
  if(!q) return renderGrid(apps);
  const filtered = apps.filter(a=>{
    return (a.name||'').toLowerCase().includes(q) || (a.description||'').toLowerCase().includes(q) || (a.author||'').toLowerCase().includes(q) || ((a.tags||[]).join(' ')).toLowerCase().includes(q);
  });
  renderGrid(filtered);
});

// Kick off
load();