---
applyTo: '**'
---
Provide project context and coding guidelines that AI should follow when generating code, answering questions, or reviewing changes.

build dialOS with 'platformio run -e m5stack-stamps3' in the root directory
upload and monitor dialOS with 'platformio run -e m5stack-stamps3 -t upload -t monitor' in the root directory
build lsp with command 'tree-sitter generate' while in directory lsp/
build the compiler with command 'dscli.ps1 setup' in the root directory
compile a ds script to a ds binary file with command 'dscli.ps1 compile <script.ds> -out <script.dsb>' in the root directory
compile a ds script and add it to dialOS applet registry with command 'dscli.ps1 compile <script.ds> -register' in the root directory
run a ds binary file with command 'dscli.ps1 run <script.ds>' in the root 
build the compiler and simulator with command 'dscli.ps1 setup' in the root directory
to see what other commands are available run 'dscli.ps1' in the root directory