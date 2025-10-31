(function () {
    const canvas = document.getElementById('canvas');
    const palette = document.querySelectorAll('.palette .widget');
    const exportBtn = document.getElementById('exportBtn');
    const exportDsBtn = document.getElementById('exportDsBtn');
    const importBtn = document.getElementById('importBtn');
    const importFile = document.getElementById('importFile');
    const previewCanvas = document.getElementById('previewCanvas');
    const previewCtx = previewCanvas && previewCanvas.getContext ? previewCanvas.getContext('2d') : null;

    let selectedEl = null;
    let dragOffset = { x: 0, y: 0 };

    // Utility: generate short ids
    function genId(type) {
        return `${type}_${Math.random().toString(36).slice(2, 8)}`;
    }

    // Palette dragstart
    palette.forEach(w => {
        w.addEventListener('dragstart', e => {
            e.dataTransfer.setData('text/widget-type', w.dataset.type);
        });
    });

    // Canvas dragover/drop
    canvas.addEventListener('dragover', e => e.preventDefault());
    canvas.addEventListener('drop', e => {
        e.preventDefault();
        const type = e.dataTransfer.getData('text/widget-type');
        if (!type) return;
        const rect = canvas.getBoundingClientRect();
        const x = Math.round(e.clientX - rect.left);
        const y = Math.round(e.clientY - rect.top);
        const el = createElement(type, x, y);
        selectElement(el);
    });

    // Create element on canvas
    function createElement(type, x = 10, y = 10) {
        const el = document.createElement('div');
        el.className = 'el';
        el.dataset.type = type;
        el.dataset.id = genId(type);
        el.style.left = (x) + 'px';
        el.style.top = (y) + 'px';
        el.style.width = '120px';
        el.style.height = '30px';
        let text = '';
        switch (type) {
            case 'label': text = 'Label'; break;
            case 'button': text = 'Button'; break;
            case 'input': text = ''; el.style.background = '#fff'; break;
            case 'circle': text = ''; el.style.borderRadius = '50%'; el.style.width = '80px'; el.style.height = '80px'; break;
            case 'list':
                // build a list container inside the element
                el.classList.add('list');
                el.style.width = '160px';
                el.style.height = '120px';
                const ul = document.createElement('ul');
                ul.className = 'list-container';
                ul.setAttribute('role', 'listbox');
                // sample items
                ['Item 1', 'Item 2', 'Item 3'].forEach((it, idx) => {
                    const li = document.createElement('li');
                    li.className = 'list-item';
                    li.tabIndex = -1;
                    li.dataset.index = idx;
                    li.textContent = it;
                    ul.appendChild(li);
                });
                el.appendChild(ul);
                // store focused index
                el.dataset.focus = '0';
                // click handler for items
                ul.addEventListener('click', e => {
                    const li = e.target.closest('.list-item');
                    if (!li) return;
                    const idx = parseInt(li.dataset.index || '0');
                    setListFocus(el, idx);
                    selectElement(el);
                    refreshPropsPanel();
                });
                break;
        }
        // For list elements we keep child nodes (UL) so don't overwrite them with textContent
        if (type !== 'list') el.textContent = text;

        // events
        el.addEventListener('mousedown', e => {
            e.stopPropagation();
            selectElement(el);
            startDrag(el, e);
        });

        canvas.appendChild(el);
        renderPreview();
        return el;
    }

    // Set focus on a list element's item index and update visuals
    function setListFocus(listEl, index) {
        if (!listEl) return;
        const ul = listEl.querySelector('.list-container');
        if (!ul) return;
        const items = Array.from(ul.querySelectorAll('.list-item'));
        const safeIndex = Math.max(0, Math.min(items.length - 1, index || 0));
        items.forEach((it, i) => {
            if (i === safeIndex) it.classList.add('focused'); else it.classList.remove('focused');
            it.dataset.index = i;
        });
        listEl.dataset.focus = safeIndex;
    }

    function getListFocus(listEl) {
        return listEl ? parseInt(listEl.dataset.focus || '0') : 0;
    }

    // Selection
    function selectElement(el) {
        if (selectedEl) selectedEl.classList.remove('selected');
        selectedEl = el;
        if (el) el.classList.add('selected');
        refreshPropsPanel();
    }

    // Deselect on canvas background click
    canvas.addEventListener('mousedown', e => {
        if (e.target === canvas) selectElement(null);
    });

    // Dragging inside canvas
    function startDrag(el, e) {
        const rect = el.getBoundingClientRect();
        const canvRect = canvas.getBoundingClientRect();
        dragOffset.x = e.clientX - rect.left;
        dragOffset.y = e.clientY - rect.top;

        function onMove(ev) {
            const x = Math.round(ev.clientX - canvRect.left - dragOffset.x);
            const y = Math.round(ev.clientY - canvRect.top - dragOffset.y);
            el.style.left = Math.max(0, Math.min(canvas.clientWidth - el.clientWidth, x)) + 'px';
            el.style.top = Math.max(0, Math.min(canvas.clientHeight - el.clientHeight, y)) + 'px';
            refreshPropsPanel(false);
        }
        function onUp() {
            window.removeEventListener('mousemove', onMove);
            window.removeEventListener('mouseup', onUp);
        }
        window.addEventListener('mousemove', onMove);
        window.addEventListener('mouseup', onUp);
    }

    // Properties UI bindings
    const propsRoot = document.getElementById('props');
    const noSel = document.getElementById('noSelection');
    const propId = document.getElementById('prop-id');
    const propType = document.getElementById('prop-type');
    const propText = document.getElementById('prop-text');
    const propX = document.getElementById('prop-x');
    const propY = document.getElementById('prop-y');
    const propW = document.getElementById('prop-w');
    const propH = document.getElementById('prop-h');
    const deleteBtn = document.getElementById('deleteBtn');

    function refreshPropsPanel(show = true) {
        if (!selectedEl) {
            propsRoot.style.display = 'none';
            noSel.style.display = 'block';
            return;
        }
        noSel.style.display = 'none';
        propsRoot.style.display = 'block';
        propId.value = selectedEl.dataset.id || '';
        propType.value = selectedEl.dataset.type || '';
        propText.value = selectedEl.textContent || '';
        propX.value = parseInt(selectedEl.style.left) || 0;
        propY.value = parseInt(selectedEl.style.top) || 0;
        propW.value = parseInt(selectedEl.style.width) || selectedEl.clientWidth;
        propH.value = parseInt(selectedEl.style.height) || selectedEl.clientHeight;
        // show list-specific props
        const listProps = document.getElementById('listProps');
        if (selectedEl.dataset.type === 'list') {
            listProps.style.display = 'block';
            const textarea = document.getElementById('prop-list-items');
            const focusInput = document.getElementById('prop-list-focus');
            const ul = selectedEl.querySelector('.list-container');
            if (ul) {
                const items = Array.from(ul.querySelectorAll('.list-item')).map(li => li.textContent || '');
                textarea.value = items.join('\n');
                focusInput.value = getListFocus(selectedEl);
            } else {
                textarea.value = '';
                focusInput.value = 0;
            }
        } else {
            listProps.style.display = 'none';
        }
    }

    // Update element when props change
    propId.addEventListener('input', () => { if (selectedEl) selectedEl.dataset.id = propId.value; });
    propText.addEventListener('input', () => { if (selectedEl) selectedEl.textContent = propText.value; });
    propX.addEventListener('input', () => { if (selectedEl) selectedEl.style.left = (parseInt(propX.value) || 0) + 'px'; });
    propY.addEventListener('input', () => { if (selectedEl) selectedEl.style.top = (parseInt(propY.value) || 0) + 'px'; });
    propW.addEventListener('input', () => { if (selectedEl) selectedEl.style.width = (parseInt(propW.value) || 10) + 'px'; });
    propH.addEventListener('input', () => { if (selectedEl) selectedEl.style.height = (parseInt(propH.value) || 10) + 'px'; });

    deleteBtn.addEventListener('click', () => {
        if (!selectedEl) return;
        selectedEl.remove();
        selectElement(null);
    });

    // List props bindings
    const propListItems = document.getElementById('prop-list-items');
    const propListFocus = document.getElementById('prop-list-focus');
    const propListUp = document.getElementById('prop-list-focus-up');
    const propListDown = document.getElementById('prop-list-focus-down');
    const propListAdd = document.getElementById('prop-list-add');
    const propListRemove = document.getElementById('prop-list-remove');

    if (propListItems) {
        propListItems.addEventListener('input', () => {
            if (!selectedEl || selectedEl.dataset.type !== 'list') return;
            const lines = propListItems.value.split(/\r?\n/).filter(Boolean);
            const ul = selectedEl.querySelector('.list-container');
            if (!ul) return;
            ul.innerHTML = '';
            lines.forEach((txt, i) => {
                const li = document.createElement('li');
                li.className = 'list-item';
                li.dataset.index = i;
                li.textContent = txt;
                ul.appendChild(li);
            });
            setListFocus(selectedEl, Math.min(getListFocus(selectedEl), Math.max(0, lines.length - 1)));
            renderPreview();
        });
    }

    if (propListFocus) {
        propListFocus.addEventListener('input', () => {
            if (!selectedEl || selectedEl.dataset.type !== 'list') return;
            const idx = parseInt(propListFocus.value || '0');
            setListFocus(selectedEl, idx);
            refreshPropsPanel(false);
            renderPreview();
        });
    }

    if (propListUp)
        propListUp.addEventListener('click', () => {
            if (selectedEl && selectedEl.dataset.type === 'list') {
                setListFocus(selectedEl, getListFocus(selectedEl) - 1);
                refreshPropsPanel(false);
                renderPreview();
            }
        });
    if (propListDown)
        propListDown.addEventListener('click', () => {
            if (selectedEl && selectedEl.dataset.type === 'list') {
                setListFocus(selectedEl, getListFocus(selectedEl) + 1);
                refreshPropsPanel(false);
                renderPreview();
            }
        });
    if (propListAdd)
        propListAdd.addEventListener('click', () => {
            if (selectedEl && selectedEl.dataset.type === 'list') {
                const ul = selectedEl.querySelector('.list-container');
                const li = document.createElement('li');
                li.className = 'list-item';
                li.textContent = 'New Item';
                ul.appendChild(li);
                setListFocus(selectedEl, ul.querySelectorAll('.list-item').length - 1);
                refreshPropsPanel(false); renderPreview();
            }
        });
    if (propListRemove)
        propListRemove.addEventListener('click', () => {
            if (selectedEl && selectedEl.dataset.type === 'list') {
                const ul = selectedEl.querySelector('.list-container');
                const idx = getListFocus(selectedEl);
                const items = ul.querySelectorAll('.list-item');
                if (items[idx]) items[idx].remove();
                setListFocus(selectedEl, Math.max(0, Math.min(items.length - 2, idx - 1)));
                refreshPropsPanel(false);
                renderPreview();
            }
        });

    // Export layout
    function exportLayout() {
        const items = Array.from(canvas.querySelectorAll('.el')).map(el => {
            const base = {
                id: el.dataset.id,
                type: el.dataset.type,
                x: parseInt(el.style.left) || 0,
                y: parseInt(el.style.top) || 0,
                w: parseInt(el.style.width) || el.clientWidth,
                h: parseInt(el.style.height) || el.clientHeight
            };
            if (el.dataset.type === 'list') {
                const lis = Array.from(el.querySelectorAll('.list-item'));
                base.items = lis.map(li => li.textContent || '');
                base.focus = parseInt(el.dataset.focus || '0');
            } else {
                base.text = el.textContent;
            }
            return base;
        });
        return { meta: { width: canvas.clientWidth, height: canvas.clientHeight }, items };
    }

    exportBtn.addEventListener('click', () => {
        const layout = exportLayout();
        const blob = new Blob([JSON.stringify(layout, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url; a.download = 'layout.json';
        document.body.appendChild(a); a.click(); a.remove();
        URL.revokeObjectURL(url);
        renderPreview();
    });

    function generateDialScriptTemplate(elementsUsed) {
        const buttonClass = `class Button {
    x: int;
    y: int;
    width: int;
    height: int;
    text: string;
    constructor(x: int, y: int, w: int, h: int, text: string) {
        assign this.x x;
        assign this.y y;
        assign this.width w;
        assign this.height h;
        assign this.text text;
    }
    draw(): void {
        // Draw button background
        os.display.drawRect(this.x, this.y, this.width, this.height, _color.darkgray, true);
        os.display.drawRect(this.x, this.y, this.width, this.height, _color.white, false);
        
        // Draw button text (centered) - improved centering for small buttons
        var textX: this.x + (this.width / 2) - (this.text.length * 3);
        var textY: this.y + (this.height / 2) - 4;
        os.display.drawText(textX, textY, this.text, _color.white, 1);
    }
    isPressed(touchX: int, touchY: int): bool {
        return (touchX >= this.x and touchX <= (this.x + this.width) and
                touchY >= this.y and touchY <= (this.y + this.height));
    }
}
`;
        const labelClass = `class Label {
    x: int;
    y: int;
    text: string;
    constructor(x: int, y: int, text: string) {
        assign this.x x;
        assign this.y y;
        assign this.text text;
    }
    draw(): void {
        os.display.drawText(this.x, this.y, this.text, _color.white, 1);
    }
}
`;

        const shapeClass = `class Circle {
    x: int;
    y: int;
    radius: int;
    constructor(x: int, y: int, radius: int) {
        assign this.x x;
        assign this.y y;
        assign this.radius radius;
    }
    draw(): void {
        os.display.drawCircle(this.x, this.y, this.radius, _color.white, true);
    }
}
`;
        const listClass = `class List {
    x: int;
    y: int;
    width: int;
    height: int;
    items: string[];
    focusIndex: int;
    constructor(x: int, y: int, w: int, h: int, items: string[]) {
        assign this.x x;
        assign this.y y;
        assign this.width w;
        assign this.height h;
        assign this.items items;
        assign this.focusIndex 0;
    }
    draw(): void {
        // Draw list background
        os.display.drawRect(this.x, this.y, this.width, this.height, _color.white, true);
        os.display.drawRect(this.x, this.y, this.width, this.height, _color.gray, false);
        // Draw items
        var padding: int = 4;
        var itemHeight: int = (this.height - (padding * 2)) / this.items.length;
        for (var i: int = 0; i < this.items.length; i = i + 1) {
            var itemY: int = this.y + padding + (i * itemHeight);
            if (i == this.focusIndex) {
                os.display.drawRect(this.x + 1, itemY, this.width - 2, itemHeight - 1, _color.blue, true);
                os.display.drawText(this.x + 6, itemY + (itemHeight / 2) - 4, this.items[i], _color.white, 1);
            }
            else {
                os.display.drawText(this.x + 6, itemY + (itemHeight / 2) - 4, this.items[i], _color.black, 1);
            }
        }
    }
}
`;

        let tpl = "";
        const usedSet = new Set(elementsUsed);
        if (usedSet.has('button')) tpl += buttonClass + '\n';
        if (usedSet.has('label')) tpl += labelClass + '\n';
        if (usedSet.has('circle')) tpl += shapeClass + '\n';
        if (usedSet.has('list')) tpl += listClass + '\n';
        return tpl;
    }

    // Export DialScript (.ds) generator
    function generateDialScript() {
        const items = Array.from(canvas.querySelectorAll('.el')).map(el => {
            const base = {
                id: el.dataset.id,
                type: el.dataset.type,
                x: parseInt(el.style.left) || 0,
                y: parseInt(el.style.top) || 0,
                w: parseInt(el.style.width) || el.clientWidth,
                h: parseInt(el.style.height) || el.clientHeight
            };
            if (el.dataset.type === 'list') {
                const lis = Array.from(el.querySelectorAll('.list-item'));
                base.items = lis.map(li => li.textContent || '');
                base.focus = parseInt(el.dataset.focus || '0');
            } else {
                base.text = el.textContent || '';
            }
            return base;
        });

        // Build a unique list of element types used and pass to template generator
        const usedTypes = Array.from(new Set(items.map(it => it.type)));
        const classTpl = generateDialScriptTemplate(usedTypes);
        console.log(classTpl);

        // Build instances
        let instances = '';
        let count = 1;
        items.forEach(it => {
            const safeId = (it.id || (it.type + count)).replace(/[^a-zA-Z0-9_]/g, '_');
            if (it.type === 'button') {
                // include text param in constructor
                const textParam = it.text ? `, ${JSON.stringify(it.text)}` : ', ""';
                instances += `var ${safeId}: Button(${it.x}, ${it.y}, ${it.w}, ${it.h}${textParam});\n`;
            } else if (it.type === 'label') {
                // create Label instance
                const txt = JSON.stringify(it.text || '');
                instances += `var ${safeId}: Label(${it.x}, ${it.y}, ${txt});\n`;
            } else if (it.type === 'circle') {
                const r = Math.round(Math.min(it.w, it.h) / 2);
                // center coordinates for circle
                const cx = it.x + r;
                const cy = it.y + r;
                instances += `var ${safeId}: Circle(${cx}, ${cy}, ${r});\n`;
            } else if (it.type === 'list') {
                // items array literal
                const arrLiteral = JSON.stringify(it.items || []);
                instances += `var ${safeId}: List(${it.x}, ${it.y}, ${it.w}, ${it.h}, ${arrLiteral});\n`;
                // optionally set initial focusIndex after construction
                if (typeof it.focus !== 'undefined') {
                    instances += `${safeId}.focusIndex = ${parseInt(it.focus)};\n`;
                }
            } else if (it.type === 'input') {
                instances += `// input ${safeId} at ${it.x},${it.y} ${it.w}x${it.h}\n`;
            }
            count++;
        });

        // Compose final script
        let script = `/* Generated by dialOS GUI Designer - DialScript snippet */\n\n`;
        script += classTpl + '\n';
        script += '// Instances\n' + instances + '\n';

        // Optionally add a small main/draw loop
        script += `function main(): void {\n`;
        // draw elements (call draw() for supported classes)
        items.forEach(it => {
            if (['button','label','circle','list'].includes(it.type)) {
                const safeId = (it.id || it.type).replace(/[^a-zA-Z0-9_]/g, '_');
                script += `    ${safeId}.draw();\n`;
            } else if (it.type === 'input') {
                // leave input rendering as-is (not implemented as a class)
                // could be added later
            }
        });
        script += `}\n\n// Start\nmain();\n`;

        return script;
    }

    exportDsBtn.addEventListener('click', () => {
        const ds = generateDialScript();
        const blob = new Blob([ds], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url; a.download = 'layout.ds';
        document.body.appendChild(a); a.click(); a.remove();
        URL.revokeObjectURL(url);
    });

    // Import
    importBtn.addEventListener('click', () => importFile.click());
    importFile.addEventListener('change', e => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = ev => {
            try {
                const layout = JSON.parse(ev.target.result);
                loadLayout(layout);
            } catch (err) { alert('Invalid JSON'); }
        };
        reader.readAsText(file);
        e.target.value = '';
    });

    function loadLayout(layout) {
        // clear
        canvas.innerHTML = '';
        if (!layout.items) return;
        layout.items.forEach(it => {
            const el = createElement(it.type, it.x, it.y);
            el.dataset.id = it.id || genId(it.type);
            if (it.type === 'list') {
                // populate list items preserving order
                const ul = el.querySelector('.list-container');
                if (ul) {
                    ul.innerHTML = '';
                    (it.items || []).forEach((txt, i) => {
                        const li = document.createElement('li');
                        li.className = 'list-item';
                        li.dataset.index = i;
                        li.textContent = txt;
                        ul.appendChild(li);
                    });
                    el.dataset.focus = (typeof it.focus !== 'undefined') ? String(it.focus) : '0';
                    setListFocus(el, parseInt(el.dataset.focus || '0'));
                }
            } else {
                el.textContent = it.text || '';
            }
            if (it.w) el.style.width = it.w + 'px';
            if (it.h) el.style.height = it.h + 'px';
        });
        selectElement(null);
        renderPreview();
    }

    // Keyboard delete
    window.addEventListener('keydown', e => {
        if (e.key === 'Delete' && selectedEl) selectedEl.remove(), selectElement(null);
        // If selected element is a list, allow ArrowUp/ArrowDown to change focused item
        if (selectedEl && selectedEl.dataset.type === 'list') {
            if (e.key === 'ArrowUp') {
                e.preventDefault();
                setListFocus(selectedEl, getListFocus(selectedEl) - 1);
                refreshPropsPanel(false);
                renderPreview();
            } else if (e.key === 'ArrowDown') {
                e.preventDefault();
                setListFocus(selectedEl, getListFocus(selectedEl) + 1);
                refreshPropsPanel(false);
                renderPreview();
            } else if (e.key === 'Enter') {
                // Placeholder: Enter could 'activate' focused item
                console.log('Activate list item', getListFocus(selectedEl));
            }
        }
    });

    // initial sample
    createElement('label', 20, 20).textContent = 'Hello';
    createElement('button', 20, 60).textContent = 'Press';

    // Render preview by mapping canvas elements into the 240x240 preview canvas
    function renderPreview() {
        if (!previewCtx) return;
        // clear
        previewCtx.clearRect(0, 0, 240, 240);
        // background (simulate display black background)
        previewCtx.fillStyle = '#000';
        previewCtx.fillRect(0, 0, 240, 240);

        const cw = canvas.clientWidth;
        const ch = canvas.clientHeight;
        const scaleX = 240 / cw;
        const scaleY = 240 / ch;

        const items = Array.from(canvas.querySelectorAll('.el')).map(el => ({
            elRef: el,
            type: el.dataset.type,
            text: el.textContent || '',
            x: parseInt(el.style.left) || 0,
            y: parseInt(el.style.top) || 0,
            w: parseInt(el.style.width) || el.clientWidth,
            h: parseInt(el.style.height) || el.clientHeight
        }));

        items.forEach(it => {
            const x = Math.round(it.x * scaleX);
            const y = Math.round(it.y * scaleY);
            const w = Math.round(it.w * scaleX);
            const h = Math.round(it.h * scaleY);

            if (it.type === 'button') {
                previewCtx.fillStyle = '#444';
                previewCtx.fillRect(x, y, w, h);
                previewCtx.strokeStyle = '#fff';
                previewCtx.lineWidth = 1;
                previewCtx.strokeRect(x, y, w, h);
                // text
                previewCtx.fillStyle = '#fff';
                previewCtx.font = '12px sans-serif';
                previewCtx.textBaseline = 'middle';
                previewCtx.textAlign = 'center';
                previewCtx.fillText(it.text || 'Button', x + w / 2, y + h / 2);
            } else if (it.type === 'label') {
                previewCtx.fillStyle = '#fff';
                previewCtx.font = '12px sans-serif';
                previewCtx.textBaseline = 'top';
                previewCtx.fillText(it.text || 'Label', x, y);
            } else if (it.type === 'circle') {
                previewCtx.fillStyle = '#fff';
                const r = Math.round(Math.min(w, h) / 2);
                previewCtx.beginPath();
                previewCtx.arc(x + r, y + r, r, 0, Math.PI * 2);
                previewCtx.fill();
            } else if (it.type === 'input') {
                previewCtx.fillStyle = '#fff';
                previewCtx.fillRect(x, y, w, h);
                previewCtx.strokeStyle = '#999';
                previewCtx.strokeRect(x, y, w, h);
            } else if (it.type === 'list') {
                // Render list background
                previewCtx.fillStyle = '#fff';
                previewCtx.fillRect(x, y, w, h);
                previewCtx.strokeStyle = '#ccc';
                previewCtx.strokeRect(x, y, w, h);
                // Render items inside list
                const el = it.elRef;
                const lis = Array.from(el.querySelectorAll('.list-item'));
                const focusIdx = parseInt(el.dataset.focus || '0');
                const padding = 4;
                const itemH = Math.max(14, Math.floor((h - padding * 2) / Math.max(1, lis.length)));
                previewCtx.font = '12px sans-serif';
                previewCtx.textBaseline = 'middle';
                previewCtx.textAlign = 'left';
                lis.forEach((li, i) => {
                    const iy = y + padding + i * itemH;
                    if (i === focusIdx) {
                        previewCtx.fillStyle = '#1e90ff';
                        previewCtx.fillRect(x + 1, iy, w - 2, itemH - 1);
                        previewCtx.fillStyle = '#fff';
                    } else {
                        previewCtx.fillStyle = '#222';
                    }
                    const text = li.textContent || '';
                    previewCtx.fillText(text, x + 6, iy + itemH / 2);
                    // separator
                    previewCtx.strokeStyle = '#eee';
                    previewCtx.beginPath();
                    previewCtx.moveTo(x + 2, iy + itemH - 1);
                    previewCtx.lineTo(x + w - 2, iy + itemH - 1);
                    previewCtx.stroke();
                });
            }
        });
    }

    // Keep preview updated during interactions
    setInterval(renderPreview, 250);

    // Small global API for programmatic focus control (useful for tests/automation)
    window.listDesigner = {
        setFocusById: function (id, idx) {
            const el = document.querySelector('.el[data-id="' + id + '"]');
            if (el && el.dataset.type === 'list') {
                setListFocus(el, idx);
                refreshPropsPanel(false);
                renderPreview();
            }
        },
        getFocusById: function (id) {
            const el = document.querySelector('.el[data-id="' + id + '"]');
            return (el && el.dataset.type === 'list') ? getListFocus(el) : null;
        }
    };
})();
