// Syntax-only class tutorial: the parser does not create an AST, ABC or SWF.
package examples {
    public class Counter {
        private var total:Number = 0; // Instance storage with an explicit AS3 type.
        public function Counter() {
            // Empty constructor; context/type validation is outside this frontend.
        }
        public function add(value:Number):Number {
            total = total + value; // Member access is implicit in this source spelling.
            return total; // Explicit semicolon is required by this syntax profile.
        }
    }
}
