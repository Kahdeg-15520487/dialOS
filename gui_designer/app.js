(function(){
    const canvas = document.getElementById('canvas');
    const palette = document.querySelectorAll('.palette .widget');
    const exportBtn = document.getElementById('exportBtn');
    const exportDsBtn = document.getElementById('exportDsBtn');
    const importBtn = document.getElementById('importBtn');
    const importFile = document.getElementById('importFile');
    const previewCanvas = document.getElementById('previewCanvas');
    const previewCtx = previewCanvas && previewCanvas.getContext ? previewCanvas.getContext('2d') : null;

    let selectedEl = null;
    let dragOffset = {x:0,y:0};

    // Utility: generate short ids
    function genId(type){
        return `${type}_${Math.random().toString(36).slice(2,8)}`;
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
        if(!type) return;
        const rect = canvas.getBoundingClientRect();
        const x = Math.round(e.clientX - rect.left);
        const y = Math.round(e.clientY - rect.top);
        const el = createElement(type, x, y);
        selectElement(el);
    });

    // Create element on canvas
    function createElement(type, x=10, y=10){
        const el = document.createElement('div');
        el.className = 'el';
        el.dataset.type = type;
        el.dataset.id = genId(type);
        el.style.left = (x)+'px';
        el.style.top = (y)+'px';
        el.style.width = '120px';
        el.style.height = '30px';
        let text = '';
        switch(type){
            case 'label': text = 'Label'; break;
            case 'button': text = 'Button'; break;
            case 'input': text = ''; el.style.background = '#fff'; break;
            case 'circle': text = ''; el.style.borderRadius = '50%'; el.style.width='80px'; el.style.height='80px'; break;
        }
        el.textContent = text;

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

    // Selection
    function selectElement(el){
        if(selectedEl) selectedEl.classList.remove('selected');
        selectedEl = el;
        if(el) el.classList.add('selected');
        refreshPropsPanel();
    }

    // Deselect on canvas background click
    canvas.addEventListener('mousedown', e => {
        if(e.target === canvas) selectElement(null);
    });

    // Dragging inside canvas
    function startDrag(el, e){
        const rect = el.getBoundingClientRect();
        const canvRect = canvas.getBoundingClientRect();
        dragOffset.x = e.clientX - rect.left;
        dragOffset.y = e.clientY - rect.top;

        function onMove(ev){
            const x = Math.round(ev.clientX - canvRect.left - dragOffset.x);
            const y = Math.round(ev.clientY - canvRect.top - dragOffset.y);
            el.style.left = Math.max(0, Math.min(canvas.clientWidth - el.clientWidth, x)) + 'px';
            el.style.top = Math.max(0, Math.min(canvas.clientHeight - el.clientHeight, y)) + 'px';
            refreshPropsPanel(false);
        }
        function onUp(){
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

    function refreshPropsPanel(show=true){
        if(!selectedEl){
            propsRoot.style.display='none';
            noSel.style.display='block';
            return;
        }
        noSel.style.display='none';
        propsRoot.style.display='block';
        propId.value = selectedEl.dataset.id || '';
        propType.value = selectedEl.dataset.type || '';
        propText.value = selectedEl.textContent || '';
        propX.value = parseInt(selectedEl.style.left)||0;
        propY.value = parseInt(selectedEl.style.top)||0;
        propW.value = parseInt(selectedEl.style.width)||selectedEl.clientWidth;
        propH.value = parseInt(selectedEl.style.height)||selectedEl.clientHeight;
    }

    // Update element when props change
    propId.addEventListener('input', ()=>{ if(selectedEl) selectedEl.dataset.id = propId.value; });
    propText.addEventListener('input', ()=>{ if(selectedEl) selectedEl.textContent = propText.value; });
    propX.addEventListener('input', ()=>{ if(selectedEl) selectedEl.style.left = (parseInt(propX.value)||0)+'px'; });
    propY.addEventListener('input', ()=>{ if(selectedEl) selectedEl.style.top = (parseInt(propY.value)||0)+'px'; });
    propW.addEventListener('input', ()=>{ if(selectedEl) selectedEl.style.width = (parseInt(propW.value)||10)+'px'; });
    propH.addEventListener('input', ()=>{ if(selectedEl) selectedEl.style.height = (parseInt(propH.value)||10)+'px'; });

    deleteBtn.addEventListener('click', ()=>{
        if(!selectedEl) return;
        selectedEl.remove();
        selectElement(null);
    });

    // Export layout
    function exportLayout(){
        const items = Array.from(canvas.querySelectorAll('.el')).map(el=>({
            id: el.dataset.id,
            type: el.dataset.type,
            text: el.textContent,
            x: parseInt(el.style.left)||0,
            y: parseInt(el.style.top)||0,
            w: parseInt(el.style.width)||el.clientWidth,
            h: parseInt(el.style.height)||el.clientHeight
        }));
        return {meta:{width:canvas.clientWidth,height:canvas.clientHeight},items};
    }

    exportBtn.addEventListener('click', ()=>{
        const layout = exportLayout();
        const blob = new Blob([JSON.stringify(layout, null, 2)], {type:'application/json'});
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url; a.download = 'layout.json';
        document.body.appendChild(a); a.click(); a.remove();
        URL.revokeObjectURL(url);
        renderPreview();
    });

    // Export DialScript (.ds) generator
    function generateDialScript(){
        // Basic class template for Button (as requested)
        const classTpl = `class Button {
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

        const items = Array.from(canvas.querySelectorAll('.el')).map(el=>({
            id: el.dataset.id,
            type: el.dataset.type,
            text: el.textContent,
            x: parseInt(el.style.left)||0,
            y: parseInt(el.style.top)||0,
            w: parseInt(el.style.width)||el.clientWidth,
            h: parseInt(el.style.height)||el.clientHeight
        }));

        // Build instances
        let instances = '';
        let count = 1;
        items.forEach(it => {
            const safeId = (it.id || (it.type+count)).replace(/[^a-zA-Z0-9_]/g,'_');
            if(it.type === 'button'){
                // include text param in constructor
                const textParam = it.text ? `, \"${it.text.replace(/\"/g,'\\\"')}\"` : ', \"\"';
                instances += `var ${safeId}: Button(${it.x}, ${it.y}, ${it.w}, ${it.h}${textParam});\n`;
            } else if(it.type === 'label'){
                // simple label mapping: drawText call in main
                instances += `// label ${safeId} at ${it.x},${it.y} text="${(it.text||'').replace(/\"/g,'\\\"')}"\n`;
            } else if(it.type === 'circle'){
                instances += `// circle ${safeId} at ${it.x},${it.y} r=${Math.round(Math.min(it.w,it.h)/2)}\n`;
            } else if(it.type === 'input'){
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
        // draw buttons
        items.forEach(it => {
            if(it.type === 'button'){
                const safeId = (it.id || 'button').replace(/[^a-zA-Z0-9_]/g,'_');
                script += `    ${safeId}.draw();\n`;
            } else if(it.type === 'label'){
                const txt = (it.text||'').replace(/\"/g,'\\\"');
                script += `    os.display.drawText(${it.x}, ${it.y}, \"${txt}\", _color.white, 1);\n`;
            } else if(it.type === 'circle'){
                const r = Math.round(Math.min(it.w,it.h)/2);
                script += `    os.display.drawCircle(${it.x + r}, ${it.y + r}, ${r}, _color.white, true);\n`;
            }
        });
        script += `}\n\n// Start\nmain();\n`;

        return script;
    }

    exportDsBtn.addEventListener('click', ()=>{
        const ds = generateDialScript();
        const blob = new Blob([ds], {type:'text/plain'});
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url; a.download = 'layout.ds';
        document.body.appendChild(a); a.click(); a.remove();
        URL.revokeObjectURL(url);
    });

    // Import
    importBtn.addEventListener('click', ()=> importFile.click());
    importFile.addEventListener('change', e=>{
        const file = e.target.files[0];
        if(!file) return;
        const reader = new FileReader();
        reader.onload = ev => {
            try{
                const layout = JSON.parse(ev.target.result);
                loadLayout(layout);
            }catch(err){ alert('Invalid JSON'); }
        };
        reader.readAsText(file);
        e.target.value = '';
    });

    function loadLayout(layout){
        // clear
        canvas.innerHTML = '';
        if(!layout.items) return;
        layout.items.forEach(it => {
            const el = createElement(it.type, it.x, it.y);
            el.dataset.id = it.id || genId(it.type);
            el.textContent = it.text||'';
            if(it.w) el.style.width = it.w+'px';
            if(it.h) el.style.height = it.h+'px';
        });
        selectElement(null);
        renderPreview();
    }

    // Keyboard delete
    window.addEventListener('keydown', e=>{
        if(e.key === 'Delete' && selectedEl) selectedEl.remove(), selectElement(null);
    });

    // initial sample
    createElement('label',20,20).textContent = 'Hello';
    createElement('button',20,60).textContent = 'Press';

    // Render preview by mapping canvas elements into the 240x240 preview canvas
    function renderPreview(){
        if(!previewCtx) return;
        // clear
        previewCtx.clearRect(0,0,240,240);
        // background (simulate display black background)
        previewCtx.fillStyle = '#000';
        previewCtx.fillRect(0,0,240,240);

        const cw = canvas.clientWidth;
        const ch = canvas.clientHeight;
        const scaleX = 240 / cw;
        const scaleY = 240 / ch;

        const items = Array.from(canvas.querySelectorAll('.el')).map(el=>({
            type: el.dataset.type,
            text: el.textContent || '',
            x: parseInt(el.style.left)||0,
            y: parseInt(el.style.top)||0,
            w: parseInt(el.style.width)||el.clientWidth,
            h: parseInt(el.style.height)||el.clientHeight
        }));

        items.forEach(it => {
            const x = Math.round(it.x * scaleX);
            const y = Math.round(it.y * scaleY);
            const w = Math.round(it.w * scaleX);
            const h = Math.round(it.h * scaleY);

            if(it.type === 'button'){
                previewCtx.fillStyle = '#444';
                previewCtx.fillRect(x,y,w,h);
                previewCtx.strokeStyle = '#fff';
                previewCtx.lineWidth = 1;
                previewCtx.strokeRect(x,y,w,h);
                // text
                previewCtx.fillStyle = '#fff';
                previewCtx.font = '12px sans-serif';
                previewCtx.textBaseline = 'middle';
                previewCtx.textAlign = 'center';
                previewCtx.fillText(it.text || 'Button', x + w/2, y + h/2);
            } else if(it.type === 'label'){
                previewCtx.fillStyle = '#fff';
                previewCtx.font = '12px sans-serif';
                previewCtx.textBaseline = 'top';
                previewCtx.fillText(it.text || 'Label', x, y);
            } else if(it.type === 'circle'){
                previewCtx.fillStyle = '#fff';
                const r = Math.round(Math.min(w,h)/2);
                previewCtx.beginPath();
                previewCtx.arc(x + r, y + r, r, 0, Math.PI*2);
                previewCtx.fill();
            } else if(it.type === 'input'){
                previewCtx.fillStyle = '#fff';
                previewCtx.fillRect(x,y,w,h);
                previewCtx.strokeStyle = '#999';
                previewCtx.strokeRect(x,y,w,h);
            }
        });
    }

    // Keep preview updated during interactions
    setInterval(renderPreview, 250);
})();
