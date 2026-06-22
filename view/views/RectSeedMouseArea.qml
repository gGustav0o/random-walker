import QtQuick 2.15

MouseArea {
    id: root

    required property real imageX
    required property real imageY
    required property real imageWidth
    required property real imageHeight
    required property int sourceWidth
    required property int sourceHeight
    required property int selectedLabel

    signal rectangleCreated(int x, int y, int width, int height)

    property real startX: 0
    property real startY: 0
    property real currentX: 0
    property real currentY: 0
    property bool drawing: false

    function clampedX(value) {
        return Math.max(imageX, Math.min(value, imageX + imageWidth))
    }

    function clampedY(value) {
        return Math.max(imageY, Math.min(value, imageY + imageHeight))
    }

    onPressed: function(mouse) {
        startX = clampedX(mouse.x)
        startY = clampedY(mouse.y)
        currentX = startX
        currentY = startY
        drawing = true
    }

    onPositionChanged: function(mouse) {
        if (!drawing)
            return
        currentX = clampedX(mouse.x)
        currentY = clampedY(mouse.y)
    }

    onReleased: {
        if (!drawing)
            return

        drawing = false

        const left = Math.min(startX, currentX)
        const top = Math.min(startY, currentY)
        const right = Math.max(startX, currentX)
        const bottom = Math.max(startY, currentY)

        const sourceLeft = Math.floor(
            (left - imageX) * sourceWidth / imageWidth)
        const sourceTop = Math.floor(
            (top - imageY) * sourceHeight / imageHeight)
        const sourceRight = Math.ceil(
            (right - imageX) * sourceWidth / imageWidth)
        const sourceBottom = Math.ceil(
            (bottom - imageY) * sourceHeight / imageHeight)

        const width = sourceRight - sourceLeft
        const height = sourceBottom - sourceTop
        if (width > 0 && height > 0)
            rectangleCreated(sourceLeft, sourceTop, width, height)
    }

    Rectangle {
        visible: root.drawing
        x: Math.min(root.startX, root.currentX)
        y: Math.min(root.startY, root.currentY)
        width: Math.abs(root.currentX - root.startX)
        height: Math.abs(root.currentY - root.startY)
        color: root.selectedLabel === 0 ? "#440000ff" : "#44ff0000"
        border.color: root.selectedLabel === 0 ? "blue" : "red"
        border.width: 1
    }
}
