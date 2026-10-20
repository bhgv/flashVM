// Syntax frontend example; this file does not imply ABC generation.
package examples {
    public namespace tools = "urn:flashvm:tools";
    public function collect(first:Number = 1, ...values:Array):Number {
        var total:Number = first;
        for each (var value:Number in values) total += value;
        return total;
    }
    public class Box {
        private var stored:Number = 0;
        tools function clear():void { stored = 0; }
        public function get value():Number { return stored; }
        public function set value(next:Number):void { stored = next; }
        public function update():void {
            use namespace tools;
            try { tools::clear(); }
            catch (error:TypeError) { stored = -1; }
            catch (error:Error) { stored = -2; }
            finally { stored += 1; }
        }
    }
}
