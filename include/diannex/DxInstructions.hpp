/*======================================================================================================================
 * Copyright © 2023 PeriBooty and Contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *====================================================================================================================*/
#ifndef LIBDIANNEX_DXINSTRUCTIONS_HPP
#define LIBDIANNEX_DXINSTRUCTIONS_HPP

namespace diannex
{
    enum class [[maybe_unused]] DxOpcode : unsigned char
    {
        nop = 0x00, // No-op

        freeloc = 0x0A, // Frees a local variable from the stack frame (IF IT EXISTS!): [ID]

        // Special register instructions
        save = 0x0B, // Copy the value on the top of the stack into the save register
        load = 0x0C, // Push the value from the save register onto the top of the stack

        // Stack instructions
        pushu = 0x0F, // Push undefined value to stack
        pushi = 0x10, // Push 32-bit int: [int value]
        pushd = 0x11, // Push 64-bit floating point: [double value]

        pushs = 0x12, // Push external string: [index]
        pushints = 0x13, // Push external interpolated string: [index, expr count]
        pushbs = 0x14, // Push internal binary string: [ID]
        pushbints = 0x15, // Push internal binary interpolated string: [ID, expr count]

        makearr = 0x16, // Construct an array based off of stack: [size]
        pusharrind = 0x17, // Extract a single value out of an array, removing the array as well (uses stack for index)
        setarrind = 0x18, // Sets a value in an array on the top of the stack (uses stack for index and value)

        setvarglb = 0x19, // Set a global variable from the stack: [string name]
        setvarloc = 0x1A, // Set a local variable from the stack: [ID]
        pushvarglb = 0x1B, // Pushes a global variable to the stack: [string name]
        pushvarloc = 0x1C, // Pushes a local variable to the stack: [ID]

        pop = 0x1D, // Discards the value on the top of the stack
        dup = 0x1E, // Duplicates the value on the top of the stack
        dup2 = 0x1F, // Duplicates the values on the top two slots of the stack

        // Operators
        add = 0x20, // Adds the two values on the top of the stack, popping them, pushing the result
        sub = 0x21, // ditto, subtracts
        mul = 0x22, // ditto, multiplies
        div = 0x23, // ditto, divides
        mod = 0x24, // ditto, modulo
        neg = 0x25, // Negates the value on the top of the stack, popping it, pushing the result
        inv = 0x26, // ditto, but inverts a boolean

        bitls = 0x27, // Peforms bitwise left-shift using the top two values of stack, popping them, pushing the result
        bitrs = 0x28, // ditto, right-shift
        _bitand = 0x29, // ditto, and
        _bitor = 0x2A, // ditto, or
        bitxor = 0x2B, // ditto, xor
        bitneg = 0x2C, // ditto, negate (~)

        pow = 0x2D, // Power binary operation using top two values of stack

        cmpeq = 0x30, // Compares the top two values of stack to check if they are equal, popping them, pushing the result
        cmpgt = 0x31, // ditto, greater than
        cmplt = 0x32, // ditto, less than
        cmpgte = 0x33, // ditto, greater than or equal
        cmplte = 0x34, // ditto, less than or equal
        cmpneq = 0x35, // ditto, not equal

        // Control flow
        j = 0x40, // Jumps to an instruction [int relative address]
        jt = 0x41, // ditto, but if the value on the top of the stack is truthy (which it pops off)
        jf = 0x42, // ditto, but if the value on the top of the stack is NOT truthy (which it pops off)
        exit = 0x43, // Exits the current stack frame
        ret = 0x44, // Exits the current stack frame, returning a value (from the stack, popping it off)
        call = 0x45, // Calls a function defined in the code [ID, int parameter count]
        callext = 0x46, // Calls a function defined by a game [string name, int parameter count]

        choicebeg = 0x47, // Switches to the choice state in the interpreter- no other choices can run and
        // only one textrun can execute until after choicesel is executed
        choiceadd = 0x48, // Adds a choice, using the stack for the text and the % chance of appearing [int relative jump address]
        choiceaddt = 0x49, // ditto, but also if an additional stack value is truthy [int relative jump address]
        choicesel = 0x4A, // Pauses the interpreter, waiting for user input to select one of the choices, then jumps to one of them, resuming

        chooseadd = 0x4B, // Adds a new address to one of the possible next statements, using stack for chances [int relative jump address] (to the current stack frame)
        chooseaddt = 0x4C, // ditto, but also if an additional stack value is truthy [int relative jump address]
        choosesel = 0x4D, // Jumps to one of the choices, using the addresses and chances/requirement values on the stack

        textrun = 0x4E, // Pauses the interpreter, running a line of text from the stack
    };
}

#endif //LIBDIANNEX_DXINSTRUCTIONS_HPP
