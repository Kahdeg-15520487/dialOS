namespace DialOS.Runtime.Bytecode;

/// <summary>
/// dialScript bytecode instruction opcodes.
/// Stack-based VM with local variables.
/// </summary>
public enum Opcode : byte
{
    // Stack operations
    Nop = 0x00,         // No operation
    Pop = 0x01,         // Pop top of stack
    Dup = 0x02,         // Duplicate top of stack
    Swap = 0x03,        // Swap top two stack items

    // Constants
    PushNull = 0x10,    // Push null
    PushTrue = 0x11,    // Push boolean true
    PushFalse = 0x12,   // Push boolean false
    PushI8 = 0x13,      // Push 8-bit signed integer (next byte)
    PushI16 = 0x14,     // Push 16-bit signed integer (next 2 bytes)
    PushI32 = 0x15,     // Push 32-bit signed integer (next 4 bytes)
    PushF32 = 0x16,     // Push 32-bit float (next 4 bytes)
    PushStr = 0x17,     // Push string constant (index in constant pool)

    // Local variables
    LoadLocal = 0x20,   // Load local variable (index)
    StoreLocal = 0x21,  // Store to local variable (index)

    // Global variables
    LoadGlobal = 0x30,  // Load global variable (name index)
    StoreGlobal = 0x31, // Store to global variable (name index)

    // Arithmetic operations
    Add = 0x40,         // Add two values
    Sub = 0x41,         // Subtract
    Mul = 0x42,         // Multiply
    Div = 0x43,         // Divide
    Mod = 0x44,         // Modulo
    Neg = 0x45,         // Negate (unary minus)

    // String operations
    StrConcat = 0x46,   // String concatenation
    TemplateFormat = 0x47, // Template string formatting (template + args)

    // Comparison operations
    Eq = 0x50,          // Equal (=)
    Ne = 0x51,          // Not equal (!=)
    Lt = 0x52,          // Less than (<)
    Le = 0x53,          // Less or equal (<=)
    Gt = 0x54,          // Greater than (>)
    Ge = 0x55,          // Greater or equal (>=)

    // Logical operations
    Not = 0x60,         // Logical NOT
    And = 0x61,         // Logical AND
    Or = 0x62,          // Logical OR

    // Control flow
    Jump = 0x70,        // Unconditional jump (offset)
    JumpIf = 0x71,      // Jump if true (offset)
    JumpIfNot = 0x72,   // Jump if false (offset)

    // Function calls
    Call = 0x80,        // Call function (function index, arg count)
    CallNative = 0x81,  // Call native function (native index, arg count)
    Return = 0x82,      // Return from function
    LoadFunction = 0x83, // Push function reference (function index u16)
    CallIndirect = 0x84, // Call function from stack (arg count u8)
    CallMethod = 0x85,  // Call method with implicit receiver (u8 argCount, u16 methodNameIdx?)

    // Object/Member access
    GetField = 0x90,    // Get object field (field name index)
    SetField = 0x91,    // Set object field (field name index)
    GetIndex = 0x92,    // Get array element
    SetIndex = 0x93,    // Set array element

    // Object creation
    NewObject = 0xA0,   // Create new object (class index)
    NewArray = 0xA1,    // Create new array (size on stack)

    // Exception handling
    Try = 0xB0,         // Set exception handler (catch offset)
    EndTry = 0xB1,      // Remove exception handler
    Throw = 0xB2,       // Throw exception (value on stack)

    // Special
    Print = 0xF0,       // Debug print (temporary)
    Halt = 0xFF,        // Halt execution
}
