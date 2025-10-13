---
applyTo: '**'
---
Provide project context and coding guidelines that AI should follow when generating code, answering questions, or reviewing changes.

build dialOS with 'platformio run -e m5stack-stamps3' in the root directory
upload and monitor dialOS with 'platformio run -e m5stack-stamps3 -t upload -t monitor' in the root directory
build lsp with command 'tree-sitter generate' while in directory lsp/
build the compiler with command 'cmake --build .' in directory compiler/build/
compile a ds script to a ds binary file with command './compiler/build/Debug/compiler.exe <script.ds> <script.dsb>' in the root directory
compile a ds script to a c array header file with command './compiler/build/Debug/compiler.exe <script.ds> <script_applet.h> --c-array' in the root directory
run a ds binary file with command './compiler/build/Debug/testvm.exe <script.dsb>' in the root directory