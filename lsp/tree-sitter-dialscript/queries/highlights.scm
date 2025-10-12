(; Minimal highlights for dialscript
  ; Map some node types to scopes used by tree-sitter highlighting
  (identifier) @variable
  (number) @number
  (string) @string
  (boolean) @constant.builtin
  (null) @constant.builtin
  (comment) @comment
  (function_declaration name: (identifier) @function)
  (call_expression function: (identifier) @function.call)
)
