// Syntax tutorial only: as3_parse accepts these forms but emits no ABC/SWF.
package examples {
    // A custom namespace introduces a qualifier, not an ordinary object field.
    public namespace tools = "urn:flashvm:tools";

    // Optional arguments precede the final rest parameter; no parameter follows rest.
    public function collect(first:Number = 1, ...values:Array):Number {
        var total:Number = first; // Begin with the explicitly supplied/default value.
        for each (var value:Number in values) {
            total += value; // for-each visits values; this file is not executed by the parser.
        }
        return total; // The declared return type is Number rather than void.
    }

    // Class members demonstrate contextual namespace attributes and accessors.
    public class Box {
        private var stored:Number = 0;
        tools function clear():void {
            stored = 0; // The custom tools namespace qualifies this method.
        }
        public function get value():Number {
            return stored; // get is contextual, not globally reserved as an identifier.
        }
        public function set value(next:Number):void {
            stored = next; // The setter's return contract uses return-only type void.
        }
        public function update():void {
            use namespace tools; // Parser recognition does not implement namespace execution.
            try {
                tools::clear(); // Qualified access must not be mistaken for a statement label.
            }
            catch (error:TypeError) {
                stored = -1; // A second catch is legal after the first handler.
            }
            catch (error:Error) {
                stored = -2;
            }
            finally {
                stored += 1; // Finally is represented syntactically, without a backend here.
            }
        }
    }
}
