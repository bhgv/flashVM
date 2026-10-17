/* Filter document paths without changing navigation or loading external services. */
(function () {
    'use strict';
    var filter = document.getElementById('filter');
    filter.addEventListener('input', function () {
        var query = filter.value.toLowerCase();
        Array.prototype.forEach.call(document.querySelectorAll('aside li'), function (row) {
            row.hidden = row.textContent.toLowerCase().indexOf(query) === -1;
        });
    });
    /* Put a linked source line in the middle of the viewport on load or navigation. */
    function revealLine() {
        if (/^#L[0-9]+$/.test(location.hash)) {
            var line = document.getElementById(location.hash.slice(1));
            if (line) { line.scrollIntoView({block: 'center'}); }
        }
    }
    window.addEventListener('hashchange', revealLine);
    revealLine();
}());
