// Syntax-only AS3 example: no ABC/SWF is emitted by as3_parse.
package examples {
    public class Counter {
        private var total:Number = 0;
        public function Counter() {}
        public function add(value:Number):Number {
            total = total + value;
            return total;
        }
    }
}
