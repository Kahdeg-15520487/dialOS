# dialOS GUI Designer (Prototype)

This is a minimal HTML/JS prototype for a GUI designer aimed at building simple layouts for dialOS apps.

Features:
- Palette with basic widgets (Label, Button, Input, Circle)
- Drag from palette onto the canvas
- Move and select elements on the canvas
- Edit properties: id, text, x/y, width/height
- Export layout as JSON and import it back

New features in this update:
- 240×240 circular preview that simulates the dial display (clipped to a circle)
- Export DialScript (.ds) button: generates a DialScript snippet containing a Button class and instances for placed widgets

How to run:
1. Open `gui_designer/index.html` in your web browser (no server required).
2. Drag widgets from the left palette into the canvas area.
3. Click an element to edit its properties in the right panel.
4. Use Export to save a JSON layout file, and Import to load it.

DialScript export
-----------------
Use the "Export DialScript (.ds)" button to produce a small `.ds` snippet. It includes a `Button` class (matching the requested template) and creates instances for each placed button. The generated script also contains simple draw calls for labels and circles so you can quickly preview or extend it.

Next steps / ideas to discuss:
- Add grid/snapping and alignment guides
- Add more widget types and configurable properties (colors, font, actions)
- Generate dialOS DialScript (`.ds`) or a small C++/layout data include compatible with the OS
- Add undo/redo and history
- Add persistence (localStorage) and preview mode

If you want, I can:
- Add JSON->ds converter that maps our widgets to DialScript code
- Integrate preview that simulates the 240x240 circular display (clip canvas to circle)
- Add an export template for `vm_builtin_applets.h` or similar project artifacts
