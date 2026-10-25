// Syntax-only tutorial: this frontend emits neither ABC nor a runnable AS3 SWF.
package examples // Keep the class in an explicitly named source namespace.
{ // Begin the package body recognized by Coco/R.


    // Store a Number and demonstrate a constructor plus one typed method.
    public class Counter // Class/type resolution remains separate from syntax acceptance.
    { // Begin the declared instance members.
        private var total:Number = 0; // Per-instance numeric storage starts at zero.


        // Construct an instance with its field initializer and no extra work.
        // Parameters: none. No explicit return value is permitted by this constructor.
        public function Counter() // Declare the constructor using the class name.
        { // A real AS3 runtime would enter this body after field initialization.
        } // This syntax example deliberately performs no constructor side effects.


        // Increase the stored total and return the resulting number.
        // Parameters: value is the numeric increment. Returns the updated total.
        public function add(value:Number):Number // The return type is explicit in the interface.
        { // Begin the method body with access to this instance's storage.
            total = total + value; // Resolve the member on this instance and store the sum.
            return total; // Return the updated value rather than the previous total.
        } // End add; checking syntax does not execute it.
    } // End the Counter declaration.
} // End the package without claiming an ABC backend exists.
