// Script companion for a host that explicitly installed AsBroadcaster; SWF6--8.
var source = {};                  // Ordinary user object, independent of the display/input frontend.
AsBroadcaster.initialize(source); // Add the live Array and three native instance methods.
var observer = {};                // A distinct object identity will receive the event as this.
observer.onValue = function(value) // One payload parameter follows the event-name argument.
{ // Begin the documented branch/scope: One payload parameter follows the event-name argument.
    // One payload parameter follows the event-name argument.
    trace(value); // Expected trace: 42; the receiver is observer.

}; // Finish the application-defined handler.
source.addListener(observer);           // Retain observer strongly in source._listeners.
source.broadcastMessage("onValue", 42); // Snapshot recipients, then invoke onValue(42).
source.removeListener(observer);        // Release this registration after synchronous delivery.
