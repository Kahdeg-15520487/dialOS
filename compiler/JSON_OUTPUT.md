# JSON Output Format

The dialScript parser can export the AST (Abstract Syntax Tree) as JSON for easy integration with other tools and programs.

## Usage

### Command Line
```bash
# Parse and output JSON
.\compiler\build\Debug\parse_file.exe myfile.ds --json

# Save to file
.\compiler\build\Debug\parse_file.exe myfile.ds --json > ast.json
```

### Build Script
```powershell
# Output JSON
.\build.ps1 lsp\timer.ds -Json

# Save to file
.\build.ps1 lsp\timer.ds -Json > timer_ast.json

# Pipe to another program
.\build.ps1 lsp\timer.ds -Json | python analyze_ast.py
```

## JSON Schema

### Root Object (Program)
```json
{
  "type": "Program",
  "statements": [/* array of statements */]
}
```

### Statement Types

#### VariableDeclaration
```json
{
  "type": "VariableDeclaration",
  "name": "variableName",
  "initializer": {/* expression */},
  "line": 10,
  "column": 5
}
```

#### Assignment
```json
{
  "type": "Assignment",
  "target": {/* expression (lvalue) */},
  "value": {/* expression */},
  "line": 10,
  "column": 5
}
```

#### FunctionDeclaration
```json
{
  "type": "FunctionDeclaration",
  "name": "functionName",
  "parameters": [/* array of parameters */],
  "returnType": {/* type node or null */},
  "body": {/* block statement */},
  "line": 10,
  "column": 1
}
```

#### ClassDeclaration
```json
{
  "type": "ClassDeclaration",
  "name": "ClassName",
  "fields": [/* array of field declarations */],
  "constructor": {/* constructor declaration or null */},
  "methods": [/* array of method declarations */],
  "line": 5,
  "column": 1
}
```

#### IfStatement
```json
{
  "type": "IfStatement",
  "condition": {/* expression */},
  "consequence": {/* block */},
  "alternative": {/* block or if statement or null */},
  "line": 15,
  "column": 5
}
```

#### WhileStatement
```json
{
  "type": "WhileStatement",
  "condition": {/* expression */},
  "body": {/* block */},
  "line": 20,
  "column": 5
}
```

#### ForStatement
```json
{
  "type": "ForStatement",
  "initializer": {/* variable declaration */},
  "condition": {/* expression */},
  "increment": {/* assignment */},
  "body": {/* block */},
  "line": 25,
  "column": 5
}
```

#### ReturnStatement
```json
{
  "type": "ReturnStatement",
  "value": {/* expression or null */},
  "line": 30,
  "column": 9
}
```

### Expression Types

#### Identifier
```json
{
  "type": "Identifier",
  "name": "variableName"
}
```

#### NumberLiteral
```json
{
  "type": "NumberLiteral",
  "value": "42",
  "isFloat": false,
  "isHex": false
}
```

#### StringLiteral
```json
{
  "type": "StringLiteral",
  "value": "Hello World"
}
```

#### BooleanLiteral
```json
{
  "type": "BooleanLiteral",
  "value": true
}
```

#### BinaryExpression
```json
{
  "type": "BinaryExpression",
  "operator": "+",  // +, -, *, /, %, =, !=, <, >, <=, >=, and, or
  "left": {/* expression */},
  "right": {/* expression */}
}
```

#### UnaryExpression
```json
{
  "type": "UnaryExpression",
  "operator": "not",  // not, -, +
  "operand": {/* expression */}
}
```

#### TernaryExpression
```json
{
  "type": "TernaryExpression",
  "condition": {/* expression */},
  "consequence": {/* expression */},
  "alternative": {/* expression */}
}
```

#### CallExpression
```json
{
  "type": "CallExpression",
  "callee": {/* expression */},
  "arguments": [/* array of expressions */]
}
```

#### MemberAccess
```json
{
  "type": "MemberAccess",
  "object": {/* expression */},
  "property": "propertyName"
}
```

#### ArrayAccess
```json
{
  "type": "ArrayAccess",
  "array": {/* expression */},
  "index": {/* expression */}
}
```

#### ConstructorCall
```json
{
  "type": "ConstructorCall",
  "typeName": "TypeName",
  "arguments": [/* array of expressions */]
}
```

#### TemplateLiteral
```json
{
  "type": "TemplateLiteral",
  "parts": [
    {
      "type": "string",
      "value": "text"
    },
    {
      "type": "expression",
      "expression": {/* expression */}
    }
  ]
}
```

