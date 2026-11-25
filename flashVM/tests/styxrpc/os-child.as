// Copyright (c) 2026 bhgv
// EN: Render a loaded child observable through one write on its first frame.
// RU: Отметить выполнение загруженного потомка записью на первом кадре.


// EN: Replace the marker byte only when the attached child actually runs.
// RU: Заменить байт маркера только после выполнения прикреплённого потомка.
function onEnterFrame() {
    if (!osChildDone) {
        childMarker = os.open("/child.txt", "w");
        childText = "";
        childBytes = childText.split("");
        childCount = childBytes.push(67);
        childWritten = childMarker.write(childBytes);
        childMarkerClosed = childMarker.close();
        osChildDone = true;
    }
}
