// Simple appstore script: fetches index.json and renders cards
const grid = document.getElementById('grid');
const meta = document.getElementById('meta');
const empty = document.getElementById('empty');
const searchInput = document.getElementById('search');
let apps = [];

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