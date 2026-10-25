// Syntax-only AS3 tutorial: these declarations do not produce ABC or a runnable SWF.
package examples // Group functions and the class in one explicit source namespace.
{ // Begin the package body.
    public namespace tools = "urn:flashvm:tools"; // Declare a custom member qualifier, not an ordinary object field.


    // Add an optional first value and every value from a final rest parameter.
    // Parameters: first defaults to 1; values receives the remaining arguments as an Array.
    // Returns the accumulated Number; type/namespace execution is outside syntax recognition.
    public function collect(first:Number = 1, ...values:Array):Number // No parameter may follow the rest parameter.
    { // Begin this function's local variables.
        var total:Number = first; // Start from the explicitly supplied or default numeric value.
        for each (var value:Number in values) // Visit values rather than property names.
        { // Add the current typed iteration value.
            total += value; // Update the local accumulator, leaving the input Array untouched.
        } // Continue until the rest-argument values have been visited.
        return total; // Return a Number instead of an implicit/declared void result.
    } // End collect; this file itself is not executed by the syntax frontend.


    // Demonstrate namespace-qualified methods and typed getter/setter declarations.
    public class Box // Actual class construction/name resolution is a later compiler stage.
    { // Begin the instance members.
        private var stored:Number = 0; // Keep this instance's numeric state private.


        // Reset the stored value through the custom tools namespace.
        // Parameters: none. Returns no value, matching the declared void result.
        tools function clear():void // Qualify this method using the earlier namespace declaration.
        { // Begin one reset operation.
            stored = 0; // Set the instance member to its initial numeric value.
        } // End the namespace-qualified method.


        // Read the stored Number through accessor syntax.
        // Parameters: none. Returns the current instance value.
        public function get value():Number // get is contextual in this declaration.
        { // Begin the getter body.
            return stored; // Produce the member value without modifying it.
        } // End the getter declaration.


        // Replace the stored Number through accessor syntax.
        // Parameters: next is the assigned value. Returns no value.
        public function set value(next:Number):void // The setter has exactly one typed argument.
        { // Begin the setter body.
            stored = next; // Publish the supplied Number in this instance.
        } // End the setter; no value is returned from a void method.


        // Demonstrate qualified calls, ordered catches and a finally block.
        // Parameters: none. Returns no value; this example only teaches recognized syntax.
        public function update():void // Do not treat parsing as proof of exception-runtime behavior.
        { // Begin the update method.
            use namespace tools; // Make the custom namespace available for member resolution.
            try // Start the protected operation.
            { // The namespace operator must not be parsed as a statement label.
                tools::clear(); // Call the method through its explicit namespace qualifier.
            } // Leave the protected body before selecting a handler.
            catch (error:TypeError) // Put the more specific type before the general Error handler.
            { // Handle the specific error case in this syntax example.
                stored = -1; // Use a distinct sentinel to explain which catch branch was chosen.
            } // End the TypeError handler.
            catch (error:Error) // A second typed handler remains legal after the first.
            { // Handle the more general error case.
                stored = -2; // Distinguish this branch from the specific type case.
            } // End the Error handler.
            finally // This clause belongs to the protected statement, not to either catch alone.
            { // Show a final operation after the protected/handler path.
                stored += 1; // Demonstrate a finally-side update without claiming a runtime backend exists.
            } // End the finally clause.
        } // End update.
    } // End Box's instance declarations.
} // End the package.