### Type Nodes

#### PrimitiveType
```json
{
  "type": "PrimitiveType",
  "kind": "int"  // int, uint, byte, short, float, bool, string, void, any
}
```

#### NamedType
```json
{
  "type": "NamedType",
  "name": "CustomType"
}
```

#### ArrayType
```json
{
  "type": "ArrayType",
  "elementType": {/* type node */}
}
```

#### NullableType
```json
{
  "type": "NullableType",
  "baseType": {/* type node */}
}
```

## Example: Simple Variable

**dialScript:**
```javascript
var sum: 10 + 20;
```

**JSON:**
```json
{
  "type": "Program",
  "statements": [
    {
      "type": "VariableDeclaration",
      "name": "sum",
      "initializer": {
        "type": "BinaryExpression",
        "operator": "+",
        "left": {
          "type": "NumberLiteral",
          "value": "10",
          "isFloat": false,
          "isHex": false
        },
        "right": {
          "type": "NumberLiteral",
          "value": "20",
          "isFloat": false,
          "isHex": false
        }
      },
      "line": 1,
      "column": 5
    }
  ]
}
```

## Example: Function

**dialScript:**
```javascript
function add(a: int, b: int): int {
    return a + b;
}
```

**JSON:**
```json
{
  "type": "FunctionDeclaration",
  "name": "add",
  "parameters": [
    {
      "type": "Parameter",
      "name": "a",
      "paramType": {
        "type": "PrimitiveType",
        "kind": "int"
      }
    },
    {
      "type": "Parameter",
      "name": "b",
      "paramType": {
        "type": "PrimitiveType",
        "kind": "int"
      }
    }
  ],
  "returnType": {
    "type": "PrimitiveType",
    "kind": "int"
  },
  "body": {
    "type": "Block",
    "statements": [
      {
        "type": "ReturnStatement",
        "value": {
          "type": "BinaryExpression",
          "operator": "+",
          "left": {
            "type": "Identifier",
            "name": "a"
          },
          "right": {
            "type": "Identifier",
            "name": "b"
          }
        }
      }
    ]
  }
}
```

## Integration Examples

### Python - AST Analysis
```python
import json
import subprocess

# Parse dialScript file
result = subprocess.run(
    ['parse_file.exe', 'myapp.ds', '--json'],
    capture_output=True,
    text=True
)

ast = json.loads(result.stdout)

# Analyze AST
for stmt in ast['statements']:
    if stmt['type'] == 'FunctionDeclaration':
        print(f"Function: {stmt['name']}")
```

### Node.js - Code Generation
```javascript
const { exec } = require('child_process');

exec('parse_file.exe myapp.ds --json', (err, stdout) => {
  const ast = JSON.parse(stdout);
  
  // Generate code from AST
  ast.statements.forEach(stmt => {
    if (stmt.type === 'VariableDeclaration') {
      console.log(`let ${stmt.name} = ...`);
    }
  });
});
```

### PowerShell - Statistics
```powershell
$ast = .\build.ps1 lsp\timer.ds -Json | ConvertFrom-Json

# Count declarations
$functions = ($ast.statements | Where-Object { $_.type -eq 'FunctionDeclaration' }).Count
$classes = ($ast.statements | Where-Object { $_.type -eq 'ClassDeclaration' }).Count
$variables = ($ast.statements | Where-Object { $_.type -eq 'VariableDeclaration' }).Count

Write-Host "Functions: $functions"
Write-Host "Classes: $classes"
Write-Host "Variables: $variables"
```

## Error Format

When there are parse errors:

```json
{
  "success": false,
  "errors": [
    "Line 5:12 - Expected ';' after statement",
    "Line 8:20 - Unexpected token 'while'"
  ]
}
```

## Use Cases

1. **Static Analysis** - Analyze code complexity, dependencies
2. **Code Generation** - Transpile to other languages
3. **Documentation** - Generate API docs from AST
4. **Linting** - Custom code quality checks
5. **IDE Integration** - Syntax highlighting, autocomplete
6. **Metrics** - Code statistics and reporting
7. **Testing** - Automated test generation

## Performance

- Small files (<100 lines): <100ms
- Medium files (100-1000 lines): <500ms
- Large files (>1000 lines): <2s

JSON output adds minimal overhead (~10ms) compared to parsing.

## See Also

- `compiler/ast.h` - AST node definitions
- `compiler/ast_json.cpp` - JSON serialization implementation
- `BUILD_SYSTEM.md` - Build script documentation
