import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtCarplay 1.0

Window {
    id: root
    
    // Configurable stream resolution and framerate
    property int streamWidth: 1920
    property int streamHeight: 1080
    property int streamFps: 60

    width: streamWidth
    height: streamHeight
    visible: true
    color: "#05070a"
    title: qsTr("Qt CarPlay")
    property bool autoConnect: true

    CarplayController {
        id: carplay
        videoSink: videoOutput.videoSink
    }

    Component.onCompleted: {
        carplay.startStream(streamWidth, streamHeight, streamFps)
    }

    Timer {
        interval: 3000
        repeat: true
        running: root.autoConnect
        triggeredOnStart: false
        onTriggered: {
            if (!carplay.streaming)
                carplay.startStream(streamWidth, streamHeight, streamFps)
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        visible: carplay.frameCount > 0
    }

    MouseArea {
        id: touchSurface
        anchors.fill: videoOutput
        enabled: carplay.streaming && carplay.frameCount > 0
        preventStealing: true
        acceptedButtons: Qt.LeftButton

        function normalized(pointX, pointY) {
            const rect = videoOutput.contentRect
            if (rect.width <= 0 || rect.height <= 0)
                return Qt.point(0.5, 0.5)

            const x = Math.max(0, Math.min(1, (pointX - rect.x) / rect.width))
            const y = Math.max(0, Math.min(1, (pointY - rect.y) / rect.height))
            return Qt.point(x, y)
        }

        onPressed: function(mouse) {
            const p = normalized(mouse.x, mouse.y)
            carplay.touchDown(p.x, p.y)
        }

        onPositionChanged: function(mouse) {
            if (!pressed)
                return
            const p = normalized(mouse.x, mouse.y)
            carplay.touchMove(p.x, p.y)
        }

        onReleased: function(mouse) {
            const p = normalized(mouse.x, mouse.y)
            carplay.touchUp(p.x, p.y)
        }

        onCanceled: {
            carplay.touchUp(0.5, 0.5)
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#05070a"
        visible: carplay.frameCount <= 0

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 520)
            spacing: 18

            Label {
                Layout.fillWidth: true
                text: "Qt CarPlay"
                color: "#f5f7fb"
                font.pixelSize: 34
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
                text: carplay.status
                color: "#aeb8c8"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 15
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 10

                Button {
                    text: "Verify Dongle"
                    onClicked: carplay.verifyHardware()
                }

                Button {
                    text: "Reconnect"
                    enabled: !carplay.streaming
                    onClicked: {
                        root.autoConnect = true
                        carplay.startStream(streamWidth, streamHeight, streamFps)
                    }
                }

                Button {
                    text: "Test Capture"
                    enabled: !carplay.streaming
                    onClicked: carplay.playCapture("/Users/niklasprobst/Desktop/QtCarplay/carplay/build/raw_capture", 30)
                }

                Button {
                    text: "Stop"
                    enabled: carplay.streaming
                    onClicked: {
                        root.autoConnect = false
                        carplay.stop()
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 40
        color: "#cc05070a"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 18

            Label {
                Layout.fillWidth: true
                text: carplay.status
                color: "#f5f7fb"
                elide: Text.ElideRight
                font.pixelSize: 13
            }

            Label {
                text: carplay.videoWidth > 0 ? carplay.videoWidth + "x" + carplay.videoHeight : "--"
                color: "#aeb8c8"
                font.pixelSize: 13
            }

            Label {
                text: carplay.frameCount + " frames"
                color: "#aeb8c8"
                font.pixelSize: 13
            }

            Label {
                text: carplay.decodedFps.toFixed(1) + " fps"
                color: "#aeb8c8"
                font.pixelSize: 13
            }

            Label {
                text: carplay.audioActive ? "audio " + carplay.audioPacketCount : "silent"
                color: carplay.audioActive ? "#b7f7ca" : "#aeb8c8"
                font.pixelSize: 13
            }
        }
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            root.autoConnect = false
            carplay.stop()
        }
    }
}
