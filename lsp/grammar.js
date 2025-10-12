module.exports = grammar({
  name: 'dialscript',

  conflicts: $ => [
    [$._lvalue, $._expression],
    [$.call_expression, $.constructor_call],
  ],

  extras: $ => [
    /\s/,           // Whitespace
    $.comment,
  ],

  rules: {
    source_file: $ => repeat($._statement),

    _statement: $ => choice(
      $.variable_declaration,
      $.assignment,
      $.function_declaration,
      $.class_declaration,
      $.if_statement,
      $.while_statement,
      $.for_statement,
      $.try_statement,
      $.return_statement,
      $.expression_statement,
    ),

    // Variable declaration: var name: Type(args) or var name: expression
    variable_declaration: $ => seq(
      'var',
      field('name', $.identifier),
      ':',
      field('value', $._expression),  // Support any expression
      ';'
    ),

    // Assignment: assign target value
    assignment: $ => seq(
      'assign',
      field('target', $._lvalue),
      field('value', $._expression),
      ';'
    ),

    _lvalue: $ => choice(
      $.identifier,
      $.member_access,
      $.array_access,
    ),

    // Function declaration
    function_declaration: $ => seq(
      'function',
      field('name', $.identifier),
      field('parameters', $.parameter_list),
      optional(seq(':', field('return_type', $.type))),
      field('body', $.block),
    ),

    parameter_list: $ => seq(
      '(',
      optional(seq(
        $.parameter,
        repeat(seq(',', $.parameter))
      )),
      ')'
    ),

    parameter: $ => seq(
      field('name', $.identifier),
      ':',
      field('type', $.type)
    ),

    // Class declaration
    class_declaration: $ => seq(
      'class',
      field('name', $.identifier),
      '{',
      repeat(choice(
        $.field_declaration,
        $.constructor_declaration,
        $.method_declaration,
      )),
      '}'
    ),

    field_declaration: $ => seq(
      field('name', $.identifier),
      ':',
      field('type', $.type),
      ';'
    ),

    constructor_declaration: $ => seq(
      'constructor',
      field('parameters', $.parameter_list),
      field('body', $.block),
    ),

    method_declaration: $ => seq(
      field('name', $.identifier),
      field('parameters', $.parameter_list),
      optional(seq(':', field('return_type', $.type))),
      field('body', $.block),
    ),

    // Statements
    if_statement: $ => prec.right(seq(
      'if',
      '(',
      field('condition', $._expression),
      ')',
      field('consequence', $.block),
      optional(seq('else', field('alternative', choice($.block, $.if_statement))))
    )),

    while_statement: $ => seq(
      'while',
      '(',
      field('condition', $._expression),
      ')',
      field('body', $.block),
    ),

    for_statement: $ => seq(
      'for',
      '(',
      field('initializer', $.variable_declaration),
      field('condition', $._expression),
      ';',
      field('increment', $.assignment),
      ')',
      field('body', $.block),
    ),

    try_statement: $ => seq(
      'try',
      field('body', $.block),
      optional(seq(
        'catch',
        '(',
        field('error', $.identifier),
        ')',
        field('handler', $.block),
      )),
      optional(seq(
        'finally',
        field('finalizer', $.block),
      )),
    ),

    return_statement: $ => seq(
      'return',
      optional($._expression),
      ';'
    ),

    expression_statement: $ => seq($._expression, ';'),

    block: $ => seq('{', repeat($._statement), '}'),

    // Expressions
    _expression: $ => choice(
      $.ternary_expression,
      $.binary_expression,
      $.unary_expression,
      $.call_expression,
      $.member_access,
      $.array_access,
      $.constructor_call,
      $.template_literal,
      $.parenthesized_expression,
      $.literal,
      $.identifier,
    ),

    binary_expression: $ => choice(
      // Comparison (use = not ==)
      prec.left(7, seq($._expression, '=', $._expression)),
      prec.left(7, seq($._expression, '!=', $._expression)),
      prec.left(8, seq($._expression, '<', $._expression)),
      prec.left(8, seq($._expression, '>', $._expression)),
      prec.left(8, seq($._expression, '<=', $._expression)),
      prec.left(8, seq($._expression, '>=', $._expression)),
      
      // Logical
      prec.left(4, seq($._expression, 'and', $._expression)),
      prec.left(3, seq($._expression, 'or', $._expression)),
      
      // Arithmetic
      prec.left(10, seq($._expression, '*', $._expression)),
      prec.left(10, seq($._expression, '/', $._expression)),
      prec.left(10, seq($._expression, '%', $._expression)),
      prec.left(9, seq($._expression, '+', $._expression)),
      prec.left(9, seq($._expression, '-', $._expression)),
    ),

    // Ternary expression: condition ? true_expr : false_expr
    ternary_expression: $ => prec.right(2, seq(
      field('condition', $._expression),
      '?',
      field('consequence', $._expression),
      ':',
      field('alternative', $._expression),
    )),

    unary_expression: $ => choice(
      prec(11, seq('not', $._expression)),
      prec(11, seq('-', $._expression)),
      prec(11, seq('+', $._expression)),
    ),

    call_expression: $ => prec(12, seq(
      field('function', choice($.identifier, $.member_access)),
      field('arguments', $.argument_list),
    )),

    argument_list: $ => seq(
      '(',
      optional(seq(
        $._expression,
        repeat(seq(',', $._expression))
      )),
      ')'
    ),

    member_access: $ => prec(13, seq(
      field('object', $._expression),
      '.',
      field('property', $.identifier),
    )),

    array_access: $ => prec(13, seq(
      field('array', $._expression),
      '[',
      field('index', $._expression),
      ']',
    )),

    constructor_call: $ => prec(12, seq(
      field('type', choice(
        $.primitive_type,
        alias($.identifier, $.type_identifier),
      )),
      field('arguments', $.argument_list),
    )),

    parenthesized_expression: $ => seq('(', $._expression, ')'),

    // Template literal: `text ${expr} more`
    template_literal: $ => seq(
      '`',
      repeat(choice(
        $.template_string,
        $.template_substitution,
      )),
      '`'
    ),

    template_string: $ => token.immediate(prec(1, /[^`$]+/)),

    template_substitution: $ => seq(
      '${',
      $._expression,
      '}'
    ),

    // Types
    type: $ => choice(
      $.primitive_type,
      $.array_type,
      $.nullable_type,
      $.identifier,  // Custom types
    ),

    primitive_type: $ => choice(
      'int', 'uint', 'byte', 'short', 'float', 'bool', 'string', 'void', 'any'
    ),

    array_type: $ => seq(
      field('element', $.type),
      '[', ']'
    ),

    nullable_type: $ => seq(
      field('base', $.type),
      '?'
    ),

    // Literals
    literal: $ => choice(
      $.number,
      $.string,
      $.boolean,
      $.null,
      $.array_literal,
    ),

    number: $ => token(choice(
      /\d+/,                    // Integer
      /\d+\.\d+/,               // Float
      /0x[0-9a-fA-F]+/,         // Hex
    )),

    string: $ => token(choice(
      seq('"', repeat(/[^"\\]|\\.*/), '"'),
      seq("'", repeat(/[^'\\]|\\.*/), "'"),
    )),

    boolean: $ => choice('true', 'false'),

    null: $ => 'null',

    array_literal: $ => seq(
      '[',
      optional(seq(
        $._expression,
        repeat(seq(',', $._expression)),
        optional(',')
      )),
      ']'
    ),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    comment: $ => token(choice(
      seq('//', /.*/),
      seq('/*', /[^*]*\*+([^/*][^*]*\*+)*/, '/')
    )),
  }
});
