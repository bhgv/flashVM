// Syntax-only interface tutorial; no runtime object is constructed by the parser.
package examples // Use the same namespace as the companion Counter class.
{ // Begin the package's declarations.


    // Describe a numeric increment operation without supplying an implementation.
    public interface ICounter // Implementing/type-checking this contract is a later compiler stage.
    { // Begin required method signatures.


        // Increase the implementation's counter by a numeric amount.
        // Parameters: value is the increment. Returns the implementation's updated total.
        function add(value:Number):Number; // An interface ends the signature with a semicolon, not a body.
    } // Finish the declared interface.
} // Finish the package body.
